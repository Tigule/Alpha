#!/bin/sh
cmake --build . --parallel "$(sysctl -n hw.ncpu)"
