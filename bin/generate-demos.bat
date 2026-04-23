@echo off
setlocal

set "ROOT=%~dp0"
set "TTS=%ROOT%tts.exe"

if not exist "%TTS%" (
    echo Missing executable: "%TTS%"
    exit /b 1
)

echo Generating English demo...
"%TTS%" "Hello from lib-say. This is an English demo sentence." -o "%ROOT%demo-en.aiff" --lang en
if errorlevel 1 exit /b 1

echo Generating French demo...
"%TTS%" "Bonjour Robin. Vive le Club Video Buggai. C'est trop sympa. Ceci est une phrase de demonstration en franssais." -o "%ROOT%demo-fr.aiff" --lang fr
if errorlevel 1 exit /b 1

echo Done.
echo Created:
echo   "%ROOT%demo-en.aiff"
echo   "%ROOT%demo-fr.aiff"

endlocal
