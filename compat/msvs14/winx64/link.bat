@echo off
if "%ProgramFiles%" == "%ProgramFiles(x86)%" goto x64_PATH
if "%ProgramFiles%" == "%ProgramW6432%" goto x86_PATH

:x64_PATH
echo "64 bit"
set PATH=%PATH%;%VS140COMNTOOLS%..\..\VC\bin\amd64;%VS140COMNTOOLS%..\..\Common7\IDE
link.exe /libpath:"%VS140COMNTOOLS%..\..\VC\lib\amd64" /libpath:"%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.0A\Lib\x64" %*
goto :eof

:x86_PATH
echo "32 bit"
set PATH=%PATH%;%VS140COMNTOOLS%..\..\VC\bin\x86_amd64;%VS140COMNTOOLS%..\..\Common7\IDE
link.exe /libpath:"%VS140COMNTOOLS%..\..\VC\lib\amd64" /libpath:"%ProgramFiles%\Microsoft SDKs\Windows\v7.0A\Lib\x64" %*
goto :eof
