@echo off
setlocal
pushd "%~dp0"

where UnrealVersionSelector.exe >nul 2>nul
if %errorlevel%==0 (
    UnrealVersionSelector.exe /projectfiles "%CD%\PixelMazeEscape.uproject"
    goto :done
)

set "UVS=%ProgramFiles(x86)%\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe"
if exist "%UVS%" (
    "%UVS%" /projectfiles "%CD%\PixelMazeEscape.uproject"
    goto :done
)

echo No se ha encontrado UnrealVersionSelector.exe.
echo Haz clic derecho sobre PixelMazeEscape.uproject y elige Generate Visual Studio project files.

:done
popd
pause
