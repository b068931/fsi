@echo off

rem Launch worker, forwarding all args
fsi-translator.exe %*

rem Capture exit code
set ec=%ERRORLEVEL%

rem Pause so user sees output
pause

rem Exit with child exit code
exit /b %ec%
