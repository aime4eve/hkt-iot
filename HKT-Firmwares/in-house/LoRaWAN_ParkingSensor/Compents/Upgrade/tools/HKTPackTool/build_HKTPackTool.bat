@echo off
rem Build HKTPackTool.exe with the in-box .NET Framework C# 5 compiler.
rem Output: HKTPackTool.exe next to this script (run it with no args for the GUI).
setlocal
set CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe
if not exist "%CSC%" set CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe
if not exist "%CSC%" (
    echo csc.exe not found - install .NET Framework 4.x Developer/Runtime
    exit /b 1
)
"%CSC%" /nologo /target:exe /codepage:65001 /optimize+ ^
    /out:"%~dp0HKTPackTool.exe" ^
    /r:System.Windows.Forms.dll /r:System.Drawing.dll ^
    "%~dp0HKTPackTool.cs"
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)
echo BUILD OK: %~dp0HKTPackTool.exe
endlocal
