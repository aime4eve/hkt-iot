## Why

The project build is failing due to a `JdkImageTransform` execution error when using JDK 21 with the current Gradle/AGP configuration. This prevents the application from compiling and running.

## What Changes

- Update Gradle Wrapper version to ensure compatibility with JDK 21.
- Update Android Gradle Plugin (AGP) version if necessary.
- Update Kotlin version if necessary to match AGP.
- Verify and update `jvmTarget` and `sourceCompatibility` settings.

## Capabilities

### New Capabilities
- `build-configuration`: Capability to successfully build the project with JDK 21.

### Modified Capabilities
<!-- No existing functional capabilities are changing, just build infra -->

## Impact

- **Build System**: `gradle-wrapper.properties`, `build.gradle.kts`, `app/build.gradle.kts`.
- **Developer Environment**: Requires compatible JDK (user has JDK 21).
