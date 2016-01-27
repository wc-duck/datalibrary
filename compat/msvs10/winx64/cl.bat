@echo off

if "%ProgramFiles%" == "%ProgramFiles(x86)%" set PATH=%PATH%;%VS100COMNTOOLS%..\..\VC\bin\amd64;%VS100COMNTOOLS%..\..\Common7\IDE
if "%ProgramFiles%" == "%ProgramW6432%"      set PATH=%PATH%;%VS100COMNTOOLS%..\..\VC\bin\x86_amd64;%VS100COMNTOOLS%..\..\Common7\IDE

cl.exe /I "%VS100COMNTOOLS%..\..\VC\include" /I "%ProgramFiles%\Microsoft SDKs\Windows\v7.0A\Include" %*