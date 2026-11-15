from __future__ import annotations

import hashlib
import os
import re
import struct
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Mapping, Sequence


VCVARS = Path(r"C:\Program Files (x86)\Microsoft Visual Studio 6.0\VC98\Bin\VCVARS32.BAT")
_DIR32 = 0x06
_REL32 = 0x14
_SECREL = 0x0B
_PROC_KINDS = {0x0204, 0x0205, 0x100A, 0x100B, 0x110F, 0x1110}


@dataclass
class _Rule:
    compiler: str
    cwd: Path
    text: str
    pch_text: str | None
    pch_cwd: Path | None
    pch_file: Path | None
    object_path: Path


@dataclass
class _Object:
    sections: list[tuple[bytes, bytes, list[tuple[int, int, int]]]]
    symbols: dict[int, tuple[str, int, int, int, int]]
    procs: dict[str, tuple[int, int, int]] = field(default_factory=dict)


def _parse_object(data: bytes) -> _Object:
    count, _, symbol_pointer, symbol_count = struct.unpack_from("<HIII", data, 2)
    optional = struct.unpack_from("<H", data, 16)[0]
    sections = []
    for index in range(count):
        header = 20 + optional + index * 40
        name = data[header:header + 8].rstrip(b"\0")
        size, pointer, relocation_pointer, _, relocation_count = struct.unpack_from("<IIIIH", data, header + 16)
        relocations = [struct.unpack_from("<IIH", data, relocation_pointer + k * 10) for k in range(relocation_count)]
        sections.append((name, data[pointer:pointer + size] if pointer else b"", relocations))
    strings = symbol_pointer + symbol_count * 18
    symbols = {}
    index = 0
    while index < symbol_count:
        at = symbol_pointer + index * 18
        raw = data[at:at + 8]
        if raw[:4] == b"\0\0\0\0":
            start = strings + struct.unpack_from("<I", raw, 4)[0]
            name = data[start:data.index(b"\0", start)].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        value, section, kind, storage, aux = struct.unpack_from("<IhHBB", data, at + 8)
        symbols[index] = (name, value, section, storage, kind)
        index += 1 + aux
    result = _Object(sections, symbols)
    for name, blob, relocations in sections:
        if name != b".debug$S" or len(blob) < 4:
            continue
        secrel = sorted((offset, symbol) for offset, symbol, kind in relocations if kind == _SECREL)
        position = 4 if struct.unpack_from("<I", blob, 0)[0] in (1, 2, 4) else 0
        while position + 4 <= len(blob):
            length, kind = struct.unpack_from("<HH", blob, position)
            if length < 2:
                break
            end = position + 2 + length
            if kind in _PROC_KINDS:
                hit = next(((offset, symbol) for offset, symbol in secrel if position < offset < end), None)
                if hit is not None and hit[1] in symbols:
                    proc_length = struct.unpack_from("<I", blob, position + 4 + 12)[0]
                    addend = struct.unpack_from("<I", blob, hit[0])[0]
                    target, value, section, _, _ = symbols[hit[1]]
                    result.procs.setdefault(target, (section, value + addend, proc_length))
            position = end
    return result


def _function(obj: _Object, names: Sequence[str]) -> tuple[bytes, list[tuple[Any, ...]], int] | None:
    for name in names:
        proc = obj.procs.get(name)
        if proc is None:
            continue
        section, start, length = proc
        if section <= 0 or section > len(obj.sections):
            return None
        _, data, relocations = obj.sections[section - 1]
        if start + length > len(data):
            return None
        code = data[start:start + length]
        found = []
        for offset, symbol, kind in relocations:
            if start <= offset < start + length:
                target, value, target_section, _, symbol_kind = obj.symbols.get(symbol, ("", 0, 0, 0, 0))
                found.append((offset - start, kind, target, value - start if target_section == section else None,
                              target_section, bool(symbol_kind & 0x20)))
        return code, sorted(found, key=lambda value: value[0]), section
    return None


def _masked(code: bytes, relocations: Sequence[tuple[Any, ...]]) -> bytes:
    mutable = bytearray(code)
    for offset, *_ in relocations:
        mutable[offset:offset + 4] = b"\0\0\0\0"
    return bytes(mutable)


class LineProof:
    def __init__(self, build: Path, cache: Path):
        self.build = build
        self.cache = cache
        self.rules: dict[tuple[str, str], _Rule] | None = None
        self.environment: dict[str, str] | None = None
        self.unavailable: str | None = None
        if not VCVARS.exists():
            self.unavailable = "unavailable: VC6 environment not found"
        elif not build.is_dir() or next(build.rglob("build.make"), None) is None:
            self.unavailable = "unavailable: no build tree"

    def _scan(self) -> None:
        self.rules = {}
        for path in self.build.rglob("build.make"):
            if not path.parent.name.endswith(".dir"):
                continue
            lines = path.read_text(errors="replace").splitlines()
            compile_lines = []
            for index, line in enumerate(lines):
                if not ("$(CXX_FLAGS)" in line or "$(C_FLAGS)" in line) or " -c " not in line:
                    continue
                if "/Fo" not in line or "/FoNUL" in line:
                    continue
                cwd = compiler = None
                for back in range(index - 1, max(-1, index - 6), -1):
                    text = lines[back].strip()
                    if compiler is None and text.endswith("@<<"):
                        compiler = text[:-3].strip()
                    if text.startswith("cd "):
                        cwd = Path(text[3:].strip())
                        break
                if cwd is None or compiler is None:
                    continue
                compile_lines.append((line, cwd, compiler))
            flags = path.parent / "flags.make"
            if not flags.exists():
                continue
            table = dict(re.findall(r"^(C(?:XX)?_(?:DEFINES|INCLUDES|FLAGS)) = (.*)$", flags.read_text(errors="replace"), re.M))
            pch = next(((line, cwd) for line, cwd, _ in compile_lines if " /Yc" in line), None)
            for line, cwd, compiler in compile_lines:
                if " /Yc" in line:
                    continue
                source = line.rsplit(" -c ", 1)[1].strip()
                output = re.search(r"/Fo(\S+)", line).group(1)
                text = self._expand(line, table)
                pch_text = pch_cwd = pch_file = None
                if " /Yu" in line and pch is not None:
                    pch_text, pch_cwd = self._expand(pch[0], table), pch[1]
                    match = re.search(r"/Fp(\S+)", line)
                    pch_file = Path(match.group(1)) if match else None
                self.rules[(str(cwd / output).lower().replace("/", "\\"), source.lower())] = _Rule(
                    compiler, cwd, text, pch_text, pch_cwd, pch_file, cwd / output)

    @staticmethod
    def _expand(line: str, table: Mapping[str, str]) -> str:
        return re.sub(r"\$\((C(?:XX)?_(?:DEFINES|INCLUDES|FLAGS))\)", lambda m: table.get(m.group(1), ""), line).strip()

    def _environment(self) -> dict[str, str]:
        if self.environment is None:
            output = subprocess.run(f'cmd /c ""{VCVARS}" >nul && set"', capture_output=True, text=True).stdout
            environment = dict(os.environ)
            for line in output.splitlines():
                key, separator, value = line.partition("=")
                if separator:
                    environment[key] = value
            temp = self.cache / "tmp"
            temp.mkdir(parents=True, exist_ok=True)
            environment["TMP"] = environment["TEMP"] = str(temp)
            self.environment = environment
        return self.environment

    def rule(self, module_name: str, source: str) -> _Rule | None:
        if self.rules is None:
            self._scan()
        assert self.rules is not None
        module = module_name.lower().replace("/", "\\")
        for (output, rule_source), rule in self.rules.items():
            if rule_source == source.lower() and output.endswith("\\" + module):
                return rule
        return None

    def _run(self, compiler: str, cwd: Path, arguments: str, tag: str) -> subprocess.CompletedProcess[str]:
        response = self.cache / f"{tag}.rsp"
        response.write_text(arguments + "\n")
        return subprocess.run([compiler, f"@{response}"], cwd=str(cwd), capture_output=True, text=True,
                              env=self._environment())

    @staticmethod
    def _strip(text: str, *prefixes: str) -> str:
        return " ".join(token for token in text.split() if not token.startswith(prefixes))

    def _pch(self, rule: _Rule) -> str | None:
        assert rule.pch_text is not None and rule.pch_cwd is not None
        stamp = hashlib.sha256()
        stamp.update(rule.pch_text.encode())
        if rule.pch_file is not None and rule.pch_file.exists():
            stamp.update(rule.pch_file.read_bytes())
        key = stamp.hexdigest()[:20]
        folder = self.cache / "pch" / key
        pch, pdb = folder / "p.pch", folder / "p.pdb"
        if not pch.exists():
            folder.mkdir(parents=True, exist_ok=True)
            text = self._strip(rule.pch_text, "/Fp", "/Fd", "/Fo", "/YX")
            text = text.replace(" -c ", f' /Fp"{pch}" /Fd"{pdb}" /Fo"{folder / "pch.obj"}" -c ', 1)
            self._run(rule.compiler, rule.pch_cwd, text, f"pch_{key}")
            if not pch.exists():
                return None
        return f'/Fp"{pch}" /Fd"{pdb}"'

    def compile(self, rule: _Rule, source_text: bytes, name: str, tag: str) -> _Object | None:
        stamp = hashlib.sha256()
        stamp.update(rule.text.encode())
        stamp.update(source_text)
        if rule.object_path.exists():
            stamp.update(rule.object_path.read_bytes())
        if rule.pch_file is not None and rule.pch_file.exists():
            stamp.update(rule.pch_file.read_bytes())
        key = stamp.hexdigest()[:24]
        cached = self.cache / "obj" / f"{key}.obj"
        if cached.exists():
            return _parse_object(cached.read_bytes())
        folder = self.cache / "src" / key
        folder.mkdir(parents=True, exist_ok=True)
        scratch = folder / name
        scratch.write_bytes(source_text)
        original_source = rule.text.rsplit(" -c ", 1)[1].strip()
        text = rule.text.rsplit(" -c ", 1)[0]
        text = self._strip(text, "/Fo", "/Fd", "/YX")
        if rule.pch_text is not None:
            pch = self._pch(rule)
            if pch is None:
                return None
            text = self._strip(text, "/Fp") + " " + pch
        else:
            text += f' /Fd"{folder / "p.pdb"}"'
        include = f'/I"{Path(original_source).parent}"'
        text = text.replace("/nologo", "/nologo " + include, 1)
        (self.cache / "obj").mkdir(parents=True, exist_ok=True)
        output = folder / "out.obj"
        result = self._run(rule.compiler, rule.cwd, f'{text} /Fo"{output}" -c "{scratch}"', tag)
        if not output.exists():
            (folder / "error.txt").write_text(result.stdout + result.stderr)
            return None
        data = output.read_bytes()
        cached.write_bytes(data)
        return _parse_object(data)


def remap_source(source: bytes, path: str, remap: Mapping[int, int]) -> bytes:
    lines = source.splitlines(keepends=True)
    escaped = path.replace("\\", "\\\\")
    out = [f'#line 1 "{escaped}"\n'.encode()]
    for number, line in enumerate(lines, 1):
        if number in remap:
            out.append(f"#line {remap[number]}\n".encode())
            out.append(line if line.endswith(b"\n") else line + b"\n")
            out.append(f"#line {number + 1}\n".encode())
        else:
            out.append(line)
    return b"".join(out)


def synthesize(base_obj: _Object, base: tuple[bytes, list[tuple[Any, ...]], int],
               fixed_obj: _Object, fixed: tuple[bytes, list[tuple[Any, ...]], int],
               rebuilt_code: bytes, rebuilt_va: int) -> tuple[bytes | None, str]:
    base_code, base_relocations, _ = base
    if len(base_code) != len(rebuilt_code) or _masked(base_code, base_relocations) != _masked(rebuilt_code, base_relocations):
        return None, "probe of the unmodified source does not reproduce the linked function"
    addresses: dict[tuple[str, int], int] = {}
    for offset, kind, name, internal, target_section, _ in base_relocations:
        if internal is not None and 0 <= internal <= len(base_code):
            continue
        addend = struct.unpack_from("<i", base_code, offset)[0]
        value = struct.unpack_from("<i", rebuilt_code, offset)[0]
        if kind == _DIR32:
            address = (value - addend) & 0xFFFFFFFF
        elif kind == _REL32:
            address = (rebuilt_va + offset + 4 + value - addend) & 0xFFFFFFFF
        else:
            return None, f"unsupported relocation type 0x{kind:x}"
        if addresses.setdefault((name, target_section), address) != address:
            return None, f"relocation symbol {name} resolves to two addresses"
    code = bytearray(fixed[0])
    for offset, kind, name, internal, target_section, is_function in fixed[1]:
        addend = struct.unpack_from("<i", code, offset)[0]
        if internal is not None and 0 <= internal <= len(code):
            address = rebuilt_va + internal
        elif (name, target_section) in addresses:
            if target_section == fixed[2] and name.startswith("."):
                return None, f"code section relative relocation {name} outside the function"
            if target_section > 0 and target_section != fixed[2] and not is_function:
                if (target_section > len(base_obj.sections) or target_section > len(fixed_obj.sections)
                        or base_obj.sections[target_section - 1][:2] != fixed_obj.sections[target_section - 1][:2]):
                    return None, f"data section of relocation symbol {name} differs between probes"
            address = addresses[(name, target_section)]
        else:
            return None, f"relocation symbol {name} is not referenced by the unmodified function"
        if kind == _DIR32:
            value = address + addend
        elif kind == _REL32:
            value = address + addend - (rebuilt_va + offset + 4)
        else:
            return None, f"unsupported relocation type 0x{kind:x}"
        struct.pack_into("<I", code, offset, value & 0xFFFFFFFF)
    return bytes(code), "synthesized"


def function_code(obj: _Object, names: Sequence[str]) -> tuple[bytes, list[tuple[Any, ...]], int] | None:
    return _function(obj, names)


__all__ = ["LineProof", "function_code", "remap_source", "synthesize", "VCVARS"]
