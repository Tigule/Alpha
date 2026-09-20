
from __future__ import annotations

from collections import defaultdict
from dataclasses import asdict, dataclass
from hashlib import sha256
from pathlib import Path
import math
import posixpath
from typing import Any, Mapping, Sequence

from .model import ArtifactData
from .compiland_policy import apply_compiland_policy
from .decode import DecodedProcedure, decode_procedure
from .disassembly import disassembly_difference
from .input_snapshot import capture_inputs
from .normalize import DecodeFailure, DecodeUnavailable, Evidence, compare_code, decode_x86
from .symbols import Compiland, FunctionSymbol, PDBData, parse_pdb


SCHEMA_VERSION = "validate/v1"
NORMALIZATION_POLICY_VERSION = "x86-evidence/v1"


@dataclass(frozen=True)
class _Function:
    rva: int
    size: int
    symbols: tuple[FunctionSymbol, ...]
    identities: tuple[str, ...]
    compiland: str
    owner_contexts: tuple[tuple[str, str, tuple[str, ...]], ...] = ()


def _file_offset(artifact: ArtifactData, rva: int) -> int | None:
    for section in artifact.sections:
        offset = rva - section.rva
        if 0 <= offset < min(section.raw_size, len(section.data)):
            return section.raw_offset + offset
    return None


def _raw_difference(original_artifact: ArtifactData, original_rva: int, original: bytes,
                    rebuilt_artifact: ArtifactData, rebuilt_rva: int, rebuilt: bytes) -> dict[str, Any]:
    common = min(len(original), len(rebuilt))
    first = next((offset for offset in range(common) if original[offset] != rebuilt[offset]), None)
    if first is None and len(original) != len(rebuilt):
        first = common
    equal = first is None

    def side(artifact: ArtifactData, function_rva: int, raw: bytes) -> dict[str, Any]:
        offset = None if equal else first
        difference_rva = None if offset is None else function_rva + offset
        function_file_offset = _file_offset(artifact, function_rva)
        return {
            "size": len(raw),
            "sha256": sha256(raw).hexdigest(),
            "function_rva": function_rva,
            "function_file_offset": function_file_offset,
            "difference_rva": difference_rva,
            "difference_file_offset": (None if offset is None or function_file_offset is None
                                       else function_file_offset + offset),
            "window_start_function_offset": offset,
            "window_hex": "" if offset is None or offset >= len(raw) else raw[offset:offset + 16].hex(),
        }

    return {
        "equal": equal,
        "first_difference_function_offset": first,
        "original": side(original_artifact, original_rva, original),
        "rebuilt": side(rebuilt_artifact, rebuilt_rva, rebuilt),
    }


def _whole_image_raw_difference(original: bytes, original_sha256: str,
                                rebuilt: bytes, rebuilt_sha256: str) -> dict[str, Any]:
    common = min(len(original), len(rebuilt))
    first = next((offset for offset in range(common) if original[offset] != rebuilt[offset]), None)
    if first is None and len(original) != len(rebuilt):
        first = common
    equal = first is None

    def side(raw: bytes, digest: str) -> dict[str, Any]:
        return {
            "size": len(raw),
            "sha256": digest,
            "window_start_file_offset": first,
            "window_hex": "" if first is None or first >= len(raw) else raw[first:first + 16].hex(),
        }

    return {
        "equal": equal,
        "first_difference_file_offset": first,
        "original": side(original, original_sha256),
        "rebuilt": side(rebuilt, rebuilt_sha256),
    }


def _pct(numerator: int, denominator: int) -> float | None:
    if denominator == 0:
        return None
    value = 100.0 * numerator / denominator
    return value if math.isfinite(value) else None


def _metric(total_count: int, total_bytes: int, matched_count: int, matched_bytes: int) -> dict[str, Any]:
    return {"total_count": total_count, "matched_count": matched_count,
            "count_percent": _pct(matched_count, total_count), "total_bytes": total_bytes,
            "matched_bytes": matched_bytes, "byte_percent": _pct(matched_bytes, total_bytes)}


def _basename(path: str) -> str:
    return path.replace("\\", "/").rstrip("/").rsplit("/", 1)[-1].lower()


def _object_key(path: str) -> str:
    name = _basename(path)
    for suffix in (".cpp.obj", ".cxx.obj", ".cc.obj", ".c.obj"):
        if name.endswith(suffix):
            name = name[: -len(suffix)] + ".obj"
            break
    return name.rsplit(":", 1)[-1]


def _archive_key(path: str) -> str | None:
    normalized = posixpath.normpath(path.replace("\\", "/")).lower()
    if normalized.endswith(".lib"):
        return normalized[:-4]
    separator = normalized.rfind(":")
    drive_separator = 1 if len(normalized) >= 3 and normalized[1:3] == ":/" else -1
    if separator >= 0 and separator != drive_separator:
        archive = normalized[:separator]
        return archive[:-4] if archive.endswith(".lib") else (archive or None)
    return None


def _owner_key(path: str) -> str:
    normalized = posixpath.normpath(path.replace("\\", "/")).lower()
    for suffix in (".cpp.obj", ".cxx.obj", ".cc.obj", ".c.obj"):
        if normalized.endswith(suffix):
            normalized = normalized[: -len(suffix)] + ".obj"
            break
    return normalized


def _member_key(path: str) -> str:
    normalized = _owner_key(path)
    separator = normalized.rfind(":")
    drive_separator = 1 if len(normalized) >= 3 and normalized[1:3] == ":/" else -1
    return normalized[separator + 1:] if separator >= 0 and separator != drive_separator else normalized


def _archive_matches(map_archive: str | None, pdb_archive: str | None) -> bool:
    if map_archive is None:
        return True
    if pdb_archive is None:
        return False
    if "/" in map_archive and "/" in pdb_archive:
        return map_archive == pdb_archive
    return map_archive.rsplit("/", 1)[-1] == pdb_archive.rsplit("/", 1)[-1]


def _member_matches(map_member: str, pdb_member: str) -> bool:
    if "/" in map_member and "/" in pdb_member:
        return map_member == pdb_member
    return map_member.rsplit("/", 1)[-1] == pdb_member.rsplit("/", 1)[-1]


def _source_path_key(path: str) -> str | None:
    normalized = posixpath.normpath(path.replace("\\", "/")).lower()
    parts = [part for part in normalized.split("/") if part and part != "."]
    if len(parts) < 2 or normalized in {".", ".."}:
        return None
    anchors = [index for index in range(len(parts) - 1)
               if parts[index] in {"wow", "engine", "storm"} and parts[index + 1] == "source"]
    if len(anchors) > 1:
        return None
    if anchors:
        return "/".join(parts[anchors[0]:])
    return normalized


def _compiland_owner_contexts(
    pdb: PDBData,
) -> dict[str, set[tuple[str | None, str, tuple[str, ...]]]]:
    result: dict[str, set[tuple[str | None, str, tuple[str, ...]]]] = defaultdict(set)
    for compiland in pdb.compilands:
        module_member = _member_key(compiland.module_name)
        object_member = _member_key(compiland.object_name) if compiland.object_name else ""
        member = module_member if module_member.endswith(".obj") else object_member
        source_files = [source for source in compiland.source_files
                        if _basename(source).endswith((".c", ".cc", ".cpp", ".cxx"))]
        source_keys = [_source_path_key(source) for source in source_files]
        sources = (() if any(key is None for key in source_keys)
                   else tuple(sorted(set(source_keys))))
        if member and sources:
            result[compiland.module_name].add((_archive_key(compiland.object_name), member, sources))
    return result


def _map_owner_identity(name: str, owner: str, sources: tuple[str, ...]) -> str:
    source_digest = sha256("\0".join(sources).encode("utf-8")).hexdigest()
    return f"map-owner:{_owner_key(owner)}:{source_digest}:{name}"


def _compiland_key(compiland: Compiland) -> str:
    translation_units = sorted({_basename(source) for source in compiland.source_files
                                if _basename(source).endswith((".c", ".cc", ".cpp", ".cxx"))})
    if len(translation_units) == 1:
        return "source:" + translation_units[0]
    return "object:" + _object_key(compiland.object_name or compiland.module_name)


def _resolve_compiland(pdb: PDBData, selector: str) -> tuple[Compiland, str]:
    requested = selector.strip().replace("\\", "/").casefold()
    if not requested:
        raise ValueError("--compiland must not be empty")

    grouped: dict[str, list[Compiland]] = defaultdict(list)
    for compiland in pdb.compilands:
        grouped[_compiland_key(compiland).casefold()].append(compiland)

    identity = requested
    occurrence: int | None = None
    if requested.startswith(("source:", "object:")):
        head, separator, suffix = requested.rpartition("#")
        if separator and suffix.isdecimal():
            identity, occurrence = head, int(suffix)

    candidates: list[tuple[Compiland, str]] = []
    if identity.startswith(("source:", "object:")):
        matches = grouped.get(identity, [])
        if occurrence is not None:
            if occurrence < len(matches):
                candidates = [(matches[occurrence], f"{identity}#{occurrence}")]
        else:
            candidates = [(value, f"{identity}#{index}") for index, value in enumerate(matches)]
    else:
        normalized = posixpath.normpath(identity)
        for key, values in grouped.items():
            for index, value in enumerate(values):
                sources = [posixpath.normpath(source.replace("\\", "/")).casefold()
                           for source in value.source_files]
                source_matches = []
                for source in sources:
                    if "/" not in normalized:
                        source_matches.append(_basename(source) == normalized)
                    else:
                        source_matches.append(source == normalized or source.endswith("/" + normalized))
                if any(source_matches):
                    candidates.append((value, f"{key}#{index}"))
        if not candidates and "/" not in normalized and normalized.endswith(".obj"):
            key = "object:" + normalized
            candidates = [(value, f"{key}#{index}") for index, value in enumerate(grouped.get(key, []))]

    candidates = list({(module.module_name, canonical): (module, canonical)
                       for module, canonical in candidates}.values())
    if not candidates:
        raise ValueError(
            f"no original PDB compiland matches {selector!r}; use a source path or a source:/object: identity"
        )
    if len(candidates) > 1:
        choices = ", ".join(canonical for _, canonical in candidates)
        raise ValueError(f"compiland selector {selector!r} is ambiguous; choose one of: {choices}")
    return candidates[0]


def _module_keys(pdb: PDBData) -> dict[str, str]:
    grouped: dict[str, set[str]] = defaultdict(set)
    for compiland in pdb.compilands:
        grouped[compiland.module_name].add(_compiland_key(compiland))
    return {module: next(iter(keys)) for module, keys in grouped.items() if len(keys) == 1}


def _symbol_identity(symbol: FunctionSymbol, module_keys: Mapping[str, str]) -> str | None:
    if not symbol.name:
        return None
    if symbol.kind == "local":
        key = module_keys.get(symbol.module)
        return None if key is None else f"local:{key}:{symbol.name}"
    return f"global:{symbol.name}"


def _section_rva(artifact: ArtifactData, section_number: int, offset: int) -> int | None:
    for section in artifact.sections:
        if section.number == section_number and section.executable and 0 <= offset < section.virtual_size:
            return section.rva + offset
    return None


def _functions(artifact: ArtifactData, pdb: PDBData) -> tuple[list[_Function], list[dict[str, Any]]]:
    module_keys = _module_keys(pdb)
    module_owner_contexts = _compiland_owner_contexts(pdb)
    map_name_rvas: dict[str, set[int]] = defaultdict(set)
    map_names_at_rva: dict[int, set[str]] = defaultdict(set)
    map_symbols_at_rva: dict[int, list[Any]] = defaultdict(list)
    for map_symbol in artifact.map_symbols:
        map_symbols_at_rva[map_symbol.rva].append(map_symbol)
        if "static" not in map_symbol.flags:
            map_name_rvas[map_symbol.name].add(map_symbol.rva)
            map_names_at_rva[map_symbol.rva].add(map_symbol.name)
    grouped: dict[tuple[int, int], list[FunctionSymbol]] = defaultdict(list)
    rejected: list[dict[str, Any]] = []
    for symbol in pdb.functions:
        rva = _section_rva(artifact, symbol.section, symbol.offset)
        if rva is None or symbol.length <= 0 or artifact.read_rva(rva, symbol.length) is None:
            rejected.append({"name": symbol.name, "module": symbol.module,
                             "reason": "PDB procedure range is outside PE raw sections"})
        else:
            grouped[(rva, symbol.length)].append(symbol)
    functions: list[_Function] = []
    for (rva, size), symbols in sorted(grouped.items()):
        identities = {value for symbol in symbols if (value := _symbol_identity(symbol, module_keys))}
        identities.update(f"map:{name}" for name in map_names_at_rva.get(rva, ()) if len(map_name_rvas[name]) == 1)
        pdb_owner_contexts = {
            context
            for symbol in symbols
            for context in module_owner_contexts.get(symbol.module, ())
        }
        owner_contexts: set[tuple[str, str, tuple[str, ...]]] = set()
        for map_symbol in map_symbols_at_rva.get(rva, ()):
            owner = map_symbol.object_name
            if not owner:
                continue
            member = _member_key(owner)
            archive = _archive_key(owner)
            for pdb_archive, pdb_member, sources in pdb_owner_contexts:
                if not _member_matches(member, pdb_member) or not _archive_matches(archive, pdb_archive):
                    continue
                identity = _map_owner_identity(map_symbol.name, owner, sources)
                identities.add(identity)
                owner_contexts.add((identity, owner, sources))
        compilands = sorted({module_keys.get(symbol.module, "unknown") for symbol in symbols})
        functions.append(_Function(rva, size, tuple(symbols), tuple(sorted(identities)),
                                   "|".join(compilands), tuple(sorted(owner_contexts))))
    for left, right in zip(functions, functions[1:]):
        if right.rva < left.rva + left.size:
            rejected.append({"name": left.identities[0] if left.identities else f"0x{left.rva:x}",
                             "reason": f"distinct PDB procedure ranges overlap at RVA 0x{right.rva:x}"})
    return functions, rejected


def _index_functions(functions: Sequence[_Function]) -> dict[str, list[_Function]]:
    result: dict[str, list[_Function]] = defaultdict(list)
    for function in functions:
        for identity in function.identities:
            result[identity].append(function)
    return result


def _candidate(original: _Function, rebuilt_index: Mapping[str, list[_Function]],
               original_identity_counts: Mapping[str, int]) -> tuple[_Function | None, str | None, str | None, str | None]:
    strong = [identity for identity in original.identities if identity.startswith("map:")]
    candidates = {(function.rva, function.size): function for identity in strong
                  for function in rebuilt_index.get(identity, ())}
    if len(candidates) == 1:
        return next(iter(candidates.values())), None, "exact_decorated_map_name", None
    if len(candidates) != 1:
        if candidates:
            return None, "ambiguous", None, None
    owner_identities = [identity for identity in original.identities if identity.startswith("map-owner:")]
    owner = [identity for identity in owner_identities
             if original_identity_counts.get(identity) == 1
             and len(rebuilt_index.get(identity, ())) == 1]
    owner_ambiguous = any(
        rebuilt_index.get(identity)
        and (original_identity_counts.get(identity) != 1 or len(rebuilt_index[identity]) != 1)
        for identity in owner_identities
    )
    owner_candidates = {(function.rva, function.size): (function, identity) for identity in owner
                        for function in rebuilt_index.get(identity, ())}
    if len(owner_candidates) == 1:
        function, identity = next(iter(owner_candidates.values()))
        return function, None, "unique_owner_qualified_map_name", identity
    if len(owner_candidates) > 1:
        return None, "ambiguous", None, None
    fallback = [identity for identity in original.identities if not identity.startswith(("map:", "map-owner:"))
                and original_identity_counts.get(identity) == 1 and len(rebuilt_index.get(identity, ())) == 1]
    fallback_candidates = {(function.rva, function.size): function for identity in fallback
                           for function in rebuilt_index.get(identity, ())}
    if len(fallback_candidates) == 1:
        return next(iter(fallback_candidates.values())), None, "unique_pdb_name_and_compiland", None
    if len(fallback_candidates) > 1:
        return None, "ambiguous", None, None
    if owner_ambiguous:
        return None, "ambiguous", None, None
    return None, "missing", None, None


def _signature(original: _Function, rebuilt: _Function, original_pdb: PDBData,
               rebuilt_pdb: PDBData) -> tuple[str, str]:
    comparisons: list[tuple[bool | None, str]] = []
    seen: set[tuple[str, int]] = set()
    for left in original.symbols:
        if left.type_index is None:
            comparisons.append((None, f"alias {left.name}: missing original function type")); continue
        left_index = original_pdb.types.aliases.get(left.type_index, left.type_index)
        if (left.name, left_index) in seen:
            continue
        seen.add((left.name, left_index))
        right_indexes = {rebuilt_pdb.types.aliases.get(right.type_index, right.type_index)
                         for right in rebuilt.symbols if right.name == left.name and right.type_index is not None}
        if len(right_indexes) != 1:
            comparisons.append((None, f"alias {left.name}: rebuilt signature identity is not unique")); continue
        value = original_pdb.types.compare(left_index, rebuilt_pdb.types, next(iter(right_indexes)))
        comparisons.append((value.equal, f"alias {left.name}: {value.reason}"))
    if comparisons and all(equal is True for equal, _ in comparisons):
        return "matched", "all distinct alias signature graphs match"
    mismatches = [reason for equal, reason in comparisons if equal is False]
    if mismatches:
        return "mismatch", mismatches[0]
    return "unsupported", comparisons[0][1] if comparisons else "no comparable function type"


def _display_name(function: _Function) -> str:
    names = sorted({symbol.display_name or symbol.name for symbol in function.symbols if symbol.display_name or symbol.name})
    return names[0] if names else f"sub_{function.rva:08x}"


def _target_maps(artifact: ArtifactData, functions: Sequence[_Function]) -> tuple[dict[int, str], dict[int, tuple[str, ...]]]:
    identity_rvas: dict[str, set[int]] = defaultdict(set)
    for function in functions:
        for identity in function.identities:
            identity_rvas[identity].add(function.rva)
    function_targets: dict[int, str] = {}
    for function in functions:
        prior_identities = [identity for identity in function.identities if not identity.startswith("map-owner:")]
        if len(prior_identities) == 1 and len(identity_rvas[prior_identities[0]]) == 1:
            function_targets[function.rva] = prior_identities[0]
    targets: dict[int, list[str]] = defaultdict(list)
    name_rvas: dict[str, set[int]] = defaultdict(set)
    for symbol in artifact.map_symbols:
        if "static" not in symbol.flags:
            name_rvas[symbol.name].add(symbol.rva)
    for symbol in artifact.map_symbols:
        if "static" not in symbol.flags and len(name_rvas[symbol.name]) == 1:
            targets[symbol.rva].append(f"map:{symbol.name}")
    owner_rvas: dict[str, set[int]] = defaultdict(set)
    owner_at_rva: dict[int, set[str]] = defaultdict(set)
    for function in functions:
        for identity, _, _ in function.owner_contexts:
            owner_rvas[identity].add(function.rva)
            owner_at_rva[function.rva].add(identity)
    for rva, identities in owner_at_rva.items():
        if targets.get(rva):
            continue
        unique = [identity for identity in identities if len(owner_rvas[identity]) == 1]
        if len(unique) == 1:
            targets[rva].append(unique[0])
    return function_targets, {rva: tuple(sorted(set(values))) for rva, values in targets.items()}


def _read_string(artifact: ArtifactData, rva: int) -> str | None:
    for section in artifact.sections:
        offset = rva - section.rva
        if offset < 0 or offset >= min(section.raw_size, len(section.data)) or section.executable:
            continue
        raw = section.data[offset:offset + 512]; end = raw.find(b"\0")
        if end < 3 or any(byte < 0x20 or byte > 0x7E for byte in raw[:end]):
            return None
        return raw[:end].decode("ascii")
    return None


def _resolve_target(artifact: ArtifactData, function_targets: Mapping[int, str],
                    map_targets: Mapping[int, tuple[str, ...]], current: _Function,
                    value: int) -> tuple[str | None, bool]:
    rva = artifact.resolve_va(value)
    if rva is None:
        return None, False
    if current.rva <= rva < current.rva + current.size:
        return f"intra-function:+0x{rva - current.rva:x}", True
    if rva in artifact.imports:
        return "import:" + artifact.imports[rva], True
    if rva in function_targets:
        return function_targets[rva], True
    names = map_targets.get(rva, ())
    if len(names) == 1:
        return names[0], True
    string = _read_string(artifact, rva)
    return (("string:" + string), True) if string is not None else (None, False)


def _code_evidence(artifact: ArtifactData, functions: Sequence[_Function], function: _Function,
                   code: bytes, function_targets: Mapping[int, str] | None = None,
                   map_targets: Mapping[int, tuple[str, ...]] | None = None,
                   decoded: DecodedProcedure | None = None) -> list[Evidence]:
    if function_targets is None or map_targets is None:
        function_targets, map_targets = _target_maps(artifact, functions)
    base = artifact.identity.image_base + function.rva
    result: list[Evidence] = []
    for instruction in (decoded or decode_procedure(code, base)).instructions:
        for local_offset, size, _, operand_index in instruction.fields:
            if not 0 <= operand_index < len(instruction.operands):
                continue
            operand = instruction.operands[operand_index]; value: int | None = None; kind = "address"
            if operand[0] == "imm" and instruction.mnemonic.startswith(("call", "j", "loop")):
                value, kind = int(operand[1]), "branch"
            elif operand[0] == "imm" and artifact.resolve_va(int(operand[1])) is not None:
                value = int(operand[1])
            elif operand[0] == "mem" and not operand[2] and not operand[3] and artifact.resolve_va(int(operand[5])) is not None:
                value = int(operand[5])
            if value is not None:
                identity, resolved = _resolve_target(artifact, function_targets, map_targets, function, value)
                result.append(Evidence(instruction.offset + local_offset, size, kind, identity, resolved))
    return result


_MACRO_ARGUMENTS = {
    "SErrPrepareAppFatal": (0, 1),
    "SErrDisplayError": (1, 2),
    "SErrDisplayErrorFmt": (1, 2),
    "SMemAlloc": (1, 2),
    "SMemFree": (1, 2),
    "SStrDupA": (1, 2),
}


def _macro_sink(target: str) -> str | None:
    for sink in _MACRO_ARGUMENTS:
        if target == f"global:{sink}" or target == f"map:{sink}" or target == f"map:_{sink}":
            return sink
        if target.startswith(f"map:?{sink}@@"):
            return sink
        stdcall = f"map:_{sink}@"
        if target.startswith(stdcall) and target[len(stdcall):].isdigit():
            return sink
    return None


def _macro_evidence(artifact: ArtifactData, functions: Sequence[_Function], function: _Function,
                    code: bytes, function_targets: Mapping[int, str],
                    map_targets: Mapping[int, tuple[str, ...]],
                    decoded: DecodedProcedure | None = None) -> list[Evidence]:
    base = artifact.identity.image_base + function.rva
    instructions = (decoded or decode_procedure(code, base)).instructions
    result: list[Evidence] = []
    ordinals: dict[str, int] = defaultdict(int)
    for call_index, call in enumerate(instructions):
        if not call.mnemonic.startswith("call") or not call.operands or call.operands[0][0] != "imm":
            continue
        target, resolved = _resolve_target(artifact, function_targets, map_targets, function, int(call.operands[0][1]))
        if not resolved or not target:
            continue
        sink = _macro_sink(target)
        if sink is None:
            continue
        ordinal = ordinals[sink]
        ordinals[sink] += 1
        pushes: list[Any] = []
        cursor = call_index - 1
        while cursor >= 0 and len(pushes) < 12:
            instruction = instructions[cursor]
            if instruction.mnemonic != "push":
                break
            pushes.append(instruction); cursor -= 1
        file_position, line_position = _MACRO_ARGUMENTS[sink]
        if max(file_position, line_position) >= len(pushes):
            continue
        file_instruction = pushes[file_position]
        line_instruction = pushes[line_position]
        if not (len(file_instruction.operands) == len(line_instruction.operands) == 1
                and file_instruction.operands[0][0] == line_instruction.operands[0][0] == "imm"
                and len(file_instruction.fields) == len(line_instruction.fields) == 1):
            continue
        file_field = file_instruction.fields[0]
        line_field = line_instruction.fields[0]
        file_value = int(file_instruction.operands[0][1]); line_value = int(line_instruction.operands[0][1])
        file_rva = artifact.resolve_va(file_value)
        source_file = _read_string(artifact, file_rva) if file_rva is not None else None
        if source_file is None or line_value <= 0 or not _basename(source_file).endswith((".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl")):
            continue
        common = f"macro:{sink}:{ordinal}"
        fo, fs, _, _ = file_field; lo, ls, _, _ = line_field
        result.append(Evidence(file_instruction.offset + fo, fs, "macro_file", common + ":file", True,
            {"sink": sink, "paired_call_verified": True, "value": source_file, "paired_line": line_value}))
        result.append(Evidence(line_instruction.offset + lo, ls, "macro_line", common + ":line", True,
            {"sink": sink, "paired_call_verified": True, "value": line_value, "paired_file": source_file}))
    return result


def _identity_report(artifact: ArtifactData, pdb: PDBData) -> dict[str, Any]:
    image = artifact.identity
    matches = (image.codeview_kind == "NB10" and image.codeview_signature is not None
               and image.codeview_signature == pdb.signature and image.codeview_age == pdb.age)
    return {"verified": matches,
            "reason": "PE NB10 signature and age match PDB info stream" if matches else "PE/PDB CodeView identity mismatch or unsupported",
            "pe": {"machine": image.machine, "image_base": image.image_base,
                   "timestamp": image.pe_timestamp, "codeview_kind": image.codeview_kind,
                   "codeview_signature": image.codeview_signature, "codeview_age": image.codeview_age,
                   "embedded_pdb_path": image.embedded_pdb_path},
            "pdb": {"container_version": pdb.container_version, "signature": pdb.signature, "age": pdb.age}}


def _compare_types(original: PDBData, rebuilt: PDBData) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    results: list[dict[str, Any]] = []; total = matched = 0
    for name in sorted(original.types.named_types):
        left_indexes = sorted({original.types.aliases.get(i, i) for i in original.types.named_types[name]})
        right_indexes = sorted({rebuilt.types.aliases.get(i, i) for i in rebuilt.types.named_types.get(name, ())})
        for occurrence, left in enumerate(left_indexes):
            total += 1; identity = f"type:{name}#{occurrence}"
            if not right_indexes:
                results.append({"identity": identity, "status": "missing", "reason": "named type absent from rebuilt PDB"}); continue
            comparisons = [(right, original.types.compare(left, rebuilt.types, right)) for right in right_indexes]
            equal = [(right, value) for right, value in comparisons if value.equal is True]
            if len(equal) == 1:
                matched += 1; results.append({"identity": identity, "status": "matched", "reason": equal[0][1].reason})
            elif len(equal) > 1:
                results.append({"identity": identity, "status": "ambiguous", "reason": "multiple rebuilt type graphs match"})
            elif any(value.equal is False for _, value in comparisons):
                reason = next(value.reason for _, value in comparisons if value.equal is False)
                results.append({"identity": identity, "status": "mismatch", "reason": reason})
            else:
                results.append({"identity": identity, "status": "unsupported", "reason": comparisons[0][1].reason})
    return results, {"total_count": total, "matched_count": matched, "count_percent": _pct(matched, total)}


def _compare_compilands(original: PDBData, rebuilt: PDBData) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    left: dict[str, list[Compiland]] = defaultdict(list); right: dict[str, list[Compiland]] = defaultdict(list)
    for value in original.compilands: left[_compiland_key(value)].append(value)
    for value in rebuilt.compilands: right[_compiland_key(value)].append(value)
    results: list[dict[str, Any]] = []; matched = 0
    for key in sorted(left):
        originals, candidates = left[key], right.get(key, [])
        for occurrence, a in enumerate(originals):
            identity = f"{key}#{occurrence}"
            if len(originals) != 1 or len(candidates) > 1:
                results.append({"identity": identity, "status": "ambiguous", "original_count": len(originals),
                                "rebuilt_count": len(candidates), "reason": "compiland key is not unique"})
            elif not candidates:
                results.append({"identity": identity, "status": "missing", "reason": "compiland absent from rebuilt PDB"})
            else:
                b = candidates[0]
                original_metadata = {"compiler": a.compiler, "language": a.language, "flags": dict(a.flags),
                                     "options": list(a.raw_options), "sources": sorted(map(_basename, a.source_files))}
                rebuilt_metadata = {"compiler": b.compiler, "language": b.language, "flags": dict(b.flags),
                                    "options": list(b.raw_options), "sources": sorted(map(_basename, b.source_files))}
                differences = [field for field in original_metadata if original_metadata[field] != rebuilt_metadata[field]]
                metadata_equal = not differences
                if metadata_equal:
                    matched += 1
                results.append({"identity": identity, "status": "matched" if metadata_equal else "metadata_mismatch",
                                "reason": "identity and compiler metadata match" if metadata_equal else "identity matched but compiler/source metadata differs",
                                "original_module": a.module_name, "rebuilt_module": b.module_name,
                                "differences": differences, "original_metadata": original_metadata,
                                "rebuilt_metadata": rebuilt_metadata})
    total = len(original.compilands)
    return results, {"total_count": total, "matched_count": matched, "count_percent": _pct(matched, total)}


def compare_loaded(original_artifact: ArtifactData, original_pdb: PDBData,
                   rebuilt_artifact: ArtifactData, rebuilt_pdb: PDBData, *,
                   strict: bool = False, compiland: str | None = None) -> dict[str, Any]:
    original_identity = _identity_report(original_artifact, original_pdb)
    rebuilt_identity = _identity_report(rebuilt_artifact, rebuilt_pdb)
    if original_artifact.fatal or rebuilt_artifact.fatal or original_pdb.fatal or rebuilt_pdb.fatal:
        raise ValueError("one or more input artifacts could not be parsed")
    if not original_identity["verified"] or not rebuilt_identity["verified"]:
        raise ValueError("PE/PDB identity verification failed")
    selected_compiland = _resolve_compiland(original_pdb, compiland) if compiland is not None else None
    original_functions, original_rejected = _functions(original_artifact, original_pdb)
    rebuilt_functions, rebuilt_rejected = _functions(rebuilt_artifact, rebuilt_pdb)
    if original_rejected or rebuilt_rejected:
        raise ValueError(
            f"invalid PDB procedure inventory: original rejected={len(original_rejected)}, "
            f"rebuilt rejected={len(rebuilt_rejected)}"
        )
    rebuilt_index = _index_functions(rebuilt_functions)
    original_identity_counts: dict[str, int] = defaultdict(int)
    for function in original_functions:
        for identity in function.identities:
            if not identity.startswith("map:"):
                original_identity_counts[identity] += 1
    rebuilt_by_range = {(function.rva, function.size): function for function in rebuilt_functions}
    left_image, right_image = original_artifact.identity, rebuilt_artifact.identity
    same_artifact = (
        (left_image.machine, left_image.image_base, left_image.pe_timestamp, left_image.codeview_kind,
         left_image.codeview_signature, left_image.codeview_age, left_image.embedded_pdb_path)
        ==
        (right_image.machine, right_image.image_base, right_image.pe_timestamp, right_image.codeview_kind,
         right_image.codeview_signature, right_image.codeview_age, right_image.embedded_pdb_path)
        and original_artifact.sections == rebuilt_artifact.sections
        and original_artifact.map_symbols == rebuilt_artifact.map_symbols
        and original_artifact.imports == rebuilt_artifact.imports
        and original_pdb == rebuilt_pdb
    )
    original_function_targets, original_map_targets = _target_maps(original_artifact, original_functions)
    rebuilt_function_targets, rebuilt_map_targets = _target_maps(rebuilt_artifact, rebuilt_functions)
    functions_to_compare = (original_functions if selected_compiland is None else
                            [function for function in original_functions
                             if any(symbol.module == selected_compiland[0].module_name
                                    for symbol in function.symbols)])
    function_results: list[dict[str, Any]] = []
    categories: dict[str, dict[str, int]] = defaultdict(lambda: {"count": 0, "bytes": 0})
    code_count = code_bytes = signature_count = signature_bytes = 0
    total_bytes = sum(function.size for function in functions_to_compare)
    per_compiland: dict[str, dict[str, int]] = defaultdict(lambda: {"total_count": 0, "total_bytes": 0,
        "code_matched_count": 0, "code_matched_bytes": 0, "signature_matched_count": 0, "signature_matched_bytes": 0})
    for original in functions_to_compare:
        cm = per_compiland[original.compiland]; cm["total_count"] += 1; cm["total_bytes"] += original.size
        if same_artifact:
            rebuilt, selection, matching_basis, matching_identity = (
                rebuilt_by_range.get((original.rva, original.size)), None,
                "identical_parsed_function_range", None
            )
            if rebuilt is None: selection = "missing"
        else:
            rebuilt, selection, matching_basis, matching_identity = _candidate(
                original, rebuilt_index, original_identity_counts
            )
        identity = original.identities[0] if len(original.identities) == 1 else f"range:0x{original.rva:x}"
        result: dict[str, Any] = {"identity": identity, "aliases": list(original.identities),
                                  "display_name": _display_name(original), "compiland": original.compiland,
                                  "original": {"rva": original.rva, "size": original.size}}
        if rebuilt is None:
            category = selection or "unsupported"
            result.update({"category": category, "code_matched": False, "signature_status": "unsupported",
                           "reason": f"rebuilt function selection is {category}"})
        else:
            result["rebuilt"] = {"rva": rebuilt.rva, "size": rebuilt.size}
            result["matching_basis"] = matching_basis
            if matching_identity is not None:
                result["matching_evidence"] = {
                    "identity": matching_identity,
                    "original_owners": sorted(owner for identity, owner, _ in original.owner_contexts
                                              if identity == matching_identity),
                    "rebuilt_owners": sorted(owner for identity, owner, _ in rebuilt.owner_contexts
                                             if identity == matching_identity),
                    "source_context": sorted({source for identity, _, sources in original.owner_contexts
                                              if identity == matching_identity for source in sources}),
                }
            signature_status, signature_reason = _signature(original, rebuilt, original_pdb, rebuilt_pdb)
            left_code = original_artifact.read_rva(original.rva, original.size)
            right_code = rebuilt_artifact.read_rva(rebuilt.rva, rebuilt.size)
            left_decoded = right_decoded = None
            outcome = None
            if left_code is None or right_code is None:
                category, reason, audit, code_matched = "unsupported", "procedure bytes unavailable", (), False
            else:
                result["raw_difference"] = _raw_difference(
                    original_artifact, original.rva, left_code,
                    rebuilt_artifact, rebuilt.rva, right_code,
                )
                try:
                    left_address = original_artifact.identity.image_base + original.rva
                    right_address = rebuilt_artifact.identity.image_base + rebuilt.rva
                    left_decoded = decode_procedure(left_code, left_address)
                    right_decoded = decode_procedure(right_code, right_address)
                    if same_artifact:
                        left_evidence = right_evidence = ()
                        left_macro = right_macro = ()
                    else:
                        left_evidence = _code_evidence(original_artifact, original_functions, original, left_code,
                                                    original_function_targets, original_map_targets, left_decoded)
                        right_evidence = _code_evidence(rebuilt_artifact, rebuilt_functions, rebuilt, right_code,
                                                     rebuilt_function_targets, rebuilt_map_targets, right_decoded)
                        left_macro = _macro_evidence(original_artifact, original_functions, original, left_code,
                                                  original_function_targets, original_map_targets, left_decoded)
                        right_macro = _macro_evidence(rebuilt_artifact, rebuilt_functions, rebuilt, right_code,
                                                   rebuilt_function_targets, rebuilt_map_targets, right_decoded)
                    outcome = compare_code(left_code, right_code, left_address, right_address,
                        left_evidence, right_evidence, left_macro, right_macro,
                        left_decoded, right_decoded)
                    category, reason, audit, code_matched = outcome.category, outcome.reason, outcome.audit, outcome.matched
                    if outcome.mismatch_offset is not None: result["mismatch_offset"] = outcome.mismatch_offset
                except (DecodeFailure, DecodeUnavailable) as exc:
                    category, reason, audit, code_matched = "unsupported", str(exc), (), False
            result.update({"category": category, "code_matched": code_matched, "signature_status": signature_status,
                           "signature_reason": signature_reason, "reason": reason, "audit": list(audit)})
            if (category == "mismatch" and outcome is not None
                    and left_decoded is not None and right_decoded is not None
                    and left_code is not None and right_code is not None):
                result["disassembly_difference"] = disassembly_difference(
                    left_code, right_code, original.rva, rebuilt.rva,
                    original_artifact.identity.image_base + original.rva,
                    rebuilt_artifact.identity.image_base + rebuilt.rva,
                    left_decoded, right_decoded, reason, outcome.mismatch_offset, audit,
                )
            if code_matched:
                code_count += 1; code_bytes += original.size; cm["code_matched_count"] += 1; cm["code_matched_bytes"] += original.size
                if signature_status == "matched":
                    signature_count += 1; signature_bytes += original.size
                    cm["signature_matched_count"] += 1; cm["signature_matched_bytes"] += original.size
        categories[result["category"]]["count"] += 1; categories[result["category"]]["bytes"] += original.size
        function_results.append(result)
    if selected_compiland is None:
        type_results, type_metrics = _compare_types(original_pdb, rebuilt_pdb)
    else:
        type_results = []
        type_metrics = {"total_count": 0, "matched_count": 0, "count_percent": None}
    compiland_results, compiland_metrics = _compare_compilands(original_pdb, rebuilt_pdb)
    if selected_compiland is not None:
        selected_identity = selected_compiland[1]
        compiland_results = [row for row in compiland_results if row["identity"] == selected_identity]
        compiland_metrics = {
            "total_count": len(compiland_results),
            "matched_count": sum(row["status"] == "matched" for row in compiland_results),
            "count_percent": _pct(sum(row["status"] == "matched" for row in compiland_results),
                                  len(compiland_results)),
        }
    report = {"schema_version": SCHEMA_VERSION, "normalization_policy_version": NORMALIZATION_POLICY_VERSION,
            "identity": {"original": original_identity, "rebuilt": rebuilt_identity},
            "metrics": {"code": _metric(len(functions_to_compare), total_bytes, code_count, code_bytes),
                "code_and_signature": _metric(len(functions_to_compare), total_bytes, signature_count, signature_bytes),
                "executable_coverage": {"known_function_bytes": total_bytes,
                    "executable_raw_bytes": original_artifact.executable_code_bytes,
                    "percent": _pct(total_bytes, original_artifact.executable_code_bytes),
                    "note": ("selected compiland function bytes; bytes outside unique PDB procedure ranges are not counted as matched or mismatched"
                             if selected_compiland is not None else
                             "coverage only; bytes outside unique PDB procedure ranges are not counted as matched or mismatched")},
                "types": type_metrics, "compilands": compiland_metrics, "categories": dict(sorted(categories.items()))},
            "functions": function_results, "types": type_results, "compilands": compiland_results,
            "per_compiland": dict(sorted(per_compiland.items())),
            "diagnostics": {"original_artifact": [asdict(value) for value in original_artifact.diagnostics],
                "rebuilt_artifact": [asdict(value) for value in rebuilt_artifact.diagnostics],
                "original_pdb": original_pdb.diagnostics, "rebuilt_pdb": rebuilt_pdb.diagnostics,
                "original_rejected_procedures": original_rejected, "rebuilt_rejected_procedures": rebuilt_rejected},
            "limitations": [
                "The denominator is unique, readable original PDB procedure ranges; aliases are coalesced by RVA and length.",
                "Executable coverage is separate and does not claim validation outside PDB procedure ranges.",
                "The supplied images have stripped PE relocations; changed address fields require exact semantic target evidence.",
                "Macro normalization requires diagnosed known-sink argument-pair evidence; absent evidence remains a mismatch."]}
    result = apply_compiland_policy(report, strict=strict)
    if selected_compiland is not None:
        result["scope"] = {
            "kind": "compiland",
            "identity": selected_compiland[1],
            "selector": compiland,
            "named_types": "omitted; per-function signatures are compared",
        }
        result["limitations"].append(
            "Targeted scope compares only the selected compiland's procedures and metadata; the named-type catalog is omitted."
        )
    return result


def compare_artifacts(original_exe: str | Path, original_pdb_path: str | Path, original_map: str | Path,
                      rebuilt_exe: str | Path, rebuilt_pdb_path: str | Path, rebuilt_map: str | Path, *,
                      strict: bool = False, compiland: str | None = None) -> dict[str, Any]:
    from .artifacts import load_artifacts
    paths = [Path(value) for value in (original_exe, original_pdb_path, original_map,
                                        rebuilt_exe, rebuilt_pdb_path, rebuilt_map)]
    snapshots = capture_inputs(paths)
    original_exe_snapshot, original_pdb_snapshot, original_map_snapshot = snapshots[:3]
    rebuilt_exe_snapshot, rebuilt_pdb_snapshot, rebuilt_map_snapshot = snapshots[3:]
    report = compare_loaded(
        load_artifacts(
            original_exe_snapshot.path,
            original_map_snapshot.path,
            pe_data=original_exe_snapshot.data,
            map_data=original_map_snapshot.data,
        ),
        parse_pdb(original_pdb_snapshot.path, data=original_pdb_snapshot.data),
        load_artifacts(
            rebuilt_exe_snapshot.path,
            rebuilt_map_snapshot.path,
            pe_data=rebuilt_exe_snapshot.data,
            map_data=rebuilt_map_snapshot.data,
        ),
        parse_pdb(rebuilt_pdb_snapshot.path, data=rebuilt_pdb_snapshot.data),
        strict=strict,
        compiland=compiland,
    )
    report["inputs"] = {
        "original": {"exe": original_exe_snapshot.record(), "pdb": original_pdb_snapshot.record(),
                     "map": original_map_snapshot.record()},
        "rebuilt": {"exe": rebuilt_exe_snapshot.record(), "pdb": rebuilt_pdb_snapshot.record(),
                    "map": rebuilt_map_snapshot.record()},
    }
    report["whole_image_raw_difference"] = _whole_image_raw_difference(
        original_exe_snapshot.data,
        original_exe_snapshot.sha256,
        rebuilt_exe_snapshot.data,
        rebuilt_exe_snapshot.sha256,
    )
    return report


__all__ = ["NORMALIZATION_POLICY_VERSION", "SCHEMA_VERSION", "compare_artifacts", "compare_loaded"]
