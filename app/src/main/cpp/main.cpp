#include <jni.h>

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "adrenotools/include/adrenotools/driver.h"
#include <dlfcn.h>

extern "C" {

void replaceDriver(const char *path) {
    std::string pp = path;
    void *libVulkan = adrenotools_open_libvulkan(
            RTLD_NOW | RTLD_LOCAL, ADRENOTOOLS_DRIVER_CUSTOM,
            path,  //
            path,                      //
            path,                      //
            "libvulkan_freedreno.so", nullptr, nullptr);
    if (!libVulkan) {
        if (!libVulkan) {
            __android_log_print(ANDROID_LOG_INFO, "DriverReplacer",
                                "Could not load vulkan library : %s!\n", dlerror());
        }
    } else {
        __android_log_print(ANDROID_LOG_INFO, "DriverReplacer", "DRIVER REPLACEMENT LOADED");
        PFN_vkCreateInstance vkCreateInstance =
                reinterpret_cast<PFN_vkCreateInstance>( dlsym(libVulkan, "vkCreateInstance"));
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) dlsym(
                libVulkan,
                "vkGetInstanceProcAddr");

        vkGetInstanceProcAddr =
                reinterpret_cast<PFN_vkGetInstanceProcAddr>( dlsym(libVulkan,
                                                                   "vkGetInstanceProcAddr"));

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "AdrenoToolsExample";
        appInfo.applicationVersion = 1;
        appInfo.pEngineName = "AdrenoToolsExample";
        appInfo.engineVersion = 1;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Create Vulkan instance
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;


        //	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        //			vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDevices" ) );
        PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
                dlsym(libVulkan, "vkEnumeratePhysicalDevices"));

        VkInstance instance;
        vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

        uint32_t numDevices = 1u;
        vkEnumeratePhysicalDevices(instance, &numDevices, NULL);
        if (numDevices == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "DriverReplacer", "NO VK DEVICES!");
        } else {
            __android_log_print(ANDROID_LOG_INFO, "DriverReplacer", "YES VK DEVICES!");
        }
    }
}

#include <game-activity/native_app_glue/android_native_app_glue.c>

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being destroyed. Use this to clean up your userData to avoid leaking
            // resources.
            //
            // We have to check if userData is assigned just in case this comes in really quickly
            break;
        default:
            break;
    }
}

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    replaceDriver("/storage/emulated/0/Download/Turnip/Turnip-v22.3.1-R2/");

    // Register an event handler for Android events
    pApp->onAppCmd = handle_cmd;

    // Set input event filters (set it to NULL if the app wants to process all inputs).
    // Note that for key inputs, this example uses the default default_key_filter()
    // implemented in android_native_app_glue.c.
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    int events;
    android_poll_source *pSource;
    do {
        // Process all pending events before running game logic.
        if (ALooper_pollAll(0, nullptr, &events, (void **) &pSource) >= 0) {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }
    } while (!pApp->destroyRequested);
}
}