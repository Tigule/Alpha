
from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Iterable, Mapping, Sequence

from .decode import (
    DataRegion,
    DecodeFailure,
    DecodeUnavailable,
    DecodedProcedure,
    Instruction,
    JumpTable,
    SelectorTable,
    decode_procedure,
    require_decoder,
)


_MACRO_SINKS = {
    "SErrPrepareAppFatal",
    "SErrDisplayError",
    "SErrDisplayErrorFmt",
    "SMemAlloc",
    "SMemFree",
    "SStrDupA",
}


@dataclass(frozen=True)
class Evidence:
    offset: int
    size: int
    kind: str
    identity: str | None
    resolved: bool = False
    detail: Mapping[str, Any] | None = None

    @classmethod
    def from_record(cls, value: Any) -> "Evidence":
        def field(name: str, default: Any = None) -> Any:
            if isinstance(value, Mapping):
                return value.get(name, default)
            return getattr(value, name, default)

        return cls(
            offset=int(field("offset")),
            size=int(field("size")),
            kind=str(field("kind")),
            identity=field("target_identity", field("identity")),
            resolved=bool(field("resolved", False)),
            detail=field("detail"),
        )


@dataclass(frozen=True)
class NormalizationResult:
    matched: bool
    category: str
    reason: str
    audit: tuple[Mapping[str, Any], ...] = ()
    mismatch_offset: int | None = None


def decode_x86(code: bytes, address: int) -> tuple[Instruction, ...]:
    return decode_procedure(code, address).instructions


def _index_evidence(records: Iterable[Any]) -> dict[tuple[int, int], Evidence]:
    result: dict[tuple[int, int], Evidence] = {}
    for raw in records:
        evidence = Evidence.from_record(raw)
        key = (evidence.offset, evidence.size)
        if key in result:
            
            result[key] = Evidence(evidence.offset, evidence.size, "ambiguous", None, False)
        else:
            result[key] = evidence
    return result


def _field_evidence(
    index: Mapping[tuple[int, int], Evidence], instruction: Instruction, field: tuple[int, int, str, int]
) -> Evidence | None:
    local_offset, size, _, _ = field
    return index.get((instruction.offset + local_offset, size))


def _same_shape(left: Instruction, right: Instruction) -> bool:
    if left.mnemonic != right.mnemonic or len(left.operands) != len(right.operands):
        return False
    for a, b in zip(left.operands, right.operands):
        if a[0] != b[0]:
            return False
        if a[0] == "reg" and a != b:
            return False
        if a[0] == "imm" and a[2] != b[2]:
            return False
        if a[0] == "mem" and (a[1:5] + a[6:]) != (b[1:5] + b[6:]):
            return False
        if a[0] == "unsupported":
            return False
    return True


def _plausible_source_file(value: Any) -> bool:
    if not isinstance(value, str) or not value or "\0" in value:
        return False
    normalized = value.replace("\\", "/").lower()
    return normalized.endswith((".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl"))


def _valid_macro_evidence(value: Evidence) -> bool:
    detail = value.detail or {}
    sink = str(detail.get("sink", "")).split("(", 1)[0].split("::")[-1]
    if sink not in _MACRO_SINKS or detail.get("paired_call_verified") is not True:
        return False
    if value.kind == "macro_line":
        line = detail.get("value")
        return isinstance(line, int) and line > 0 and _plausible_source_file(detail.get("paired_file"))
    if value.kind == "macro_file":
        return _plausible_source_file(detail.get("value")) and isinstance(detail.get("paired_line"), int) and detail["paired_line"] > 0
    return False


def _semantic_identity_matches(left: Evidence, right: Evidence,
                               left_instructions: Sequence[Instruction],
                               right_instructions: Sequence[Instruction]) -> bool:
    prefix = "intra-function:+0x"
    internal = bool(left.identity and right.identity and
                    (left.identity.startswith(prefix) or right.identity.startswith(prefix)))
    if not internal:
        return left.identity == right.identity
    if not (left.identity.startswith(prefix) and right.identity.startswith(prefix)):
        return False
    try:
        left_offset = int(left.identity[len(prefix):], 16)
        right_offset = int(right.identity[len(prefix):], 16)
    except ValueError:
        return False
    left_index = next((i for i, instruction in enumerate(left_instructions) if instruction.offset == left_offset), None)
    right_index = next((i for i, instruction in enumerate(right_instructions) if instruction.offset == right_offset), None)
    return left_index is not None and left_index == right_index


def _matching_macro_pair(left: Evidence | None, right: Evidence | None) -> bool:
    return bool(left and right and left.resolved and right.resolved and left.identity
                and left.identity == right.identity and left.kind == right.kind
                and left.kind in {"macro_file", "macro_line"}
                and _valid_macro_evidence(left) and _valid_macro_evidence(right))


def _jump_table_evidence(procedure: DecodedProcedure) -> list[Evidence]:
    instruction_indexes = {instruction.offset: index for index, instruction in enumerate(procedure.instructions)}
    result: list[Evidence] = []
    for table in procedure.jump_tables:
        instruction = procedure.instructions[instruction_indexes[table.instruction_offset]]
        field = next((field for field in instruction.fields if field[3] == table.operand_index
                      and field[2] == "displacement"), None)
        if field is None:
            continue
        local_offset, size, _, _ = field
        result.append(Evidence(
            instruction.offset + local_offset,
            size,
            "jump_table",
            f"switch-dispatch:{instruction_indexes[instruction.offset]}",
            True,
        ))
    return result


def _selector_table_evidence(procedure: DecodedProcedure) -> list[Evidence]:
    instruction_indexes = {instruction.offset: index for index, instruction in enumerate(procedure.instructions)}
    result: list[Evidence] = []
    for selector in procedure.selector_tables:
        instruction = procedure.instructions[instruction_indexes[selector.instruction_offset]]
        field = next((field for field in instruction.fields if field[3] == selector.operand_index
                      and field[2] == "displacement"), None)
        if field is None:
            continue
        local_offset, size, _, _ = field
        result.append(Evidence(
            instruction.offset + local_offset,
            size,
            "selector_table",
            f"switch-selector:{instruction_indexes[selector.dispatch_instruction_offset]}",
            True,
        ))
    return result


def located_data_tokens(procedure: DecodedProcedure) -> tuple[tuple[int, tuple[Any, ...]], ...]:
    instruction_indexes = {instruction.offset: index for index, instruction in enumerate(procedure.instructions)}
    table_groups: dict[int, list[JumpTable]] = {}
    for table in procedure.jump_tables:
        table_groups.setdefault(table.offset, []).append(table)
    tables = {offset: values[0] for offset, values in table_groups.items()}
    instruction_ends = {instruction.offset + instruction.size for instruction in procedure.instructions}
    alignments: dict[int, tuple[int, int | None, int]] = {}
    for table in procedure.jump_tables:
        candidates = [end for end in instruction_ends if 0 <= table.offset - end < 4]
        if not candidates:
            continue
        start = max(candidates)
        raw = next((region.raw[start - region.offset:table.offset - region.offset]
                    for region in procedure.data_regions
                    if region.offset <= start <= table.offset <= region.offset + len(region.raw)), b"")
        if len(raw) != table.offset - start or any(byte not in {0x90, 0xCC} for byte in raw):
            continue
        fill = raw[0] if raw and all(byte == raw[0] for byte in raw) else None
        if raw and fill is None:
            continue
        alignments[start] = (table.offset, fill, len(raw))
    table_bytes = {
        byte
        for table in procedure.jump_tables
        for byte in range(table.offset, table.offset + table.entry_size * len(table.targets))
    }
    tokens: list[tuple[int, tuple[Any, ...]]] = []
    emitted_alignments: set[int] = set()
    for region in procedure.data_regions:
        cursor = region.offset
        end = region.offset + len(region.raw)
        while cursor < end:
            alignment = alignments.get(cursor)
            if alignment is not None and cursor not in emitted_alignments:
                table_offset, fill, size = alignment
                tokens.append((cursor, ("table_alignment", fill, size)))
                emitted_alignments.add(cursor)
                cursor = table_offset
                continue
            table = tables.get(cursor)
            if table is not None:
                targets = tuple(instruction_indexes[target] for target in table.targets)
                dispatches = tuple(sorted(instruction_indexes[value.instruction_offset]
                                          for value in table_groups[table.offset]))
                tokens.append((cursor, ("jump_table", dispatches, targets)))
                cursor += table.entry_size * len(table.targets)
                continue
            start = cursor
            while cursor < end and cursor not in tables:
                if cursor in table_bytes:
                    raise DecodeFailure(f"partial jump table in data partition at byte {cursor}")
                cursor += 1
            tokens.append((start, ("data", region.raw[start - region.offset:cursor - region.offset])))
    return tuple(tokens)


def _data_tokens(procedure: DecodedProcedure) -> tuple[tuple[Any, ...], ...]:
    return tuple(token for _, token in located_data_tokens(procedure))


def _compare_data_tokens(left: tuple[tuple[Any, ...], ...], right: tuple[tuple[Any, ...], ...],
                         allow_alignment_resize: bool) -> tuple[bool, list[Mapping[str, Any]]]:
    if len(left) != len(right):
        return False, []
    audit: list[Mapping[str, Any]] = []
    for a, b in zip(left, right):
        if a == b:
            continue
        if (allow_alignment_resize and a[0] == b[0] == "table_alignment"
                and (a[1] == b[1] or a[2] == 0 or b[2] == 0)):
            audit.append({"kind": "table_alignment", "original_size": a[2], "rebuilt_size": b[2],
                          "fill": a[1] if a[2] else b[1]})
            continue
        return False, []
    return True, audit


def compare_code(
    original: bytes,
    rebuilt: bytes,
    original_address: int,
    rebuilt_address: int,
    original_evidence: Sequence[Any] = (),
    rebuilt_evidence: Sequence[Any] = (),
    original_macro_evidence: Sequence[Any] = (),
    rebuilt_macro_evidence: Sequence[Any] = (),
    original_decoded: DecodedProcedure | None = None,
    rebuilt_decoded: DecodedProcedure | None = None,
) -> NormalizationResult:
    raw_bytes_equal = original == rebuilt
    try:
        left_procedure = original_decoded or decode_procedure(original, original_address)
        right_procedure = rebuilt_decoded or decode_procedure(rebuilt, rebuilt_address)
        left = left_procedure.instructions
        right = right_procedure.instructions
    except DecodeUnavailable:
        raise
    except DecodeFailure as exc:
        return NormalizationResult(False, "unsupported", str(exc))
    if len(left) != len(right):
        return NormalizationResult(
            False,
            "mismatch",
            "instruction counts differ; width changes require aligned, proven macro instructions",
        )
    reloc_left = _index_evidence(original_evidence)
    reloc_right = _index_evidence(rebuilt_evidence)
    for value in _jump_table_evidence(left_procedure):
        reloc_left[(value.offset, value.size)] = value
    for value in _jump_table_evidence(right_procedure):
        reloc_right[(value.offset, value.size)] = value
    for value in _selector_table_evidence(left_procedure):
        reloc_left[(value.offset, value.size)] = value
    for value in _selector_table_evidence(right_procedure):
        reloc_right[(value.offset, value.size)] = value
    macro_left = _index_evidence(original_macro_evidence)
    macro_right = _index_evidence(rebuilt_macro_evidence)
    audit: list[Mapping[str, Any]] = []
    used_macro = False
    used_macro_width_change = False
    for a, b in zip(left, right):
        if not _same_shape(a, b):
            return NormalizationResult(False, "mismatch", "instruction semantics differ", tuple(audit), a.offset)
        if len(a.fields) != len(b.fields):
            return NormalizationResult(False, "mismatch", "encoded field layout differs", tuple(audit), a.offset)
        mutable_a = bytearray(a.raw)
        mutable_b = bytearray(b.raw)
        for field_a, field_b in zip(a.fields, b.fields):
            oa, sa, kind_a, _ = field_a
            ob, sb, kind_b, _ = field_b
            if sa != sb or kind_a != kind_b:
                macro_a = _field_evidence(macro_left, a, field_a)
                macro_b = _field_evidence(macro_right, b, field_b)
                exact_push_width_change = (
                    a.mnemonic == b.mnemonic == "push"
                    and len(a.operands) == len(b.operands) == 1
                    and a.operands[0][0] == b.operands[0][0] == "imm"
                    and {a.raw[0], b.raw[0]} == {0x6A, 0x68}
                    and {len(a.raw), len(b.raw)} == {2, 5}
                    and oa == ob == 1
                )
                if not (
                    kind_a == kind_b
                    and len(a.fields) == len(b.fields) == 1
                    and exact_push_width_change
                    and _matching_macro_pair(macro_a, macro_b)
                ):
                    return NormalizationResult(False, "mismatch", "encoded field widths differ", tuple(audit), a.offset)
                used_macro = True
                used_macro_width_change = True
                audit.append({"kind": "macro", "instruction_offset_original": a.offset,
                              "instruction_offset_rebuilt": b.offset, "field": kind_a,
                              "size_original": sa, "size_rebuilt": sb, "identity": macro_a.identity,
                              "sink": (macro_a.detail or {}).get("sink"),
                              "original_value": (macro_a.detail or {}).get("value"),
                              "rebuilt_value": (macro_b.detail or {}).get("value")})
                mutable_a = mutable_b
                continue
            bytes_match = mutable_a[oa : oa + sa] == mutable_b[ob : ob + sb]
            semantic_a = _field_evidence(reloc_left, a, field_a)
            semantic_b = _field_evidence(reloc_right, b, field_b)
            if bytes_match and (semantic_a or semantic_b):
                semantic_matches = bool(
                    semantic_a
                    and semantic_b
                    and semantic_a.resolved
                    and semantic_b.resolved
                    and semantic_a.identity
                    and _semantic_identity_matches(semantic_a, semantic_b, left, right)
                    and semantic_a.kind == semantic_b.kind
                )
                if not semantic_matches:
                    macro_a = _field_evidence(macro_left, a, field_a)
                    macro_b = _field_evidence(macro_right, b, field_b)
                    if _matching_macro_pair(macro_a, macro_b):
                        used_macro = True
                        audit.append({"kind": "macro", "instruction_offset_original": a.offset,
                                      "instruction_offset_rebuilt": b.offset, "field": kind_a,
                                      "size": sa, "identity": macro_a.identity,
                                      "sink": (macro_a.detail or {}).get("sink"),
                                      "original_value": (macro_a.detail or {}).get("value"),
                                      "rebuilt_value": (macro_b.detail or {}).get("value")})
                        continue
                    return NormalizationResult(
                        False,
                        "mismatch",
                        "equal encoding resolves to different semantic targets",
                        tuple(audit),
                        a.offset,
                    )
            if bytes_match:
                continue
            ea = semantic_a
            eb = semantic_b
            category = "relocation"
            if not (ea and eb and ea.resolved and eb.resolved and ea.identity
                    and _semantic_identity_matches(ea, eb, left, right) and ea.kind == eb.kind):
                ea = _field_evidence(macro_left, a, field_a)
                eb = _field_evidence(macro_right, b, field_b)
                category = "macro"
                if not (
                    ea
                    and eb
                    and ea.resolved
                    and eb.resolved
                    and ea.identity
                    and ea.identity == eb.identity
                    and ea.kind == eb.kind
                    and ea.kind in {"macro_file", "macro_line"}
                    and _valid_macro_evidence(ea)
                    and _valid_macro_evidence(eb)
                ):
                    return NormalizationResult(
                        False, "mismatch", "changed encoded value lacks matching resolved evidence", tuple(audit), a.offset
                    )
                used_macro = True
            mutable_a[oa : oa + sa] = b"\0" * sa
            mutable_b[ob : ob + sb] = b"\0" * sb
            audit.append(
                {
                    "kind": category,
                    "instruction_offset_original": a.offset,
                    "instruction_offset_rebuilt": b.offset,
                    "field": kind_a,
                    "size": sa,
                    "identity": ea.identity,
                    **({"sink": (ea.detail or {}).get("sink"),
                        "original_value": (ea.detail or {}).get("value"),
                        "rebuilt_value": (eb.detail or {}).get("value")} if category == "macro" else {}),
                }
            )
        if mutable_a != mutable_b:
            return NormalizationResult(False, "mismatch", "non-normalizable opcode bytes differ", tuple(audit), a.offset)
    left_data = _data_tokens(left_procedure)
    right_data = _data_tokens(right_procedure)
    data_match, data_audit = _compare_data_tokens(left_data, right_data, used_macro_width_change)
    if not data_match:
        left_tables = [token for token in left_data if token[0] == "jump_table"]
        right_tables = [token for token in right_data if token[0] == "jump_table"]
        reason = "jump-table case order or target semantics differ" if left_tables != right_tables else "embedded data or padding differs"
        return NormalizationResult(False, "mismatch", reason, tuple(audit))
    audit.extend(data_audit)
    for token in left_data:
        if token[0] == "jump_table":
            audit.append({"kind": "jump_table", "dispatch_instruction_indexes": list(token[1]),
                          "case_target_instruction_indexes": list(token[2])})
    category = "macro_normalized" if used_macro else ("exact" if raw_bytes_equal else "relocation_normalized")
    return NormalizationResult(True, category, "all changed fields have matching resolved evidence", tuple(audit))
