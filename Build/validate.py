#!/usr/bin/env python3

from __future__ import annotations

import argparse
import bisect
import collections
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable, Iterator, Sequence


PDB2_SIGNATURE = b"Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0"
TYPE_INDEX_BEGIN = 0x1000

# CodeView type leaves used by VC6 PDBs.
LF_MODIFIER = 0x1001
LF_POINTER = 0x1002
LF_ARRAY = 0x1003
LF_CLASS = 0x1004
LF_STRUCTURE = 0x1005
LF_UNION = 0x1006
LF_ENUM = 0x1007
LF_PROCEDURE = 0x1008
LF_MFUNCTION = 0x1009
LF_VTSHAPE = 0x000A
LF_ARGLIST = 0x1201
LF_FIELDLIST = 0x1203
LF_BITFIELD = 0x1205
LF_METHODLIST = 0x1206

LF_ENUMERATE = 0x0403
LF_BCLASS = 0x1400
LF_VBCLASS = 0x1401
LF_IVBCLASS = 0x1402
LF_INDEX = 0x1404
LF_MEMBER = 0x1405
LF_STMEMBER = 0x1406
LF_METHOD = 0x1407
LF_NESTTYPE = 0x1408
LF_VFUNCTAB = 0x1409
LF_ONEMETHOD = 0x140B
LF_VFUNCOFF = 0x140C

CV_PROP_FORWARD_REF = 0x0080

# CodeView symbol records used for function locals. VC6 emits the *_ST
# variants, whose names are length-prefixed; the non-ST variants use
# null-terminated names.
S_REGISTER_ST = 0x1001
S_BPREL32_ST = 0x1006
S_REGREL32_ST = 0x100C
S_REGISTER = 0x1106
S_BPREL32 = 0x110B
S_REGREL32 = 0x1111

MAP_BASE = re.compile(r"Preferred load address is\s+([0-9A-Fa-f]+)", re.I)
MAP_SECTION = re.compile(
    r"^\s*(?P<segment>[0-9A-Fa-f]{4}):(?P<offset>[0-9A-Fa-f]{8})\s+"
    r"(?P<length>[0-9A-Fa-f]+)H\s+(?P<name>\S+)\s+(?P<class>\S+)",
    re.I,
)
MAP_PUBLIC = re.compile(
    r"^\s*(?P<segment>[0-9A-Fa-f]{4}):(?P<offset>[0-9A-Fa-f]{8})\s+"
    r"(?P<name>\S+)\s+(?P<address>[0-9A-Fa-f]{8})"
    r"(?P<tail>.*?\S+\.obj)\s*$",
    re.I,
)
DISASSEMBLY = re.compile(
    r"^\s*(?P<address>[0-9A-Fa-f]{8}):"
    r"(?P<bytes>(?:\s+[0-9A-Fa-f]{2})+)\s+"
    r"(?P<mnemonic>\S+)(?:\s+(?P<operand>.*?))?\s*$"
)
ADDRESS_TOKEN = re.compile(r"(?<![0-9A-Fa-f])([0-9A-Fa-f]{8})(?![0-9A-Fa-f])")
SOURCE_FILE = re.compile(
    rb"(?<![ -~])[ -~]{3,1024}\.(?:c|cc|cpp|cxx|h|hh|hpp|inl|ipp)\x00",
    re.I,
)


class ValidationError(RuntimeError):
    pass


@dataclass(frozen=True)
class TypeRecord:
    leaf: int
    payload: bytes


@dataclass(frozen=True)
class Procedure:
    name: str
    obj: str
    type_index: int
    segment: int
    offset: int
    length: int
    locals: tuple[str, ...] = ()


@dataclass
class MapSymbol:
    name: str
    obj: str
    address: int
    segment: int
    offset: int
    is_function: bool
    length: int | None = None


@dataclass(frozen=True)
class Instruction:
    address: int
    data: bytes
    mnemonic: str
    operand: str


@dataclass
class Difference:
    category: str
    key: str
    reference: str = ""
    actual: str = ""
    compiland: str = ""

    def as_dict(self) -> dict[str, str]:
        return {
            "category": self.category,
            "key": self.key,
            "compiland": self.compiland,
            "reference": self.reference,
            "actual": self.actual,
        }


@dataclass
class Comparison:
    title: str
    reference_total: int
    actual_total: int
    matched: int = 0
    differences: list[Difference] = field(default_factory=list)

    @property
    def mismatch_count(self) -> int:
        return len(self.differences)

    def as_dict(self) -> dict[str, Any]:
        counts = collections.Counter(item.category for item in self.differences)
        return {
            "title": self.title,
            "reference_total": self.reference_total,
            "actual_total": self.actual_total,
            "matched": self.matched,
            "difference_count": self.mismatch_count,
            "categories": dict(sorted(counts.items())),
            "differences": [item.as_dict() for item in self.differences],
        }


def pages_for(size: int, page_size: int) -> int:
    return (size + page_size - 1) // page_size


def read_stream(data: bytes, pages: Sequence[int], size: int, page_size: int) -> bytes:
    return b"".join(
        data[page * page_size:(page + 1) * page_size] for page in pages
    )[:size]


def normalize_object(name: str) -> str:
    name = name.replace("\\", "/").rsplit("/", 1)[-1]
    name = name.rsplit(":", 1)[-1].lower()
    for suffix in (".cpp.obj", ".cxx.obj", ".cc.obj", ".c.obj"):
        if name.endswith(suffix):
            return name[:-len(suffix)] + ".obj"
    return name


def procedure_identity(name: str) -> str:
    marker = "@?%"
    if marker in name:
        prefix = name.split(marker, 1)[0]
        anonymous = "`anonymous namespace'::"
        if prefix.startswith("??0"):
            class_name = prefix[3:]
            identity = f"{anonymous}{class_name}::{class_name}"
        elif prefix.startswith("??1"):
            class_name = prefix[3:]
            identity = f"{anonymous}{class_name}::~{class_name}"
        elif prefix.startswith("?"):
            components = prefix[1:].split("@")
            function = components[0]
            scopes = list(reversed(components[1:]))
            identity = anonymous + "::".join(scopes + [function])
        else:
            identity = name
    elif name.startswith("?"):
        end = name.find("@@")
        identity = name[:end + 2] if end >= 0 else name
    else:
        match = re.fullmatch(r"(_[^@]+)@\d+", name)
        identity = match.group(1) if match else name
    return normalize_line_template_arguments(identity)


LINE_TEMPLATE_ARGUMENTS = {
    "TSFixedArray_": 2,
    "TSGrowableArray_": 2,
}


def normalize_line_template_arguments(name: str) -> str:
    replacements = []
    for template_name, argument_index in LINE_TEMPLATE_ARGUMENTS.items():
        search_from = 0
        marker = template_name + "<"
        while True:
            marker_start = name.find(marker, search_from)
            if marker_start < 0:
                break
            args_start = marker_start + len(marker)
            depth = 0
            args_end = -1
            separators = []
            for position in range(args_start, len(name)):
                char = name[position]
                if char == "<":
                    depth += 1
                elif char == ">":
                    if depth == 0:
                        args_end = position
                        break
                    depth -= 1
                elif char == "," and depth == 0:
                    separators.append(position)
            if args_end < 0:
                break
            bounds = [args_start] + [position + 1 for position in separators]
            ends = separators + [args_end]
            if argument_index < len(bounds):
                value_start = bounds[argument_index]
                value_end = ends[argument_index]
                value = name[value_start:value_end]
                stripped = value.strip()
                if re.fullmatch(r"-?\d+", stripped):
                    leading = value[:len(value) - len(value.lstrip())]
                    trailing = value[len(value.rstrip()):]
                    replacements.append(
                        (value_start, value_end, leading + "__LINE__" + trailing)
                    )
            # Advance only past this marker's start so nested occurrences of
            # the same template name are discovered independently.
            search_from = marker_start + len(marker)
    for value_start, value_end, replacement in sorted(
        replacements, reverse=True
    ):
        name = name[:value_start] + replacement + name[value_end:]
    return name


def pascal_string(data: bytes, offset: int) -> tuple[str, int]:
    if offset >= len(data):
        raise ValidationError("truncated CodeView string")
    size = data[offset]
    end = offset + 1 + size
    if end > len(data):
        raise ValidationError("truncated CodeView string")
    return data[offset + 1:end].decode("latin1", "replace"), end


def numeric_leaf(data: bytes, offset: int) -> tuple[int | str, int]:
    if offset + 2 > len(data):
        raise ValidationError("truncated CodeView numeric leaf")
    leaf = struct.unpack_from("<H", data, offset)[0]
    if leaf < 0x8000:
        return leaf, offset + 2
    formats: dict[int, tuple[str, int]] = {
        0x8000: ("<b", 1),
        0x8001: ("<h", 2),
        0x8002: ("<H", 2),
        0x8003: ("<i", 4),
        0x8004: ("<I", 4),
        0x8009: ("<q", 8),
        0x800A: ("<Q", 8),
    }
    info = formats.get(leaf)
    if info is None:
        # Floating-point and variable-sized numeric leaves are uncommon in
        # layouts. Preserve their identity without guessing their width.
        return f"numeric-leaf:{leaf:#x}", len(data)
    fmt, size = info
    start = offset + 2
    if start + size > len(data):
        raise ValidationError("truncated CodeView numeric value")
    return struct.unpack_from(fmt, data, start)[0], start + size


class Pdb2:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if not self.data.startswith(PDB2_SIGNATURE):
            raise ValidationError(f"{path} is not a Microsoft PDB 2.0 file")
        self.streams = self._parse_streams()
        if len(self.streams) < 4:
            raise ValidationError(f"{path} has no TPI/DBI streams")
        self.types = TypeTable(self.streams[2])
        self.procedures = self._parse_procedures()

    def _parse_streams(self) -> list[bytes]:
        page_size, _, _, root_size, _ = struct.unpack_from(
            "<IHHII", self.data, len(PDB2_SIGNATURE)
        )
        root_page_count = pages_for(root_size, page_size)
        root_pages = struct.unpack_from(
            f"<{root_page_count}H", self.data, 60
        )
        root = read_stream(self.data, root_pages, root_size, page_size)
        stream_count = struct.unpack_from("<H", root, 0)[0]
        sizes = [
            struct.unpack_from("<I", root, 4 + index * 8)[0]
            for index in range(stream_count)
        ]
        position = 4 + stream_count * 8
        page_lists: list[Sequence[int]] = []
        for size in sizes:
            count = pages_for(size, page_size)
            page_lists.append(
                struct.unpack_from(f"<{count}H", root, position) if count else ()
            )
            position += count * 2
        return [
            read_stream(self.data, pages, size, page_size)
            for pages, size in zip(page_lists, sizes)
        ]

    def _modules(self) -> Iterator[tuple[str, int, int]]:
        dbi = self.streams[3]
        if len(dbi) < 64:
            return
        module_size = struct.unpack_from("<I", dbi, 24)[0]
        position = 64
        end = min(len(dbi), position + module_size)
        while position + 64 <= end:
            fixed = dbi[position:position + 64]
            stream, symbol_size = struct.unpack_from("<hI", fixed, 34)
            strings = dbi[position + 64:end]
            try:
                first_end = strings.index(0)
                object_start = first_end + 1
                second_end = strings.index(0, object_start)
            except ValueError:
                break
            obj = normalize_object(
                strings[object_start:second_end].decode("latin1", "replace")
            )
            record_size = 64 + second_end + 1
            position += (record_size + 3) & ~3
            yield obj, stream, symbol_size

    def _parse_procedures(self) -> list[Procedure]:
        procedures: dict[tuple[str, str, int, int], Procedure] = {}
        procedure_locals: dict[tuple[str, str, int, int], list[str]] = (
            collections.defaultdict(list)
        )
        for module_obj, stream_index, symbol_size in self._modules():
            if (
                stream_index < 0
                or stream_index >= len(self.streams)
                or symbol_size <= 0
            ):
                continue
            symbols = self.streams[stream_index][:symbol_size]
            obj = module_obj
            position = 0
            active_procedure: tuple[str, str, int, int] | None = None
            procedure_end = -1
            while position + 4 <= len(symbols):
                if active_procedure is not None and position >= procedure_end:
                    active_procedure = None
                record_size, record_type = struct.unpack_from(
                    "<HH", symbols, position
                )
                record_end = position + 2 + record_size
                if record_size < 2 or record_end > len(symbols):
                    break
                payload = symbols[position + 4:record_end]
                if record_type in (0x0009, 0x1101) and len(payload) >= 5:
                    # S_OBJNAME_ST / S_OBJNAME. The first dword is a signature.
                    raw = payload[4:]
                    if record_type == 0x0009 and raw:
                        size = raw[0]
                        raw = raw[1:1 + size]
                    else:
                        raw = raw.split(b"\0", 1)[0]
                    if raw:
                        obj = normalize_object(raw.decode("latin1", "replace"))
                elif record_type in (0x100A, 0x100B, 0x110F, 0x1110):
                    if len(payload) >= 36:
                        procedure_end = struct.unpack_from("<I", payload, 4)[0]
                        length = struct.unpack_from("<I", payload, 12)[0]
                        type_index = struct.unpack_from("<I", payload, 24)[0]
                        offset, segment = struct.unpack_from("<IH", payload, 28)
                        if record_type in (0x100A, 0x100B):
                            name_size = payload[35]
                            name = payload[36:36 + name_size].decode(
                                "latin1", "replace"
                            )
                        else:
                            name = payload[35:].split(b"\0", 1)[0].decode(
                                "latin1", "replace"
                            )
                        procedure = Procedure(
                            name, obj, type_index, segment, offset, length
                        )
                        active_procedure = (obj, name, segment, offset)
                        procedures[active_procedure] = procedure
                elif active_procedure is not None:
                    local_name = self._local_name(record_type, payload)
                    if local_name:
                        procedure_locals[active_procedure].append(local_name)
                position = record_end
        return [
            Procedure(
                procedure.name,
                procedure.obj,
                procedure.type_index,
                procedure.segment,
                procedure.offset,
                procedure.length,
                tuple(sorted(procedure_locals[key])),
            )
            for key, procedure in procedures.items()
        ]

    @staticmethod
    def _local_name(record_type: int, payload: bytes) -> str | None:
        if record_type == S_BPREL32_ST:
            if len(payload) < 9:
                return None
            # Positive EBP offsets are parameters; negative offsets are locals.
            if struct.unpack_from("<i", payload, 0)[0] >= 0:
                return None
            name, _ = pascal_string(payload, 8)
            return name
        if record_type == S_BPREL32:
            if len(payload) < 9 or struct.unpack_from("<i", payload, 0)[0] >= 0:
                return None
            return payload[8:].split(b"\0", 1)[0].decode("latin1", "replace")
        if record_type == S_REGISTER_ST:
            if len(payload) < 7:
                return None
            name, _ = pascal_string(payload, 6)
            return name
        if record_type == S_REGISTER:
            if len(payload) < 7:
                return None
            return payload[6:].split(b"\0", 1)[0].decode("latin1", "replace")
        if record_type == S_REGREL32_ST:
            if len(payload) < 11:
                return None
            name, _ = pascal_string(payload, 10)
            return name
        if record_type == S_REGREL32:
            if len(payload) < 11:
                return None
            return payload[10:].split(b"\0", 1)[0].decode("latin1", "replace")
        return None


class TypeTable:
    def __init__(self, stream: bytes):
        if len(stream) < 56:
            raise ValidationError("truncated PDB TPI stream")
        version, header_size, begin, end = struct.unpack_from("<IIII", stream, 0)
        if version != 19961031 or begin != TYPE_INDEX_BEGIN:
            raise ValidationError(
                f"unsupported PDB TPI version/index range: {version}/{begin:#x}"
            )
        self.records: dict[int, TypeRecord] = {}
        position = header_size
        index = begin
        while position + 4 <= len(stream) and index < end:
            size, leaf = struct.unpack_from("<HH", stream, position)
            record_end = position + 2 + size
            if size < 2 or record_end > len(stream):
                raise ValidationError("corrupt PDB TPI record")
            self.records[index] = TypeRecord(
                leaf, stream[position + 4:record_end]
            )
            position = record_end
            index += 1
        self._canonical_cache: dict[int, Any] = {}
        self._layout_cache: dict[int, Any] = {}
        self._field_cache: dict[int, tuple[Any, ...]] = {}
        self._method_cache: dict[int, tuple[Any, ...]] = {}

    def canonical(self, type_index: int, active: frozenset[int] = frozenset()) -> Any:
        if type_index < TYPE_INDEX_BEGIN:
            # CodeView can encode a 32-bit near pointer either as a simple
            # T_32P* type (0x04xx) or as an LF_POINTER with ptrtype=near32 and
            # size=4 (attributes 0x040a). VC6 emits both forms for equivalent
            # source declarations, including array parameters.
            if type_index & 0xF00 == 0x400:
                return (
                    "pointer",
                    0x40A,
                    ("primitive", f"{type_index & 0xFF:#06x}"),
                )
            return ("primitive", f"{type_index:#06x}")
        if type_index in self._canonical_cache:
            return self._canonical_cache[type_index]
        if type_index in active:
            return ("recursive",)
        record = self.records.get(type_index)
        if record is None:
            return ("missing-type",)
        next_active = active | {type_index}
        data = record.payload
        try:
            if record.leaf == LF_MODIFIER:
                underlying, attrs = struct.unpack_from("<IH", data)
                value = ("modifier", attrs, self.canonical(underlying, next_active))
            elif record.leaf == LF_POINTER:
                underlying, attrs = struct.unpack_from("<II", data)
                value = ("pointer", attrs, self.canonical(underlying, next_active))
            elif record.leaf == LF_ARRAY:
                element, index_type = struct.unpack_from("<II", data)
                size, position = numeric_leaf(data, 8)
                name, _ = pascal_string(data, position)
                value = (
                    "array",
                    size,
                    name,
                    self.canonical(element, next_active),
                    self.canonical(index_type, next_active),
                )
            elif record.leaf in (LF_CLASS, LF_STRUCTURE, LF_UNION, LF_ENUM):
                kind, name, _, _, _ = self._named_header(type_index)
                value = ("named", kind, name)
            elif record.leaf == LF_PROCEDURE:
                return_type = struct.unpack_from("<I", data, 0)[0]
                call, _, count = struct.unpack_from("<BBH", data, 4)
                args = struct.unpack_from("<I", data, 8)[0]
                value = (
                    "procedure",
                    call,
                    count,
                    self.canonical(return_type, next_active),
                    self._arguments(args, next_active),
                )
            elif record.leaf == LF_MFUNCTION:
                return_type, class_type, this_type = struct.unpack_from(
                    "<III", data, 0
                )
                call, _, count = struct.unpack_from("<BBH", data, 12)
                args, adjustment = struct.unpack_from("<Ii", data, 16)
                value = (
                    "member-function",
                    call,
                    count,
                    adjustment,
                    self.canonical(return_type, next_active),
                    self.canonical(class_type, next_active),
                    self.canonical(this_type, next_active),
                    self._arguments(args, next_active),
                )
            elif record.leaf == LF_BITFIELD:
                underlying = struct.unpack_from("<I", data, 0)[0]
                length, position = struct.unpack_from("<BB", data, 4)
                value = (
                    "bitfield",
                    length,
                    position,
                    self.canonical(underlying, next_active),
                )
            elif record.leaf == LF_VTSHAPE:
                value = ("vtshape", data.hex())
            else:
                value = ("leaf", f"{record.leaf:#06x}")
        except (struct.error, ValidationError):
            value = ("malformed", f"{record.leaf:#06x}")
        self._canonical_cache[type_index] = value
        return value

    def _arguments(self, type_index: int, active: frozenset[int]) -> tuple[Any, ...]:
        record = self.records.get(type_index)
        if not record or record.leaf != LF_ARGLIST or len(record.payload) < 4:
            return (self.canonical(type_index, active),)
        count = struct.unpack_from("<I", record.payload, 0)[0]
        available = (len(record.payload) - 4) // 4
        count = min(count, available)
        return tuple(
            self.canonical(value, active)
            for value in struct.unpack_from(f"<{count}I", record.payload, 4)
        )

    def _named_header(
        self, type_index: int
    ) -> tuple[str, str, int, int, int | str]:
        record = self.records[type_index]
        data = record.payload
        kinds = {
            LF_CLASS: "class",
            LF_STRUCTURE: "struct",
            LF_UNION: "union",
            LF_ENUM: "enum",
        }
        kind = kinds[record.leaf]
        count, properties = struct.unpack_from("<HH", data, 0)
        if record.leaf in (LF_CLASS, LF_STRUCTURE):
            field = struct.unpack_from("<I", data, 4)[0]
            size, position = numeric_leaf(data, 16)
        elif record.leaf == LF_UNION:
            field = struct.unpack_from("<I", data, 4)[0]
            size, position = numeric_leaf(data, 8)
        else:
            field = struct.unpack_from("<I", data, 8)[0]
            size = 0
            position = 12
        name, _ = pascal_string(data, position)
        name = normalize_line_template_arguments(name)
        return kind, name, count, properties, size

    def named_layouts(self) -> dict[tuple[str, str], Any]:
        candidates: dict[tuple[str, str], tuple[int, int]] = {}
        for index, record in self.records.items():
            if record.leaf not in (LF_CLASS, LF_STRUCTURE, LF_UNION, LF_ENUM):
                continue
            try:
                kind, name, count, properties, size = self._named_header(index)
            except (struct.error, ValidationError):
                continue
            if not name or properties & CV_PROP_FORWARD_REF:
                continue
            key = (kind, name)
            previous = candidates.get(key)
            if previous is None or count > previous[0]:
                candidates[key] = (count, index)
        # PDBs can contain equivalent duplicate definitions. Select the most
        # detailed definition before resolving any recursive field lists.
        return {
            key: self.layout(value[1]) for key, value in candidates.items()
        }

    def layout(self, type_index: int) -> Any:
        if type_index in self._layout_cache:
            return self._layout_cache[type_index]
        kind, name, count, _, size = self._named_header(type_index)
        record = self.records[type_index]
        if record.leaf in (LF_CLASS, LF_STRUCTURE, LF_UNION):
            field_index = struct.unpack_from("<I", record.payload, 4)[0]
        else:
            field_index = struct.unpack_from("<I", record.payload, 8)[0]
        members = self._field_list(field_index, frozenset())
        value = (kind, name, size, count, tuple(sorted(members, key=repr)))
        self._layout_cache[type_index] = value
        return value

    def _method_list(self, type_index: int) -> tuple[Any, ...]:
        if type_index in self._method_cache:
            return self._method_cache[type_index]
        record = self.records.get(type_index)
        if not record or record.leaf != LF_METHODLIST:
            return ()
        data = record.payload
        position = 0
        result = []
        while position + 8 <= len(data):
            attrs = struct.unpack_from("<H", data, position)[0]
            # VC6 PDB 2.0 method-list entries place two reserved bytes between
            # the attributes and the 32-bit type index.
            method_type = struct.unpack_from("<I", data, position + 4)[0]
            position += 8
            method_property = (attrs >> 2) & 7
            vtable_offset = None
            if method_property in (4, 6) and position + 4 <= len(data):
                vtable_offset = struct.unpack_from("<i", data, position)[0]
                position += 4
            result.append(
                (attrs, vtable_offset, self.canonical(method_type))
            )
            while position < len(data) and data[position] >= 0xF0:
                position += max(1, data[position] & 0x0F)
        # An LF_METHOD record describes an overload set. VC6 orders its
        # LF_METHODLIST entries by internal type-index allocation, which can
        # differ between otherwise equivalent builds.
        value = tuple(sorted(result, key=repr))
        self._method_cache[type_index] = value
        return value

    def _field_list(
        self, type_index: int, active: frozenset[int]
    ) -> list[Any]:
        if type_index in active:
            return []
        if type_index in self._field_cache:
            return list(self._field_cache[type_index])
        record = self.records.get(type_index)
        if not record or record.leaf != LF_FIELDLIST:
            return []
        data = record.payload
        position = 0
        result: list[Any] = []
        next_active = active | {type_index}
        while position < len(data):
            if data[position] >= 0xF0:
                position += max(1, data[position] & 0x0F)
                continue
            if position + 2 > len(data):
                break
            leaf = struct.unpack_from("<H", data, position)[0]
            position += 2
            try:
                if leaf == LF_MEMBER:
                    attrs = struct.unpack_from("<H", data, position)[0]
                    member_type = struct.unpack_from("<I", data, position + 2)[0]
                    offset, name_at = numeric_leaf(data, position + 6)
                    name, position = pascal_string(data, name_at)
                    result.append(
                        ("member", name, offset, attrs, self.canonical(member_type))
                    )
                elif leaf == LF_STMEMBER:
                    attrs = struct.unpack_from("<H", data, position)[0]
                    member_type = struct.unpack_from("<I", data, position + 2)[0]
                    name, position = pascal_string(data, position + 6)
                    result.append(
                        ("static", name, attrs, self.canonical(member_type))
                    )
                elif leaf == LF_BCLASS:
                    attrs = struct.unpack_from("<H", data, position)[0]
                    base_type = struct.unpack_from("<I", data, position + 2)[0]
                    offset, position = numeric_leaf(data, position + 6)
                    result.append(
                        ("base", offset, attrs, self.canonical(base_type))
                    )
                elif leaf in (LF_VBCLASS, LF_IVBCLASS):
                    attrs = struct.unpack_from("<H", data, position)[0]
                    base_type, vbptr_type = struct.unpack_from(
                        "<II", data, position + 2
                    )
                    vbptr_offset, position = numeric_leaf(data, position + 10)
                    vbtable_index, position = numeric_leaf(data, position)
                    result.append(
                        (
                            "virtual-base",
                            vbptr_offset,
                            vbtable_index,
                            attrs,
                            self.canonical(base_type),
                            self.canonical(vbptr_type),
                        )
                    )
                elif leaf == LF_ENUMERATE:
                    attrs = struct.unpack_from("<H", data, position)[0]
                    value, name_at = numeric_leaf(data, position + 2)
                    name, position = pascal_string(data, name_at)
                    result.append(("enumerator", name, value, attrs))
                elif leaf == LF_METHOD:
                    count = struct.unpack_from("<H", data, position)[0]
                    method_list = struct.unpack_from("<I", data, position + 2)[0]
                    name, position = pascal_string(data, position + 6)
                    name = normalize_line_template_arguments(name)
                    result.append(
                        ("methods", name, count, self._method_list(method_list))
                    )
                elif leaf == LF_ONEMETHOD:
                    attrs = struct.unpack_from("<H", data, position)[0]
                    method_type = struct.unpack_from("<I", data, position + 2)[0]
                    position += 6
                    method_property = (attrs >> 2) & 7
                    vtable_offset = None
                    if method_property in (4, 6):
                        vtable_offset = struct.unpack_from("<i", data, position)[0]
                        position += 4
                    name, position = pascal_string(data, position)
                    name = normalize_line_template_arguments(name)
                    result.append(
                        (
                            "method",
                            name,
                            attrs,
                            vtable_offset,
                            self.canonical(method_type),
                        )
                    )
                elif leaf == LF_INDEX:
                    # Two reserved bytes precede the continuation index.
                    continuation = struct.unpack_from("<I", data, position + 2)[0]
                    position += 6
                    result.extend(self._field_list(continuation, next_active))
                elif leaf == LF_NESTTYPE:
                    nested_type = struct.unpack_from("<I", data, position + 2)[0]
                    name, position = pascal_string(data, position + 6)
                    result.append(
                        ("nested", name, self.canonical(nested_type))
                    )
                elif leaf == LF_VFUNCTAB:
                    # VC6 emits two reserved bytes before the 32-bit type
                    # index in an LF_VFUNCTAB field-list record.
                    table_type = struct.unpack_from("<I", data, position + 2)[0]
                    position += 6
                    result.append(("vfunctab", self.canonical(table_type)))
                elif leaf == LF_VFUNCOFF:
                    table_type, offset = struct.unpack_from("<Ii", data, position)
                    position += 8
                    result.append(
                        ("vfuncoff", offset, self.canonical(table_type))
                    )
                else:
                    # Unknown variable-length subrecords cannot be skipped
                    # safely. Keep the parsed prefix rather than inventing data.
                    result.append(("unknown-field-leaf", f"{leaf:#06x}"))
                    break
            except (struct.error, ValidationError):
                result.append(("malformed-field-leaf", f"{leaf:#06x}"))
                break
        self._field_cache[type_index] = tuple(result)
        return result


def compact(value: Any, limit: int = 300) -> str:
    text = repr(value)
    return text if len(text) <= limit else text[:limit - 3] + "..."


def describe_locals(groups: Sequence[tuple[str, ...]]) -> str:
    count = sum(len(names) for names in groups)
    noun = "local" if count == 1 else "locals"
    formatted_groups = [list(names) for names in groups]
    if len(formatted_groups) == 1:
        text = f"{count} {noun}: {formatted_groups[0]}"
    else:
        text = (
            f"{count} {noun} across {len(groups)} overloads: "
            f"{formatted_groups}"
        )
    return text if len(text) <= 1000 else text[:997] + "..."


def compare_pdbs(reference: Pdb2, actual: Pdb2) -> tuple[Comparison, Comparison]:
    reference_groups: dict[tuple[str, str], list[Procedure]] = collections.defaultdict(list)
    actual_groups: dict[tuple[str, str], list[Procedure]] = collections.defaultdict(list)
    for procedure in reference.procedures:
        reference_groups[(procedure.obj, procedure_identity(procedure.name))].append(
            procedure
        )
    for procedure in actual.procedures:
        actual_groups[(procedure.obj, procedure_identity(procedure.name))].append(
            procedure
        )

    signature_result = Comparison(
        "Function signatures", len(reference_groups), len(actual_groups)
    )
    for key in sorted(set(reference_groups) | set(actual_groups)):
        expected = reference_groups.get(key, [])
        found = actual_groups.get(key, [])
        label = f"{key[0]}:{key[1]}"
        if not found:
            signature_result.differences.append(
                Difference("missing", label, compiland=key[0])
            )
            continue
        if not expected:
            signature_result.differences.append(
                Difference("extra", label, compiland=key[0])
            )
            continue
        expected_signatures = sorted(
            (
                procedure_identity(item.name),
                compact(reference.types.canonical(item.type_index), 1000),
            )
            for item in expected
        )
        actual_signatures = sorted(
            (
                procedure_identity(item.name),
                compact(actual.types.canonical(item.type_index), 1000),
            )
            for item in found
        )
        expected_local_sets = sorted(
            (
                compact(reference.types.canonical(item.type_index), 1000),
                item.locals,
            )
            for item in expected
        )
        actual_local_sets = sorted(
            (
                compact(actual.types.canonical(item.type_index), 1000),
                item.locals,
            )
            for item in found
        )
        matched = True
        if expected_signatures != actual_signatures:
            signature_result.differences.append(
                Difference(
                    "signature",
                    label,
                    compact(expected_signatures),
                    compact(actual_signatures),
                    key[0],
                )
            )
            matched = False
        if expected_local_sets != actual_local_sets:
            signature_result.differences.append(
                Difference(
                    "locals",
                    label,
                    describe_locals([item.locals for item in expected]),
                    describe_locals([item.locals for item in found]),
                    key[0],
                )
            )
            matched = False
        if matched:
            signature_result.matched += 1

    reference_layouts = reference.types.named_layouts()
    actual_layouts = actual.types.named_layouts()
    type_result = Comparison(
        "Classes/structures", len(reference_layouts), len(actual_layouts)
    )
    for key in sorted(set(reference_layouts) | set(actual_layouts)):
        label = f"{key[0]} {key[1]}"
        if key not in actual_layouts:
            type_result.differences.append(Difference("missing", label))
        elif key not in reference_layouts:
            type_result.differences.append(Difference("extra", label))
        elif reference_layouts[key] != actual_layouts[key]:
            type_result.differences.append(
                Difference(
                    "layout",
                    label,
                    compact(reference_layouts[key]),
                    compact(actual_layouts[key]),
                )
            )
        else:
            type_result.matched += 1
    return signature_result, type_result


class LinkerMap:
    def __init__(self, path: Path):
        self.path = path
        text = path.read_text(encoding="latin1")
        base_match = MAP_BASE.search(text)
        if not base_match:
            raise ValidationError(f"{path} has no preferred load address")
        self.image_base = int(base_match.group(1), 16)
        segment_bases: dict[int, int] = {}
        segment_ends: dict[int, int] = {}
        symbols: list[MapSymbol] = []
        in_publics = False
        for line in text.splitlines():
            section = MAP_SECTION.match(line)
            if section:
                segment = int(section["segment"], 16)
                offset = int(section["offset"], 16)
                length = int(section["length"], 16)
                # Segment base is learned exactly from public addresses below.
                segment_ends[segment] = max(
                    segment_ends.get(segment, 0), offset + length
                )
                continue
            if "Publics by Value" in line:
                in_publics = True
                continue
            if not in_publics:
                continue
            match = MAP_PUBLIC.match(line)
            if not match:
                continue
            tail = match["tail"].strip()
            object_match = re.search(r"(\S+\.obj)\s*$", tail, re.I)
            if not object_match:
                continue
            flags = tail[:object_match.start()].strip().lower().split()
            segment = int(match["segment"], 16)
            offset = int(match["offset"], 16)
            address = int(match["address"], 16)
            segment_bases.setdefault(segment, address - offset)
            symbols.append(
                MapSymbol(
                    match["name"],
                    normalize_object(object_match.group(1)),
                    address,
                    segment,
                    offset,
                    "f" in flags,
                )
            )
        self.symbols = sorted(symbols, key=lambda item: (item.address, item.name))
        self.functions = [item for item in self.symbols if item.is_function]
        unique_addresses = sorted({item.address for item in self.functions})
        for symbol in self.functions:
            position = bisect.bisect_right(unique_addresses, symbol.address)
            if position < len(unique_addresses):
                symbol.length = unique_addresses[position] - symbol.address
            else:
                base = segment_bases.get(symbol.segment)
                end = segment_ends.get(symbol.segment)
                if base is not None and end is not None:
                    symbol.length = base + end - symbol.address
        self._addresses = [item.address for item in self.symbols]

    def resolve(self, address: int) -> str | None:
        position = bisect.bisect_right(self._addresses, address) - 1
        if position < 0:
            return None
        symbol = self.symbols[position]
        delta = address - symbol.address
        if delta > 0x10000:
            return None
        return symbol.name if delta == 0 else f"{symbol.name}+{delta:#x}"


@dataclass(frozen=True)
class PeSection:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int


class PeImage:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if self.data[:2] != b"MZ":
            raise ValidationError(f"{path} is not a PE image")
        pe_offset = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe_offset:pe_offset + 4] != b"PE\0\0":
            raise ValidationError(f"{path} has no PE signature")
        coff = pe_offset + 4
        _, section_count, _, _, _, optional_size, _ = struct.unpack_from(
            "<HHIIIHH", self.data, coff
        )
        optional = coff + 20
        magic = struct.unpack_from("<H", self.data, optional)[0]
        if magic != 0x10B:
            raise ValidationError(f"{path} is not a 32-bit PE image")
        self.image_base = struct.unpack_from("<I", self.data, optional + 28)[0]
        self.image_size = struct.unpack_from("<I", self.data, optional + 56)[0]
        self.sections: list[PeSection] = []
        section_table = optional + optional_size
        for index in range(section_count):
            position = section_table + index * 40
            name = self.data[position:position + 8].split(b"\0", 1)[0].decode(
                "latin1", "replace"
            )
            virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
                "<IIII", self.data, position + 8
            )
            self.sections.append(
                PeSection(name, virtual_address, virtual_size, raw_offset, raw_size)
            )
        self.source_file_addresses = self._source_file_addresses()

    def contains_va(self, address: int) -> bool:
        return self.image_base <= address < self.image_base + self.image_size

    def section_for_va(self, address: int) -> PeSection | None:
        rva = address - self.image_base
        for section in self.sections:
            size = max(section.virtual_size, section.raw_size)
            if section.virtual_address <= rva < section.virtual_address + size:
                return section
        return None

    def _source_file_addresses(self) -> set[int]:
        result: set[int] = set()
        for section in self.sections:
            raw = self.data[
                section.raw_offset:section.raw_offset + section.raw_size
            ]
            # Prefix the section so the negative-lookbehind also works at zero.
            for match in SOURCE_FILE.finditer(b"\0" + raw):
                start = match.start() - 1
                result.add(
                    self.image_base + section.virtual_address + start
                )
        return result


def find_dumpbin(root: Path) -> Path:
    found = shutil.which("dumpbin")
    if found:
        return Path(found)
    candidates = []
    msvc_dir = os.environ.get("MSVCDir")
    if msvc_dir:
        candidates.append(Path(msvc_dir) / "Bin" / "DUMPBIN.EXE")
    candidates.extend(
        [
            root / "VC6" / "VC98" / "Bin" / "DUMPBIN.EXE",
            Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"))
            / "Microsoft Visual Studio 6.0"
            / "VC98"
            / "Bin"
            / "DUMPBIN.EXE",
        ]
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise ValidationError(
        "DUMPBIN.EXE was not found; run from VCVARS32 or provide --dumpbin"
    )


def disassemble(path: Path, dumpbin: Path) -> list[Instruction]:
    environment = os.environ.copy()
    visual_studio_root = dumpbin.parent.parent.parent
    tool_paths = [
        dumpbin.parent,
        visual_studio_root / "Common" / "MSDev98" / "Bin",
    ]
    environment["PATH"] = os.pathsep.join(
        [str(item) for item in tool_paths] + [environment.get("PATH", "")]
    )
    try:
        result = subprocess.run(
            [str(dumpbin), "/nologo", "/disasm", str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
            text=True,
            errors="replace",
            env=environment,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        detail = ""
        if isinstance(exc, subprocess.CalledProcessError) and exc.stderr:
            detail = f": {exc.stderr.strip()}"
        raise ValidationError(
            f"could not disassemble {path}: {exc}{detail}"
        ) from exc
    instructions = []
    for line in result.stdout.splitlines():
        match = DISASSEMBLY.match(line)
        if not match:
            continue
        instructions.append(
            Instruction(
                int(match["address"], 16),
                bytes.fromhex(match["bytes"]),
                match["mnemonic"].lower(),
                (match["operand"] or "").strip().lower(),
            )
        )
    if not instructions:
        raise ValidationError(f"DUMPBIN returned no instructions for {path}")
    return instructions


class InstructionIndex:
    def __init__(self, instructions: Sequence[Instruction]):
        self.instructions = list(instructions)
        self.addresses = [item.address for item in self.instructions]

    def range(self, start: int, length: int) -> list[Instruction]:
        first = bisect.bisect_left(self.addresses, start)
        end = bisect.bisect_left(self.addresses, start + length)
        return self.instructions[first:end]


def immediate_value(operand: str) -> int | None:
    token = operand.rsplit(",", 1)[-1].strip()
    if token.endswith("h") and re.fullmatch(r"[0-9a-f]+h", token):
        return int(token[:-1], 16)
    if re.fullmatch(r"\d+", token):
        return int(token)
    return None


def encoded_image_addresses(
    instruction: Instruction,
    image: PeImage,
) -> set[int]:
    return {
        struct.unpack_from("<I", instruction.data, offset)[0]
        for offset in range(max(0, len(instruction.data) - 3))
        if image.contains_va(
            struct.unpack_from("<I", instruction.data, offset)[0]
        )
    }


def canonical_instructions(
    instructions: Sequence[Instruction],
    function: MapSymbol,
    linker_map: LinkerMap,
    image: PeImage,
) -> tuple[str, ...]:
    # MAP symbol lengths include linker alignment between functions. Alignment
    # varies as preceding functions change and is not part of the function.
    instructions = list(instructions)
    while instructions and instructions[-1].mnemonic in ("nop", "int"):
        instructions.pop()

    file_indices: set[int] = set()
    line_indices: set[int] = set()
    for index, instruction in enumerate(instructions):
        addresses = {
            int(value, 16)
            for value in ADDRESS_TOKEN.findall(instruction.operand)
        }
        addresses.update(encoded_image_addresses(instruction, image))
        if addresses & image.source_file_addresses:
            file_indices.add(index)
    for file_index in file_indices:
        for index in range(max(0, file_index - 3), min(len(instructions), file_index + 4)):
            instruction = instructions[index]
            if instruction.mnemonic not in ("push", "mov"):
                continue
            value = immediate_value(instruction.operand)
            if value is not None and 0 < value <= 200000:
                line_indices.add(index)

    result = []
    for index, instruction in enumerate(instructions):
        # __FILE__ and __LINE__ are diagnostic metadata rather than program
        # logic. VC6 may load these arguments in either order, so retaining
        # placeholder instructions would still create a false mismatch.
        if index in file_indices or index in line_indices:
            continue
        operand = re.sub(r"\s+", " ", instruction.operand)
        encoded = [f"{value:02x}" for value in instruction.data]
        addresses = [int(value, 16) for value in ADDRESS_TOKEN.findall(
            instruction.operand
        )]
        relative_target = None
        if len(instruction.data) == 5 and instruction.data[0] in (0xE8, 0xE9):
            displacement = struct.unpack_from("<i", instruction.data, 1)[0]
            relative_target = instruction.address + 5 + displacement
        elif (
            len(instruction.data) == 6
            and instruction.data[0] == 0x0F
            and 0x80 <= instruction.data[1] <= 0x8F
        ):
            displacement = struct.unpack_from("<i", instruction.data, 2)[0]
            relative_target = instruction.address + 6 + displacement
        if (
            relative_target is not None
            and not (
                function.address
                <= relative_target
                < function.address + (function.length or 0)
            )
        ):
            encoded[-4:] = ["??"] * 4

        def replace_address(match: re.Match[str]) -> str:
            address = int(match.group(1), 16)
            if address in image.source_file_addresses:
                return "<__file__>"
            if function.address <= address < function.address + (function.length or 0):
                return f"<self+{address - function.address:#x}>"
            if image.contains_va(address):
                resolved = linker_map.resolve(address)
                return f"<symbol:{resolved}>" if resolved else "<image-address>"
            return match.group(0).lower()

        operand = ADDRESS_TOKEN.sub(replace_address, operand)
        for address in addresses:
            if not image.contains_va(address):
                continue
            absolute = struct.pack("<I", address)
            for byte_offset in range(max(0, len(instruction.data) - 3)):
                if instruction.data[byte_offset:byte_offset + 4] == absolute:
                    encoded[byte_offset:byte_offset + 4] = ["??"] * 4
            # Relative external calls/jumps encode a displacement rather than
            # the displayed VA. Internal branch displacements remain exact.
            is_relative_branch = (
                instruction.mnemonic == "call"
                or instruction.mnemonic == "jmp"
                or instruction.mnemonic.startswith("j")
            )
            is_internal = (
                function.address
                <= address
                < function.address + (function.length or 0)
            )
            if is_relative_branch and not is_internal and len(encoded) >= 5:
                encoded[-4:] = ["??"] * 4
        if index in line_indices:
            token = instruction.operand.rsplit(",", 1)[-1].strip()
            if token:
                operand = operand[: len(operand) - len(token)] + "<__line__>"
            # Preserve the instruction opcode/shape but ignore the macro value.
            if instruction.mnemonic == "push" and len(encoded) > 1:
                encoded[1:] = ["??"] * (len(encoded) - 1)
            elif instruction.mnemonic == "mov" and len(encoded) >= 4:
                encoded[-4:] = ["??"] * 4
        result.append(
            f"{''.join(encoded)} | {instruction.mnemonic} {operand}".rstrip()
        )
    return tuple(result)


def first_instruction_difference(
    reference: Sequence[str], actual: Sequence[str]
) -> tuple[str, str]:
    for expected, found in zip(reference, actual):
        if expected != found:
            return expected, found
    if len(reference) != len(actual):
        expected = reference[len(actual)] if len(reference) > len(actual) else "<end>"
        found = actual[len(reference)] if len(actual) > len(reference) else "<end>"
        return expected, found
    return "", ""


def unique_function_map(
    linker_map: LinkerMap,
) -> dict[tuple[str, str], MapSymbol]:
    grouped: dict[tuple[str, str], list[MapSymbol]] = collections.defaultdict(list)
    for symbol in linker_map.functions:
        grouped[(symbol.obj, symbol.name)].append(symbol)
    return {
        key: min(values, key=lambda item: item.address)
        for key, values in grouped.items()
    }


def compare_bytecode(
    reference_map: LinkerMap,
    actual_map: LinkerMap,
    reference_image: PeImage,
    actual_image: PeImage,
    reference_disassembly: Sequence[Instruction],
    actual_disassembly: Sequence[Instruction],
) -> Comparison:
    expected_functions = unique_function_map(reference_map)
    actual_functions = unique_function_map(actual_map)
    result = Comparison(
        "Compiland bytecode", len(expected_functions), len(actual_functions)
    )
    expected_index = InstructionIndex(reference_disassembly)
    actual_index = InstructionIndex(actual_disassembly)
    for key in sorted(set(expected_functions) | set(actual_functions)):
        expected = expected_functions.get(key)
        actual = actual_functions.get(key)
        label = f"{key[0]}:{key[1]}"
        if actual is None:
            result.differences.append(
                Difference("missing", label, compiland=key[0])
            )
            continue
        if expected is None:
            result.differences.append(
                Difference("extra", label, compiland=key[0])
            )
            continue
        if not expected.length or not actual.length:
            result.differences.append(
                Difference(
                    "unknown-boundary",
                    label,
                    str(expected.length),
                    str(actual.length),
                    key[0],
                )
            )
            continue
        expected_code = canonical_instructions(
            expected_index.range(expected.address, expected.length),
            expected,
            reference_map,
            reference_image,
        )
        actual_code = canonical_instructions(
            actual_index.range(actual.address, actual.length),
            actual,
            actual_map,
            actual_image,
        )
        if expected_code != actual_code:
            expected_first, actual_first = first_instruction_difference(
                expected_code, actual_code
            )
            category = (
                "instruction-count"
                if len(expected_code) != len(actual_code)
                else "instruction"
            )
            result.differences.append(
                Difference(
                    category,
                    label,
                    f"{len(expected_code)} instructions; first: {expected_first}",
                    f"{len(actual_code)} instructions; first: {actual_first}",
                    key[0],
                )
            )
        else:
            result.matched += 1
    return result


def print_comparison(comparison: Comparison, max_details: int) -> None:
    categories = collections.Counter(
        item.category for item in comparison.differences
    )
    print(comparison.title)
    print(
        f"  reference={comparison.reference_total} "
        f"actual={comparison.actual_total} "
        f"matched={comparison.matched} "
        f"differences={comparison.mismatch_count}"
    )
    if categories:
        print(
            "  "
            + ", ".join(
                f"{name}={count}" for name, count in sorted(categories.items())
            )
        )
    grouped_differences: dict[str, list[Difference]] = collections.defaultdict(list)
    for difference in comparison.differences:
        grouped_differences[difference.category].append(difference)
    selected_differences: list[Difference] = []
    if grouped_differences and max_details > 0:
        category_names = sorted(grouped_differences)
        per_category, remainder = divmod(max_details, len(category_names))
        for index, category in enumerate(category_names):
            limit = per_category + (1 if index < remainder else 0)
            selected_differences.extend(grouped_differences[category][:limit])
    for difference in selected_differences:
        location = (
            f" [{difference.compiland}]" if difference.compiland else ""
        )
        print(f"  {difference.category}: {difference.key}{location}")
        if difference.reference or difference.actual:
            print(f"    reference: {difference.reference or '<none>'}")
            print(f"    actual:    {difference.actual or '<none>'}")
    omitted = comparison.mismatch_count - len(selected_differences)
    if omitted > 0:
        print(f"  ... {omitted} more differences omitted")
    print()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--reference-pdb",
        type=Path,
        default=Path("WoW/Client/Wowae.pdb"),
    )
    parser.add_argument(
        "--actual-pdb",
        type=Path,
        default=Path("Build/WoW/Wow.pdb"),
    )
    parser.add_argument(
        "--reference-exe",
        type=Path,
        default=Path("WoW/Client/WowClient.exe"),
    )
    parser.add_argument(
        "--actual-exe",
        type=Path,
        default=Path("Build/WoW/Wow.exe"),
    )
    parser.add_argument(
        "--reference-map",
        type=Path,
        default=Path("WoW/Client/MapFiles/Wowae.map"),
    )
    parser.add_argument(
        "--actual-map",
        type=Path,
        default=Path("Build/WoW/Wow.map"),
    )
    parser.add_argument("--dumpbin", type=Path)
    parser.add_argument("--skip-types", action="store_true")
    parser.add_argument("--skip-bytecode", action="store_true")
    parser.add_argument("--max-details", type=int, default=50)
    parser.add_argument("--json", type=Path, dest="json_path")
    parser.add_argument(
        "--fail-on",
        choices=("never", "types", "bytecode", "any"),
        default="never",
        help="control the exit status; reporting is always performed",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.skip_types and args.skip_bytecode:
        parser.error("both validation passes were disabled")
    required = []
    if not args.skip_types:
        required.extend((args.reference_pdb, args.actual_pdb))
    if not args.skip_bytecode:
        required.extend(
            (
                args.reference_exe,
                args.actual_exe,
                args.reference_map,
                args.actual_map,
            )
        )
    for path in required:
        if not path.is_file():
            parser.error(f"required file does not exist: {path}")

    comparisons: list[Comparison] = []
    type_difference = False
    bytecode_difference = False
    try:
        if not args.skip_types:
            reference_pdb = Pdb2(args.reference_pdb)
            actual_pdb = Pdb2(args.actual_pdb)
            signature_result, layout_result = compare_pdbs(
                reference_pdb, actual_pdb
            )
            comparisons.extend((signature_result, layout_result))
            type_difference = bool(
                signature_result.differences or layout_result.differences
            )
        if not args.skip_bytecode:
            reference_map = LinkerMap(args.reference_map)
            actual_map = LinkerMap(args.actual_map)
            reference_image = PeImage(args.reference_exe)
            actual_image = PeImage(args.actual_exe)
            dumpbin = args.dumpbin or find_dumpbin(Path.cwd())
            bytecode_result = compare_bytecode(
                reference_map,
                actual_map,
                reference_image,
                actual_image,
                disassemble(args.reference_exe, dumpbin),
                disassemble(args.actual_exe, dumpbin),
            )
            comparisons.append(bytecode_result)
            bytecode_difference = bool(bytecode_result.differences)
    except ValidationError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    if args.json_path:
        args.json_path.parent.mkdir(parents=True, exist_ok=True)
        args.json_path.write_text(
            json.dumps(
                {"comparisons": [item.as_dict() for item in comparisons]},
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
    for comparison in comparisons:
        print_comparison(comparison, max(0, args.max_details))

    should_fail = (
        args.fail_on == "any" and (type_difference or bytecode_difference)
        or args.fail_on == "types" and type_difference
        or args.fail_on == "bytecode" and bytecode_difference
    )
    return 1 if should_fail else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except BrokenPipeError:
        sys.stdout = open(os.devnull, "w")
