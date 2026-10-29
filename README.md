<div align="center">
<h1>Tigule Alpha</h1>

<a href="https://tigule.github.io/Alpha"><img src="https://tigule.github.io/Alpha/badges/implemented.svg"></a> <a href="https://tigule.github.io/Alpha"><img src="https://tigule.github.io/Alpha/badges/code-functions.svg"></a>

This project has 3 goals: to understand and document, reconstruct, and preserve the earliest archived alpha version of  
World of Warcraft (version 0.5.3, build 3368)
</div>

**Disclaimer**:
*You cannot play retail World of Warcraft with this*. The code only communicates with an early protocol and assets. This is simply a time capsule for those after us.

## Contribution Guide

1. C/C++ standards must be restricted to the features available in Visual C++ 6.0 (1998)
2. Use [Windows datatypes](https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types). Non-windows targets can use the same datatypes through `otherwin32.h`
3. Validate your output to check if it's a measurable improvement

We are aiming to achieve ~100% accuracy before supporting other OS and architecture targets.

## Building

Install these dependencies:
- CMake: [cmake.org](https://cmake.org/download/).
- Visual Studio 6.0 Professional Edition: [archive.org](https://archive.org/details/vsp600enu)
- Service Pack 5 for Microsoft Visual Studio 6.0: [archive.org](https://archive.org/details/X08-02111)
- Visual C++ 6.0 Processor Pack [archive.org](https://archive.org/download/en_vs6_ent/vcpp5.exe)
- Microsoft Platform SDK November 2001: [archive.org](https://archive.org/download/msdn-full/Platform%20SDK%20and%20DDKs/)
- DirectX 9.0b SDK: [archive.org](https://archive.org/details/dx90bsdk)

You will save time by using my CI files directly: [VC6SP5.zip](https://tigule.org/files/ci/VC6SP5.zip), [PSDK2001.zip](https://tigule.org/files/ci/PSDK2001.zip), [DXSDK90.zip](https://tigule.org/files/ci/DXSDK90.zip)

### Windows

- `vc6-setup.bat`: Produce NMake makefiles for Visual C++ 6.0. Visual C++ 6.0 must be installed to `C:\Program Files (x86)\Microsoft Visual Studio 6.0\VC98`, or you need to edit the vc6 script's paths.

### macOS

- `wine.sh`: Install, setup cmake, build, and run Visual C++ 6.0 makefiles via Wine. **This can take up to 15 minutes the first time.**
- `wine-crossover.sh`: Calls wine.sh using CrossOver's wine prefix.

### Linux

- `wine.sh`: Install, setup cmake, build, and run Visual C++ 6.0 makefiles via Wine. **This can take up to 15 minutes the first time.**

## Validation

### Full

The full validation process may take a couple minutes to run. It's not meant for quick iterative development, rather double checking your work.

1. Install validation dependencies once from Build/Validate/:
`pip install -r requirements.txt`

2. From Build/:
`python validate_bytecode.py --json validation.json --text validation.txt --web explorer`

3. Open Build/explorer/index.html in your web browser.

### Targeted

The targeted validation process produces a quick report on one compiland. This is what you should use when working on a file.

1. Install validation dependencies once from Build/Validate/:
`pip install -r requirements.txt`

2. From Build/, select a source path:
`python validate_bytecode.py --compiland WoW/Source/Client.cpp --text validation.txt`

## Running

[You need a server to connect to, of course.](https://github.com/The-Alpha-Project/alpha-core)

You can copy everything in your game client folder to `WoW/Client/` to keep track of things easily. The instructions here are written with this as an example path.

The original game client was meant to use a launcher so you have to pass `-uptodate` when you run it directly.  
The Alpha Project stores credentials in wow.ses, bypassing the original launcher-authentication flow. Be sure to create a text file in the same folder called `wow.ses` containing your username/password on separate lines.

## Notes

Original executables:
* `WoW.exe`: Launcher and patcher
* `WoWClient.exe`: Game client
* `WowError.exe`: Crash reporter and log viewer
* `WowUpdateHelper.exe`: Helps the patcher update itself

Toolchain:
* Visual C++ 6.0 SP5
* Microsoft Platform SDK November 2001 (5.2.3590.2)

Libraries:
* `DirectX` 9.0b
* `Expat` 1.95.5
* `FreeType` 2.0
* `Lua` 5.0
* `Zlib` 1.1.3
