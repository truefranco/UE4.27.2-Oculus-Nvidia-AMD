@echo off

set src=%~dp0
set dst=%~dp0\..\

set libName=HoloLensBuildLib.lib
set includeName=HoloLensBuildLib.h

REM Source Files
set x64LibSrc="%src%\x64\Release\%libName%"
set x86LibSrc="%src%\Release\%libName%"
set includeHeaderSrc="%src%\HoloLensBuildLib\%includeName%"

REM Destination Files
set x64LibDst="%dst%\Lib\x64\%libName%"
set x86LibDst="%dst%\Lib\x86\%libName%"
set includeHeaderDst="%dst%\Include\%includeName%"

echo Copy Windows Mixed Reality libs built from this MixedRealityInterop.sln to the Unreal WindowsMixedReality plugin.
echo.

REM Remove any read-only flags
if exist %x64LibDst% attrib -r %x64LibDst%
if exist %x86LibDst% attrib -r %x86LibDst%
if exist %includeHeaderDst% attrib -r %includeHeaderDst%

REM Copy Libs
xcopy /Y %x64LibSrc% %x64LibDst%
xcopy /Y %x86LibSrc% %x86LibDst%

if not exist %x64LibSrc% (
	echo.
	echo ERROR: Could not find %x64LibSrc%.
	echo.
)

if not exist %x86LibSrc% (
	echo.
	echo ERROR: Could not find %x86LibSrc%.
	echo.
)

REM Copy Include Header
xcopy /Y %includeHeaderSrc% %includeHeaderDst%

if not exist %includeHeaderSrc% (
	echo.
	echo ERROR: Could not find %includeHeaderSrc%.
	echo.
)

pause