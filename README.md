<div align="center">
<h1>Tigule Alpha</h1>

This project has 3 goals: to **understand and document**, **recompile**, and **preserve** an alpha version of World of Warcraft - version 0.5.3 (build 3368) released on 2003-12-11.  
The significance of this build is because it includes pdb/map files for its Windows executables **and it's the earliest patch we have currently**.
</div>

To accomplish these goals we're rewriting every function in the original `WoWClient.exe`, and loading these replacements at runtime using Detours. With this approach the game client will stay playable while we work towards complete coverage.

Cross-platform support is a secondary goal after producing a functional Windows client. We can source functions from **0.5.5 (build 3494)** (OS X) and **0.7.0 (build 3694)** (Linux). The result would be an approximation of a cross-platform 0.5.3 client however we can't confirm what's authentic for the time.

When this project is complete we should have ~400k lines of code and 22k+ functions in this repo. These figures are based on information from the original client.

> [!IMPORTANT]
> I can't provide a link to the original install disc, try checking Google/archive.org for `"World of Warcraft Alpha 0.5.3 3368"`

## Contribution Guide

This isn't a reimplementation project taking advantage of modern standards. We're supporting the original toolchain and following every bit of original information we can use.

1. C/C++ standards must be restricted to the features available in Visual C++ 6.0 (1998)
2. Use [Windows datatypes](https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types). All platforms used the same datatypes at the time.

All of the original source file paths have been created as empty files ahead of time, and registered in CMake. Use them as you write functions. You should not need to add any new .cpp files, but feel free to create headers.

## Building

Firstly, you'll need to install CMake no matter what OS you're on: [cmake.org](https://cmake.org/download/)

### Windows

> [!TIP]
> This has been tested with [Visual Studio 2022](https://visualstudio.microsoft.com/vs/).  

Older Visual Studio versions are untested and depending how far back you go, may have incompatibilities with newer CMake. The codebase should work anyways. Good luck if you go that route!

Inside the Build folder there are a couple scripts:
- `vs2022-setup.bat`: Have CMake produce a Visual Studio 2022 solution.
- `vc6-setup.bat`: Have CMake produce NMake makefiles for Visual C++ 6.0. Visual C++ 6.0 must be installed to `C:\Program Files (x86)\Microsoft Visual Studio 6.0\`

### macOS

> [!TIP]
> This has been tested with [Xcode](https://developer.apple.com/xcode/) and [Visual Studio Code](https://developer.apple.com/xcode/).

Inside the Build folder there are several scripts:
- `xcode-setup.sh`: Have CMake produce an Xcode project targeting native macOS.
- `native-setup.sh`: Have CMake produce Makefiles targeting your default toolchain.
- `mingw-setup.sh`: Have CMake produce Makefiles targeting MinGW.
- `vc6-wine-setup.bat`: Have CMake produce NMake makefiles for Visual C++ 6.0. Intended to run using `wine <bat file>`. Visual C++ 6.0 must be installed to your `drive_c` folder as `C:\VS6`. **There is some extra delay when setting up/building in wine.**

### Linux

> [!TIP]
> This has been tested with [Visual Studio Code](https://developer.apple.com/xcode/).

Inside the Build folder there are a few scripts:
- `native-setup.sh`: Have CMake produce Makefiles targeting your default toolchain.
- `mingw-setup.sh`: Have CMake produce Makefiles targeting MinGW.
- `vc6-wine-setup.bat`: Have CMake produce NMake makefiles for Visual C++ 6.0. Intended to run using `wine <bat file>`. Visual C++ 6.0 must be installed to your `drive_c` folder as `C:\VS6`. **There is some extra delay when setting up/building in wine.**

## Running

You can copy everything in your game client folder to `WoW/Client/` to keep track of things easily. The instructions here are written with this as an example path.

The original game client was meant to use a launcher so you have to pass `-uptodate` if you launch it directly, and be sure to create a file in the same folder called `wow.ses` containing your username/password on separate lines.

### Hook

The hook can run in two ways, either by proxying fmod.dll calls so it gets loaded on startup *or* through injection (use whatever injector you want). The first method is more reliable and recommended.

To set up the dll proxy
1. Rename the original `WoW/Client/fmod.dll` to `WoW/Client/fmod2.dll`
2. Copy `WoW/WowHook.dll` to `WoW/Client/fmod.dll`
3. Start `WoW/Client/WoWClient.exe -uptodate`

### Standalone

This will be in an incomplete state until the hook covers a majority of critical functions.

1. Set your working directory to the `WoW/Client/` folder.
2. Run the `Wow` project.

## Notes

Original executables:
* `WoW.exe`: This was produced from UpdateClient.cpp, it was called WoW due to the update process during the alpha/early beta. The map file is UpdateClient.map
* `WoWClient.exe`: This was produced from Client.cpp, it was called WoWClient due to the update process during the alpha/early beta. The pdb file is Wowae.pdb (Wow + Assertions Enabled build)
* `WowError.exe`: todo explanation
* `WowUpdateHelper.exe`: todo explanation

Libraries:
* `Expat` 1.95.5
* `FreeType` 2.0
* `Lua` 5.0
* `Zlib` 1.1.4
