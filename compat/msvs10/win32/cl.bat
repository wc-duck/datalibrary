@echo off
set PATH=%PATH%;%VS100COMNTOOLS%..\..\VC\bin;%VS100COMNTOOLS%..\..\Common7\IDE

cl.exe /I "%VS100COMNTOOLS%..\..\VC\include" /I "%ProgramFiles%/Microsoft SDKs\Windows\v7.0A\Include" %*
