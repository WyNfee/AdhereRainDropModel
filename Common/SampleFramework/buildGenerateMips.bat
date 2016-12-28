@echo off

REM %1 -- FXC, %2 -- OUTPUT folder, %3 -- input folder (project dir), %4 -- platform, %5 -- config
setlocal
call %3\..\..\common\commonshader.bat %1 %2 %3 %4 %5 %6 %7 %8

ECHO Building GenerateMips.hlsl

set C=%cmdline% GenerateMips.hlsl /Gec

%C% /EVSMain /Tvs_5_0 /Fc"%~2Media/Shaders/GenMipsVS.lst" /Fo"%~2Media/Shaders/GenMipsVS.bin"
%C% /EPSMain /Tps_5_0 /Fc"%~2Media/Shaders/GenMipsPS1D.lst" /Fo"%~2Media/Shaders/GenMipsPS1D.bin" /DTex1D=1
%C% /EPSMain /Tps_5_0 /Fc"%~2Media/Shaders/GenMipsPS2D.lst" /Fo"%~2Media/Shaders/GenMipsPS2D.bin" /DTex2D=1
%C% /EPSMain /Tps_5_0 /Fc"%~2Media/Shaders/GenMipsPS3D.lst" /Fo"%~2Media/Shaders/GenMipsPS3D.bin" /DTex3D=1
