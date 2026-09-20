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

// 协议金向量真源在仓库级 shared/fixtures，双端共享（android-v2 M1 设计 §5）
sourceSets["test"].resources.srcDir(rootProject.file("../shared/fixtures"))

dependencies {
    testImplementation(kotlin("test"))
    testImplementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.7.3")
}
