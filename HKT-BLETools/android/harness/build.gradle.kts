import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    id("org.jetbrains.kotlin.jvm")
    id("org.jetbrains.kotlin.plugin.serialization")
    application
}

kotlin {
    compilerOptions {
        jvmTarget = JvmTarget.JVM_17
    }
}
// Java 侧同样显式钉住 17：不配置时 compileJava 默认跟随构建 JVM（如 IDE 注入的 JBR 21），
// 与 Kotlin jvmTarget 不一致会触发 compileKotlin 的 JVM-target 校验失败
java {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
}

dependencies {
    implementation(project(":core-protocol"))
    implementation(project(":core-device"))
    implementation(project(":core-ble"))
    implementation(project(":core-ota"))
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.9.0")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.7.3")
}

application {
    mainClass.set("com.hkt.ble.bletools.harness.MainKt")
}
