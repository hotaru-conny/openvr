//============ 著作権 (c) Valve Corporation、全著作権所有。 ============
#include "device_provider.h"

#include "driverlog.h"

//-----------------------------------------------------------------------------
// 目的: HmdDriverFactoryからポインターを受け取った後、vrserverによって呼び出されます。
// リソースの割り当てはここで行う必要があります（**コンストラクタではなく**）。
//-----------------------------------------------------------------------------
vr::EVRInitError MyDeviceProvider::Init(vr::IVRDriverContext* pDriverContext)
{
	// サーバーにコールを行うために、ドライバーコンテキストを初期化する必要があります。
	// OpenVRは、これを行うためのマクロを提供しています。
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

	// コントローラーをシステムに追加します。
	// まず、コントローラーデバイスを実際にインスタンス化する必要があります。
	// コンストラクタでコントローラーの役割を受け取るようにしましたので、それぞれの役割を渡しましょう。
	my_left_controller_device_ = std::make_unique< MyControllerDeviceDriver >(vr::TrackedControllerRole_LeftHand);
	my_right_controller_device_ = std::make_unique< MyControllerDeviceDriver >(vr::TrackedControllerRole_RightHand);

	// 今度は、vrserverにコントローラーについて知らせる必要があります。
	// 最初の引数はデバイスのシリアル番号で、すべてのデバイスで一意である必要があります。
	// インスタンス化時にドライバー設定から取得しました。
	// そして、MyGetSerialNumber()で関数を抜けて関数外に渡すことができます。
	// 最初に左手のコントローラーを追加します（特定の順序はありません）。
	// デバイスを実際に作成できたことを確認してください。
	// TrackedDeviceAddedがtrueを返すと、デバイスがSteamVRに追加されたことを意味します。
	if (!vr::VRServerDriverHost()->TrackedDeviceAdded(my_left_controller_device_->MyGetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, my_left_controller_device_.get()))
	{
		DriverLog("左コントローラーデバイスの作成に失敗しました！");
		// 失敗した？ 早めに戻ります。
		return vr::VRInitError_Driver_Unknown;
	}


	// 今度は、右手
	// デバイスを実際に作成できたことを確認してください。
	// TrackedDeviceAddedがtrueを返すと、デバイスがSteamVRに追加されたことを意味します。
	if (!vr::VRServerDriverHost()->TrackedDeviceAdded(my_right_controller_device_->MyGetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, my_right_controller_device_.get()))
	{
		DriverLog("右コントローラーデバイスの作成に失敗しました！");
		// 失敗した？ 早めに戻ります。
		return vr::VRInitError_Driver_Unknown;
	}

	return vr::VRInitError_None;
}

//-----------------------------------------------------------------------------
// 目的: ランタイムに、どのバージョンのAPIをターゲットにしているかを伝えます。
// 使用しているヘッダーのヘルパー変数には、この情報が含まれており、ここで返されます。
//-----------------------------------------------------------------------------
const char* const* MyDeviceProvider::GetInterfaceVersions()
{
	return vr::k_InterfaceVersions;
}

//-----------------------------------------------------------------------------
// 目的: この関数は非推奨であり、呼び出されません。 ただし、それでも定義する必要があります。そうしないと、コンパイルできません。
//-----------------------------------------------------------------------------
bool MyDeviceProvider::ShouldBlockStandbyMode()
{
	return false;
}

//-----------------------------------------------------------------------------
// 目的: これはvrserverのメインループで呼び出されます。
// ドライバーはここで作業を行うことができますが、この作業が比較的安価であることを確認する必要があります。
// ここで行う良いことは、ランタイムやアプリケーションからのイベントのポーリングです。
//-----------------------------------------------------------------------------
void MyDeviceProvider::RunFrame()
{
	// デバイスにフレームを実行させます
	if (my_left_controller_device_ != nullptr)
	{
		my_left_controller_device_->MyRunFrame();
	}

	if (my_right_controller_device_ != nullptr)
	{
		my_right_controller_device_->MyRunFrame();
	}

	// 今、このフレームに送信されたイベントを処理します。
	vr::VREvent_t vrevent{};
	while (vr::VRServerDriverHost()->PollNextEvent(&vrevent, sizeof(vr::VREvent_t)))
	{
		if (my_left_controller_device_ != nullptr)
		{
			my_left_controller_device_->MyProcessEvent(vrevent);
		}

		if (my_right_controller_device_ != nullptr)
		{
			my_right_controller_device_->MyProcessEvent(vrevent);
		}
	}
}

//-----------------------------------------------------------------------------
// 目的: この関数はシステムが非アクティブな状態に入ったときに呼び出されます。
// デバイスは、ディスプレイをオフにしたり、低消費電力モードに入ったりして、それらを保護する必要があります。
//-----------------------------------------------------------------------------
void MyDeviceProvider::EnterStandby()
{
}

//-----------------------------------------------------------------------------
// 目的: この関数は、システムが一定の期間非アクティブであった後に呼び出されます。ここでディスプレイまたはデバイスを再起動します。
//-----------------------------------------------------------------------------
void MyDeviceProvider::LeaveStandby()
{
}

//-----------------------------------------------------------------------------
// 目的: この関数は、vrserverからドライバーがアンロードされる直前に呼び出されます。
// ドライバーは、ここで獲得したリソースを解放する必要があります。
// この関数が呼び出される前に、サーバーへのすべての呼び出しが有効であることが保証されていますが、
// それ以降は呼び出せなくなります。
//-----------------------------------------------------------------------------
void MyDeviceProvider::Cleanup()
{
	// コントローラーデバイスはすでに非アクティブになっています。 これらを破壊しましょう。
	my_left_controller_device_ = nullptr;
	my_right_controller_device_ = nullptr;
}
