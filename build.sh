#!/bin/sh
set -eu

cd -- "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

if [ "${1:-}" = vc6 ]; then
    shift
    Build/wine.sh install
    Build/wine.sh setup "$@"
    Build/wine.sh build
    printf '%s\n' "Built $PWD/Build/WoW/bin/Wowae.exe"
    exit
fi

case "$(uname -s)" in
    Darwin) preset=macos-makefile output="World of Warcraft.app" ;;
    Linux) preset=linux output=Wow64ae ;;
    *) printf '%s\n' "error: unsupported host $(uname -s), use build.bat on Windows" >&2; exit 1 ;;
esac

cmake --preset "$preset" "$@"
cmake --build --preset "$preset" --parallel
printf '%s\n' "Built $PWD/Build/$preset/WoW/bin/$output"
