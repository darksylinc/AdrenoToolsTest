# Test to check AdrenoTools

**WARNING:** This test is NOT working.

The code calls `replaceDriver("/storage/emulated/0/Download/Turnip/Turnip-v22.3.1-R2/");` with a hardcoded path and assumes the driver is called `libvulkan_freedreno.so` (which you can change in `replaceDriver`). Change them if you have to.

Right now it doesn't work because no matter what I do; I always get:

> NO VK DEVICES!

I know that it is supposed to work fine on my phone because PPSSPP manages to load the driver fine, reports a different driver string and even displays graphical glitches when using this other driver.

# Build instructions

Open `build.gradle.kts` in Android Studio and build it.
I'm using Android Studio 2023.1.1 Patch 2.