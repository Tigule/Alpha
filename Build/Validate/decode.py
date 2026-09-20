
from __future__ import annotations

from dataclasses import dataclass
import struct
from typing import Any


class DecodeUnavailable(RuntimeError):
    pass


class DecodeFailure(ValueError):
    pass


@dataclass(frozen=True)
class Instruction:
    offset: int
    size: int
    raw: bytes
    mnemonic: str
    operands: tuple[tuple[Any, ...], ...]
    fields: tuple[tuple[int, int, str, int], ...]
    regs_read: tuple[str, ...] = ()
    regs_write: tuple[str, ...] = ()
    register_access_known: bool = False


@dataclass(frozen=True)
class DataRegion:
    offset: int
    raw: bytes


@dataclass(frozen=True)
class JumpTable:
    offset: int
    entry_size: int
    targets: tuple[int, ...]
    instruction_offset: int
    operand_index: int
    selector_offset: int | None = None


@dataclass(frozen=True)
class SelectorTable:
    offset: int
    values: bytes
    instruction_offset: int
    operand_index: int
    dispatch_instruction_offset: int


@dataclass(frozen=True)
class DecodedProcedure:
    instructions: tuple[Instruction, ...]
    data_regions: tuple[DataRegion, ...]
    jump_tables: tuple[JumpTable, ...]
    selector_tables: tuple[SelectorTable, ...] = ()


def _capstone() -> tuple[Any, Any]:
    try:
        import capstone
        from capstone import x86_const
    except ImportError as exc:  
        raise DecodeUnavailable(
            "Capstone is required; install Build/Validate/requirements.txt into "
            "a local virtual environment"
        ) from exc
    return capstone, x86_const


def require_decoder() -> None:
    _capstone()


def _instruction(decoder: Any, x86: Any, code: bytes, address: int, offset: int) -> Instruction:
    raw_instruction = next(decoder.disasm(code[offset:], address + offset, count=1), None)
    if raw_instruction is None or raw_instruction.address != address + offset:
        raise DecodeFailure(f"decoder stopped at reachable byte {offset} of {len(code)}")
    operands: list[tuple[Any, ...]] = []
    for operand in raw_instruction.operands:
        if operand.type == x86.X86_OP_REG:
            operands.append(("reg", raw_instruction.reg_name(operand.reg), operand.size))
        elif operand.type == x86.X86_OP_IMM:
            operands.append(("imm", int(operand.imm), operand.size))
        elif operand.type == x86.X86_OP_MEM:
            mem = operand.mem
            operands.append(
                (
                    "mem",
                    raw_instruction.reg_name(mem.segment) if mem.segment else "",
                    raw_instruction.reg_name(mem.base) if mem.base else "",
                    raw_instruction.reg_name(mem.index) if mem.index else "",
                    int(mem.scale),
                    int(mem.disp),
                    operand.size,
                )
            )
        else:
            operands.append(("unsupported", int(operand.type), operand.size))
    fields: list[tuple[int, int, str, int]] = []
    if raw_instruction.imm_size:
        fields.append((raw_instruction.imm_offset, raw_instruction.imm_size, "immediate", len(operands) - 1))
    if raw_instruction.disp_size:
        memory_operands = [i for i, operand in enumerate(operands) if operand[0] == "mem"]
        if not memory_operands:
            raise DecodeFailure(f"decoder reported a displacement without a memory operand at byte {offset}")
        fields.append((raw_instruction.disp_offset, raw_instruction.disp_size, "displacement", memory_operands[-1]))
    try:
        read_ids, write_ids = raw_instruction.regs_access()
        regs_read = tuple(sorted(raw_instruction.reg_name(value) for value in read_ids))
        regs_write = tuple(sorted(raw_instruction.reg_name(value) for value in write_ids))
        access_known = True
    except Exception:  
        regs_read = regs_write = ()
        access_known = False
    return Instruction(
        offset=offset,
        size=raw_instruction.size,
        raw=bytes(raw_instruction.bytes),
        mnemonic=raw_instruction.mnemonic,
        operands=tuple(operands),
        fields=tuple(fields),
        regs_read=regs_read,
        regs_write=regs_write,
        register_access_known=access_known,
    )


def _direct_target(instruction: Instruction, address: int, length: int) -> int | None:
    if not instruction.operands or instruction.operands[0][0] != "imm":
        return None
    target = int(instruction.operands[0][1]) - address
    return target if 0 <= target < length else None


def _register_aliases(register: str) -> set[str]:
    groups = (
        {"eax", "ax", "al", "ah"}, {"ebx", "bx", "bl", "bh"},
        {"ecx", "cx", "cl", "ch"}, {"edx", "dx", "dl", "dh"},
        {"esi", "si"}, {"edi", "di"}, {"ebp", "bp"}, {"esp", "sp"},
    )
    return next((group for group in groups if register in group), {register})


def _writes_register(instruction: Instruction, register: str) -> bool:
    if _control_flow_barrier(instruction) or not instruction.register_access_known:
        return True
    if (instruction.mnemonic == "lea" and len(instruction.operands) == 2
            and instruction.operands[0][0] == "reg" and instruction.operands[0][1] == register
            and instruction.operands[1][0] == "mem"
            and instruction.operands[1][1] == "" and instruction.operands[1][2] == register
            and instruction.operands[1][3] == "" and instruction.operands[1][4] == 1
            and instruction.operands[1][5] == 0):
        return False
    return bool(set(instruction.regs_write) & _register_aliases(register))


def _control_flow_barrier(instruction: Instruction) -> bool:
    return (instruction.mnemonic.startswith("call") or instruction.mnemonic.startswith("int")
            or instruction.mnemonic in {"syscall", "sysenter"})


def _constant_register(block: list[Instruction], before: int, register: str) -> int | None:
    for index in range(before - 1, -1, -1):
        instruction = block[index]
        if not _writes_register(instruction, register):
            continue
        if (instruction.mnemonic == "mov" and len(instruction.operands) == 2
                and instruction.operands[0][0] == "reg"
                and instruction.operands[0][1] == register
                and instruction.operands[1][0] == "imm"):
            value = int(instruction.operands[1][1])
            return value if 0 <= value < 0x10000 else None
        return None
    return None


def _guarded_count(index_register: str, block: list[Instruction], address: int,
                   dispatch_offset: int) -> int | None:
    for index in range(len(block) - 2, -1, -1):
        candidate = block[index]
        if _control_flow_barrier(candidate) or not candidate.register_access_known:
            return None
        if candidate.mnemonic != "cmp":
            continue
        if (len(candidate.operands) != 2 or candidate.operands[0][0] != "reg"
                or candidate.operands[0][1] != index_register):
            continue
        bound_operand = candidate.operands[1]
        if bound_operand[0] == "imm":
            bound = int(bound_operand[1])
        elif bound_operand[0] == "reg":
            bound = _constant_register(block, index, str(bound_operand[1]))
        else:
            bound = None
        if bound is None or not 0 <= bound < 0x10000:
            continue
        branch_index: int | None = None
        for candidate_index in range(index + 1, len(block)):
            value = block[candidate_index]
            if _control_flow_barrier(value) or not value.register_access_known:
                break
            if value.mnemonic in {"ja", "jnbe", "jae", "jnb"}:
                branch_index = candidate_index
                break
            if "eflags" in value.regs_write:
                break
        if branch_index is None:
            continue
        branch = block[branch_index]
        target = _direct_target(branch, address, 1 << 32)
        if target is None or candidate.offset < target <= dispatch_offset:
            continue
        if any(_writes_register(value, index_register) for value in block[index + 1:]):
            return None
        if branch.mnemonic in {"ja", "jnbe"}:
            
            return bound + 1
        if branch.mnemonic in {"jae", "jnb"}:
            
            return bound
    return None


def _selector_table(dispatch: Instruction, block: list[Instruction], code: bytes,
                    address: int, dispatch_register: str) -> SelectorTable | None:
    if not block:
        return None
    low_registers = {"eax": "al", "ebx": "bl", "ecx": "cl", "edx": "dl"}
    producer_index = next((index for index in range(len(block) - 1, max(-1, len(block) - 5), -1)
                           if len(block[index].operands) == 2
                           and block[index].mnemonic in {"movzx", "mov"}
                           and block[index].operands[0][0] == "reg"
                           and block[index].operands[1][0] == "mem"
                           and ((block[index].mnemonic == "movzx"
                                 and block[index].operands[0][1] == dispatch_register)
                                or (block[index].mnemonic == "mov"
                                    and block[index].operands[0][1] == low_registers.get(dispatch_register)
                                    and block[index].operands[0][2] == 1))), None)
    if producer_index is None:
        return None
    producer = block[producer_index]
    destination, source = producer.operands
    if source[0] != "mem":
        return None
    if producer.mnemonic == "movzx":
        if destination[1] != dispatch_register:
            return None
    else:
        if destination[1] != low_registers.get(dispatch_register) or destination[2] != 1:
            return None
        zero_index = next((index for index in range(producer_index - 1, max(-1, producer_index - 4), -1)
                           if block[index].mnemonic == "xor"
                           and len(block[index].operands) == 2
                           and block[index].operands[0] == block[index].operands[1]
                           and block[index].operands[0][0] == "reg"
                           and block[index].operands[0][1] == dispatch_register), None)
        if zero_index is None or any(_writes_register(value, dispatch_register)
                                     for value in block[zero_index + 1:producer_index]):
            return None
    if any(_writes_register(value, dispatch_register) for value in block[producer_index + 1:]):
        return None
    source_registers = [value for value in (source[2], source[3]) if value]
    if len(source_registers) != 1 or source[1] or source[6] != 1 or source[4] != 1:
        return None
    selector_count = _guarded_count(str(source_registers[0]), block[:producer_index], address, dispatch.offset)
    selector_offset = int(source[5]) - address
    if (selector_count is None or selector_count <= 0 or selector_offset < 0
            or selector_offset + selector_count > len(code)):
        return None
    return SelectorTable(selector_offset, code[selector_offset:selector_offset + selector_count],
                         producer.offset, 1, dispatch.offset)


def _jump_table(instruction: Instruction, block: list[Instruction],
                code: bytes, address: int) -> tuple[JumpTable, SelectorTable | None] | None:
    if instruction.mnemonic != "jmp":
        return None
    for operand_index, operand in enumerate(instruction.operands):
        if (operand[0] != "mem" or operand[1] or operand[2] or not operand[3]
                or operand[4] != 4 or operand[6] != 4):
            continue
        table_offset = int(operand[5]) - address
        if table_offset < 0 or table_offset + 4 > len(code) or (address + table_offset) % 4:
            continue
        selector = _selector_table(instruction, block, code, address, str(operand[3]))
        entry_count = ((max(selector.values) + 1) if selector is not None and selector.values
                       else _guarded_count(str(operand[3]), block, address, instruction.offset))
        if entry_count is None or table_offset + entry_count * 4 > len(code):
            continue
        targets: list[int] = []
        cursor = table_offset
        for _ in range(entry_count):
            target = struct.unpack_from("<I", code, cursor)[0] - address
            if not 0 <= target < len(code):
                targets = []
                break
            targets.append(target)
            cursor += 4
        if targets:
            return (JumpTable(table_offset, 4, tuple(targets), instruction.offset, operand_index,
                              selector.offset if selector is not None else None), selector)
    return None


def decode_procedure(code: bytes, address: int) -> DecodedProcedure:
    capstone, x86 = _capstone()
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    if not code:
        return DecodedProcedure((), (), (), ())

    instructions: dict[int, Instruction] = {}
    occupied: dict[int, int] = {}
    tables: dict[tuple[int, int], JumpTable] = {}
    selectors: dict[tuple[int, int], SelectorTable] = {}
    pending = [0]
    queued = {0}

    def enqueue(offset: int | None) -> None:
        if offset is not None and offset not in queued and offset not in instructions:
            queued.add(offset)
            pending.append(offset)

    def discover_table(instruction: Instruction, block: list[Instruction]) -> None:
        discovery = _jump_table(instruction, block, code, address)
        if discovery is None:
            return
        table, selector = discovery
        for existing in tables.values():
            if existing.offset == table.offset and existing.targets != table.targets:
                raise DecodeFailure(f"conflicting jump-table interpretations at byte {table.offset}")
        tables[(table.offset, table.instruction_offset)] = table
        if selector is not None:
            selectors[(selector.offset, selector.instruction_offset)] = selector
        for case_target in table.targets:
            enqueue(case_target)

    while pending:
        cursor = pending.pop()
        block: list[Instruction] = []
        while 0 <= cursor < len(code):
            if cursor in instructions:
                existing = instructions[cursor]
                if existing.mnemonic == "jmp" and existing.operands and existing.operands[0][0] == "mem":
                    discover_table(existing, block)
                break
            if cursor in occupied:
                raise DecodeFailure(f"control flow enters the middle of an instruction at byte {cursor}")
            instruction = _instruction(decoder, x86, code, address, cursor)
            end = cursor + instruction.size
            if end > len(code):
                raise DecodeFailure(f"reachable instruction extends past procedure range at byte {cursor}")
            overlap = next((byte for byte in range(cursor, end) if byte in occupied), None)
            if overlap is not None:
                raise DecodeFailure(f"overlapping reachable instructions at byte {overlap}")
            instructions[cursor] = instruction
            for byte in range(cursor, end):
                occupied[byte] = cursor

            target = _direct_target(instruction, address, len(code))
            is_call = instruction.mnemonic.startswith("call")
            is_jump = instruction.mnemonic.startswith("j") or instruction.mnemonic.startswith("loop")
            is_conditional = is_jump and instruction.mnemonic != "jmp"
            if (is_call or is_jump) and target is not None:
                enqueue(target)
            if instruction.mnemonic == "jmp" and target is None:
                discover_table(instruction, block)

            terminates = (
                instruction.mnemonic == "jmp"
                or instruction.mnemonic.startswith("ret")
                or instruction.mnemonic in {"int3", "ud2", "hlt", "iret", "iretd"}
            )
            if terminates:
                break
            block.append(instruction)
            cursor = end

    for table in tables.values():
        table_end = table.offset + table.entry_size * len(table.targets)
        overlap = next((byte for byte in range(table.offset, table_end) if byte in occupied), None)
        if overlap is not None:
            raise DecodeFailure(f"jump table overlaps reachable instruction at byte {overlap}")
        for target in table.targets:
            if target not in instructions:
                raise DecodeFailure(f"jump table targets undecoded byte {target}")
    for selector in selectors.values():
        selector_end = selector.offset + len(selector.values)
        overlap = next((byte for byte in range(selector.offset, selector_end) if byte in occupied), None)
        if overlap is not None:
            raise DecodeFailure(f"selector table overlaps reachable instruction at byte {overlap}")

    data_regions: list[DataRegion] = []
    cursor = 0
    while cursor < len(code):
        if cursor in occupied:
            cursor += 1
            continue
        start = cursor
        while cursor < len(code) and cursor not in occupied:
            cursor += 1
        data_regions.append(DataRegion(start, code[start:cursor]))

    return DecodedProcedure(
        tuple(instructions[offset] for offset in sorted(instructions)),
        tuple(data_regions),
        tuple(tables[key] for key in sorted(tables)),
        tuple(selectors[key] for key in sorted(selectors)),
    )
