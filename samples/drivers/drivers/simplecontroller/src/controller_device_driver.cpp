//============ Valve Corporationによる著作権、全ての権利を保有します。 ============
#include "controller_device_driver.h"

#include "driverlog.h"
#include "vrmath.h"

// 設定を取得するための文字列の変数を作成しましょう。
// ここは設定がすべて格納されるセクションです。セクション名は任意ですが、
// ドライバ固有の設定を保存する場合、セクションをドライバ識別子で名前空間化するのが最適です
// 例えば、"<my_driver>_<section>"とします。
static const char *my_controller_main_settings_section = "driver_simplecontroller";

// 個々の右/左手の設定セクション
static const char *my_controller_right_settings_section = "driver_simplecontroller_left_controller";
static const char *my_controller_left_settings_section = "driver_simplecontroller_right_controller";

// これらは設定で値を取得したいキーです
static const char *my_controller_settings_key_model_number = "mycontroller_model_number";
static const char *my_controller_settings_key_serial_number = "mycontroller_serial_number";


MyControllerDeviceDriver::MyControllerDeviceDriver(vr::ETrackedControllerRole role)
{
	// アクティブ化されたかどうかを追跡するためのメンバーを設定します
	is_active_ = false;

	// コンストラクタは、コントローラが左手か右手かに関する情報を提供する役割引数を取ります。
	// 後で使用するためにこれを保存しましょう。
	my_controller_role_ = role;

	// SteamVRの設定にモデル番号とシリアル番号が格納されています。これらを取得する必要があります。
	// 他のIVRSettingsメソッド（int32、float、boolを取得するためのもの）はデータを返しますが、
	// 文字列は異なります。
	char model_number[1024];
	vr::VRSettings()->GetString(my_controller_main_settings_section, my_controller_settings_key_model_number, model_number, sizeof(model_number));
	my_controller_model_number_ = model_number;

	// "handedness"に応じてシリアル番号を取得します
	char serial_number[1024];
	vr::VRSettings()->GetString(my_controller_role_ == vr::TrackedControllerRole_LeftHand ? my_controller_left_settings_section : my_controller_right_settings_section,
		my_controller_settings_key_serial_number, serial_number, sizeof(serial_number));
	my_controller_serial_number_ = serial_number;

	// IVRDriverLogの周りにラップされたロギングラッパーの使用例です
	// SteamVRログ（SteamVRハンバーガーメニュー>開発者設定>Webコンソール）では、ドライバーのプレフィックスが
	// "<driver_name>:"です。これをトップ検索バーで検索して、ログに記録した情報を見つけることができます。
	DriverLog("My Controller Model Number: %s", my_controller_model_number_.c_str());
	DriverLog("My Controller Serial Number: %s", my_controller_serial_number_.c_str());
}

// vrserverがIServerTrackedDeviceProviderのIVRServerDriverHost::TrackedDeviceAddedを呼び出した後に呼び出されます。
vr::EVRInitError MyControllerDeviceDriver::Activate(uint32_t unObjectId)
{
	// アクティブ化されたかどうかを追跡するためのメンバーを設定します
	is_active_ = true;

	// デバイスインデックスを追跡しましょう。後で役立ちます。
	my_controller_index_ = unObjectId;

	// プロパティはコンテナに格納されています。通常、デバイスインデックスごとに1つのコンテナがあります。これを取得して
	// 設定したいプロパティを設定する必要があります。
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(my_controller_index_);

	// これで、コンテナを取得したので、プロパティの設定を開始しましょう。
	// 利用可能なプロパティのリストはvr::ETrackedDevicePropertyに含まれています。

	// 最初にモデル番号を設定しましょう。
	vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, my_controller_model_number_.c_str());

	// コンストラクタで受け取った役割をSteamVRに伝えましょう。
	vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32, my_controller_role_);

	// 今度は入力を設定しましょう。

	// これはUIにこのコントローラーのバインディングを表示するためのものです。
	// また、既存のアプリケーションのデフォルトバインディングも設定します。
	// 注：ワイルドカード{<driver_name>}を使用して、ドライバーのルートフォルダーの場所に一致させることができます
	// 使用されているドライバーを示します。
	vr::VRProperties()->SetStringProperty(container, vr::Prop_InputProfilePath_String, "{simplecontroller}/input/mycontroller_profile.json");

	// すべてのコンポーネントのハンドルを設定しましょう。
	// これらは入力プロファイルでも定義されていますが、
	// 入力を更新するためにそれらのハンドルを取得する必要があります。

	// "A"ボタンを設定しましょう。タッチとクリックのコンポーネントがあると定義されています。
	vr::VRDriverInput()->CreateBooleanComponent(container, "/input/a/touch", &input_handles_[MyComponent_a_touch]);
	vr::VRDriverInput()->CreateBooleanComponent(container, "/input/a/click", &input_handles_[MyComponent_a_click]);

	// トリガーを設定しましょう。値とクリックのコンポーネントが定義されています。

	// CreateScalarComponentには以下が必要です:
	// EVRScalarType - デバイスが絶対位置を提供できるか、前回どこにあったかに対して相対位置のみを提供できるかを指定します。
	// 絶対位置を取得できます。
	// EVRScalarUnits - デバイスが2つの「サイド」を持っているかどうか。これにより、有効な入力の範囲が-1になります
	// 1。そうでなければ、0から1までです。私たちは1つの「サイド」しか持っていませんので、私たちの場合はonesidedです。
	vr::VRDriverInput()->CreateScalarComponent(container, "/input/trigger/value", &input_handles_[MyComponent_trigger_value], vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
	vr::VRDriverInput()->CreateBooleanComponent(container, "/input/trigger/click", &input_handles_[MyComponent_trigger_click]);

	// ハプティクスコンポーネントを作成します。
	// これらはデバイス全体でグローバルであり、デバイスごとに1つしか持つことができません。
	vr::VRDriverInput()->CreateHapticComponent(container, "/output/haptic", &input_handles_[MyComponent_haptic]);

	my_pose_update_thread_ = std::thread(&MyControllerDeviceDriver::MyPoseUpdateThread, this);

	// すべてが正常にアクティブ化されました！
	// SteamVRにエラーがないことを伝えるために、これを行います。
	return vr::VRInitError_None;
}

// HMDの場合、ここでvr::IVRDisplayComponent、vr::IVRVirtualDisplay、またはvr::IVRDirectModeComponentの実装を返します。
// ただし、これはコントローラーのデモのための単純な例なので、ここではnullptrを返します。
void *MyControllerDeviceDriver::GetComponent(const char *pchComponentNameAndVersion)
{
	return nullptr;
}

// これはvrserverがアプリケーションからドライバーにデバッグリクエストがあったときに呼び出されます。
// 応答とリクエストに含まれるものは、アプリケーションとドライバーが自分で解決する必要があります。
void MyControllerDeviceDriver::DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize)
{
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

// これはvrserverによって決して呼び出されません。
// ただし、vr::VRServerDriverHost::TrackedDevicePoseUpdatedにデータを提供するために役立ちます。
vr::DriverPose_t MyControllerDeviceDriver::GetPose()
{
	// Hmdの姿勢を取得して、コントローラーの姿勢を基にします。

	// まず、姿勢を更新したことをランタイムに通知するために提出する構造体を初期化します。
	vr::DriverPose_t pose = {0};

	// これらは有効なクォータニオンに設定する必要があります。それ以外はデバイスが表示されません。
	pose.qWorldFromDriverRotation.w = 1.f;
	pose.qDriverFromHeadRotation.w = 1.f;

	vr::TrackedDevicePose_t hmd_pose{};

	// GetRawTrackedDevicePosesは配列が必要です。
	// hmdの姿勢のみが欲しいので、配列のインデックス0にあるhmdの姿勢だけを渡すことができます
	vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0.f, &hmd_pose, 1);

	// GetRawTrackedDevicePosesが返す3x4行列からhmdの位置を取得します
	const vr::HmdVector3_t hmd_position = HmdVector3_From34Matrix(hmd_pose.mDeviceToAbsoluteTracking);
	// GetRawTrackedDevicePosesが返す3x4行列からhmdの方向を取得します
	const vr::HmdQuaternion_t hmd_orientation = HmdQuaternion_FromMatrix(hmd_pose.mDeviceToAbsoluteTracking);

	// コントローラーを90度ピッチして、コントローラーの表面が私たちに向かっているようにします
	const vr::HmdQuaternion_t offset_orientation = HmdQuaternion_FromEulerAngles(0.f, DEG_TO_RAD(90.f), 0.f);

	// オフセットを適用したhmd方向を姿勢方向に設定します。
	pose.qRotation = hmd_orientation * offset_orientation;

	const vr::HmdVector3_t offset_position = {
		my_controller_role_ == vr::TrackedControllerRole_LeftHand ? -0.15f : 0.15f, // 役割に応じてコントローラーを左/右に移動させます。 0.15m
		0.1f,																		// もう少し上に移動して表示されるようにします
		-0.5f,																		// コントローラーをhmdの前方に0.5m配置します。
	};

	// オフセットをhmdクォータニオンで回転させます（コントローラーが常に私たちに向かっているように）、その後hmdの位置を加えて位置を設定します。
	const vr::HmdVector3_t position = hmd_position + (offset_position * hmd_orientation);

	// 位置を姿勢にコピーします
	pose.vecPosition[0] = position.v[0];
	pose.vecPosition[1] = position.v[1];
	pose.vecPosition[2] = position.v[2];

	// 提供した姿勢は有効です。
	// これは設定されるべきです
	pose.poseIsValid = true;

	// デバイスは常に接続されています。
	// 実際の物理デバイスでは、切断された場合はfalseに設定し、SteamVRのアイコンがデバイスが切断されたことを示すように更新されます
	pose.deviceIsConnected = true;

	// トラッキングの状態。仮想デバイスの場合、常にokですが、これはデバイスのトラッキングの状態についてランタイムに通知し、
	// ユーザーに通知するアイコンを更新するために異なるように設定される場合があります
	// デバイスの追跡
	pose.result = vr::TrackingResult_Running_OK;

	return pose;
}

void MyControllerDeviceDriver::MyPoseUpdateThread()
{
	while (is_active_)
	{
		// vrserverにトラッキングされたデバイスの姿勢が更新されたことを通知し、GetPose()が返す姿勢を渡します。
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(my_controller_index_, GetPose(), sizeof(vr::DriverPose_t));

		// 5ミリ秒ごとに姿勢を更新します。
		// 実際には、デバイスから新しいデータがあるたびに姿勢を更新する必要があります。
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

// デバイスがスタンバイモードに入るべきときにvrserverによって呼び出されます。
// デバイスはその低消費電力モードに配置されるべきです。
// ここでは何もする必要がないので、ログに何かを記録しましょう。
void MyControllerDeviceDriver::EnterStandby()
{
	DriverLog("%s hand has been put on standby", my_controller_role_ == vr::TrackedControllerRole_LeftHand ? "Left" : "Right");
}

// デバイスが非アクティブにする必要があるときにvrserverによって呼び出されます。
// 通常、セッションの終わりに
// デバイスはここで割り当てられたリソースを解放する必要があります。
void MyControllerDeviceDriver::Deactivate()
{
	// 実行中のポーズスレッドに参加しましょう
	// まず、is_active_を確認してfalseに設定して、ループを抜けることができるようにします
	// 実行中であれば、スレッド上の.join()を呼び出します
	if (is_active_.exchange(false))
	{
		my_pose_update_thread_.join();
	}

	// コントローラーインデックスを解除します（Deactivate()が呼び出された後はもうvrserverを呼び出したくありません）
	my_controller_index_ = vr::k_unTrackedDeviceIndexInvalid;
}


// IServerTrackedDeviceProviderがRunFrame()メソッドを呼び出したときにIServerTrackedDeviceProviderによって呼び出されます。
// これはITrackedDeviceServerDriverインターフェイスの一部ではありません。私たち自身が作成しました。
void MyControllerDeviceDriver::MyRunFrame()
{
	// 入力をここで更新します。ハードウェアからの実際の入力の場合、これは別のスレッドで読み取られる可能性があります。
	vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[MyComponent_a_click], false, 0);
	vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[MyComponent_a_touch], false, 0);

	vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[MyComponent_trigger_click], false, 0);
	vr::VRDriverInput()->UpdateScalarComponent(input_handles_[MyComponent_trigger_value], 0.f, 0);

	// トリガー値を1に設定したい場合は、次のようにします：
	// vr::VRDriverInput()->UpdateScalarComponent( input_handles_[ MyComponent_trigger_value ], 1.f, 0 );

	// またはAボタンがクリックされたことを示す場合：
	// vr::VRDriverInput()->UpdateBooleanComponent( input_handles_[ MyComponent_a_click ], true, 0 );
}


// IServerTrackedDeviceProviderがイベントキューからイベントをポップするときにIServerTrackedDeviceProviderによって呼び出されます。
// これはITrackedDeviceServerDriverインターフェイスの一部ではありません。私たち自身が作成しました。
void MyControllerDeviceDriver::MyProcessEvent(const vr::VREvent_t &vrevent)
{
	switch (vrevent.eventType)
	{
	// ハプティックイベントを待機します
	case vr::VREvent_Input_HapticVibration:
	{
		// イベントがこのデバイスのものであることを確認する必要があります。
		// したがって、イベントのハンドルと私たちのハプティックコンポーネントのハンドルを比較します

		if (vrevent.data.hapticVibration.componentHandle == input_handles_[MyComponent_haptic])
		{
			// イベントは私たちのために意図されていました！
			// 現在は、これを実装しませんが、応答する準備ができています。
		}
	}
	break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// 目的: IServerTrackedDeviceProviderには、vrserverに追加されるためにシリアル番号が必要です。
// これはITrackedDeviceServerDriverインターフェイスの一部ではありません。私たち自身が作成しました。
//-----------------------------------------------------------------------------
const std::string& MyControllerDeviceDriver::MyGetSerialNumber()
{
	return my_controller_serial_number_;
}
