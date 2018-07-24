@echo fix msvs8 compile!

@echo off
if "%ProgramFiles%" == "%ProgramFiles(x86)%" set PATH=%PATH%;%VS80COMNTOOLS%..\..\..\VC\bin\amd64;%VS80COMNTOOLS%..\..\..\Common7\IDE
if "%ProgramFiles%" == "%ProgramW6432%"      set PATH=%PATH%;%VS80COMNTOOLS%..\..\..\VC\bin\x86_amd64;%VS80COMNTOOLS%..\..\..\Common7\IDE

cl.exe /I "%VS80COMNTOOLS%..\..\..\VC\include" %*