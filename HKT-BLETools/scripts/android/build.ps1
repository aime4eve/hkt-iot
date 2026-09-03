# 构建脚本：自动使用本机 JDK 17，避免 JAVA_HOME 指向无效的 jdk-21
$env:JAVA_HOME = "D:\Java\jdk-17"
& "$PSScriptRoot\..\..\android\gradlew.bat" @args
