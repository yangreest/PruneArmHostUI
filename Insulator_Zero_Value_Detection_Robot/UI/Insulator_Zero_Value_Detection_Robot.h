#pragma once
#include <mutex>
#include <QtWidgets/QMainWindow>
#include "ui_Insulator_Zero_Value_Detection_Robot.h"
#include "Config/ConfigManager.h"
#include "DeviceCom/IDeviceCom.h"
#include "Log/ScanS_FC.h"
#include "Log/ScanS_WriteLog.h"
#include "Protocol/WHSDControlBoradProtocol.h"
#include "Tools/XInputHelper.h"
#include "Camera/CameraBase.h"
#include "Config/XmlManagerWindow.h"
#include <opencv2/opencv.hpp>

class Insulator_Zero_Value_Detection_Robot : public QMainWindow
{
	Q_OBJECT

public:
	Insulator_Zero_Value_Detection_Robot(QWidget* parent = nullptr);
	~Insulator_Zero_Value_Detection_Robot();
	// 在头文件中声明函数
	static QImage Mat2QImage(const cv::Mat& mat);

private slots:
	void On_timer_timeout();
	void On_timerInput_timeout();

	//void On_ZeroTest_Click();
	//void On_Setting_Click();
	//void captureCurrentWindow();
	//void On_SetFileName_Click();
	
	// 推杆电机伸出
    void On_PushRodExtend_Click();
    // 推杆电机收回
	void On_PushRodRetract_Click();
    // 推杆电机停止
    void On_PushRodStop_Click();
	// 推杆电机刹车
    void On_PushRodBrake_Click();

	// 无刷电机开始	
    void On_MotorStart_Click();
    // 无刷电机停止
    void On_MotorStop_Click();

	// 紧急停止
    void On_EmergencyStop_Click();

	// 速度切换（radioButton）
    void On_SpeedChange1_Click(bool checked);
	void On_SpeedChange2_Click(bool checked);
	// 方向切换
    void On_DirChange1_Click(bool checked);
	void On_DirChange2_Click(bool checked);


private:
	Ui::Insulator_Zero_Value_Detection_RobotClass ui;

	void Callback_DeviceHeartBeat(const CDeviceHeartBeat& b);

	void InitUI();

	void InitParam();

	void BindAction();

	void ComDeviceConnectionChanged(const bool connected, int guid, int index);

	void CallBack_ControllerState(int t, const ControllerState* p);

	void RefreshControllerState(const ControllerState* p);


	void savePixmap(const QPixmap& pixmap);

	ControllerState m_memControllerState;

	CXInputHelper* m_pXInputHelper;

	IDeviceCom* m_pComDevice;

	CConfigManager* m_pConfig;

	CWriteLog* m_pDeviceLog;

	CWHSDControlBoardProtocol* m_pWHSDControlBoardProtocol;

	XmlManagerWindow* xmlManagerWindow;

	std::mutex m_mutexDeviceInfoLock;

	CDeviceHeartBeat m_memDeviceHeartBeat;

	CCFRD_Time m_time_LastHeartBeatTime;

	uint64_t m_nHeartBeatCount;

	bool m_bControlBroadConnected;

	std::mutex m_mutexXInput;

	QTimer* m_pTimer;
	QTimer* m_pTimerInput;

	int m_nLastDir;


	uint64_t m_nTimeCount;

	bool m_bLastButton;

	QString	m_strFileName;

	bool continueStreaming;

public:
	// 摄像头
	void CameraConnect();
	void NewCameraConnect();
	ICameraBase* m_pC1;
	ICameraBase* m_pC2;

	std::string m_strLeftIp;
	std::string m_strRightIp;
};
