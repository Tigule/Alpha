from __future__ import annotations

from dataclasses import dataclass
import hashlib
import os
from pathlib import Path
import stat
from typing import Iterable


class InputSnapshotError(OSError):
    pass


@dataclass(frozen=True)
class InputSnapshot:
    path: Path
    data: bytes
    size: int
    sha256: str

    def record(self) -> dict[str, object]:
        return {
            "path": str(self.path),
            "size": self.size,
            "sha256": self.sha256,
        }


def _stat_signature(value: os.stat_result) -> tuple[int, int, int, int, int, int]:
    return (
        value.st_dev,
        value.st_ino,
        value.st_mode,
        value.st_size,
        value.st_mtime_ns,
        value.st_ctime_ns,
    )


def capture_input(path: str | Path) -> InputSnapshot:
    try:
        normalized = Path(path).expanduser().resolve(strict=True)
        path_before = normalized.stat()
        if not stat.S_ISREG(path_before.st_mode):
            raise InputSnapshotError(f"{normalized}: input is not a regular file")
        with normalized.open("rb", buffering=0) as source:
            descriptor_before = os.fstat(source.fileno())
            if not os.path.samestat(path_before, descriptor_before):
                raise InputSnapshotError(f"{normalized}: input changed while it was opened")
            if not stat.S_ISREG(descriptor_before.st_mode):
                raise InputSnapshotError(f"{normalized}: input is not a regular file")
            data = source.read()
            descriptor_after = os.fstat(source.fileno())
        path_after = normalized.stat()
    except InputSnapshotError:
        raise
    except OSError as exc:
        raise InputSnapshotError(f"{Path(path).expanduser()}: cannot capture input: {exc}") from exc
    if _stat_signature(descriptor_before) != _stat_signature(descriptor_after):
        raise InputSnapshotError(f"{normalized}: input changed while it was read")
    if (
        _stat_signature(path_before) != _stat_signature(path_after)
        or not os.path.samestat(descriptor_after, path_after)
    ):
        raise InputSnapshotError(f"{normalized}: input path changed while it was read")
    if len(data) != descriptor_after.st_size:
        raise InputSnapshotError(f"{normalized}: captured size does not match input metadata")
    return InputSnapshot(normalized, data, len(data), hashlib.sha256(data).hexdigest())


def capture_inputs(paths: Iterable[str | Path]) -> tuple[InputSnapshot, ...]:
    return tuple(capture_input(path) for path in paths)


__all__ = ["InputSnapshot", "InputSnapshotError", "capture_input", "capture_inputs"]
