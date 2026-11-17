<div align="center">
<h1>Tigule Alpha</h1>

<a href="https://tigule.github.io/Alpha"><img src="https://tigule.github.io/Alpha/badges/implemented.svg"></a> <a href="https://tigule.github.io/Alpha"><img src="https://tigule.github.io/Alpha/badges/code-functions.svg"></a>
</div>

**Disclaimer**:
*You cannot play retail World of Warcraft with this*. The code only communicates with an early protocol and assets. This is simply a time capsule for those after us.

This project tries to preserve a client from World of Warcraft (version 0.5.3, build 3368), dated December 2003.

It has 3 goals: to understand and document, reconstruct, and preserve legacy code.

## Contribution Guide

1. C/C++ standards must be restricted to the features available in Visual C++ 6.0 (1998)
2. Use [Windows datatypes](https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types). Non-windows targets still use the same datatypes through `otherwin32.h`
3. Validate your output to confirm it's an improvement or has no regressions.

## Building

[CMake](https://cmake.org/download/) is the build system regardless of your target. Be sure to install it.

### Visual C++ 6.0

This is the original toolchain and target. It's the slowest but most correct option, and the only one that builds the original x86 bytecode (`BUILD_PORTABLE=OFF`).

Install these dependencies yourself, or skip below to grab everything pre-packaged for ease of use.
- Visual Studio 6.0 Professional Edition: [archive.org](https://archive.org/details/vsp600enu)
- Service Pack 5 for Microsoft Visual Studio 6.0: [archive.org](https://archive.org/details/X08-02111)
- Visual C++ 6.0 Processor Pack [archive.org](https://archive.org/download/en_vs6_ent/vcpp5.exe)
- Microsoft Platform SDK November 2001: [archive.org](https://archive.org/download/msdn-full/Platform%20SDK%20and%20DDKs/)
- DirectX 9.0b SDK: [archive.org](https://archive.org/details/dx90bsdk)

I've compiled them together here:
[VC6SP5.zip](https://tigule.org/files/ci/VC6SP5.zip), [PSDK2001.zip](https://tigule.org/files/ci/PSDK2001.zip), [DXSDK90.zip](https://tigule.org/files/ci/DXSDK90.zip).

Run `vc6-setup.bat` once to configure the project and then run `vc6-build.bat`. The executable will be `Build/WoW/bin/Wowae.exe` after.

```bat
cd Build
.\vc6-setup.bat
.\vc6-build.bat
```

Pass `-DBUILD_FIXES=OFF` to CMake to leave out Tigule's fixes when you want output that matches the original client.

#### Wine or CrossOver

On macOS or Linux, `wine.sh` downloads the packages above into a Wine prefix and drives the same VC6 build. Use `wine-crossover.sh` instead to build in a CrossOver bottle (`Tigule` by default, or set `CROSSOVER_BOTTLE`).

```sh
cd Build
./wine.sh all
```

`./wine.sh` with no command lists the steps (`install`, `setup`, `build`, `run`, `run-ref`).

### Visual Studio 2026

Install [Visual Studio Community](https://visualstudio.microsoft.com/downloads/) and the "Desktop development with C++" workload.

For MIDI ambience on 32-bit (not supported on 64-bit), set `DXSDK_DIR` to a DirectX SDK, such as [DXSDK2010.zip](https://tigule.org/files/ci/DXSDK2010.zip). This is optional.

```bat
cmake --preset windows-x86
cmake --build --preset windows-x86-release
```

`cmake --preset windows-x86` generates `Build/windows-x86/BuildWoW.sln`, which you can open in Visual Studio instead of building from the command line. It has the Debug, Release and Release_Assertions_Enabled configurations. The executable will be `Build/windows-x86/WoW/bin/Release/Wow.exe`.

For a 64-bit build, use the `windows-x64` presets instead. The executable will be `Build/windows-x64/WoW/bin/Release/Wow64.exe`.

### macOS

Install Xcode, or only its Command Line Tools for the Makefiles preset below. The port targets Apple Silicon (arm64).

```sh
cmake --preset macos
cmake --build --preset macos-release
```

The Xcode project in `Build/macos/` has the Debug, Release and Release_Assertions_Enabled configurations. The bundle will be `Build/macos/WoW/bin/Release/World of Warcraft.app`.

With only the Command Line Tools, use the `macos-makefile` preset instead. The bundle will be `Build/macos-makefile/WoW/bin/World of Warcraft.app`.

```sh
cmake --preset macos-makefile
cmake --build --preset macos-makefile
```

### Linux

The port targets x86_64 and uses SDL2 for the window and input, and OpenAL for sound. On Ubuntu 22.04:

```sh
sudo apt install clang cmake pkg-config libsdl2-dev libopenal-dev libgl-dev libglu1-mesa-dev libx11-dev libstdc++-12-dev
```

```sh
cmake --preset linux
cmake --build --preset linux
```

The executable will be `Build/linux/WoW/bin/Wow64ae`. Pass `-DCMAKE_BUILD_TYPE=Release` or `Debug` to `cmake --preset linux` for another configuration.

## Running

[You need a server to connect to, of course.](https://github.com/The-Alpha-Project/alpha-core)

Copy your game install to `WoW/Client/`.

The original game client was meant to use a launcher for authentication, most people bypass that, so you'll have to use `-uptodate` when you run it.  

The client runs from the folder its executable is in, so copy the executable there first:
* Windows: copy the `.exe` into `WoW/Client/`. The 64-bit build also needs `fmod64.dll`, if needed copy it from `Engine/Source/FMOD/api/fmod64.dll` into `WoW/Client/`.
* macOS: copy `World of Warcraft.app` into `WoW/Client/`
* Linux: copy `Wow64ae` into `WoW/Client/` (a symlink won't work), then run `./Wow64ae -uptodate`. Settings and crash logs go to `~/.blizzard/`.

alpha-core can accept a "legacy wow.ses" auth, a custom interface MPQ with auth, or a proper launcher.  
The quickest but least secure is legacy wow.ses, create a text file in the client folder called `wow.ses` with your username/password on separate lines.

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
* `Zlib` 1.1.4 library (C:), 1.1.3 headers (D:)
