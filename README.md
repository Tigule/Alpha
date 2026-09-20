<div align="center">
<h1>Tigule Alpha</h1>

<a href="https://tigule.github.io/Alpha"><img src="https://tigule.github.io/Alpha/badges/implemented.svg"></a> <a href="https://tigule.github.io/Alpha"><img src="https://tigule.github.io/Alpha/badges/code-functions.svg"></a>

This project has 3 goals: to **understand and document**, **recompile**, and **preserve** an alpha version of World of Warcraft - version 0.5.3 (build 3368) released on 2003-12-11.
</div>

## Contribution Guide

This isn't a reimplementation project taking advantage of modern standards. We're supporting the original toolchain and following every bit of original information we can use. This version came with pdb/map files.

1. C/C++ standards must be restricted to the features available in Visual C++ 6.0 (1998)
2. Use [Windows datatypes](https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types). All platforms used the same datatypes at the time.
3. All of the original source file paths have been created. You do not need to add any new files.

As far as how to get it "right" - I use IDA Pro to disassemble code and separately I'll parse the PDB for extra information. I don't have any tools I can provide you.

Supporting a variety of OS and architecture targets is reserved for after completing the 32-bit VC6 Windows target.  
We will source functions from **0.5.5 (build 3494)** (OS X) and **0.7.0 (build 3694)** (Linux) to derive an authentic multiplatform implementation.

## Building

Install these dependencies:
- CMake: [cmake.org](https://cmake.org/download/).
- Visual Studio 6.0 Professional Edition: [archive.org](https://archive.org/details/vsp600enu)
- Service Pack 5 for Microsoft Visual Studio 6.0: [archive.org](https://archive.org/details/X08-02111)
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
* `WoW.exe`: This was produced from UpdateClient.cpp, it was called WoW due to the update process during the alpha/early beta. The map file is UpdateClient.map
* `WoWClient.exe`: This was produced from Client.cpp, it was called WoWClient due to the update process during the alpha/early beta. The pdb file is Wowae.pdb (Wow + Assertions Enabled build)
* `WowError.exe`: todo explanation
* `WowUpdateHelper.exe`: todo explanation

Toolchain:
* Visual C++ 6.0 SP5
* Microsoft Platform SDK November 2001 (5.2.3590.2)

Libraries:
* `DirectX` 9.0b
* `Expat` 1.95.5
* `FreeType` 2.0
* `Lua` 5.0
* `Zlib` 1.1.4
