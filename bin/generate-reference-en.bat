@echo off
setlocal

set "OUT=%~dp0reference-en"

if exist "%OUT%\ours-*.aiff" (
    del /q "%OUT%\ours-*.aiff"
)

python "%~dp0generate-reference-en.py"
exit /b %errorlevel%
