

from __future__ import annotations

from pathlib import Path
import os
import sys


BUILD_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(BUILD_DIR))


posix_python = BUILD_DIR / "Validate/.venv/bin/python"
windows_python = BUILD_DIR / "Validate/.venv/Scripts/python.exe"
local_python = posix_python if posix_python.exists() else windows_python
local_environment = BUILD_DIR / "Validate/.venv"
if __name__ == "__main__" and local_python.exists() and Path(sys.prefix).resolve() != local_environment.resolve():
    try:
        import capstone
    except ImportError:
        os.execv(str(local_python), [str(local_python), str(Path(__file__).resolve()), *sys.argv[1:]])

from Validate.cli import main


if __name__ == "__main__":
    raise SystemExit(main())
