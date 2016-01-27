@echo off
if "%ProgramFiles%" == "%ProgramFiles(x86)%" set PATH=%PATH%;%VS140COMNTOOLS%..\..\VC\bin\amd64;%VS140COMNTOOLS%..\..\Common7\IDE
if "%ProgramFiles%" == "%ProgramW6432%"      set PATH=%PATH%;%VS140COMNTOOLS%..\..\VC\bin\x86_amd64;%VS140COMNTOOLS%..\..\Common7\IDE

cl.exe /I "%VS140COMNTOOLS%..\..\VC\include" /I "%ProgramFiles%\Microsoft SDKs\Windows\v7.1\Include" %*