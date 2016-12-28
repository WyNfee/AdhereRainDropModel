@echo off

REM %1 -- FXC, %2 -- OUTPUT folder, %3 -- input folder (project dir), %4 -- platform, %5 -- config
setlocal
call %3\..\..\common\commonshader.bat %1 %2 %3 %4 %5 %6 %7 %8

ECHO Building ATGFont.hlsl

set C=%cmdline% ATGFont.hlsl /Gec

%C% /EFontVertexShader /Tvs_5_0 /Fc"%~2Media/Shaders/ATGFontVS.lst" /Fo"%~2Media/Shaders/ATGFontVS.bin"
%C% /EFontPixelShader /Tps_5_0 /Fc"%~2Media/Shaders/ATGFontPS.lst" /Fo"%~2Media/Shaders/ATGFontPS.bin"
