
from __future__ import annotations

from dataclasses import dataclass, field, replace
from pathlib import Path
import struct
from typing import Any, Iterable, Mapping


class PDBError(ValueError):
    pass


@dataclass(frozen=True)
class FunctionSymbol:
    module: str
    name: str
    display_name: str | None
    section: int
    offset: int
    length: int
    type_index: int | None
    kind: str
    status: str = "parsed"
    reason: str | None = None


@dataclass(frozen=True)
class Compiland:
    module_name: str
    object_name: str
    compiler: str | None = None
    language: int | None = None
    flags: Mapping[str, Any] = field(default_factory=dict)
    raw_options: tuple[str, ...] = ()
    source_files: tuple[str, ...] = ()
    normalized_key: str | None = None
    status: str = "parsed"
    reason: str | None = None


@dataclass(frozen=True)
class TypeMember:
    kind: str
    name: str | None = None
    type_index: int | None = None
    offset: int | None = None
    attributes: int = 0
    value: int | None = None
    extra_types: tuple[int, ...] = ()
    extra_values: tuple[int, ...] = ()


@dataclass(frozen=True)
class TypeNode:
    index: int
    kind: str
    name: str | None = None
    size: int | None = None
    scalars: tuple[tuple[str, Any], ...] = ()
    refs: tuple[tuple[str, int], ...] = ()
    members: tuple[TypeMember, ...] = ()
    supported: bool = True
    reason: str | None = None


@dataclass(frozen=True)
class TypeComparison:
    equal: bool | None
    reason: str


@dataclass
class TypeRepository:
    records: dict[int, TypeNode] = field(default_factory=dict)
    named_types: dict[str, list[int]] = field(default_factory=dict)
    diagnostics: list[str] = field(default_factory=list)
    aliases: dict[int, int] = field(default_factory=dict)

    def resolve(self, index: int) -> TypeNode | None:
        if index < 0x1000:
            mode, base, subtype = (index >> 8) & 7, (index >> 4) & 15, index & 15
            return TypeNode(index, "primitive", scalars=(("mode", mode), ("base", base), ("subtype", subtype)))
        return self.records.get(self.aliases.get(index, index))

    def compare(self, index: int, other: "TypeRepository", other_index: int) -> TypeComparison:
        active: set[tuple[int, int]] = set()
        proven: set[tuple[int, int]] = set()

        def cmp(aidx: int, bidx: int, where: str) -> tuple[bool | None, str]:
            key = (aidx, bidx)
            if key in proven or key in active:
                return True, "recursive edge is consistent"
            a, b = self.resolve(aidx), other.resolve(bidx)
            if a is None or b is None:
                return None, f"{where}: missing type record"
            if not a.supported or not b.supported:
                return None, f"{where}: unsupported {a.reason or a.kind} / {b.reason or b.kind}"
            if a.kind != b.kind:
                return False, f"{where}: kind differs ({a.kind} != {b.kind})"
            if a.name != b.name or a.size != b.size or a.scalars != b.scalars:
                return False, f"{where}: type attributes differ"
            if len(a.refs) != len(b.refs) or len(a.members) != len(b.members):
                return False, f"{where}: child/member count differs"
            active.add(key)
            for (an, ai), (bn, bi) in zip(a.refs, b.refs):
                if an != bn:
                    active.remove(key)
                    return False, f"{where}: reference roles differ"
                equal, reason = cmp(ai, bi, f"{where}.{an}")
                if equal is not True:
                    active.remove(key)
                    return equal, reason
            for pos, (am, bm) in enumerate(zip(a.members, b.members)):
                scalar_a = (am.kind, am.name, am.offset, am.attributes, am.value, am.extra_values)
                scalar_b = (bm.kind, bm.name, bm.offset, bm.attributes, bm.value, bm.extra_values)
                if scalar_a != scalar_b or len(am.extra_types) != len(bm.extra_types):
                    active.remove(key)
                    return False, f"{where}.member[{pos}]: declaration/layout differs"
                ar = (() if am.type_index is None else (am.type_index,)) + am.extra_types
                br = (() if bm.type_index is None else (bm.type_index,)) + bm.extra_types
                if len(ar) != len(br):
                    active.remove(key)
                    return False, f"{where}.member[{pos}]: type arity differs"
                for child_a, child_b in zip(ar, br):
                    equal, reason = cmp(child_a, child_b, f"{where}.member[{pos}]")
                    if equal is not True:
                        active.remove(key)
                        return equal, reason
            active.remove(key)
            proven.add(key)
            return True, "structural type graphs match"

        try:
            equal, reason = cmp(index, other_index, "type")
        except RecursionError:
            return TypeComparison(
                None,
                "type graph exceeds safe comparison recursion depth",
            )
        return TypeComparison(equal, reason)


@dataclass
class PDBData:
    signature: int | None
    age: int | None
    container_version: str
    fatal: bool
    functions: list[FunctionSymbol] = field(default_factory=list)
    compilands: list[Compiland] = field(default_factory=list)
    types: TypeRepository = field(default_factory=TypeRepository)
    diagnostics: list[str] = field(default_factory=list)


class _Reader:
    def __init__(self, data: bytes, label: str = "record") -> None:
        self.data, self.pos, self.label = data, 0, label

    def take(self, size: int) -> bytes:
        if size < 0 or self.pos + size > len(self.data):
            raise PDBError(f"truncated {self.label} at byte {self.pos}")
        value = self.data[self.pos : self.pos + size]
        self.pos += size
        return value

    def unpack(self, fmt: str) -> tuple[Any, ...]:
        size = struct.calcsize("<" + fmt)
        return struct.unpack("<" + fmt, self.take(size))


class _MSF2:
    MAGIC = b"Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0"

    def __init__(self, data: bytes) -> None:
        self.data = data
        if len(data) < 60 or data[:44] != self.MAGIC:
            raise PDBError("not a Microsoft PDB/MSF 2.00 file")
        self.page_size, self.fpm, self.page_count, root_size, _ = struct.unpack_from("<IHHII", data, 44)
        if self.page_size not in (512, 1024, 2048, 4096):
            raise PDBError(f"invalid MSF page size {self.page_size}")
        if self.page_count == 0 or self.page_count * self.page_size != len(data):
            raise PDBError("MSF page count does not match file size")
        root_pages = self._page_numbers(data, 60, _pages(root_size, self.page_size), "root stream")
        root = self._join(root_pages, root_size, "root stream")
        if len(root) < 4:
            raise PDBError("truncated stream directory")
        stream_count, reserved = struct.unpack_from("<HH", root)
        desc_end = 4 + stream_count * 8
        if desc_end > len(root):
            raise PDBError("truncated stream descriptor table")
        sizes = [struct.unpack_from("<I", root, 4 + i * 8)[0] for i in range(stream_count)]
        cursor = desc_end
        self.streams: list[bytes | None] = []
        for number, size in enumerate(sizes):
            if size == 0xFFFFFFFF:
                self.streams.append(None)
                continue
            count = _pages(size, self.page_size)
            pages = self._page_numbers(root, cursor, count, f"stream {number}")
            cursor += count * 2
            self.streams.append(self._join(pages, size, f"stream {number}"))
        if cursor != len(root):
            raise PDBError("stream directory has trailing or missing page numbers")

    def _page_numbers(self, source: bytes, offset: int, count: int, label: str) -> tuple[int, ...]:
        end = offset + count * 2
        if end > len(source):
            raise PDBError(f"truncated {label} page list")
        pages = struct.unpack_from("<" + "H" * count, source, offset) if count else ()
        if any(page >= self.page_count for page in pages):
            raise PDBError(f"{label} references a page outside the file")
        return pages

    def _join(self, pages: Iterable[int], size: int, label: str) -> bytes:
        value = b"".join(self.data[p * self.page_size : (p + 1) * self.page_size] for p in pages)
        if len(value) < size:
            raise PDBError(f"truncated {label}")
        return value[:size]

    def stream(self, number: int) -> bytes:
        if number < 0 or number >= len(self.streams) or self.streams[number] is None:
            raise PDBError(f"missing stream {number}")
        return self.streams[number] or b""


def _pages(size: int, page_size: int) -> int:
    return (size + page_size - 1) // page_size


def _pascal(data: bytes, pos: int) -> tuple[str, int]:
    if pos >= len(data):
        raise PDBError("missing length-prefixed string")
    size = data[pos]
    end = pos + 1 + size
    if end > len(data):
        raise PDBError("truncated length-prefixed string")
    
    
    return data[pos + 1 : end].decode("latin-1"), end


def _zstring(data: bytes, pos: int) -> tuple[str, int]:
    try:
        end = data.index(0, pos)
    except ValueError as exc:
        raise PDBError("unterminated string") from exc
    return data[pos:end].decode("latin-1"), end + 1


_NUMERIC_FORMATS = {
    0x8000: "b", 0x8001: "h", 0x8002: "H", 0x8003: "i", 0x8004: "I",
    0x8009: "q", 0x800A: "Q",
}


def _numeric(data: bytes, pos: int) -> tuple[int, int]:
    if pos + 2 > len(data):
        raise PDBError("truncated numeric leaf")
    leaf = struct.unpack_from("<H", data, pos)[0]
    if leaf < 0x8000:
        return leaf, pos + 2
    fmt = _NUMERIC_FORMATS.get(leaf)
    if fmt is None:
        raise PDBError(f"unsupported numeric leaf 0x{leaf:04x}")
    size = struct.calcsize("<" + fmt)
    if pos + 2 + size > len(data):
        raise PDBError("truncated numeric value")
    return int(struct.unpack_from("<" + fmt, data, pos + 2)[0]), pos + 2 + size


def _parse_tpi(data: bytes) -> TypeRepository:
    repo = TypeRepository()
    if len(data) < 56:
        raise PDBError("truncated TPI header")
    version, header_size, first, last, record_bytes = struct.unpack_from("<IIIII", data)
    if header_size < 20 or header_size > len(data) or first < 0x1000 or last < first:
        raise PDBError("invalid TPI header")
    end = header_size + record_bytes
    if end > len(data):
        raise PDBError("TPI record bytes exceed stream")
    pos, index = header_size, first
    while pos < end and index < last:
        if pos + 4 > end:
            raise PDBError("truncated TPI record header")
        length, leaf = struct.unpack_from("<HH", data, pos)
        stop = pos + 2 + length
        if length < 2 or stop > end:
            raise PDBError(f"invalid TPI record length at type 0x{index:x}")
        body = data[pos + 4 : stop]
        try:
            node = _type_record(index, leaf, body)
        except (PDBError, struct.error) as exc:
            node = TypeNode(index, f"leaf_0x{leaf:04x}", supported=False, reason=str(exc))
        repo.records[index] = node
        if node.name and node.kind in {"class", "structure", "union", "enum"}:
            repo.named_types.setdefault(node.name, []).append(index)
        pos, index = stop, index + 1
    if pos != end or index != last:
        raise PDBError("TPI record count/byte range mismatch")
    unsupported: dict[str, int] = {}
    resolved_forwards = 0
    for node in repo.records.values():
        if node.reason != "forward declaration" or not node.name:
            continue
        candidates = [candidate for candidate in repo.named_types.get(node.name, ())
                      if candidate != node.index and repo.records[candidate].supported
                      and repo.records[candidate].kind == node.kind]
        
        if len(candidates) == 1:
            repo.aliases[node.index] = candidates[0]
            resolved_forwards += 1
    for node in repo.records.values():
        if not node.supported and node.index not in repo.aliases:
            reason = node.reason or node.kind
            unsupported[reason] = unsupported.get(reason, 0) + 1
    if resolved_forwards:
        repo.diagnostics.append(f"resolved forward declarations: {resolved_forwards}")
    for reason, count in sorted(unsupported.items()):
        repo.diagnostics.append(f"unsupported types ({count}): {reason}")
    return repo


def _type_record(index: int, leaf: int, b: bytes) -> TypeNode:
    u16 = lambda o: struct.unpack_from("<H", b, o)[0]
    u32 = lambda o: struct.unpack_from("<I", b, o)[0]
    if leaf == 0x1001:  
        return TypeNode(index, "modifier", scalars=(("attributes", u16(4)),), refs=(("underlying", u32(0)),))
    if leaf == 0x1002:  
        target, attr = struct.unpack_from("<II", b)
        pointer_type = attr & 0x1F
        if 3 <= pointer_type <= 9:
            return TypeNode(
                index,
                "pointer",
                scalars=(("attributes", attr),),
                refs=(("pointee", target),),
                supported=False,
                reason=f"unsupported based-pointer kind {pointer_type}",
            )
        refs: tuple[tuple[str, int], ...] = (("pointee", target),)
        scalars: tuple[tuple[str, Any], ...] = (("attributes", attr),)
        mode = (attr >> 5) & 7
        if mode in (2, 3):  
            if len(b) < 14: raise PDBError("truncated member pointer")
            refs += (("containing_class", u32(8)),)
            scalars += (("member_pointer_representation", u16(12)),)
        return TypeNode(index, "pointer", scalars=scalars, refs=refs)
    if leaf == 0x1008:  
        ret = u32(0); call, attrs, count, args = struct.unpack_from("<BBHI", b, 4)
        return TypeNode(index, "procedure", scalars=(("calling_convention", call), ("attributes", attrs), ("parameter_count", count)), refs=(("return", ret), ("arguments", args)))
    if leaf == 0x1009:  
        ret, cls, this = struct.unpack_from("<III", b); call, attrs, count, args, adjust = struct.unpack_from("<BBHIi", b, 12)
        return TypeNode(index, "member_function", scalars=(("calling_convention", call), ("attributes", attrs), ("parameter_count", count), ("this_adjust", adjust)), refs=(("return", ret), ("class", cls), ("this", this), ("arguments", args)))
    if leaf == 0x1201:  
        count = u32(0)
        if 4 + count * 4 > len(b): raise PDBError("truncated argument list")
        return TypeNode(index, "argument_list", refs=tuple((f"argument[{i}]", u32(4 + i * 4)) for i in range(count)))
    if leaf == 0x1205:  
        target, length, position = struct.unpack_from("<IBB", b)
        return TypeNode(index, "bitfield", scalars=(("length", length), ("position", position)), refs=(("underlying", target),))
    if leaf == 0x1206:  
        members: list[TypeMember] = []; pos = 0
        while pos < len(b):
            if pos + 8 > len(b): raise PDBError("truncated method list")
            attr, method_type = u16(pos), u32(pos + 4); pos += 8; values: tuple[int, ...] = ()
            if ((attr >> 2) & 7) in (4, 6):
                if pos + 4 > len(b): raise PDBError("truncated introducing method")
                values = (u32(pos),); pos += 4
            members.append(TypeMember("method", type_index=method_type, attributes=attr, extra_values=values))
        return TypeNode(index, "method_list", members=tuple(members))
    if leaf == 0x1203:  
        return TypeNode(index, "field_list", members=_field_list(b))
    if leaf == 0x000A:  
        count = u16(0); need = (count + 1) // 2
        if 2 + need > len(b): raise PDBError("truncated virtual table shape")
        return TypeNode(index, "vtable_shape", scalars=(("count", count), ("descriptors", b[2:2 + need].hex())))
    if leaf in (0x1003, 0x1503):  
        element, idx = struct.unpack_from("<II", b); size, pos = _numeric(b, 8)
        name, _ = (_pascal if leaf == 0x1003 else _zstring)(b, pos)
        return TypeNode(index, "array", name=name or None, size=size, refs=(("element", element), ("index", idx)))
    if leaf in (0x1004, 0x1005, 0x1504, 0x1505):
        count, props, fields, derived, vshape = struct.unpack_from("<HHIII", b)
        size, pos = _numeric(b, 16); name, _ = (_pascal if leaf < 0x1500 else _zstring)(b, pos)
        kind = "class" if leaf in (0x1004, 0x1504) else "structure"
        if props & 0x80:
            return TypeNode(index, kind, name=name, size=size, scalars=(("properties", props), ("member_count", count)), supported=False, reason="forward declaration")
        return TypeNode(index, kind, name=name, size=size, scalars=(("properties", props), ("member_count", count)), refs=(("fields", fields), ("derived", derived), ("vtable_shape", vshape)))
    if leaf in (0x1006, 0x1506):
        count, props, fields = struct.unpack_from("<HHI", b); size, pos = _numeric(b, 8); name, _ = (_pascal if leaf == 0x1006 else _zstring)(b, pos)
        if props & 0x80: return TypeNode(index, "union", name=name, size=size, supported=False, reason="forward declaration")
        return TypeNode(index, "union", name=name, size=size, scalars=(("properties", props), ("member_count", count)), refs=(("fields", fields),))
    if leaf in (0x1007, 0x1507):
        count, props, underlying, fields = struct.unpack_from("<HHII", b); name, _ = (_pascal if leaf == 0x1007 else _zstring)(b, 12)
        if props & 0x80: return TypeNode(index, "enum", name=name, supported=False, reason="forward declaration")
        return TypeNode(index, "enum", name=name, scalars=(("properties", props), ("member_count", count)), refs=(("underlying", underlying), ("fields", fields)))
    return TypeNode(index, f"leaf_0x{leaf:04x}", supported=False, reason=f"unsupported type leaf 0x{leaf:04x}")


def _field_list(b: bytes) -> tuple[TypeMember, ...]:
    out: list[TypeMember] = []
    pos = 0
    while pos < len(b):
        if not any(b[pos:]):
            break
        if b[pos] >= 0xF0:
            padding = b[pos] & 15
            if padding == 0:
                raise PDBError("invalid zero-length field-list padding")
            if pos + padding > len(b):
                raise PDBError("field-list padding exceeds record")
            pos += padding
            continue
        if pos + 2 > len(b):
            raise PDBError("truncated field-list leaf")
        leaf = struct.unpack_from("<H", b, pos)[0]
        pos += 2
        if leaf in (0x1405, 0x150D):  
            attr, typ = struct.unpack_from("<HI", b, pos)
            pos += 6
            offset, pos = _numeric(b, pos)
            name, pos = (_pascal if leaf == 0x1405 else _zstring)(b, pos)
            out.append(TypeMember("member", name, typ, offset, attr))
        elif leaf in (0x1406, 0x150E):
            attr, typ = struct.unpack_from("<HI", b, pos)
            pos += 6
            name, pos = (_pascal if leaf == 0x1406 else _zstring)(b, pos)
            out.append(TypeMember("static_member", name, typ, attributes=attr))
        elif leaf in (0x1407, 0x150F):
            count, methods = struct.unpack_from("<HI", b, pos)
            pos += 6
            name, pos = (_pascal if leaf == 0x1407 else _zstring)(b, pos)
            out.append(TypeMember("overloaded_method", name, methods, attributes=count))
        elif leaf in (0x140B, 0x1511):
            attr, typ = struct.unpack_from("<HI", b, pos)
            pos += 6
            values: tuple[int, ...] = ()
            if ((attr >> 2) & 7) in (4, 6):
                values = (struct.unpack_from("<I", b, pos)[0],)
                pos += 4
            name, pos = (_pascal if leaf == 0x140B else _zstring)(b, pos)
            out.append(TypeMember("method", name, typ, attributes=attr, extra_values=values))
        elif leaf == 0x1400:
            attr, typ = struct.unpack_from("<HI", b, pos)
            pos += 6
            offset, pos = _numeric(b, pos)
            out.append(TypeMember("base_class", type_index=typ, offset=offset, attributes=attr))
        elif leaf in (0x1401, 0x1402):
            attr, base, vbptr = struct.unpack_from("<HII", b, pos)
            pos += 10
            vbpoff, pos = _numeric(b, pos)
            vboff, pos = _numeric(b, pos)
            kind = "virtual_base" if leaf == 0x1401 else "indirect_virtual_base"
            out.append(
                TypeMember(
                    kind,
                    type_index=base,
                    attributes=attr,
                    extra_types=(vbptr,),
                    extra_values=(vbpoff, vboff),
                )
            )
        elif leaf == 0x1404:
            padding, typ = struct.unpack_from("<HI", b, pos)
            if padding:
                raise PDBError("nonzero LF_INDEX padding")
            pos += 6
            out.append(TypeMember("continuation", type_index=typ))
        elif leaf == 0x1409:
            padding, typ = struct.unpack_from("<HI", b, pos)
            if padding:
                raise PDBError("nonzero LF_VFUNCTAB padding")
            pos += 6
            out.append(TypeMember("vfunctab", type_index=typ))
        elif leaf in (0x1408, 0x1510):
            padding, typ = struct.unpack_from("<HI", b, pos)
            if padding:
                raise PDBError("nonzero LF_NESTTYPE padding")
            pos += 6
            name, pos = (_pascal if leaf == 0x1408 else _zstring)(b, pos)
            out.append(TypeMember("nested_type", name, typ))
        elif leaf in (0x0403, 0x1502):
            attr = struct.unpack_from("<H", b, pos)[0]
            pos += 2
            value, pos = _numeric(b, pos)
            name, pos = (_pascal if leaf == 0x0403 else _zstring)(b, pos)
            out.append(TypeMember("enumerator", name, attributes=attr, value=value))
        else:
            raise PDBError(f"unsupported field-list leaf 0x{leaf:04x}")
    return tuple(out)


@dataclass
class _Module:
    stream: int
    symbol_bytes: int
    module: str
    obj: str
    source_count: int


def _parse_dbi(msf: _MSF2, data: bytes, diagnostics: list[str]) -> tuple[list[FunctionSymbol], list[Compiland]]:
    if len(data) < 64: raise PDBError("truncated DBI header")
    h = struct.unpack_from("<iIIHHHHHHiiiiIiiHHI", data)
    if h[0] != -1: raise PDBError("invalid DBI signature")
    module_size, section_contrib_size, section_map_size, source_size = h[9], h[10], h[11], h[12]
    if min(module_size, section_contrib_size, section_map_size, source_size) < 0: raise PDBError("negative DBI substream size")
    module_start, module_end = 64, 64 + module_size
    if module_end > len(data): raise PDBError("DBI module substream exceeds stream")
    modules: list[_Module] = []; pos = module_start
    while pos < module_end:
        if pos + 64 > module_end: raise PDBError("truncated DBI module record")
        stream = struct.unpack_from("<H", data, pos + 34)[0]
        symbol_bytes = struct.unpack_from("<I", data, pos + 36)[0]
        source_count = struct.unpack_from("<H", data, pos + 48)[0]
        module, p = _zstring(data, pos + 64); obj, p = _zstring(data, p); pos = (p + 3) & ~3
        if pos > module_end: raise PDBError("DBI module names exceed substream")
        modules.append(_Module(stream, symbol_bytes, module, obj, source_count))
    source_start = module_end + section_contrib_size + section_map_size
    if source_start + source_size > len(data): raise PDBError("DBI source substream exceeds stream")
    sources = _source_files(data[source_start : source_start + source_size], len(modules), diagnostics)
    functions: list[FunctionSymbol] = []; compilands: list[Compiland] = []
    for number, module in enumerate(modules):
        compiler: str | None = None; language: int | None = None; flags: dict[str, Any] = {}; options: tuple[str, ...] = (); reason: str | None = None
        if module.stream != 0xFFFF and module.symbol_bytes:
            try:
                stream = msf.stream(module.stream)
                compiler, language, flags, options, found = _module_symbols(
                    stream, module.symbol_bytes, module.module
                )
                functions.extend(found)
            except PDBError as exc:
                raise PDBError(
                    f"incomplete symbol inventory: module {module.module}: {exc}"
                ) from exc
        key_source = module.obj.replace("\\", "/").rsplit("/", 1)[-1].lower()
        compilands.append(Compiland(module.module, module.obj, compiler, language, flags, options, sources[number] if number < len(sources) else (), key_source, "unsupported" if reason else "parsed", reason))
    return functions, compilands


def _source_files(data: bytes, module_count: int, diagnostics: list[str]) -> list[tuple[str, ...]]:
    result = [() for _ in range(module_count)]
    if not data: return result
    if len(data) < 4: raise PDBError("truncated source file information")
    mods, total = struct.unpack_from("<HH", data)
    if mods != module_count:
        diagnostics.append(f"source module count {mods} differs from DBI module count {module_count}")
    if 4 + mods * 4 + total * 4 > len(data): raise PDBError("truncated source file tables")
    starts = struct.unpack_from("<" + "H" * mods, data, 4) if mods else ()
    counts = struct.unpack_from("<" + "H" * mods, data, 4 + mods * 2) if mods else ()
    offsets_at = 4 + mods * 4
    offsets = struct.unpack_from("<" + "I" * total, data, offsets_at) if total else ()
    names_at = offsets_at + total * 4
    for i in range(min(mods, module_count)):
        names: list[str] = []
        if starts[i] + counts[i] > total: raise PDBError("source file range exceeds table")
        for item in range(starts[i], starts[i] + counts[i]):
            
            name, _ = _pascal(data, names_at + offsets[item]); names.append(name)
        result[i] = tuple(names)
    return result


def _module_symbols(data: bytes, byte_count: int, module: str) -> tuple[str | None, int | None, dict[str, Any], tuple[str, ...], list[FunctionSymbol]]:
    if len(data) < 4: raise PDBError("truncated module symbol stream")
    signature = struct.unpack_from("<I", data)[0]
    if signature not in (1, 2, 4): raise PDBError(f"unsupported module CodeView signature {signature}")
    limit = min(len(data), byte_count)
    if byte_count > len(data): raise PDBError("module symbol byte count exceeds stream")
    pos = 4; compiler: str | None = None; language: int | None = None; flags: dict[str, Any] = {}; options: tuple[str, ...] = (); functions: list[FunctionSymbol] = []
    while pos < limit:
        if pos + 4 > limit: raise PDBError("truncated symbol record header")
        length, kind = struct.unpack_from("<HH", data, pos); end = pos + 2 + length
        if length < 2 or end > limit: raise PDBError("invalid module symbol record length")
        b = data[pos + 4 : end]
        if kind in (0x100A, 0x100B, 0x110F, 0x1110):
            if len(b) < 35: raise PDBError("truncated procedure symbol")
            proc_len = struct.unpack_from("<I", b, 12)[0]; type_index = struct.unpack_from("<I", b, 24)[0]; offset = struct.unpack_from("<I", b, 28)[0]; section = struct.unpack_from("<H", b, 32)[0]
            name, _ = (_pascal if kind < 0x1100 else _zstring)(b, 35)
            functions.append(FunctionSymbol(module, name, name, section, offset, proc_len, type_index, "local" if kind in (0x100A, 0x110F) else "global"))
        elif kind == 0x0001 and len(b) >= 4:
            machine, raw_flags = b[0], int.from_bytes(b[1:4], "little"); compiler, _ = _pascal(b, 4); language = raw_flags & 0xFF; flags = {"machine": machine, "raw": raw_flags, "mode32": bool(raw_flags & (1 << 19))}
        elif kind in (0x1013, 0x1116) and len(b) >= 18:
            raw_flags, machine, fe_ma, fe_mi, fe_build, be_ma, be_mi, be_build = struct.unpack_from("<IHHHHHHH", b)
            compiler, tail = (_pascal if kind == 0x1013 else _zstring)(b, 18); language = raw_flags & 0xFF
            flags = {"raw": raw_flags, "machine": machine, "front_end": (fe_ma, fe_mi, fe_build), "back_end": (be_ma, be_mi, be_build), "edit_and_continue": bool(raw_flags & 0x100), "ltcg": bool(raw_flags & 0x400), "security_checks": bool(raw_flags & 0x2000)}
            option_data = b[tail:]
            options = tuple(
                part.decode("latin-1")
                for part in option_data.split(b"\0")
                if part
            )
        pos = end
    return compiler, language, flags, options, functions


def parse_pdb(path: str | Path) -> PDBData:
    diagnostics: list[str] = []
    try:
        data = Path(path).read_bytes()
        msf = _MSF2(data)
        info = msf.stream(1)
        if len(info) < 12: raise PDBError("truncated PDB information stream")
        version, signature, age = struct.unpack_from("<III", info)
        types = _parse_tpi(msf.stream(2))
        diagnostics.extend(types.diagnostics)
        functions, compilands = _parse_dbi(msf, msf.stream(3), diagnostics)
        checked: list[FunctionSymbol] = []
        for function in functions:
            node = types.resolve(function.type_index) if function.type_index is not None else None
            if node is None:
                checked.append(replace(function, status="unsupported", reason="missing function type"))
            elif not node.supported or node.kind not in ("procedure", "member_function"):
                checked.append(replace(function, status="unsupported", reason=node.reason or f"unexpected function type {node.kind}"))
            else:
                checked.append(function)
        return PDBData(signature, age, "MSF/PDB 2.00", False, checked, compilands, types, diagnostics)
    except (OSError, PDBError, struct.error, OverflowError) as exc:
        diagnostics.append(str(exc))
        return PDBData(None, None, "unknown", True, diagnostics=diagnostics)


__all__ = [
    "Compiland", "FunctionSymbol", "PDBData", "PDBError", "TypeComparison",
    "TypeMember", "TypeNode", "TypeRepository", "parse_pdb",
]
