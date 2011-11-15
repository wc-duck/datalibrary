@echo off
if "%ProgramFiles%" == "%ProgramFiles(x86)%" goto x64_PATH
if "%ProgramFiles%" == "%ProgramW6432%" goto x86_PATH

:x64_PATH
set PATH=%PATH%;%VS80COMNTOOLS%..\..\..\VC\bin\amd64;%VS80COMNTOOLS%..\..\..\Common7\IDE
link.exe /libpath:"%VS80COMNTOOLS%..\..\..\VC\lib\amd64" /libpath:"%VS80COMNTOOLS%..\..\..\VC\PlatformSDK\lib\amd64" %*
goto :eof

:x86_PATH
set PATH=%PATH%;%VS80COMNTOOLS%..\..\..\VC\bin\x86_amd64;%VS80COMNTOOLS%..\..\..\Common7\IDE
link.exe /libpath:"%VS80COMNTOOLS%..\..\..\VC\lib\amd64" /libpath:"%VS80COMNTOOLS%..\..\..\VC\PlatformSDK\lib\amd64" %*
goto :eof

