## 1. Gradle Wrapper Upgrade

- [x] 1.1 Update `gradle-wrapper.properties` to use Gradle 8.5 (or newer compatible version).
- [x] 1.2 Verify Gradle wrapper checksum (optional but good practice).

## 2. Configuration Cache Fix

- [x] 2.1 Disable configuration cache in `gradle.properties` if not already present.
- [x] 2.2 Verify if the serialization error persists.

## 3. AGP & Build Config

- [x] 3.1 Update AGP version in `build.gradle.kts` if needed (currently 8.2.0, might need 8.3+).
- [x] 3.2 Update Kotlin version in `build.gradle.kts` if needed (currently 1.9.20).
- [x] 3.3 Verify `jvmTarget` is set correctly to 17 or higher in `app/build.gradle.kts`.

## 4. Verification

- [x] 4.1 Run `./gradlew clean assembleDebug` successfully.
