## Context

The current build environment uses Gradle 8.4 and AGP 8.2.0. The user is running JDK 21.
Gradle 8.4 supports up to JDK 19 officially, and experimental support for JDK 20/21.
However, `JdkImageTransform` issues with `jlink` often indicate a mismatch between the JDK used to run Gradle and the toolchain expected by AGP.

According to Gradle compatibility matrix:
- Gradle 8.5 supports JDK 21 fully.
- AGP 8.3+ is recommended for newer JDKs and Android 34.

## Goals / Non-Goals

**Goals:**
- Fix the `JdkImageTransform` failure.
- Ensure the project builds successfully with `./gradlew assembleDebug`.
- Maintain compatibility with `compileSdk = 34`.

**Non-Goals:**
- Refactoring application code.
- Changing `minSdk` or `targetSdk` unless required by tools.

## Decisions

### 1. Upgrade Gradle Wrapper
Upgrade to Gradle 8.5 or newer (e.g., 8.6 or 8.7) to fully support JDK 21.
**Rationale**: Gradle 8.4 has incomplete support for JDK 21.

### 2. Upgrade AGP (if needed)
Upgrade AGP to 8.3.0 or newer if Gradle upgrade alone doesn't fix it.
Current: 8.2.0.
**Rationale**: Newer AGP versions handle JDK 21 toolchains better.

### 3. Disable Configuration Cache (Temporary)
If the error persists as a serialization issue, we might disable configuration cache in `gradle.properties`.
**Rationale**: The error log mentioned `Configuration cache state could not be cached`.

## Risks / Trade-offs

- **Risk**: Upgrading Gradle/AGP might require other dependency updates.
- **Mitigation**: Check for deprecated APIs and update `build.gradle` accordingly.
