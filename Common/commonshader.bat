REM %1% -- FXC
REM %2% -- OUTPUT folder
REM %3% -- input folder (project dir)
REM %4% -- platform
REM %5% -- config

if not exist %2 mkdir %2
if not exist "%~2Media" mkdir "%~2Media"
if not exist "%~2Media\Shaders" mkdir "%~2Media\Shaders"

SET cmdline=%~1 /Zi /Zpr

if "a%~5%" == "aDebug" (
   echo Compiling shaders with optimization disabled...
   SET cmdline=%cmdline% /O0 /Od
)

if "a%~5%" == "aDebug12" (
   echo Compiling shaders with optimization disabled...
   SET cmdline=%cmdline% /O0 /Od /DDX12
)

if "a%~5%" == "aRelease" (
   echo Compiling shaders with optimization enabled...
   SET cmdline=%cmdline% /O3
)

if "a%~5%" == "aRelease12" (
   echo Compiling shaders with optimization enabled...
   SET cmdline=%cmdline% /O3 /DDX12
)

if "a%~5%" == "aProfile" (
   echo Compiling shaders with optimization enabled...
   SET cmdline=%cmdline% /O3
)

if "a%~5%" == "aProfile12" (
   echo Compiling shaders with optimization enabled...
   SET cmdline=%cmdline% /O3 /DDX12
)

if "a%~4%" == "aDurango" (
   echo Compiling shaders for XboxOne...
   SET cmdline=%cmdline% /D_XBOX_ONE=1 /D_DURANGO=1 /D__XBOX_FULL_PRECOMPILE_PROMISE
) else (
   echo Compiling shaders for x64...
)


