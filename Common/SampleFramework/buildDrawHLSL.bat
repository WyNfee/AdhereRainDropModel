REM @echo off

REM %1 -- FXC, %2 -- OUTPUT folder, %3 -- input folder (project dir), %4 -- platform, %5 -- config
setlocal
echo RUNNING %3\..\..\common\commonshader.bat %1 %2 %3 %4 %5 %6 %7 %8
call %3\..\..\common\commonshader.bat %1 %2 %3 %4 %5 %6 %7 %8

ECHO Building Draw.hlsl

set C=%cmdline% Draw.hlsl

%C% /EFullScreenQuadVS /Tvs_5_0 /Fc"%~2Media/Shaders/FullScreenQuadVS.lst" /Fo"%~2Media/Shaders/FullScreenQuadVS.xvs"
%C% /EFullScreenQuadGS /Tgs_5_0 /Fc"%~2Media/Shaders/FullScreenQuadGS.lst" /Fo"%~2Media/Shaders/FullScreenQuadGS.xgs"
%C% /EFullScreenQuadPS /Tps_5_0 /Fc"%~2Media/Shaders/FullScreenQuadPS.lst" /Fo"%~2Media/Shaders/FullScreenQuadPS.xps"
%C% /EFullScreenColoredQuadPS /Tps_5_0 /Fc"%~2Media/Shaders/FullScreenColoredQuadPS.lst" /Fo"%~2Media/Shaders/FullScreenColoredQuadPS.xps"

%C% /ELineVS /Tvs_5_0 /Fc"%~2Media/Shaders/LineVS.lst" /Fo"%~2Media/Shaders/LineVS.xvs"
%C% /ELinePS /Tps_5_0 /Fc"%~2Media/Shaders/LinePS.lst" /Fo"%~2Media/Shaders/LinePS.xps"
