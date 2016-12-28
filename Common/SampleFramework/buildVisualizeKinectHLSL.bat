@echo off

REM %1 -- FXC, %2 -- OUTPUT folder, %3 -- input folder (project dir), %4 -- platform, %5 -- config
setlocal
call %3\..\..\common\commonshader.bat %1 %2 %3 %4 %5 %6 %7 %8

ECHO Building VisualizeKinect.hlsl

set C=%cmdline% VisualizeKinect.hlsl

%C% /EVisualizeKinectColorPS /Tps_5_0 /Fc"%~2Media/Shaders/VisualizeKinectColorPS.lst" /Fo"%~2Media/Shaders/VisualizeKinectColorPS.xps"
%C% /EVisualizeKinectIrPS /Tps_5_0 /Fc"%~2Media/Shaders/VisualizeKinectIrPS.lst" /Fo"%~2Media/Shaders/VisualizeKinectIrPS.xps"