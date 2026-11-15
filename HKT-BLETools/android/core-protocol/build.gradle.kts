import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    id("org.jetbrains.kotlin.jvm")
    id("org.jetbrains.kotlin.plugin.serialization")
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

// 协议金向量真源在仓库级 shared/fixtures，双端共享（android-v2 M1 设计 §5）
sourceSets["test"].resources.srcDir(rootProject.file("../shared/fixtures"))

dependencies {
    testImplementation(kotlin("test"))
    testImplementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.7.3")
}
