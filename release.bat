@echo off
cd /d "%~dp0"

echo ============================================
echo   HKT BLETools Release
echo ============================================
echo.

REM [1/3] read version from app\build.gradle.kts
echo [1/3] Read version...
set "vn="
set "vc="
for /f "tokens=3" %%a in ('findstr /c:"versionName =" app\build.gradle.kts') do set "vn=%%a"
for /f "tokens=3" %%a in ('findstr /c:"versionCode =" app\build.gradle.kts') do set "vc=%%a"
if defined vn set "vn=%vn:~1,-1%"
echo     versionName = %vn%
echo     versionCode = %vc%
if not defined vn ( echo [ERROR] read versionName failed & goto :error )
if not defined vc ( echo [ERROR] read versionCode failed & goto :error )

REM [2/3] build
echo.
echo [2/3] Build assembleDevDebug...
powershell -ExecutionPolicy Bypass -File build.ps1 assembleDevDebug
set "SRC=app\build\outputs\apk\dev\debug\app-dev-debug.apk"
if not exist "%SRC%" ( echo [ERROR] build failed, %SRC% not found & goto :error )

REM [3/3] rename
echo.
echo [3/3] Generate APK...
del /q "HKT_BLETools_*.apk" 2>nul
set "OUT=HKT_BLETools_%vn%(%vc%).apk"
copy /y "%SRC%" "%OUT%" >nul

echo.
echo ============================================
echo   Done: %OUT%
echo ============================================
echo.
if /i not "%~1"=="nopause" pause
exit /b 0

:error
echo.
echo [FAILED]
if /i not "%~1"=="nopause" pause
exit /b 1
