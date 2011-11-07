@echo off
if "%ProgramFiles%" == "%ProgramFiles(x86)%" goto x64_PATH
if "%ProgramFiles%" == "%ProgramW6432%" goto x86_PATH

@set PATH=%PATH%;%VS100COMNTOOLS%..\..\VC\bin\;%VS100COMNTOOLS%..\..\Common7\IDE

:x64_PATH
@link.exe /libpath:"%VS100COMNTOOLS%..\..\VC\lib" /libpath:"%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.0A\Lib" %*
goto :eof

:x86_PATH
@link.exe /libpath:"%VS100COMNTOOLS%..\..\VC\lib" /libpath:"%ProgramFiles%\Microsoft SDKs\Windows\v7.0A\Lib" %*
goto :eof
