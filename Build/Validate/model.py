
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Mapping


@dataclass(frozen=True)
class Diagnostic:
    severity: str
    code: str
    message: str
    context: Mapping[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class ImageIdentity:
    path: Path
    machine: int
    image_base: int
    pe_timestamp: int
    codeview_kind: str | None
    codeview_signature: int | None
    codeview_age: int | None
    embedded_pdb_path: str | None


@dataclass(frozen=True)
class ValueEvidence:
    offset: int
    size: int
    kind: str
    target_identity: str | None
    resolved: bool
    detail: Mapping[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class Section:
    number: int
    name: str
    rva: int
    virtual_size: int
    raw_offset: int
    raw_size: int
    characteristics: int
    data: bytes

    @property
    def executable(self) -> bool:
        return bool(self.characteristics & 0x20000000)


@dataclass(frozen=True)
class MapSymbol:
    name: str
    rva: int
    segment: int
    offset: int
    flags: tuple[str, ...] = ()
    object_name: str | None = None


@dataclass(frozen=True)
class ArtifactData:
    identity: ImageIdentity
    sections: tuple[Section, ...]
    map_symbols: tuple[MapSymbol, ...]
    imports: Mapping[int, str]
    diagnostics: tuple[Diagnostic, ...] = ()
    fatal: bool = False

    @property
    def executable_code_bytes(self) -> int:
        return sum(section.raw_size for section in self.sections if section.executable)

    def read_rva(self, rva: int, size: int) -> bytes | None:
        if size < 0:
            return None
        for section in self.sections:
            offset = rva - section.rva
            if offset >= 0 and offset + size <= min(section.raw_size, len(section.data)):
                return section.data[offset : offset + size]
        return None

    def resolve_va(self, value: int) -> int | None:
        rva = value - self.identity.image_base
        return rva if any(s.rva <= rva < s.rva + s.virtual_size for s in self.sections) else None
