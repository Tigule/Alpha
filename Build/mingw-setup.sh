#!/bin/sh
cmake -DCMAKE_BUILD_TYPE=Release_Assertions_Enabled -DCMAKE_TOOLCHAIN_FILE=Toolchains/mingw-i686.cmake ..
