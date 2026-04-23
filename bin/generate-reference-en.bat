@echo off
setlocal

python "%~dp0generate-reference-en.py"
exit /b %errorlevel%
