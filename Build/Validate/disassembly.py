from __future__ import annotations

from difflib import SequenceMatcher
from typing import Any, Mapping, Sequence

from .decode import DecodedProcedure, Instruction
from .normalize import located_data_tokens


_CONTEXT = 10
_MAX_ALIGNMENT_PRODUCT = 1_000_000
_MAX_ALIGNMENT_INSTRUCTIONS = 4096
_DATA_WIDTH = 8


def _shape(instruction: Instruction) -> tuple[Any, ...]:
    operands: list[tuple[Any, ...]] = []
    for operand in instruction.operands:
        if operand[0] == "imm":
            operands.append(("imm", operand[2]))
        elif operand[0] == "mem":
            operands.append(("mem",) + operand[1:5] + operand[6:])
        else:
            operands.append(operand)
    return (instruction.mnemonic, tuple(operands))


def _instruction_record(instruction: Instruction, index: int, rva: int, va: int) -> dict[str, Any]:
    text = instruction.mnemonic
    if instruction.op_str:
        text += f" {instruction.op_str}"
    return {
        "index": index,
        "function_offset": instruction.offset,
        "rva": rva + instruction.offset,
        "va": va + instruction.offset,
        "bytes_hex": instruction.raw.hex(),
        "mnemonic": instruction.mnemonic,
        "operands": instruction.op_str,
        "text": text,
    }


def _audit_notes(audit: Sequence[Mapping[str, Any]]) -> dict[tuple[int, int], str]:
    notes: dict[tuple[int, int], set[str]] = {}
    for item in audit:
        left = item.get("instruction_offset_original")
        right = item.get("instruction_offset_rebuilt")
        kind = item.get("kind")
        if isinstance(left, int) and isinstance(right, int) and kind in {"relocation", "macro"}:
            notes.setdefault((left, right), set()).add(str(kind))
    return {
        key: ", ".join(f"{kind}-normalized field" for kind in sorted(kinds))
        for key, kinds in notes.items()
    }


def _paired_status(left: Instruction, right: Instruction,
                   audit_notes: Mapping[tuple[int, int], str]) -> tuple[str, str | None]:
    note = audit_notes.get((left.offset, right.offset))
    if note is not None:
        return "context", note
    if _shape(left) != _shape(right):
        return "changed", "instruction differs"
    if left.raw != right.raw:
        return "changed", "bytes differ"
    return "context", None


def _instruction_pairs(left: Sequence[Instruction], right: Sequence[Instruction],
                       audit_notes: Mapping[tuple[int, int], str]) -> tuple[list[dict[str, Any]], int, str]:
    rows: list[dict[str, Any]] = []
    if len(left) == len(right):
        for left_index, (a, b) in enumerate(zip(left, right)):
            status, note = _paired_status(a, b, audit_notes)
            rows.append({"status": status, "left": left_index, "right": left_index, "note": note})
        return rows, 0, "lockstep"

    bounded = (len(left) <= _MAX_ALIGNMENT_INSTRUCTIONS
               and len(right) <= _MAX_ALIGNMENT_INSTRUCTIONS
               and len(left) * len(right) <= _MAX_ALIGNMENT_PRODUCT)
    if not bounded:
        common = min(len(left), len(right))
        anchor = common
        for index in range(common):
            status, note = _paired_status(left[index], right[index], audit_notes)
            rows.append({"status": status, "left": index, "right": index, "note": note})
            if anchor == common and _shape(left[index]) != _shape(right[index]):
                anchor = index
        for index in range(common, len(left)):
            rows.append({"status": "deleted", "left": index, "right": None, "note": None})
        for index in range(common, len(right)):
            rows.append({"status": "inserted", "left": None, "right": index, "note": None})
        return rows, min(anchor, max(0, len(rows) - 1)), "bounded_lockstep"

    matcher = SequenceMatcher(None, [_shape(value) for value in left],
                              [_shape(value) for value in right], autojunk=False)
    anchor: int | None = None
    for tag, left_start, left_end, right_start, right_end in matcher.get_opcodes():
        if tag == "equal":
            for left_index, right_index in zip(range(left_start, left_end), range(right_start, right_end)):
                status, note = _paired_status(left[left_index], right[right_index], audit_notes)
                rows.append({"status": status, "left": left_index, "right": right_index, "note": note})
            continue
        if anchor is None:
            anchor = len(rows)
        paired = min(left_end - left_start, right_end - right_start)
        for delta in range(paired):
            rows.append({"status": "changed", "left": left_start + delta,
                         "right": right_start + delta, "note": "instruction differs"})
        for left_index in range(left_start + paired, left_end):
            rows.append({"status": "deleted", "left": left_index, "right": None, "note": None})
        for right_index in range(right_start + paired, right_end):
            rows.append({"status": "inserted", "left": None, "right": right_index, "note": None})
    return rows, anchor or 0, "structural_sequence"


def _instruction_difference(original_rva: int, rebuilt_rva: int,
                            original_va: int, rebuilt_va: int,
                            left: DecodedProcedure, right: DecodedProcedure,
                            reason: str, mismatch_offset: int | None,
                            audit: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    notes = _audit_notes(audit)
    pairs, structural_anchor, method = _instruction_pairs(left.instructions, right.instructions, notes)
    anchor = structural_anchor
    if mismatch_offset is not None:
        left_index = next((index for index, instruction in enumerate(left.instructions)
                           if instruction.offset <= mismatch_offset < instruction.offset + instruction.size), None)
        if left_index is not None:
            anchor = next((index for index, row in enumerate(pairs) if row["left"] == left_index), anchor)
    if pairs and mismatch_offset is not None:
        pairs[anchor]["status"] = "changed"
        pairs[anchor]["note"] = reason
    start = max(0, anchor - _CONTEXT)
    end = min(len(pairs), anchor + _CONTEXT + 1)
    rows: list[dict[str, Any]] = []
    for index, pair in enumerate(pairs[start:end], start):
        left_index = pair["left"]
        right_index = pair["right"]
        row: dict[str, Any] = {
            "status": pair["status"],
            "focus": index == anchor,
            "original": (None if left_index is None else _instruction_record(
                left.instructions[left_index], left_index, original_rva, original_va)),
            "rebuilt": (None if right_index is None else _instruction_record(
                right.instructions[right_index], right_index, rebuilt_rva, rebuilt_va)),
        }
        if pair.get("note") is not None:
            row["note"] = pair["note"]
        rows.append(row)
    return {
        "kind": "instructions",
        "reason": reason,
        "original_instruction_count": len(left.instructions),
        "rebuilt_instruction_count": len(right.instructions),
        "alignment_method": method,
        "omitted_before": start,
        "omitted_after": len(pairs) - end,
        "rows": rows,
    }


def _raw_record(raw: bytes, offset: int, size: int, rva: int, va: int,
                directive: str, text: str) -> dict[str, Any]:
    return {
        "function_offset": offset,
        "rva": rva + offset,
        "va": va + offset,
        "bytes_hex": raw[offset:offset + size].hex(),
        "directive": directive,
        "text": text,
    }


def _data_token_rows(raw: bytes, rva: int, va: int, offset: int,
                     token: tuple[Any, ...] | None) -> list[dict[str, Any]]:
    if token is None:
        return []
    if token[0] == "jump_table":
        dispatches = list(token[1])
        targets = token[2]
        rows = []
        for index, target in enumerate(targets):
            item_offset = offset + index * 4
            record = _raw_record(raw, item_offset, 4, rva, va, "dd", f"dd instruction[{target}]")
            record["target_instruction_indexes"] = [target]
            record["dispatch_instruction_indexes"] = dispatches
            rows.append(record)
        return rows
    if token[0] == "table_alignment":
        size = int(token[2])
    else:
        size = len(token[1])
    rows = []
    for item_offset in range(offset, offset + size, _DATA_WIDTH):
        width = min(_DATA_WIDTH, offset + size - item_offset)
        values = raw[item_offset:item_offset + width]
        text = "db " + ", ".join(f"0x{value:02x}" for value in values)
        rows.append(_raw_record(raw, item_offset, width, rva, va, "db", text))
    return rows


def _data_difference(original: bytes, rebuilt: bytes,
                     original_rva: int, rebuilt_rva: int,
                     original_va: int, rebuilt_va: int,
                     left: DecodedProcedure, right: DecodedProcedure,
                     reason: str, audit: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    left_tokens = located_data_tokens(left)
    right_tokens = located_data_tokens(right)
    left_values = [token for _, token in left_tokens]
    right_values = [token for _, token in right_tokens]
    if "jump-table" in reason:
        left_indexes = [index for index, token in enumerate(left_values) if token[0] == "jump_table"]
        right_indexes = [index for index, token in enumerate(right_values) if token[0] == "jump_table"]
        pair_index = next((index for index, pair in enumerate(zip(left_indexes, right_indexes))
                           if left_values[pair[0]] != right_values[pair[1]]),
                          min(len(left_indexes), len(right_indexes)))
        left_token_index = left_indexes[pair_index] if pair_index < len(left_indexes) else None
        right_token_index = right_indexes[pair_index] if pair_index < len(right_indexes) else None
    else:
        allow_alignment_resize = any(item.get("kind") == "macro"
                                     and "size_original" in item and "size_rebuilt" in item for item in audit)
        left_token_index = right_token_index = None
        for index, (a, b) in enumerate(zip(left_values, right_values)):
            equal = a == b
            if (not equal and allow_alignment_resize and a[0] == b[0] == "table_alignment"
                    and (a[1] == b[1] or a[2] == 0 or b[2] == 0)):
                equal = True
            if not equal:
                left_token_index = right_token_index = index
                break
        if left_token_index is None:
            common = min(len(left_values), len(right_values))
            left_token_index = common if common < len(left_values) else None
            right_token_index = common if common < len(right_values) else None
    left_offset, left_token = ((left_tokens[left_token_index]) if left_token_index is not None else (0, None))
    right_offset, right_token = ((right_tokens[right_token_index]) if right_token_index is not None else (0, None))
    left_rows = _data_token_rows(original, original_rva, original_va, left_offset, left_token)
    right_rows = _data_token_rows(rebuilt, rebuilt_rva, rebuilt_va, right_offset, right_token)
    pairs: list[tuple[str, dict[str, Any] | None, dict[str, Any] | None]] = []
    common = min(len(left_rows), len(right_rows))
    jump_pair = bool(left_token is not None and right_token is not None
                     and left_token[0] == right_token[0] == "jump_table")
    for index in range(common):
        if jump_pair:
            status = "context" if left_token[2][index] == right_token[2][index] else "changed"
        else:
            status = "context" if (left_rows[index]["bytes_hex"] == right_rows[index]["bytes_hex"]
                                   and left_rows[index]["text"] == right_rows[index]["text"]) else "changed"
        pairs.append((status, left_rows[index], right_rows[index]))
    pairs.extend(("deleted", value, None) for value in left_rows[common:])
    pairs.extend(("inserted", None, value) for value in right_rows[common:])
    focus = next((index for index, value in enumerate(pairs) if value[0] != "context"), None)
    dispatch_mismatch = bool(jump_pair and left_token[1] != right_token[1])
    if dispatch_mismatch and pairs:
        status, left_record, right_record = pairs[0]
        pairs[0] = ("changed", left_record, right_record)
        focus = 0
    if focus is None:
        focus = 0
    start = max(0, focus - _CONTEXT)
    end = min(len(pairs), focus + _CONTEXT + 1)
    rows = []
    for index, (status, left_record, right_record) in enumerate(pairs[start:end], start):
        row: dict[str, Any] = {"status": status, "focus": index == focus,
                               "original": left_record, "rebuilt": right_record}
        if index == focus:
            row["note"] = ("jump-table dispatch instruction indexes differ"
                           if dispatch_mismatch
                           else reason)
        elif (jump_pair and status == "context" and left_record is not None and right_record is not None
              and left_record["bytes_hex"] != right_record["bytes_hex"]):
            row["note"] = "raw address differs; semantic target instruction index matches"
        rows.append(row)
    return {
        "kind": "data",
        "reason": reason,
        "original_instruction_count": len(left.instructions),
        "rebuilt_instruction_count": len(right.instructions),
        "alignment_method": "semantic_data_token",
        "omitted_before": start,
        "omitted_after": len(pairs) - end,
        "rows": rows,
    }


def disassembly_difference(original: bytes, rebuilt: bytes,
                           original_rva: int, rebuilt_rva: int,
                           original_va: int, rebuilt_va: int,
                           left: DecodedProcedure, right: DecodedProcedure,
                           reason: str, mismatch_offset: int | None,
                           audit: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    if reason in {"embedded data or padding differs", "jump-table case order or target semantics differ"}:
        return _data_difference(original, rebuilt, original_rva, rebuilt_rva,
                                original_va, rebuilt_va, left, right, reason, audit)
    return _instruction_difference(original_rva, rebuilt_rva,
                                   original_va, rebuilt_va, left, right,
                                   reason, mismatch_offset, audit)
