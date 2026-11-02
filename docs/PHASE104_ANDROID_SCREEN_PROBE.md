# Phase 104: Android touchscreen native compatibility diagnostic

This is an installable **Android screen** test app for the independently authored SiN compatibility components, not a game or VR port.

## What you will see

The application is named **SiN Native Probe**. It has a touchscreen button to run JNI/native checks and a text panel with individual PASS/FAIL results. It also runs the diagnostic when opened.

The Java Activity loads three shared libraries: libsin_compat_host_abi.so, libsin_compat_event_bridge.so and libsin_android_screen_probe.so.

Its JNI self-test creates a **fabricated** player, weapon and viewmodel via the original-free host callback contract. It calls the actual bridge for session attachment, viewmodel creation, Source-style zoom input in VR mode, ammo/reload transfer, map unload and rejection of a stale entity generation. It reports **PASS** only if every check succeeds.

This does **not** render SiN gameplay, load a BSP, invoke SourceVR, establish headset tracking or launch a standalone VR game. The screen is a native Android Activity showing diagnostics, not game graphics.

## Build (Android SDK + NDK)

The project lives in android-screen-test/ and uses Java 17, Gradle 8.9, Android Gradle Plugin 8.7.3, Android platform 35 and NDK 27.2.12479018. Native build configuration is the root CMakeLists.txt with SIN_BUILD_ANDROID_SCREEN_PROBE=ON. Debug builds target **ARM64** and **x86_64**.

~~~sh
gradle -p android-screen-test :app:assembleDebug --no-daemon
# Install on an Android phone with USB debugging:
adb install -r android-screen-test/app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n org.sincompat.screenprobe/.MainActivity
~~~

The GitHub Actions android-screen-probe.yml workflow builds a **debug-signed APK** and verifies that all three native libraries are packaged for both architectures. The x86_64 emulator job installs and opens the app, checks the JNI result in logcat and uploads a screenshot.

After the workflow passes, download the APK artifact from its successful GitHub Actions run under Actions. Artifact retention is 14 days.

An ARM64 Android phone or Quest can run this as a **2D diagnostic app**, subject to installation permissions. Even if it works on Quest, that is not XR rendering or playable SiN.

## IP

The APK contains only original project code and synthetic test data. No original SiN DLLs, BSPs, maps or SourceVR binaries are bundled. SourceVR gameplay-module support and real Source engine integration milestone R1 remain unresolved.
