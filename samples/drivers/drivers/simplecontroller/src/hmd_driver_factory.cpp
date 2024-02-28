//============ 著作権 (c) Valve Corporation、全著作権所有。 ============
#include "device_provider.h"
#include "openvr_driver.h"
#include <cstring>

#if defined( _WIN32 )
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#elif defined( __GNUC__ ) || defined( COMPILER_GCC ) || defined( __APPLE__ )
#define HMD_DLL_EXPORT extern "C" __attribute__( ( visibility( "default" ) ) )
#define HMD_DLL_IMPORT extern "C"
#else
#error "サポートされていないプラットフォームです。"
#endif

MyDeviceProvider device_provider;

//-----------------------------------------------------------------------------
// 目的: これは共有ライブラリからエクスポートされ、vrserverによってドライバーへのエントリーポイントとして呼び出されます。
// ここで、IServerTrackedDeviceProviderへのポインターを返す必要があります。他のサンプルを参照してください。
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
	// HmdDriverFactory呼び出しでそれを求められた場合、ここでデバイスプロバイダーを返します。
	if (0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName))
	{
		return &device_provider;
	}

	// それ以外の場合は、ランタイムにこのインターフェースがないことを伝えます。
	if (pReturnCode)
		*pReturnCode = vr::VRInitError_Init_InterfaceNotFound;

	return NULL;
}
