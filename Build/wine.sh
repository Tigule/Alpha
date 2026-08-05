#!/bin/sh
set -eu

VC6_URL="https://tigule.org/files/ci/VC6.zip"
DXSDK_URL="https://tigule.org/files/ci/DXSDK90.zip"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v4.3.2/cmake-4.3.2-windows-x86_64.msi"
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CROSSOVER_BOTTLE=${CROSSOVER_BOTTLE:-}
if [ -z "${CROSSOVER_ROOT:-}" ]; then
    if [ -d "$HOME/Applications/CrossOver.app" ]; then
        CROSSOVER_ROOT="$HOME/Applications/CrossOver.app/Contents/SharedSupport/CrossOver"
    else
        CROSSOVER_ROOT="/Applications/CrossOver.app/Contents/SharedSupport/CrossOver"
    fi
fi
if [ -n "$CROSSOVER_BOTTLE" ]; then
    WINEPREFIX=${WINEPREFIX:-"$HOME/Library/Application Support/CrossOver/Bottles/$CROSSOVER_BOTTLE"}
    WINE="$CROSSOVER_ROOT/bin/wine"
else
    WINEPREFIX=${WINEPREFIX:-"$HOME/.wine"}
    WINE=wine
fi
DRIVE_C="$WINEPREFIX/drive_c"
CACHE_DIR=${XDG_CACHE_HOME:-"$HOME/.cache"}
CACHE_DIR="$CACHE_DIR/tigule-alpha-wine"

VC6_DIR="$DRIVE_C/VC6"
VCVARS_BAT="$VC6_DIR/VC98/Bin/VCVARS32.BAT"
DXSDK_DIR="$DRIVE_C/DXSDK"
DXSDK_LIB="$DXSDK_DIR/Lib/d3dx9.lib"
CMAKE_EXE='C:\Program Files\CMake\bin\cmake.exe'
CMAKE_EXE_UNIX="$DRIVE_C/Program Files/CMake/bin/cmake.exe"
WOW_EXE=${WOW_EXE:-"$SCRIPT_DIR/WoW/Wow.exe"}
WOW_CLIENT_DIR="$SCRIPT_DIR/../WoW/Client"
WOW_CLIENT_EXE="$WOW_CLIENT_DIR/TiguleClient.exe"
WOW_REF_EXE="$WOW_CLIENT_DIR/WoWClient.exe"
if [ "$(uname -s)" = Darwin ]; then
    WOW_REF_EXE="$WOW_CLIENT_DIR/WoWMacClient.exe"
fi

die() {
    printf '%s\n' "error: $*" >&2
    exit 1
}

usage() {
    cat <<EOF
Usage: $(basename "$0") <command>

Commands:
  install  Download and install VC6, DirectX SDK, and CMake
  setup    Configure this checkout with VC6 NMake makefiles
  build    Build the configured tree with nmake
  run      Copy Wow.exe into WoW/Client and start TiguleClient.exe
  run-ref  Start the original reference client (WoWMacClient.exe on macOS)
  all      Run install, setup, then build

Environment:
  WINEPREFIX  Wine prefix to use (default: $HOME/.wine)
  CROSSOVER_BOTTLE  Use CrossOver's named bottle instead of Wine
  CROSSOVER_ROOT  CrossOver installation root
  WOW_EXE  Wow.exe path (default: <repo>/Build/WoW/Wow.exe)
EOF
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "$1 is required but was not found in PATH"
}

require_wine_prefix() {
    [ -x "$WINE" ] || die "Wine runner was not found: $WINE"
    [ -d "$DRIVE_C" ] || die "Wine prefix is not initialized: $DRIVE_C"
}

run_wine() {
    if [ -n "$CROSSOVER_BOTTLE" ]; then
        "$WINE" --bottle "$CROSSOVER_BOTTLE" "$@"
    else
        "$WINE" "$@"
    fi
}

download() {
    url=$1
    dest=$2

    if [ -f "$dest" ]; then
        printf '%s\n' "Using cached $(basename "$dest")"
        return
    fi

    mkdir -p "$(dirname "$dest")"

    if command -v curl >/dev/null 2>&1; then
        curl -fL "$url" -o "$dest"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$dest" "$url"
    else
        die "curl or wget is required to download $(basename "$dest")"
    fi
}

patch_vcvars() {
    require_command perl

    [ -f "$VCVARS_BAT" ] || die "VCVARS32.BAT was not found at $VCVARS_BAT"

    perl -0pi -e 's|set VSCommonDir=C:\\PROGRA~2\\Microsoft Visual Studio 6.0\\Common|set VSCommonDir=C:\\VC6\\Common|g; s|set MSDevDir=C:\\PROGRA~2\\Microsoft Visual Studio 6.0\\Common\\msdev98|set MSDevDir=C:\\VC6\\Common\\MSDev98|g; s|set MSVCDir=C:\\PROGRA~2\\Microsoft Visual Studio 6.0\\VC98|set MSVCDir=C:\\VC6\\VC98|g' "$VCVARS_BAT"
}

install_vc6() {
    vc6_zip="$CACHE_DIR/VC6.zip"

    download "$VC6_URL" "$vc6_zip"

    if [ ! -f "$VCVARS_BAT" ]; then
        require_command unzip
        mkdir -p "$VC6_DIR"
        unzip -q "$vc6_zip" -d "$VC6_DIR"
    fi

    patch_vcvars
}

install_dxsdk() {
    dxsdk_zip="$CACHE_DIR/DXSDK90.zip"

    if [ -f "$DXSDK_DIR/Include/d3dx9.h" ] && [ -f "$DXSDK_LIB" ]; then
        printf '%s\n' "DirectX SDK is already installed in this Wine prefix"
        return
    fi

    download "$DXSDK_URL" "$dxsdk_zip"
    require_command unzip
    mkdir -p "$DXSDK_DIR"
    unzip -q "$dxsdk_zip" -d "$DXSDK_DIR"

    [ -f "$DXSDK_DIR/Include/d3dx9.h" ] || die "DirectX SDK extraction completed, but $DXSDK_DIR/Include/d3dx9.h was not found"
    [ -f "$DXSDK_LIB" ] || die "DirectX SDK extraction completed, but $DXSDK_LIB was not found"
}

install_cmake() {
    cmake_msi="$CACHE_DIR/cmake-4.3.2-windows-x86_64.msi"

    if [ -f "$CMAKE_EXE_UNIX" ]; then
        printf '%s\n' "CMake is already installed in this Wine prefix"
        return
    fi

    download "$CMAKE_URL" "$cmake_msi"
    cmake_msi_win=$(run_wine winepath.exe -w "$cmake_msi")
    WINEDEBUG=-all run_wine msiexec /i "$cmake_msi_win" /qn /norestart

    [ -f "$CMAKE_EXE_UNIX" ] || die "CMake installer completed, but $CMAKE_EXE_UNIX was not found"
}

install() {
    require_wine_prefix
    install_vc6
    install_dxsdk
    install_cmake
}

require_toolchain() {
    require_wine_prefix
    [ -f "$VCVARS_BAT" ] || die "VC6 is not installed. Run: $(basename "$0") install"
    [ -f "$DXSDK_DIR/Include/d3dx9.h" ] || die "DirectX SDK is not installed. Run: $(basename "$0") install"
    [ -f "$DXSDK_LIB" ] || die "DirectX SDK is not installed. Run: $(basename "$0") install"
    [ -f "$CMAKE_EXE_UNIX" ] || die "CMake is not installed in Wine. Run: $(basename "$0") install"
}

run_vc6_cmd() {
    require_toolchain

    tmp_bat=$(mktemp "${TMPDIR:-/tmp}/tigule-alpha-wine.XXXXXX.bat")
    trap 'rm -f "$tmp_bat"' EXIT HUP INT TERM

    build_dir_win=$(run_wine winepath.exe -w "$SCRIPT_DIR")
    tmp_bat_win=$(run_wine winepath.exe -w "$tmp_bat")

    {
        printf '@echo off\r\n'
        printf 'call "C:\\VC6\\VC98\\Bin\\VCVARS32.BAT"\r\n'
        printf 'set DXSDK_DIR=C:\\DXSDK\r\n'
        printf 'cd /d "%s"\r\n' "$build_dir_win"
        while [ "$#" -gt 0 ]; do
            printf '%s\r\n' "$1"
            shift
        done
    } > "$tmp_bat"

    status=0
    WINEDEBUG=-all run_wine cmd /c "$tmp_bat_win" || status=$?
    rm -f "$tmp_bat"
    trap - EXIT HUP INT TERM
    return "$status"
}

setup() {
    run_vc6_cmd "\"$CMAKE_EXE\" -DCMAKE_BUILD_TYPE=Release -G \"NMake Makefiles\" .."
}

build() {
    run_vc6_cmd "nmake"
}

run() {
    require_wine_prefix
    [ -f "$WOW_EXE" ] || die "Wow.exe was not found: $WOW_EXE"

    mkdir -p "$WOW_CLIENT_DIR"
    cp "$WOW_EXE" "$WOW_CLIENT_EXE"
    cd "$WOW_CLIENT_DIR"
    WINEDEBUG=-all run_wine "$WOW_CLIENT_EXE" -uptodate -windowed "$@"
}

run_ref() {
    require_wine_prefix
    [ -f "$WOW_REF_EXE" ] || die "Reference client was not found: $WOW_REF_EXE"

    cd "$WOW_CLIENT_DIR"
    WINEDEBUG=-all run_wine "$WOW_REF_EXE" -uptodate -windowed "$@"
}

case "${1:-}" in
    install)
        install
        ;;
    setup)
        setup
        ;;
    build)
        build
        ;;
    run)
        shift
        run "$@"
        ;;
    run-ref)
        shift
        run_ref "$@"
        ;;
    all)
        install
        setup
        build
        ;;
    -h|--help|help|"")
        usage
        ;;
    *)
        usage >&2
        die "unknown command: $1"
        ;;
esac
