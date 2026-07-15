#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import shutil
import struct
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Symbol:
    name: str
    obj: str
    address: int
    section: str = ".text"
    length: int | None = None
    source_length: int | None = None
    data: bytes | None = None


MAP_PUBLIC = re.compile(
    r"^\s*(?P<seg>[0-9A-Fa-f]{4}):(?P<off>[0-9A-Fa-f]{8})\s+"
    r"(?P<name>\S+)\s+(?P<addr>[0-9A-Fa-f]{8})\s+"
    r"(?P<flags>f(?:\s+i)?)\s+(?P<obj>\S+\.obj)\s*$"
)
MAP_BASE = re.compile(r"Preferred load address is\s+([0-9A-Fa-f]+)", re.I)
OBJ_SYMBOL = re.compile(
    r"^\s*(?P<value>[0-9A-Fa-f]+)\s+(?P<flags>\S+)\s+(?P<section>\S+)\s+"
    r"(?P<name>\S+)\s*$"
)


def run(tool: str, *args: str) -> str:
    try:
        result = subprocess.run(
            [tool, *args], check=True, text=True, errors="replace",
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )
        return result.stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        raise RuntimeError(f"could not run {tool}: {exc}") from exc


def tool(*names: str) -> str:
    for name in names:
        found = shutil.which(name)
        if found:
            return found
    raise RuntimeError(f"none of these tools was found: {', '.join(names)}")


def object_name(name: str) -> str:
    """Normalize MAP/PDB paths and linker prefixes to a local object name."""
    name = name.replace("\\", "/").rsplit("/", 1)[-1]
    name = name.rsplit(":", 1)[-1].lower()
    for suffix in (".cpp.obj", ".cxx.obj", ".c.obj"):
        if name.endswith(suffix):
            return name[:-len(suffix)] + ".obj"
    return name


def _pages(size: int, page_size: int) -> int:
    return (size + page_size - 1) // page_size


def _stream(data: bytes, pages: tuple[int, ...], size: int, page_size: int) -> bytes:
    return b"".join(data[page * page_size:(page + 1) * page_size] for page in pages)[:size]


def _old_line_ranges(data: bytes) -> dict[int, int]:
    """Return relative source-line lengths keyed by function section offset."""
    if len(data) < 4:
        return {}
    file_count, _ = struct.unpack_from("<HH", data)
    if 4 + file_count * 4 > len(data):
        return {}
    ranges: dict[int, list[int]] = {}
    file_offsets = struct.unpack_from(f"<{file_count}I", data, 4)
    for file_offset in file_offsets:
        if file_offset + 4 > len(data):
            continue
        segment_count = struct.unpack_from("<H", data, file_offset)[0]
        ptr_base = file_offset + 4
        starts_base = ptr_base + segment_count * 4
        if starts_base + segment_count * 8 > len(data):
            continue
        blocks = struct.unpack_from(f"<{segment_count}I", data, ptr_base)
        for index, block_offset in enumerate(blocks):
            start, end = struct.unpack_from("<II", data, starts_base + index * 8)
            if block_offset + 4 > len(data):
                continue
            _, line_count = struct.unpack_from("<HH", data, block_offset)
            offsets_at = block_offset + 4
            lines_at = offsets_at + line_count * 4
            if lines_at + line_count * 2 > len(data):
                continue
            offsets = struct.unpack_from(f"<{line_count}I", data, offsets_at)
            lines = struct.unpack_from(f"<{line_count}H", data, lines_at)
            for offset, line in zip(offsets, lines):
                if start <= offset < end:
                    current = ranges.setdefault(offset, [line, line])
                    current[0] = min(current[0], line)
                    current[1] = max(current[1], line)
    return {offset: line_range[1] - line_range[0] + 1 for offset, line_range in ranges.items()}


def parse_legacy_pdb(path: Path) -> list[Symbol]:
    """Read VC6 PDB 2.0 module streams and their CodeView procedures."""
    data = path.read_bytes()
    signature = b"Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0"
    if not data.startswith(signature):
        raise RuntimeError(f"{path} is not a legacy PDB 2.0 file")
    page_size, _, _, root_size, _ = struct.unpack_from("<IHHII", data, len(signature))
    root_pages = struct.unpack_from(f"<{_pages(root_size, page_size)}H", data, 60)
    root = _stream(data, root_pages, root_size, page_size)
    stream_count = struct.unpack_from("<H", root, 0)[0]
    sizes = [struct.unpack_from("<I", root, 4 + i * 8)[0] for i in range(stream_count)]
    pos = 4 + stream_count * 8
    pages = []
    for size in sizes:
        count = _pages(size, page_size)
        pages.append(struct.unpack_from(f"<{count}H", root, pos) if count else ())
        pos += count * 2
    dbi = _stream(data, pages[3], sizes[3], page_size)
    module_size = struct.unpack_from("<I", dbi, 24)[0]
    modules = []
    pos = 64
    end = pos + module_size
    while pos < end:
        fixed = dbi[pos:pos + 64]
        stream, sym_size = struct.unpack_from("<hI", fixed, 34)
        strings = dbi[pos + 64:end]
        first_end = strings.index(0)
        obj_start = first_end + 1
        second_end = strings.index(0, obj_start)
        obj = object_name(strings[obj_start:second_end].decode("latin1"))
        header_size = 64 + second_end + 1
        pos += (header_size + 3) & ~3
        old_line_size = struct.unpack_from("<I", fixed, 40)[0]
        modules.append((obj, stream, sym_size, old_line_size))

    result = []
    for obj, stream, sym_size, old_line_size in modules:
        if stream < 0 or not sym_size:
            continue
        symbols = _stream(data, pages[stream], sizes[stream], page_size)[:sym_size]
        line_ranges = _old_line_ranges(
            _stream(data, pages[stream], sizes[stream], page_size)[sym_size:sym_size + old_line_size]
        )
        # The DBI module name is often only the containing .lib.  S_OBJNAME
        # carries the actual contributing .obj name used by the linker MAP.
        if len(symbols) >= 8:
            first_size, first_type = struct.unpack_from("<HH", symbols)
            first_pos = 0
            if first_type == 0 and first_size == 2:
                first_pos = 4
                first_size, first_type = struct.unpack_from("<HH", symbols, first_pos)
            if first_type == 0x0009:
                obj = object_name(symbols[first_pos + 8:first_pos + 2 + first_size].split(b"\0", 1)[0].decode("latin1"))
        pos = 0
        while pos + 4 <= len(symbols):
            record_size, record_type = struct.unpack_from("<HH", symbols, pos)
            record_end = pos + 2 + record_size
            if record_size < 2 or record_end > len(symbols):
                break
            if record_type in (0x100A, 0x100B) and record_size >= 37:
                payload = symbols[pos + 4:record_end]
                code_length = struct.unpack_from("<I", payload, 12)[0]
                offset, segment = struct.unpack_from("<IH", payload, 28)
                name_length = payload[35]
                name = payload[36:36 + name_length].decode("latin1", "replace")
                result.append(Symbol(name, obj, offset, str(segment), code_length,
                                     line_ranges.get(offset)))
            pos = record_end
    return result


def parse_map(path: Path) -> tuple[int, list[Symbol], dict[int, int]]:
    text = path.read_text(encoding="latin1")
    base_match = MAP_BASE.search(text)
    if not base_match:
        raise RuntimeError(f"no preferred load address in {path}")
    symbols = []
    segment_bases = {}
    in_publics = False
    for line in text.splitlines():
        if "Publics by Value" in line:
            in_publics = True
            continue
        if not in_publics:
            continue
        match = MAP_PUBLIC.match(line)
        if match and match["flags"].startswith("f"):
            segment = int(match["seg"], 16)
            offset = int(match["off"], 16)
            address = int(match["addr"], 16)
            segment_bases.setdefault(segment, address - offset)
            symbols.append(Symbol(match["name"], object_name(match["obj"]), address))
    if not symbols:
        raise RuntimeError(f"no public symbols found in {path}")
    # MAP files are normally sorted by address, but old linkers occasionally
    # emit a late COMDAT block out of order.  Boundaries must be address-based.
    symbols.sort(key=lambda item: item.address)
    for current, following in zip(symbols, symbols[1:]):
        if following.address > current.address:
            current.length = following.address - current.address
    return int(base_match.group(1), 16), symbols, segment_bases


def section_bytes(path: Path, section: str, objdump: str) -> bytes:
    output = run(objdump, "-s", "-j", section, str(path))
    data = bytearray()
    for line in output.splitlines():
        fields = line.split()
        if not fields or not re.fullmatch(r"[0-9A-Fa-f]+", fields[0]):
            continue
        for field in fields[1:]:
            if re.fullmatch(r"[0-9A-Fa-f]{8}", field):
                data.extend(bytes.fromhex(field))
            else:
                break
    return bytes(data)


def section_vma(path: Path, section: str, objdump: str) -> int:
    for line in run(objdump, "-h", str(path)).splitlines():
        fields = line.split()
        if len(fields) >= 5 and fields[1] == section:
            return int(fields[3], 16)
    raise RuntimeError(f"section {section} was not found in {path}")


def local_symbols(path: Path, objdump: str) -> list[Symbol]:
    output = run(objdump, "-t", str(path))
    section_output = run(objdump, "-h", str(path))
    sections = {}
    text_sections = []
    for line in section_output.splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[0].isdigit() and re.fullmatch(r"[0-9A-Fa-f]+", fields[2]):
            index, name, size = int(fields[0]), fields[1], int(fields[2], 16)
            sections[index + 1] = (name, size)
            if name == ".text" or name.startswith(".text$"):
                text_sections.append((index + 1, size))
    found = []
    for line in output.splitlines():
        match = re.search(
            r"\(sec\s+(-?\d+)\).*?\(ty\s+20\).*?\(scl\s+[23]\).*?"
            r"0x([0-9A-Fa-f]+)\s+(\S+)\s*$", line
        )
        if not match:
            continue
        section_number = int(match.group(1))
        section_info = sections.get(section_number)
        if not section_info or not section_info[0].startswith(".text"):
            continue
        section = f"{section_info[0]}@{section_number}"
        found.append(Symbol(match.group(3), path.name, int(match.group(2), 16), section))
    found.sort(key=lambda item: item.address)
    for current, following in zip(found, found[1:]):
        if current.section == following.section and following.address > current.address:
            current.length = following.address - current.address
    section_sizes = {
        f"{section}@{number}": size
        for number, (section, size) in sections.items()
        if f"{section}@{number}" in {item.section for item in found}
    }
    for item in found:
        item.length = item.length or section_sizes.get(item.section)
    text_data = section_bytes(path, ".text", objdump)
    section_data = {}
    text_offset = 0
    for number, size in text_sections:
        section_data[f".text@{number}"] = text_data[text_offset:text_offset + size]
        text_offset += size
    for item in found:
        text = section_data[item.section]
        if item.length and item.address + item.length <= len(text):
            item.data = text[item.address:item.address + item.length]
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=Path("Build"))
    parser.add_argument("--client", type=Path, default=Path("WoW/Client/WoWClient.exe"))
    parser.add_argument("--pdb", type=Path, default=Path("WoW/Client/Wowae.pdb"))
    parser.add_argument("--map", type=Path, default=Path("WoW/Client/MapFiles/Wowae.map"))
    args = parser.parse_args()
    for path in (args.build, args.client, args.pdb, args.map):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")
    try:
        objdump = tool("llvm-objdump", "objdump")
        _, map_symbols, segment_bases = parse_map(args.map)
        source_lengths = {}
        for symbol in parse_legacy_pdb(args.pdb):
            try:
                symbol.address = segment_bases[int(symbol.section)] + symbol.address
            except KeyError:
                continue
            if symbol.source_length is not None:
                source_lengths[(symbol.obj, symbol.address)] = symbol.source_length
        reference = [
            Symbol(symbol.name, symbol.obj, symbol.address, length=symbol.length,
                   source_length=source_lengths.get((symbol.obj, symbol.address)))
            for symbol in map_symbols
        ]
        text_vma = section_vma(args.client, ".text", objdump)
        reference_data = section_bytes(args.client, ".text", objdump)
        local = {}
        object_paths = sorted(
            path for path in args.build.rglob("*")
            if path.is_file()
            and path.suffix.lower() in {".obj", ".o"}
            and not {"CompilerIdC", "CompilerIdCXX"}.intersection(path.parts)
        )
        for path in object_paths:
            for symbol in local_symbols(path, objdump):
                local[(object_name(symbol.obj), symbol.name)] = symbol
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    counts = [len(reference), 0, 0, 0, 0, 0, 0]
    reference_keys = set()
    mismatches = []
    for expected in reference:
        key = (expected.obj, expected.name)
        reference_keys.add(key)
        actual = local.get(key)
        if actual is None:
            counts[5] += 1
            continue
        # Some MAP entries are aliases or the final symbol in a section and
        # therefore have no recoverable reference byte length. Presence is
        # still a valid exact match for those reference symbols.
        if expected.length is None:
            counts[1] += 1
            continue
        if actual.source_length is not None and actual.source_length != expected.source_length:
            counts[4] += 1
            mismatches.append(("Source-Length Mismatch", expected, actual))
        elif actual.length != expected.length:
            counts[3] += 1
            mismatches.append(("Byte-Length Mismatch", expected, actual))
        elif actual.data != reference_data[expected.address - text_vma:expected.address - text_vma + expected.length]:
            counts[2] += 1
            mismatches.append(("Byte-Output Mismatch", expected, actual))
        else:
            counts[1] += 1
    counts[6] = len(set(local) - reference_keys)
    mismatch = sum(counts[2:5])

    def print_table(headers: list[str], values: list[int]) -> None:
        widths = [max(len(header), len(str(value))) for header, value in zip(headers, values)]
        print("  ".join(f"{header:<{width}}" for header, width in zip(headers, widths)).rstrip())
        print("  ".join(f"{value:<{width}}" for value, width in zip(values, widths)).rstrip())

    def print_rows(headers: list[str], rows: list[list[str]]) -> None:
        widths = [len(header) for header in headers]
        for row in rows:
            for index, value in enumerate(row):
                widths[index] = max(widths[index], len(value))
        print("  ".join(f"{header:<{width}}" for header, width in zip(headers, widths)).rstrip())
        for row in rows:
            print("  ".join(f"{value:<{width}}" for value, width in zip(row, widths)).rstrip())

    print_table(
        ["Total", "Exact Match", "Mismatch", "Missing", "Extra"],
        [counts[0], counts[1], mismatch, counts[5], counts[6]],
    )
    print()
    print_table(
        ["Byte-Output Mismatch", "Byte-Length Mismatch", "Source-Length Mismatch"],
        counts[2:5],
    )
    print()
    print("-" * 80)
    print("Mismatches")
    if mismatches:
        print_rows(
            ["Type", "Object", "Symbol", "Reference Length", "Local Length"],
            [[kind, expected.obj, expected.name, str(expected.length or ""),
              str(actual.length or "")] for kind, expected, actual in mismatches],
        )
    else:
        print("None")
    print()
    print("Extra")
    extras = sorted(set(local) - reference_keys)
    if extras:
        print_rows(
            ["Object", "Symbol"],
            [[obj, local[(obj, name)].name] for obj, name in extras],
        )
    else:
        print("None")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
