@echo off
call "C:\VS6\VC98\Bin\VCVARS32.BAT"
cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..
