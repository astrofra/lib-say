@echo off
setlocal

set "ROOT=%~dp0"
set "TTS=%ROOT%tts.exe"

if not exist "%TTS%" (
    echo Missing executable: "%TTS%"
    exit /b 1
)

echo Generating English debug report...
"%TTS%" "Hello from lib-say. This is an English demo sentence." --lang en --debug-report "%ROOT%demo-en-debug.txt" --dry-run
if errorlevel 1 exit /b 1

echo Generating French debug report...
"%TTS%" "Bonjour depuis lib-say. Ceci est une phrase de demonstration en francais." --lang fr --debug-report "%ROOT%demo-fr-debug.txt" --dry-run
if errorlevel 1 exit /b 1

echo Done.
echo Created:
echo   "%ROOT%demo-en-debug.txt"
echo   "%ROOT%demo-fr-debug.txt"

endlocal
