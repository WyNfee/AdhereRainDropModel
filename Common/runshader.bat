REM %1% -- Command line
REM %2% -- OUTPUT folder
REM %3% -- Shader model
REM %4% -- Entry point
REM %5% -- Shader file name (including extension)
REM %6% -- HLSL source

%~1 /T%3 /E%4 /Qstrip_debug /Fc"%~2Media\Shaders\%5.lst" /Fo"%~2Media\Shaders\%5" /Fd"%~2Media\Shaders\%5.pdb" %6
