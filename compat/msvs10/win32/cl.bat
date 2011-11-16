@echo off
set PATH=%PATH%;%VS100COMNTOOLS%..\..\VC\bin;%VS100COMNTOOLS%..\..\Common7\IDE

if "%ProgramFiles%" == "%ProgramFiles(x86)%" goto x64_PATH
if "%ProgramFiles%" == "%ProgramW6432%" goto x86_PATH

:x64_PATH
cl.exe /I "%VS100COMNTOOLS%..\..\VC\include" /I "%ProgramFiles(x86)%Microsoft SDKs\Windows\v7.0A\Include" %*
goto :eof

:x86_PATH
cl.exe /I "%VS100COMNTOOLS%..\..\VC\include" /I "%ProgramFiles%Microsoft SDKs\Windows\v7.0A\Include" %*
goto :eof
