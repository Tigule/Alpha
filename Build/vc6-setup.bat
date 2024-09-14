@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 6.0\VC98\Bin\VCVARS32.BAT"
cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..
