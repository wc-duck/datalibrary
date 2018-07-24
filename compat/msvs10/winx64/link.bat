@echo off
if "%ProgramFiles%" == "%ProgramFiles(x86)%" goto x64_PATH
if "%ProgramFiles%" == "%ProgramW6432%" goto x86_PATH

:x64_PATH
set PATH=%VS100COMNTOOLS%..\..\VC\bin\amd64;%VS100COMNTOOLS%..\..\Common7\IDE;%PATH%
link.exe /libpath:"%VS100COMNTOOLS%..\..\VC\lib\amd64" /libpath:"%WindowsSdkDir%\Lib\x64" %*
goto :eof

:x86_PATH
set PATH=%VS100COMNTOOLS%..\..\VC\bin\x86_amd64;%VS100COMNTOOLS%..\..\Common7\IDE;%PATH%
link.exe /libpath:"%VS100COMNTOOLS%..\..\VC\lib\amd64" /libpath:"%WindowsSdkDir%\Lib\x64" %*
goto :eof
