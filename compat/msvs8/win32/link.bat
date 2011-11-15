@echo off
@set PATH=%PATH%;%VS80COMNTOOLS%..\..\..\VC\bin\;%VS80COMNTOOLS%..\..\..\Common7\IDE
@link.exe /libpath:"%VS80COMNTOOLS%..\..\..\VC\lib" /libpath:"%VS80COMNTOOLS%..\..\..\VC\PlatformSDK\lib" %*
