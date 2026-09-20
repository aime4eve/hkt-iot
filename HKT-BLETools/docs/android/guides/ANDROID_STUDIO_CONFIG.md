# Android Studio 配置说明文档

> 项目: BLETools  
> 创建日期: 2026-03-15  
> 最后更新: 2026-03-15

---

## 目录

1. [环境要求](#环境要求)
2. [项目配置](#项目配置)
3. [构建配置](#构建配置)
4. [代码样式](#代码样式)
5. [运行配置](#运行配置)
6. [Lint检查](#lint检查)
7. [ProGuard混淆](#proguard混淆)
8. [构建变体](#构建变体)
9. [常见问题](#常见问题)

---

## 环境要求

| 组件 | 要求版本 | 说明 |
|------|---------|------|
| JDK | 17+ | 推荐使用 JDK 17 LTS |
| Android SDK | 34+ | compileSdk 34 |
| Gradle | 8.4 | 通过 gradle-wrapper 管理 |
| Android Studio | 2022.3+ | 推荐最新版本 |

### SDK Manager 需要安装的组件

```
Platforms:
- Android API 34 (compileSdk)
- Android API 33 (minSdk 兼容)

Build Tools:
- 34.0.0

Command Line Tools:
- latest
```

---

## 项目配置

### 1. local.properties

SDK 路径配置:

```properties
sdk.dir=D\:\\Android\\SDK
```

### 2. settings.gradle.kts

项目仓库配置:

```kotlin
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "BLETools"
include(":app")
```

### 3. gradle.properties

全局 Gradle 配置:

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `org.gradle.jvmargs` | `-Xmx4096m` | JVM 内存大小 |
| `org.gradle.caching` | `true` | 启用构建缓存 |
| `org.gradle.configuration-cache` | `true` | 启用配置缓存 |
| `org.gradle.parallel` | `true` | 启用并行构建 |
| `android.useAndroidX` | `true` | 使用 AndroidX |
| `kotlin.code.style` | `official` | Kotlin 代码风格 |

---

## 构建配置

### 根 build.gradle.kts

```kotlin
plugins {
    id("com.android.application") version "8.2.0" apply false
    id("org.jetbrains.kotlin.android") version "1.9.20" apply false
}
```

### app/build.gradle.kts 关键配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `namespace` | `com.hkt.ble.bletools` | 应用包名 |
| `compileSdk` | `34` | 编译 SDK |
| `minSdk` | `26` | 最低支持版本 |
| `targetSdk` | `34` | 目标版本 |
| `versionCode` | `1` | 版本号 |
| `versionName` | `2.1` | 版本名称 |

### Java/Kotlin 版本

```kotlin
compileOptions {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
}
kotlinOptions {
    jvmTarget = "17"
}
```

---

## 代码样式

### 配置的文件

| 文件 | 作用 |
|------|------|
| `.idea/codeStyles/Project.xml` | 项目级代码样式 |
| `.idea/codeStyles/codeStyleConfig.xml` | 代码样式配置 |
| `.idea/fileTemplateSettings.xml` | 文件头注释模板 |
| `.editorconfig` | 编辑器统一配置 |

### 代码规范要点

- **行宽限制**: 120 字符
- **缩进**: 4 空格
- **导入排序**: Android → AndroidX → com → 其他 → kotlin
- **文件头**: 自动添加版权信息

### Kotlin 模板

| 缩写 | 说明 |
|------|------|
| `fun` | 函数模板 |
| `data` | Data Class 模板 |
| `singleton` | 单例对象模板 |
| `logd` | Debug 日志模板 |
| `loge` | Error 日志模板 |

---

## 运行配置

### 构建变体

| 配置名称 | 环境 | 类型 | 用途 |
|---------|------|------|------|
| `App Dev Debug` | dev | debug | 开发调试 |
| `App Prod Debug` | prod | debug | 生产调试 |
| `App Dev Release` | dev | release | 开发发布 |

### 构建命令

```bash
# 开发版调试构建
./gradlew assembleDevDebug

# 开发版发布构建
./gradlew assembleDevRelease

# 生产版调试构建
./gradlew assembleProdDebug

# 生产版发布构建
./gradlew assembleProdRelease

# 构建所有变体
./gradlew assemble
```

### APK 输出路径

```
app/build/outputs/apk/
├── dev/
│   ├── debug/
│   └── release/
└── prod/
    ├── debug/
    └── release/
```

---

## Lint 检查

### 配置位置

`app/lint.xml`

### 严重级别 (Error)

- 安全性问题 (AuthLeak, HardwareIds)
- 权限问题 (MissingPermission)
- 资源问题 (MissingResource, DuplicateActivity)
- 性能问题 (Overdraw, ScrollViewCount)

### 警告级别 (Warning)

- 过时依赖 (ObsoleteGradleDependency)
- 硬编码文本 (HardcodedText)
- API 兼容性 (AppCompatMethod)

### 忽略级别 (Ignore)

- 实验性特性 (Experimental)
- 版本检查 (NewerVersionAvailable)

### 运行 Lint

```bash
# 运行 Lint 检查
./gradlew lint

# 生成 HTML 报告
./gradlew lintDebug

# 生成 XML 报告
./gradlew lintRelease
```

---

## ProGuard 混淆

### 配置位置

`app/proguard-rules.pro`

### 保留的类

- Kotlin 标准库
- Apache POI
- ZXing
- Nordic BLE
- AndroidX
- 应用实体类

### 混淆规则

- 保留 `Parcelable` 实现
- 保留 `Serializable` 类
- 保留枚举类型
- Release 版本移除日志

---

## 构建变体

### 环境配置

| 维度 | 值 | BASE_URL |
|------|-----|----------|
| dev | 开发环境 | `https://dev-api.example.com` |
| prod | 生产环境 | `https://api.example.com` |

### 自动生成的 BuildConfig

```kotlin
buildConfigField("String", "BASE_URL", "\"https://dev-api.example.com\"")
```

---

## 常见问题

### Q1: 构建失败提示 "SDK location not found"

**解决方案**: 检查 `local.properties` 中的 SDK 路径是否正确

### Q2: Gradle 同步失败

**解决方案**: 
1. 删除 `.gradle` 目录
2. 删除 `app/build` 目录
3. 重新运行 `./gradlew clean`

### Q3: Java 版本不匹配

**解决方案**: 
```bash
# 设置 JAVA_HOME
export JAVA_HOME=/path/to/jdk-17
```

### Q4: 依赖下载慢

**解决方案**: 在 `gradle.properties` 中添加国内镜像

```properties
org.gradle.daemon=false
```

### Q5: Lint 检查过于严格

**解决方案**: 已在 `lint.xml` 中配置合适的规则，如需调整可修改对应规则的 severity

---

## 附录

### Gradle Wrapper 升级

```bash
# 升级 Gradle 版本
./gradlew wrapper --gradle-version=8.4

# 验证版本
./gradlew --version
```

### 常用命令

| 命令 | 说明 |
|------|------|
| `./gradlew clean` | 清理构建 |
| `./gradlew build` | 完整构建 |
| `./gradlew assembleDebug` | 构建调试 APK |
| `./gradlew assembleRelease` | 构建发布 APK |
| `./gradlew dependencies` | 查看依赖树 |
| `./gradlew lint` | 代码检查 |
| `./gradlew test` | 运行单元测试 |

### 目录结构

```
BLETools/
├── app/
│   ├── build.gradle.kts    # 应用模块构建配置
│   ├── lint.xml            # Lint 配置
│   └── proguard-rules.pro  # 混淆规则
├── build.gradle.kts        # 根项目构建配置
├── gradle.properties        # Gradle 全局配置
├── settings.gradle.kts     # 项目设置
├── local.properties        # 本地 SDK 配置
├── gradle/wrapper/         # Gradle Wrapper
└── .idea/                  # IDE 配置
    ├── codeStyles/         # 代码样式
    ├── templates/          # 文件模板
    └── runConfigurations/ # 运行配置
```

---

**文档版本**: V1.0  
**维护者**: Sisyphus AI Agent
