@echo off

REM %1 -- FXC, %2 -- OUTPUT folder, %3 -- input folder (project dir), %4 -- platform, %5 -- config
setlocal
call %3\..\..\common\commonshader.bat %1 %2 %3 %4 %5 %6 %7 %8

ECHO Building Mesh.hlsl

set C=%cmdline% Mesh.hlsl


%C% /EVSSceneMain /Tvs_5_0 /Fc"%~2Media/Shaders/MeshSimpleVS.lst" /Fo"%~2Media/Shaders/MeshSimpleVS.bin"
%C% /EPSSceneMain /Tps_5_0 /Fc"%~2Media/Shaders/MeshSimplePS.lst" /Fo"%~2Media/Shaders/MeshSimplePS.bin"

