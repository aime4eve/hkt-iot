plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

android {
    namespace = "com.hkt.ble.bletools"
    compileSdk = 34

    // M8 版本方案（用户裁决 2026-09-21，09-22 两次澄清）：versionName = "V<主>.<次>.<修订>b<构建数>"，
    // 如 V6.1.1b26——b 后即自增构建数（无 + 号、无独立 beta 序号）。基线（VERSION_BASE）随版本发布手写；
    // 构建数每次打包自动 +1——计数持久化在
    // android/version.properties（gitignore，本机独立计数），仅当本次任务图包含
    // assemble/bundle/install 打包任务时递增；IDE sync、test 等不计。
    val VERSION_BASE = "V6.1.1b"
    val counterFile = rootProject.file("version.properties")
    var buildCount = counterFile.takeIf { it.exists() }
        ?.readText()?.trim()?.toIntOrNull() ?: 0
    val isPackageBuild = gradle.startParameter.taskNames.any {
        it.contains("assemble", true) || it.contains("bundle", true) || it.contains("install", true)
    }
    if (isPackageBuild) {
        buildCount += 1
        counterFile.writeText(buildCount.toString())
    }

    buildFeatures {
        buildConfig = true
        compose = true
    }

    lint {
        disable += "MissingTranslation"
        abortOnError = false
        checkReleaseBuilds = true
        htmlReport = true
        xmlReport = true
    }

    testOptions {
        unitTests.isReturnDefaultValues = true
    }

    defaultConfig {
        applicationId = "com.hkt.ble.bletools"
        minSdk = 26
        targetSdk = 34
        // versionName 基线+构建数见文件头注释；versionCode 保持旧式单调递增 = YYYYMMDDNN，
        // 保证对现网 3.21（versionCode 20260906）可直接覆盖升级。
        versionCode = 2026092101
        versionName = "$VERSION_BASE$buildCount"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // 启用多DEX
        multiDexEnabled = true
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
        debug {
            isDebuggable = true
            isMinifyEnabled = false
        }
    }
    
    // 构建变体
    flavorDimensions += "environment"
    productFlavors {
        create("dev") {
            dimension = "environment"
            applicationIdSuffix = ".dev"
            versionNameSuffix = "-dev"
            buildConfigField("String", "BASE_URL", "\"https://dev-api.example.com\"")
        }
        create("prod") {
            dimension = "environment"
            buildConfigField("String", "BASE_URL", "\"https://api.example.com\"")
        }
    }
    
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
        // 启用所有警告
        allWarningsAsErrors = false
    }
    
    // 打包选项
    packaging {
        resources {
            excludes += setOf(
                "/META-INF/AL2.0",
                "/META-INF/LGPL2.1",
                "META-INF/DEPENDENCIES",
                "META-INF/LICENSE",
                "META-INF/LICENSE.txt",
                "META-INF/NOTICE",
                "META-INF/NOTICE.txt"
            )
        }
    }
}

dependencies {
    implementation(project(":core-protocol"))
    implementation(project(":core-device"))
    implementation(project(":core-ota"))
    implementation(project(":core-ble"))
    implementation(project(":designsystem"))

    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.8.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.swiperefreshlayout:swiperefreshlayout:1.2.0-alpha01")

    // Compose（BOM 统一版本）
    implementation(platform("androidx.compose:compose-bom:2024.09.03"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.ui:ui-tooling-preview")
    debugImplementation("androidx.compose.ui:ui-tooling")
    implementation("androidx.activity:activity-compose:1.9.2")
    implementation("androidx.navigation:navigation-compose:2.8.2")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.8.6")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.9.0")

    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.9.0")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    implementation("com.journeyapps:zxing-android-embedded:4.3.0")
    implementation("com.google.zxing:core:3.4.1")
    implementation("no.nordicsemi.android.support.v18:scanner:1.5.0")
}
