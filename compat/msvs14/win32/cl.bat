@echo off
set PATH=%PATH%;%VS140COMNTOOLS%..\..\VC\bin;%VS140COMNTOOLS%..\..\Common7\IDE

cl.exe /I "%VS140COMNTOOLS%..\..\VC\include" /I "%ProgramFiles%/Microsoft SDKs\Windows\v7.1\Include" %*