@echo off
if "%ProgramFiles%" == "%ProgramFiles(x86)%" goto x64_PATH
if "%ProgramFiles%" == "%ProgramW6432%" goto x86_PATH

:x64_PATH
set PATH=%PATH%;%VS140COMNTOOLS%..\..\VC\bin\amd64;%VS140COMNTOOLS%..\..\Common7\IDE
cl.exe /I "%VS140COMNTOOLS%..\..\VC\include" /I "%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.0A\Include" %*
goto :eof

:x86_PATH
set PATH=%PATH%;%VS140COMNTOOLS%..\..\VC\bin\x86_amd64;%VS140COMNTOOLS%..\..\Common7\IDE
cl.exe /I "%VS140COMNTOOLS%..\..\VC\include" /I "%ProgramFiles%\Microsoft SDKs\Windows\v7.0A\Include" %*
goto :eof
