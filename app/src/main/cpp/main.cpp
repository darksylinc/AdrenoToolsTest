#include <jni.h>

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

#include "sds/sds_fstream.h"

#include <sys/stat.h>

#define VK_NO_PROTOTYPES
#include <dlfcn.h>
#include <vulkan/vulkan.h>
#include "adrenotools/include/adrenotools/driver.h"

void copyFile( const std::string &srcFolder, const std::string &dstFolder, const char *filename )
{
	sds::fstream inputFile( srcFolder + filename, sds::fstream::InputEnd, false );

	if( inputFile.is_open() )
	{
		const size_t sizeBytes = inputFile.getFileSize( false );
		inputFile.seek( 0, sds::fstream::beg );
		std::vector<char> fileData;
		fileData.resize( sizeBytes );
		inputFile.read( fileData.data(), sizeBytes );

		sds::fstream outputFile( dstFolder + filename, sds::fstream::OutputDiscard );
		if( outputFile.is_open() )
		{
			outputFile.write( fileData.data(), sizeBytes );
		}
		else
		{
			__android_log_print( ANDROID_LOG_ERROR, "DriverReplacer", "Could not write to file %s!\n",
								 ( dstFolder + filename ).c_str() );
		}
	}
	else
	{
		__android_log_print( ANDROID_LOG_ERROR, "DriverReplacer", "Could not open file %s!\n",
							 ( srcFolder + filename ).c_str() );
	}
}

void replaceDriver( const char *path, const char *hooksDir, const char *driverName )
{
	std::string pp = path;

	mkdir( ( pp + "temp" ).c_str(), S_IRWXU | S_IRWXG );

	// String nativeLibDir = getApplicationLibraryDir( appInfo );
	// std::string nativeLibDir = GetJavaString( env, jNativeLibDir );

	void *libVulkan = adrenotools_open_libvulkan( RTLD_NOW | RTLD_LOCAL, ADRENOTOOLS_DRIVER_CUSTOM,
												  ( pp + "temp" ).c_str(),  //
												  hooksDir,                 //
												  path,                     //
												  driverName, nullptr, nullptr );
	if( !libVulkan )
	{
		if( !libVulkan )
		{
			__android_log_print( ANDROID_LOG_INFO, "DriverReplacer",
								 "Could not load vulkan library : %s!\n", dlerror() );
		}
	}
	else
	{
		__android_log_print( ANDROID_LOG_INFO, "DriverReplacer", "DRIVER REPLACEMENT LOADED" );
		PFN_vkCreateInstance vkCreateInstance =
			reinterpret_cast<PFN_vkCreateInstance>( dlsym( libVulkan, "vkCreateInstance" ) );
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
			(PFN_vkGetInstanceProcAddr)dlsym( libVulkan, "vkGetInstanceProcAddr" );

		vkGetInstanceProcAddr =
			reinterpret_cast<PFN_vkGetInstanceProcAddr>( dlsym( libVulkan, "vkGetInstanceProcAddr" ) );

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

		//	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices =
		// reinterpret_cast<PFN_vkEnumeratePhysicalDevices>( 			vkGetInstanceProcAddr( instance,
		//"vkEnumeratePhysicalDevices" ) );
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices =
			reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
				dlsym( libVulkan, "vkEnumeratePhysicalDevices" ) );
		PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties =
			reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
				dlsym( libVulkan, "vkGetPhysicalDeviceProperties" ) );

		VkInstance instance;
		vkCreateInstance( &instanceCreateInfo, nullptr, &instance );

		uint32_t numDevices = 1u;
		vkEnumeratePhysicalDevices( instance, &numDevices, NULL );
		if( numDevices == 0 )
		{
			__android_log_print( ANDROID_LOG_ERROR, "DriverReplacer", "NO VK DEVICES!" );
		}
		else
		{
			__android_log_print( ANDROID_LOG_INFO, "DriverReplacer", "YES VK DEVICES!" );

			std::vector<VkPhysicalDevice> pd;
			pd.resize( numDevices );
			vkEnumeratePhysicalDevices( instance, &numDevices, pd.data() );

			for( uint32_t i = 0u; i < numDevices; ++i )
			{
				VkPhysicalDeviceProperties deviceProps;
				vkGetPhysicalDeviceProperties( pd[i], &deviceProps );

				// Generic version routine that matches SaschaWillems's VulkanCapsViewer
				const uint32_t driverVersionMajor = ( deviceProps.driverVersion >> 22u ) & 0x3ff;
				const uint32_t driverVersionMinor = ( deviceProps.driverVersion >> 12u ) & 0x3ff;
				const uint32_t driverVersionRelease = ( deviceProps.driverVersion ) & 0xfff;

				__android_log_print( ANDROID_LOG_INFO, "DriverReplacer",
									 "Device %i: %s.\n"
									 "Vulkan API %i.%i.%i\n"
									 "Driver Version: %i.%i.%i (%u)",
									 i, deviceProps.deviceName,  //
									 VK_API_VERSION_MAJOR( deviceProps.apiVersion ),
									 VK_API_VERSION_MINOR( deviceProps.apiVersion ),
									 VK_API_VERSION_PATCH( deviceProps.apiVersion ),  //
									 driverVersionMajor, driverVersionMinor, driverVersionRelease,
									 deviceProps.driverVersion );
			}
		}
	}
}

extern "C" {
#include <game-activity/native_app_glue/android_native_app_glue.c>
}

// THIS FUNCTION MAY LEAK JAVA HANDLES. DO NOT USE IN PRODUCTION.
std::string getNativeLibraryDir( struct android_app *app )
{
	JNIEnv *env = nullptr;
	app->activity->vm->AttachCurrentThread( &env, nullptr );

	const jclass contextClass = env->GetObjectClass( app->activity->javaGameActivity );
	const jmethodID getApplicationContextMethod =
		env->GetMethodID( contextClass, "getApplicationContext", "()Landroid/content/Context;" );
	const jobject contextObject =
		env->CallObjectMethod( app->activity->javaGameActivity, getApplicationContextMethod );
	const jmethodID getApplicationInfoMethod =
		env->GetMethodID( contextClass, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;" );
	const jobject applicationInfoObject =
		env->CallObjectMethod( contextObject, getApplicationInfoMethod );
	const jfieldID nativeLibraryDirField = env->GetFieldID( env->GetObjectClass( applicationInfoObject ),
															"nativeLibraryDir", "Ljava/lang/String;" );
	const jobject nativeLibraryDirObject =
		env->GetObjectField( applicationInfoObject, nativeLibraryDirField );
	const jmethodID getBytesMethod = env->GetMethodID( env->GetObjectClass( nativeLibraryDirObject ),
													   "getBytes", "(Ljava/lang/String;)[B" );
	const auto bytesObject = static_cast<jbyteArray>(
		env->CallObjectMethod( nativeLibraryDirObject, getBytesMethod, env->NewStringUTF( "UTF-8" ) ) );
	const size_t length = env->GetArrayLength( bytesObject );
	const jbyte *const bytes = env->GetByteArrayElements( bytesObject, nullptr );
	const std::string libDir( reinterpret_cast<const char *>( bytes ), length );
	app->activity->vm->DetachCurrentThread();
	return libDir;
}

extern "C" {

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd( android_app *pApp, int32_t cmd )
{
	switch( cmd )
	{
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
bool motion_event_filter_func( const GameActivityMotionEvent *motionEvent )
{
	auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
	return ( sourceClass == AINPUT_SOURCE_CLASS_POINTER || sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK );
}

/*!
 * This the main entry point for a native activity
 */
void android_main( struct android_app *pApp )
{
	const char *srcFolder =
		"/storage/emulated/0/Android/data/com.example.adrenotoolstest2/files/Adreno505/";
	const char *dstFolder = "/data/user/0/com.example.adrenotoolstest2/files/";

	/*copyFile( srcFolder, dstFolder, "notadreno_utils.so" );
	copyFile( srcFolder, dstFolder, "notdmabufheap.so" );
	copyFile( srcFolder, dstFolder, "notgsl.so" );
	copyFile( srcFolder, dstFolder, "notllvm-glnext.so" );
	copyFile( srcFolder, dstFolder, "notllvm-qgl.so" );*/
	copyFile( srcFolder, dstFolder, "vulkan.msm8937_custom.so" );
	replaceDriver( dstFolder, getNativeLibraryDir( pApp ).c_str(), "vulkan.msm8937_custom.so" );

	// Register an event handler for Android events
	pApp->onAppCmd = handle_cmd;

	// Set input event filters (set it to NULL if the app wants to process all inputs).
	// Note that for key inputs, this example uses the default default_key_filter()
	// implemented in android_native_app_glue.c.
	android_app_set_motion_event_filter( pApp, motion_event_filter_func );

	// This sets up a typical game/event loop. It will run until the app is destroyed.
	int events;
	android_poll_source *pSource;
	do
	{
		// Process all pending events before running game logic.
		if( ALooper_pollAll( 0, nullptr, &events, (void **)&pSource ) >= 0 )
		{
			if( pSource )
			{
				pSource->process( pApp, pSource );
			}
		}
	} while( !pApp->destroyRequested );
}
}
