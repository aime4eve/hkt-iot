@echo off
chcp 65001 >nul
title BLETools 模拟器启动器
set "JAVA_HOME=D:\Java\jdk-17"
set "ANDROID_SDK_ROOT=D:\Android\sdk"
set "ADB=%ANDROID_SDK_ROOT%\platform-tools\adb.exe"

echo ============================================
echo   BLETools 模拟器一键启动
echo ============================================
echo.
echo [1/4] 启动模拟器 ble_test ...
start "" "%ANDROID_SDK_ROOT%\emulator\emulator.exe" -avd ble_test -no-snapshot -gpu auto

echo [2/4] 等待开机 ...
"%ADB%" wait-for-device
:waitboot
"%ADB%" shell getprop sys.boot_completed >"%TEMP%\bc.txt" 2>nul
findstr "1" "%TEMP%\bc.txt" >nul && goto booted
timeout /t 2 >nul
goto waitboot
:booted
echo       开机完成

set "APK=%~dp0app\build\outputs\apk\prod\debug\app-prod-debug.apk"
if exist "%APK%" (
  echo [3/4] 安装最新 APK ...
  copy /Y "%APK%" "%TEMP%\bletools.apk" >nul
  "%ADB%" install -r -g "%TEMP%\bletools.apk" >nul
  echo       安装完成
) else (
  echo [3/4] 未找到 APK，跳过安装 ^(路径: %APK%^)
  echo       请先编译: ./build.ps1 assembleProdDebug
)

echo [4/4] 启动 BLETools ...
"%ADB%" shell monkey -p com.hkt.ble.bletools -c android.intent.category.LAUNCHER 1 >nul

echo.
echo ============================================
echo   完成! 可在模拟器窗口中使用 BLETools
echo ============================================
pause
