
from __future__ import annotations

import re
import struct
from pathlib import Path

from .model import ArtifactData, Diagnostic, ImageIdentity, MapSymbol, Section


class ArtifactParseError(ValueError):
    pass


_MAP_TIMESTAMP = re.compile(r"^\s*Timestamp is\s+([0-9a-f]{8})\b", re.IGNORECASE)
_MAP_BASE = re.compile(r"^\s*Preferred load address is\s+([0-9a-f]{8})\b", re.IGNORECASE)
_MAP_SECTION = re.compile(
    r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+([0-9a-f]{8})H\s+(\S+)\s+(\S+)\s*$",
    re.IGNORECASE,
)
_MAP_SYMBOL = re.compile(
    r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})(?:\s+(.*?))?\s*$",
    re.IGNORECASE,
)
_MAP_ROW_PREFIX = re.compile(r"^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+", re.IGNORECASE)
_OBJECT = re.compile(r"(?:^|\s)(\S+\.obj)(?:\s|$)", re.IGNORECASE)


class _PE:
    def __init__(self, path: Path, raw: bytes):
        self.path = path
        self.raw = raw
        self.identity: ImageIdentity
        self.sections: tuple[Section, ...]
        self.size_of_image = 0
        self.section_alignment = 0
        self.characteristics = 0
        self.directories: tuple[tuple[int, int], ...] = ()
        self.virtual_section_overlaps: tuple[tuple[str, str, int, int], ...] = ()

        pe_offset = self._unpack("<I", 0x3C, "DOS e_lfanew")[0]
        if pe_offset < 0x40 or pe_offset + 24 > len(raw):
            self.fail(f"{path}: PE header offset is outside the file")
        if raw[:2] != b"MZ" or raw[pe_offset : pe_offset + 4] != b"PE\0\0":
            self.fail(f"{path}: not a valid PE image")

        machine, section_count, timestamp, _, _, optional_size, characteristics = self._unpack(
            "<HHIIIHH", pe_offset + 4, "COFF header"
        )
        if machine != 0x14C:
            self.fail(f"{path}: expected I386 PE, found machine 0x{machine:04x}")
        if not 1 <= section_count <= 96:
            self.fail(f"{path}: unreasonable PE section count {section_count}")
        optional = pe_offset + 24
        if optional_size < 96 or optional + optional_size > len(raw):
            self.fail(f"{path}: truncated PE32 optional header")
        magic = self._unpack("<H", optional, "optional-header magic")[0]
        if magic != 0x10B:
            self.fail(f"{path}: expected PE32 optional header, found 0x{magic:04x}")

        image_base = self._unpack("<I", optional + 28, "image base")[0]
        section_alignment = self._unpack("<I", optional + 32, "section alignment")[0]
        file_alignment = self._unpack("<I", optional + 36, "file alignment")[0]
        size_of_image = self._unpack("<I", optional + 56, "SizeOfImage")[0]
        size_of_headers = self._unpack("<I", optional + 60, "SizeOfHeaders")[0]
        number_of_directories = self._unpack("<I", optional + 92, "data-directory count")[0]
        if not self._is_power_of_two(section_alignment) or not self._is_power_of_two(file_alignment):
            self.fail(f"{path}: invalid PE section or file alignment")
        if size_of_image == 0 or size_of_headers == 0 or size_of_headers > len(raw):
            self.fail(f"{path}: invalid PE image/header size")
        directory_capacity = (optional_size - 96) // 8
        if number_of_directories > directory_capacity:
            self.fail(f"{path}: data-directory table exceeds optional header")
        directories = [
            self._unpack("<II", optional + 96 + index * 8, "data directory")
            for index in range(number_of_directories)
        ]
        self.directories = tuple(directories)
        self.size_of_image = size_of_image
        self.section_alignment = section_alignment
        self.characteristics = characteristics

        section_table = optional + optional_size
        table_end = section_table + section_count * 40
        if table_end > len(raw) or table_end > size_of_headers:
            self.fail(f"{path}: section table is outside the PE headers")
        sections: list[Section] = []
        raw_ranges: list[tuple[int, int, str]] = []
        file_backed_rva_ranges: list[tuple[int, int, str]] = []
        virtual_rva_ranges: list[tuple[int, int, str]] = []
        for number in range(1, section_count + 1):
            offset = section_table + (number - 1) * 40
            header = raw[offset : offset + 40]
            if len(header) != 40:
                self.fail(f"{path}: truncated section header {number}")
            name_bytes = header[:8].split(b"\0", 1)[0]
            try:
                name = name_bytes.decode("ascii")
            except UnicodeDecodeError:
                self.fail(f"{path}: non-ASCII section name in section {number}")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", header, 8)
            section_characteristics = struct.unpack_from("<I", header, 36)[0]
            if rva >= size_of_image or (
                virtual_size and rva + virtual_size > size_of_image + section_alignment
            ):
                self.fail(f"{path}: section {name!r} lies outside SizeOfImage")
            virtual_rva_ranges.append((rva, rva + max(virtual_size, raw_size), name))
            if raw_size:
                raw_end = raw_offset + raw_size
                if raw_offset < size_of_headers or raw_end < raw_offset or raw_end > len(raw):
                    self.fail(f"{path}: section {name!r} raw range is outside the file")
                raw_ranges.append((raw_offset, raw_end, name))
                file_backed_rva_ranges.append((rva, rva + raw_size, name))
                section_data = raw[raw_offset:raw_end]
            else:
                section_data = b""
            sections.append(
                Section(
                    number=number,
                    name=name,
                    rva=rva,
                    virtual_size=virtual_size,
                    raw_offset=raw_offset,
                    raw_size=raw_size,
                    characteristics=section_characteristics,
                    data=section_data,
                )
            )
        raw_ranges.sort()
        for previous, current in zip(raw_ranges, raw_ranges[1:]):
            if current[0] < previous[1]:
                self.fail(f"{path}: sections {previous[2]!r} and {current[2]!r} overlap in the file")
        file_backed_rva_ranges.sort()
        for previous, current in zip(file_backed_rva_ranges, file_backed_rva_ranges[1:]):
            if current[0] < previous[1]:
                self.fail(
                    f"{path}: sections {previous[2]!r} and {current[2]!r} overlap in file-backed RVAs"
                )
        virtual_rva_ranges.sort()
        self.virtual_section_overlaps = tuple(
            (left[2], right[2], right[0], min(left[1], right[1]))
            for left, right in zip(virtual_rva_ranges, virtual_rva_ranges[1:])
            if right[0] < left[1]
        )
        
        
        
        self.sections = tuple(sections)
        codeview_kind, codeview_signature, codeview_age, pdb_path = self._read_codeview()
        self.identity = ImageIdentity(
            path=path,
            machine=machine,
            image_base=image_base,
            pe_timestamp=timestamp,
            codeview_kind=codeview_kind,
            codeview_signature=codeview_signature,
            codeview_age=codeview_age,
            embedded_pdb_path=pdb_path,
        )

    @staticmethod
    def _is_power_of_two(value: int) -> bool:
        return value > 0 and value & (value - 1) == 0

    def fail(self, message: str) -> None:
        raise ArtifactParseError(message)

    def _unpack(self, fmt: str, offset: int, what: str) -> tuple[int, ...]:
        size = struct.calcsize(fmt)
        if offset < 0 or offset + size > len(self.raw):
            self.fail(f"{self.path}: truncated {what}")
        return struct.unpack_from(fmt, self.raw, offset)

    def read_rva(self, rva: int, size: int, what: str) -> bytes:
        if rva < 0 or size < 0 or rva + size > self.size_of_image:
            self.fail(f"{self.path}: {what} RVA range is outside SizeOfImage")
        for section in self.sections:
            offset = rva - section.rva
            if offset >= 0 and offset + size <= min(section.raw_size, len(section.data)):
                return section.data[offset : offset + size]
        self.fail(f"{self.path}: {what} RVA 0x{rva:x} is not file-backed")

    def _raw_offset(self, rva: int, size: int) -> int:
        for section in self.sections:
            offset = rva - section.rva
            if offset >= 0 and offset + size <= min(section.raw_size, len(section.data)):
                return section.raw_offset + offset
        self.fail(f"{self.path}: RVA 0x{rva:x} is not file-backed")

    def c_string_rva(self, rva: int, what: str, limit: int = 4096) -> str:
        if rva < 0 or rva >= self.size_of_image:
            self.fail(f"{self.path}: {what} RVA is outside SizeOfImage")
        
        
        value = bytearray()
        for index in range(limit):
            byte = self.read_rva(rva + index, 1, what)[0]
            if byte == 0:
                try:
                    return value.decode("ascii")
                except UnicodeDecodeError:
                    self.fail(f"{self.path}: {what} is not ASCII")
            value.append(byte)
        self.fail(f"{self.path}: unterminated or oversized {what}")

    def _directory(self, index: int) -> tuple[int, int]:
        return self.directories[index] if index < len(self.directories) else (0, 0)

    def _read_codeview(self) -> tuple[str | None, int | None, int | None, str | None]:
        rva, size = self._directory(6)
        if rva == 0 and size == 0:
            return None, None, None, None
        if rva == 0 or size == 0 or size % 28:
            self.fail(f"{self.path}: malformed PE debug directory")
        records = self.read_rva(rva, size, "debug directory")
        identities: list[tuple[str, int | None, int | None, str | None]] = []
        for offset in range(0, len(records), 28):
            _, _, _, _, kind, data_size, data_rva, data_pointer = struct.unpack_from(
                "<IIHHIIII", records, offset
            )
            if kind != 2:
                continue
            if data_size < 4:
                self.fail(f"{self.path}: truncated CodeView debug record")
            if data_pointer:
                if data_pointer + data_size > len(self.raw):
                    self.fail(f"{self.path}: CodeView debug record is outside the file")
                if data_rva:
                    rva_record = self.read_rva(data_rva, data_size, "CodeView debug record")
                    pointer_record = self.raw[data_pointer : data_pointer + data_size]
                    if (
                        data_pointer != self._raw_offset(data_rva, data_size)
                        or rva_record != pointer_record
                    ):
                        self.fail(f"{self.path}: CodeView raw pointer and RVA disagree")
                record = self.raw[data_pointer : data_pointer + data_size]
            else:
                record = self.read_rva(data_rva, data_size, "CodeView debug record")
            if record[:4] == b"NB10":
                if len(record) < 16:
                    self.fail(f"{self.path}: truncated NB10 debug record")
                _, signature, age = struct.unpack_from("<III", record, 4)
                pdb_path = record[16:].split(b"\0", 1)[0].decode("utf-8", "replace") or None
                identities.append(("NB10", signature, age, pdb_path))
            elif record[:4] == b"RSDS":
                if len(record) < 25:
                    self.fail(f"{self.path}: truncated RSDS debug record")
                age = struct.unpack_from("<I", record, 20)[0]
                pdb_path = record[24:].split(b"\0", 1)[0].decode("utf-8", "replace") or None
                identities.append(("RSDS", None, age, pdb_path))
        if not identities:
            return None, None, None, None
        if any(identity != identities[0] for identity in identities[1:]):
            self.fail(f"{self.path}: conflicting CodeView identities in debug directory")
        return identities[0]

    def imports(self) -> dict[int, str]:
        rva, size = self._directory(1)
        if rva == 0 and size == 0:
            return {}
        if rva == 0 or size < 20 or size % 20:
            self.fail(f"{self.path}: malformed PE import directory")
        count = size // 20
        if count > 65536:
            self.fail(f"{self.path}: unreasonable import-descriptor count")
        resolved: dict[int, str] = {}
        terminated = False
        for index in range(count):
            descriptor = self.read_rva(rva + index * 20, 20, "import descriptor")
            original_thunk, timestamp, forwarder, name_rva, first_thunk = struct.unpack("<IIIII", descriptor)
            if not any((original_thunk, timestamp, forwarder, name_rva, first_thunk)):
                terminated = True
                break
            if not name_rva or not first_thunk:
                self.fail(f"{self.path}: import descriptor {index} is missing its DLL or IAT")
            dll_name = self.c_string_rva(name_rva, "import DLL name").lower()
            lookup_rva = original_thunk or first_thunk
            for thunk_index in range(self.size_of_image // 4):
                lookup_slot = lookup_rva + thunk_index * 4
                iat_slot = first_thunk + thunk_index * 4
                value = struct.unpack("<I", self.read_rva(lookup_slot, 4, "import lookup thunk"))[0]
                if value == 0:
                    break
                if value & 0x80000000:
                    target = f"{dll_name}!#{value & 0xffff}"
                else:
                    self.read_rva(value, 2, "import hint")  
                    function_name = self.c_string_rva(value + 2, "import function name")
                    target = f"{dll_name}!{function_name}"
                if iat_slot + 4 > self.size_of_image:
                    self.fail(f"{self.path}: import IAT slot is outside SizeOfImage")
                self.read_rva(iat_slot, 4, "import IAT slot")
                if iat_slot in resolved:
                    self.fail(f"{self.path}: duplicate IAT slot at RVA 0x{iat_slot:x}")
                resolved[iat_slot] = target
            else:
                self.fail(f"{self.path}: unterminated import thunk table for {dll_name}")
            
            if struct.unpack("<I", self.read_rva(iat_slot, 4, "IAT terminator"))[0] != 0:
                self.fail(f"{self.path}: import thunk table for {dll_name} has no terminator")
        if not terminated:
            self.fail(f"{self.path}: import directory has no null descriptor")
        return resolved


def _map_fail(path: Path, message: str) -> None:
    raise ArtifactParseError(f"{path}: {message}")


def _parse_map(
    path: Path,
    image_base: int,
    data: bytes | None = None,
) -> tuple[int, int, tuple[tuple[int, int, int, str], ...], tuple[MapSymbol, ...]]:
    try:
        raw = path.read_bytes() if data is None else data
        lines = raw.decode("latin-1").splitlines()
    except OSError as exc:
        raise ArtifactParseError(f"{path}: cannot read MAP file: {exc}") from exc
    timestamp: int | None = None
    preferred_base: int | None = None
    section_rows: list[tuple[int, int, int, str]] = []
    symbols: list[MapSymbol] = []
    symbol_state: str | None = None
    saw_public_table = False
    for line_number, line in enumerate(lines, 1):
        match = _MAP_TIMESTAMP.match(line)
        if match:
            found = int(match.group(1), 16)
            if timestamp is not None and timestamp != found:
                _map_fail(path, f"conflicting timestamps at line {line_number}")
            timestamp = found
            continue
        match = _MAP_BASE.match(line)
        if match:
            found = int(match.group(1), 16)
            if preferred_base is not None and preferred_base != found:
                _map_fail(path, f"conflicting preferred image bases at line {line_number}")
            preferred_base = found
            continue
        if "Address         Publics by Value" in line:
            symbol_state = "public"
            saw_public_table = True
            continue
        if re.match(r"^\s*Static symbols\s*$", line, re.IGNORECASE):
            symbol_state = "static"
            continue
        section_match = _MAP_SECTION.match(line)
        if section_match and symbol_state is None:
            segment, offset, length = (int(section_match.group(i), 16) for i in (1, 2, 3))
            section_rows.append((segment, offset, length, section_match.group(4)))
            continue
        if symbol_state is None or not line.strip():
            continue
        symbol_match = _MAP_SYMBOL.match(line)
        if not symbol_match:
            if _MAP_ROW_PREFIX.match(line):
                _map_fail(path, f"malformed symbol row at line {line_number}")
            continue
        segment = int(symbol_match.group(1), 16)
        offset = int(symbol_match.group(2), 16)
        name = symbol_match.group(3)
        address = int(symbol_match.group(4), 16)
        remainder = symbol_match.group(5) or ""
        owner_match = _OBJECT.search(remainder)
        object_name = owner_match.group(1) if owner_match else None
        metadata = remainder.split()
        if object_name:
            metadata = [token for token in metadata if token != object_name]
        flags = tuple(metadata + [symbol_state])
        if address < image_base:
            _map_fail(path, f"symbol {name!r} at line {line_number} has an address below image base")
        symbols.append(
            MapSymbol(
                name=name,
                rva=address - image_base,
                segment=segment,
                offset=offset,
                flags=flags,
                object_name=object_name,
            )
        )
    if timestamp is None:
        _map_fail(path, "missing `Timestamp is` identity")
    if preferred_base is None:
        _map_fail(path, "missing `Preferred load address is` identity")
    if not section_rows:
        _map_fail(path, "missing section contribution table")
    if not saw_public_table or not symbols:
        _map_fail(path, "missing public/static symbol rows")
    return timestamp, preferred_base, tuple(section_rows), tuple(symbols)


def _diagnostic(code: str, message: str, **context: object) -> Diagnostic:
    return Diagnostic("error", code, message, context)


def _map_layout_diagnostics(
    map_path: Path,
    sections: tuple[Section, ...],
    section_rows: tuple[tuple[int, int, int, str], ...],
    symbols: tuple[MapSymbol, ...],
) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    grouped: dict[int, list[tuple[int, int, str]]] = {}
    for segment, offset, length, name in section_rows:
        grouped.setdefault(segment, []).append((offset, length, name))
    expected_segments = set(range(1, len(sections) + 1))
    if set(grouped) != expected_segments:
        diagnostics.append(
            _diagnostic(
                "map_section_count_mismatch",
                f"{map_path}: MAP section groups do not match the PE section count",
                map_segments=sorted(grouped),
                pe_sections=len(sections),
            )
        )

    bases: dict[int, set[int]] = {}
    for symbol in symbols:
        bases.setdefault(symbol.segment, set()).add(symbol.rva - symbol.offset)
        if not any(
            section.rva <= symbol.rva < section.rva + max(section.virtual_size, section.raw_size)
            for section in sections
        ):
            diagnostics.append(
                _diagnostic(
                    "map_symbol_outside_image",
                    f"{map_path}: MAP symbol {symbol.name!r} lies outside all PE sections",
                    segment=symbol.segment,
                    offset=f"0x{symbol.offset:x}",
                    rva=f"0x{symbol.rva:x}",
                )
            )
            break

    ordered_section_bases = [section.rva for section in sections]
    for segment in sorted(expected_segments & set(grouped)):
        section_index = segment - 1
        section = sections[section_index]
        total_extent = max((offset + length for offset, length, _ in grouped[segment]), default=0)
        if total_extent > max(section.virtual_size, section.raw_size):
            diagnostics.append(
                _diagnostic(
                    "map_section_extent_mismatch",
                    f"{map_path}: MAP segment {segment} extends past its PE section",
                    map_extent=f"0x{total_extent:x}",
                    pe_extent=f"0x{max(section.virtual_size, section.raw_size):x}",
                )
            )
        for offset, length, name in grouped[segment]:
            if offset + length > 0x100000000:
                diagnostics.append(
                    _diagnostic(
                        "map_section_range_invalid",
                        f"{map_path}: MAP contribution {name!r} has an overflowing range",
                        segment=segment,
                        offset=f"0x{offset:x}",
                        length=f"0x{length:x}",
                    )
                )
                break
        candidate_bases = bases.get(segment, set())
        if not candidate_bases:
            
            
            continue
        if len(candidate_bases) != 1:
            diagnostics.append(
                _diagnostic(
                    "map_segment_address_mismatch",
                    f"{map_path}: symbols in MAP segment {segment} disagree on its PE address",
                    candidate_rvas=[f"0x{value:x}" for value in sorted(candidate_bases)],
                )
            )
            continue
        base = next(iter(candidate_bases))
        if section_index >= len(sections) or base != ordered_section_bases[section_index]:
            diagnostics.append(
                _diagnostic(
                    "map_section_location_mismatch",
                    f"{map_path}: MAP segment {segment} does not begin at the corresponding PE section",
                    map_rva=f"0x{base:x}",
                    pe_rva=(f"0x{ordered_section_bases[section_index]:x}" if section_index < len(sections) else None),
                )
            )
            continue
    for symbol in symbols:
        if symbol.segment not in grouped:
            diagnostics.append(
                _diagnostic(
                    "map_symbol_segment_missing",
                    f"{map_path}: symbol {symbol.name!r} refers to absent MAP segment {symbol.segment}",
                )
            )
            break
        extent = max((offset + length for offset, length, _ in grouped[symbol.segment]), default=0)
        if symbol.offset >= extent:
            diagnostics.append(
                _diagnostic(
                    "map_symbol_outside_segment",
                    f"{map_path}: symbol {symbol.name!r} lies outside MAP segment {symbol.segment}",
                    offset=f"0x{symbol.offset:x}",
                    extent=f"0x{extent:x}",
                )
            )
            break
    return diagnostics


def load_artifacts(
    pe_path: str | Path,
    map_path: str | Path,
    *,
    pe_data: bytes | None = None,
    map_data: bytes | None = None,
) -> ArtifactData:

    pe_path = Path(pe_path)
    map_path = Path(map_path)
    if pe_data is None:
        try:
            pe_bytes = pe_path.read_bytes()
        except OSError as exc:
            raise ArtifactParseError(f"{pe_path}: cannot read PE file: {exc}") from exc
    else:
        pe_bytes = pe_data
    pe = _PE(pe_path, pe_bytes)
    map_timestamp, map_base, map_sections, map_symbols = _parse_map(
        map_path,
        pe.identity.image_base,
        map_data,
    )
    diagnostics: list[Diagnostic] = []
    diagnostics.extend(
        Diagnostic(
            "info",
            "pe_virtual_section_overlap",
            f"{pe_path}: declared virtual spans for {left!r} and {right!r} overlap in an uninitialized tail",
            {"overlap_start_rva": f"0x{start:x}", "overlap_end_rva": f"0x{end:x}"},
        )
        for left, right, start, end in pe.virtual_section_overlaps
    )
    if map_timestamp != pe.identity.pe_timestamp:
        diagnostics.append(
            _diagnostic(
                "map_timestamp_mismatch",
                f"{map_path}: MAP timestamp does not match {pe_path}",
                map_timestamp=f"0x{map_timestamp:08x}",
                pe_timestamp=f"0x{pe.identity.pe_timestamp:08x}",
            )
        )
    if map_base != pe.identity.image_base:
        diagnostics.append(
            _diagnostic(
                "map_image_base_mismatch",
                f"{map_path}: MAP preferred load address does not match {pe_path}",
                map_base=f"0x{map_base:08x}",
                pe_base=f"0x{pe.identity.image_base:08x}",
            )
        )
    diagnostics.extend(_map_layout_diagnostics(map_path, pe.sections, map_sections, map_symbols))
    imports = pe.imports()
    base_reloc_rva, base_reloc_size = pe._directory(5)
    if base_reloc_rva == 0 and base_reloc_size == 0 and pe.characteristics & 0x0001:
        diagnostics.append(
            Diagnostic(
                "info",
                "relocations_stripped",
                f"{pe_path}: image is marked relocation-stripped and has no base-relocation directory",
                {"characteristics": f"0x{pe.characteristics:04x}"},
            )
        )
    return ArtifactData(
        identity=pe.identity,
        sections=pe.sections,
        map_symbols=map_symbols,
        imports=imports,
        diagnostics=tuple(diagnostics),
        fatal=any(diagnostic.severity == "error" for diagnostic in diagnostics),
    )
