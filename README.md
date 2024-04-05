# Test example to check AdrenoTools

The important bits are in [main.cpp](app/src/main/cpp/main.cpp):

```cpp
// #define USE_QUALCOMM_DRIVER
const char *srcFolder = pApp->activity->externalDataPath;
const char *dstFolder = pApp->activity->internalDataPath;
#ifndef USE_QUALCOMM_DRIVER
	const char *vulkanLibName = "libvulkan_freedreno.so";
#else
	const char *vulkanLibName = "vulkan.ad0667.so";
#endif
```

Uncomment `#define USE_QUALCOMM_DRIVER` to use the (un)official drivers from Qualcomm.
Leave it commented to use Turnip instead.

You may have to modify the hardcoded filenames if you have to.

**The code expects you to manually unzip the driver into `/storage/emulated/0/Android/data/com.example.adrenotoolstest2/files/`.**

**You will first have to install the app (i.e. run it once), it will fail. And now that `/storage/emulated/0/Android/data/com.example.adrenotoolstest2/files/` has been created, you can put the driver files there.**

In most Android distros, you can access it via USB storage from the computer and sometimes via the Files app.
In some rare cases, your Android distro may block access to that path so you'll have to use another folder and modify the source code a little bit.

# Build instructions

Open `build.gradle.kts` in Android Studio and build it.
I'm using Android Studio 2023.1.1 Patch 2.

# I don't see anything! It's a black screen!

This project does NOT render anything.

It simply dumps success / error and other diagnostic information (such as driver name and version as reported by Vulkan) to logcat and then runs indefinitely doing nothing.

This project aims to be as simple as possible to test libadrenotools.