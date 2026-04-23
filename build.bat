@echo off
setlocal

for %%I in ("%~dp0.") do set "ROOT=%%~fI"
set "BUILD_DIR=%ROOT%\build"
set "CONFIG=Release"

if /I not "%~1"=="" (
    set "CONFIG=%~1"
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo Missing CMake in PATH.
    exit /b 1
)

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    if errorlevel 1 exit /b 1
)

echo Configuring %CONFIG% build...
cmake -S "%ROOT%" -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1

echo Building tts.exe...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --clean-first --target tts
if errorlevel 1 exit /b 1

if exist "%BUILD_DIR%\say_lua.vcxproj" (
    echo Building say.dll...
    cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target say_lua
    if errorlevel 1 exit /b 1
) else (
    echo Lua module target not configured. Skipping say.dll.
)

echo Done.
echo Built:
echo   "%ROOT%\bin\tts.exe"
if exist "%ROOT%\bin\lua\lua54.dll" echo   "%ROOT%\bin\lua\lua54.dll"
if exist "%ROOT%\bin\lua\say.dll" echo   "%ROOT%\bin\lua\say.dll"

endlocal
