#include "Insulator_Zero_Value_Detection_Robot.h"
#include "Config/ConfigManager.h"

#include "Log/ScanS_WriteLog.h"
#include "Protocol/WHSDControlBoradProtocol.h"
#include "Tools/Tools.h"
#include <QTimer>
#include <QScreen>
#include <QApplication>
#include <QMessageBox.h>
#include <QDateTime>
#include <QFileDialog.h>

Insulator_Zero_Value_Detection_Robot::Insulator_Zero_Value_Detection_Robot(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	InitParam();
	InitUI();
	BindAction();
}

Insulator_Zero_Value_Detection_Robot::~Insulator_Zero_Value_Detection_Robot()
{
	continueStreaming = false;
}

void Insulator_Zero_Value_Detection_Robot::InitUI()
{
	//showMaximized();
}

void Insulator_Zero_Value_Detection_Robot::InitParam()
{

	m_strFileName = "C:";

	// 使用标志位控制循环
	continueStreaming = true;

	m_pDeviceLog = new CWriteLog(WHSD_Tools::GetAbsolutePath("Log\\DeviceLog.txt"), 10000, 250);
	m_pDeviceLog->BeginWork();
	m_pConfig = new CConfigManager();
	m_pConfig->Read(WHSD_Tools::GetAbsolutePath("Config.xml"));
	m_pXInputHelper = new CXInputHelper(0);
	m_pXInputHelper->RegisterControllerStateCallBack(std::bind(
		&Insulator_Zero_Value_Detection_Robot::CallBack_ControllerState, this, std::placeholders::_1,
		std::placeholders::_2));
	m_pXInputHelper->BeginWork();

	// 获取设备信息

	// 串口通讯：配置中 Ip 字段为串口名（如COM3），Port 字段为波特率（如115200）
	m_pComDevice = IDeviceCom::GetIDeviceCom(2);
	m_pWHSDControlBoardProtocol = new CWHSDControlBoardProtocol(
		m_pConfig->m_memControlBoardConfig.m_wDeviceHeartBeat);

	auto pWHSDControlBoardProtocol = m_pWHSDControlBoardProtocol;
	auto pDeviceCom = m_pComDevice;
	pDeviceCom->SetParam(m_pConfig->m_memControlBoardConfig.m_strIp.c_str(),
		m_pConfig->m_memControlBoardConfig.m_wPort);
	pDeviceCom->RegisterReadDataCallBack(std::bind(&CWHSDControlBoardProtocol::ReceiveNewData,
		pWHSDControlBoardProtocol, std::placeholders::_1,
		std::placeholders::_2));
	pDeviceCom->RegisterConnectStatusCallBack(std::bind(
		&Insulator_Zero_Value_Detection_Robot::ComDeviceConnectionChanged, this,
		std::placeholders::_1, std::placeholders::_2, 0));

	pWHSDControlBoardProtocol->RegisterSendFunction(
		std::bind(&IDeviceCom::Write, pDeviceCom, std::placeholders::_1, std::placeholders::_2));
	pWHSDControlBoardProtocol->RegisterDeviceLog(std::bind(&CWriteLog::Write, m_pDeviceLog, std::placeholders::_1));
	pWHSDControlBoardProtocol->RegisterDeviceHeartBeat(
		std::bind(&Insulator_Zero_Value_Detection_Robot::Callback_DeviceHeartBeat, this, std::placeholders::_1));
	//pWHSDControlBoardProtocol->RegisterOTAStatus(std::bind(&MainForm::Callback_OTAStatus, this,
	//	std::placeholders::_1,
	//	std::placeholders::_2, std::placeholders::_3));
	//pWHSDControlBoardProtocol->RegisterXRaySendResult(xRayResult);
	pDeviceCom->BeginWork();
	pWHSDControlBoardProtocol->BeginWork();

	if (m_pConfig->m_memCCameraConfig.m_bNewCamera)
	{
		std::thread td(&Insulator_Zero_Value_Detection_Robot::NewCameraConnect, this);
		td.detach();
	}
	else
	{
		m_strLeftIp = m_pConfig->m_memCCameraConfig.m_strLeftIp;
		m_strRightIp = m_pConfig->m_memCCameraConfig.m_strRightIp;

		//m_pC1 = ICameraBase::GetCameraObj(0);
		//m_pC2 = ICameraBase::GetCameraObj(0);

		//auto handleR = reinterpret_cast<HWND>(ui.label_22->winId());// winId的功能是获取窗口句柄
		//auto handleL = reinterpret_cast<HWND>(ui.label_9->winId());

		//m_pC1->RegisterVideoViewHandle(handleL);
		//m_pC2->RegisterVideoViewHandle(handleR);

		//std::thread td(&Insulator_Zero_Value_Detection_Robot::CameraConnect, this);
		//td.detach();
	}

}

void Insulator_Zero_Value_Detection_Robot::BindAction()
{
	m_pTimer = new QTimer(this);
	m_pTimer->setInterval(100);
	connect(m_pTimer, &QTimer::timeout, this, &Insulator_Zero_Value_Detection_Robot::On_timer_timeout);
	m_pTimer->start();

	m_pTimerInput = new QTimer(this);
	m_pTimerInput->setInterval(10);
	connect(m_pTimerInput, &QTimer::timeout, this, &Insulator_Zero_Value_Detection_Robot::On_timerInput_timeout);
	m_pTimerInput->start();


	connect(ui.pushButton, &QPushButton::clicked, this, &Insulator_Zero_Value_Detection_Robot::On_PushRodRetract_Click);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &Insulator_Zero_Value_Detection_Robot::On_PushRodExtend_Click);
	connect(ui.pushButton_3, &QPushButton::clicked, this, &Insulator_Zero_Value_Detection_Robot::On_MotorStart_Click);//
	connect(ui.pushButton_4, &QPushButton::clicked, this, &Insulator_Zero_Value_Detection_Robot::On_MotorStop_Click);
	connect(ui.pushButton_5, &QPushButton::clicked, this, &Insulator_Zero_Value_Detection_Robot::On_PushRodBrake_Click);
	connect(ui.pushButton_6, &QPushButton::clicked, this, &Insulator_Zero_Value_Detection_Robot::On_PushRodStop_Click);
	connect(ui.pushButton_7, &QPushButton::clicked, this, &Insulator_Zero_Value_Detection_Robot::On_EmergencyStop_Click);

	connect(ui.radioButton_5, &QPushButton::toggled, this, &Insulator_Zero_Value_Detection_Robot::On_SpeedChange1_Click);
	connect(ui.radioButton_6, &QPushButton::toggled, this, &Insulator_Zero_Value_Detection_Robot::On_SpeedChange2_Click);
	connect(ui.radioButton_3, &QPushButton::toggled, this, &Insulator_Zero_Value_Detection_Robot::On_DirChange1_Click);
	connect(ui.radioButton_4, &QPushButton::toggled, this, &Insulator_Zero_Value_Detection_Robot::On_DirChange2_Click);
}

void Insulator_Zero_Value_Detection_Robot::CallBack_ControllerState(int t, const ControllerState* p)
{
	std::lock_guard<std::mutex> g(m_mutexXInput);
	memcpy(&m_memControllerState, p, sizeof(ControllerState));
}

void Insulator_Zero_Value_Detection_Robot::On_timer_timeout()
{
	// 定时查询RSSI
	if (m_nTimeCount++ % 10 == 0)
	{
		auto cmds = CWHSDControlBoardProtocol::QueryRSSI();
		//m_pComDevice->Write(cmds.data(), cmds.size());
	}

	ui.label_34->setText(QString::number(m_nHeartBeatCount));
	ui.label_3->setText(m_bControlBroadConnected ? "已连接" : "未连接");
	const auto memDeviceHeartBeat = m_memDeviceHeartBeat;

	// 推杆1状态
	std::string strPushRod1Status("停止");
	switch (memDeviceHeartBeat.m_cPushRod1Status)
	{
	case 0: strPushRod1Status = "停止"; break;
	case 1: strPushRod1Status = "伸出中"; break;
	case 2: strPushRod1Status = "收回中"; break;
	default: break;
	}
	ui.label->setText(QString::fromStdString(strPushRod1Status));

	// 推杆2状态
	std::string strPushRod2Status("停止");
	switch (memDeviceHeartBeat.m_cPushRod2Status)
	{
	case 0: strPushRod2Status = "停止"; break;
	case 1: strPushRod2Status = "伸出中"; break;
	case 2: strPushRod2Status = "收回中"; break;
	default: break;
	}
	ui.label_6->setText(QString::fromStdString(strPushRod2Status));

	// 母线电压（单位0.01V）
	float voltage = memDeviceHeartBeat.m_wBusVoltage / 100.0f;
	ui.label_13->setText(QString::number(voltage, 'f', 2));

	// 故障码
	if (memDeviceHeartBeat.m_cFaultCode != 0)
	{
		std::string faultStr;
		if (memDeviceHeartBeat.m_cFaultCode & 0x01) faultStr += "左无刷故障 ";
		if (memDeviceHeartBeat.m_cFaultCode & 0x02) faultStr += "右无刷故障";
		ui.label_8->setText(QString::fromStdString(faultStr));
	}
	else
	{
		ui.label_8->setText("正常");
	}
}

void Insulator_Zero_Value_Detection_Robot::On_timerInput_timeout()
{
	ControllerState tp;
	{
		std::lock_guard<std::mutex> g(m_mutexXInput);
		memcpy(&tp, &m_memControllerState, sizeof(ControllerState));
	}

	//// 手柄按键：紧急停止
	//if (!m_bLastButton && tp.buttons[5])
	//{
	//	auto cmds = CWHSDControlBoardProtocol::EmergencyStop();
	//	m_pComDevice->Write(cmds.data(), cmds.size());
	//}
	//m_bLastButton = tp.buttons[5];

	//ui.label_20->setText(m_bLastButton ? "ON" : "OFF");
	//if (m_nLastDir == 0)
	//{
	//	switch (tp.dpad)
	//	{
	//	case 1: // 上：推杆1伸出（高速）
	//	{
	//		auto cmds = CWHSDControlBoardProtocol::PushRod1Extend(SpeedLevel::High);
	//		m_pComDevice->Write(cmds.data(), cmds.size());
	//		ui.label_2->setText("上");
	//		break;
	//	}
	//	case 2: // 右：左无刷开
	//	{
	//		auto cmds = CWHSDControlBoardProtocol::LeftBrushlessOn();
	//		m_pComDevice->Write(cmds.data(), cmds.size());
	//		ui.label_2->setText("右");
	//		break;
	//	}
	//	case 3: // 下：推杆1收回（高速）
	//	{
	//		auto cmds = CWHSDControlBoardProtocol::PushRod1Retract(SpeedLevel::High);
	//		m_pComDevice->Write(cmds.data(), cmds.size());
	//		ui.label_2->setText("下");
	//		break;
	//	}
	//	case 4: // 左：右无刷开
	//	{
	//		auto cmds = CWHSDControlBoardProtocol::RightBrushlessOn();
	//		m_pComDevice->Write(cmds.data(), cmds.size());
	//		ui.label_2->setText("左");
	//		break;
	//	}
	//	case 0:
	//	default:
	//	{
	//		ui.label_2->setText("");
	//		break;
	//	}
	//	}
	//}
	//else if (m_nLastDir == 2 || m_nLastDir == 4)
	//{
	//	if (tp.dpad == 0)
	//	{
	//		// 无刷电机停止
	//		auto cmds = CWHSDControlBoardProtocol::BothBrushlessOff();
	//		m_pComDevice->Write(cmds.data(), cmds.size());
	//	}
	//}
	//else if (m_nLastDir == 1 || m_nLastDir == 3)
	//{
	//	if (tp.dpad == 0)
	//	{
	//		// 推杆停止
	//		auto cmds = CWHSDControlBoardProtocol::PushRod1Stop();
	//		m_pComDevice->Write(cmds.data(), cmds.size());
	//	}
	//}
	//m_nLastDir = tp.dpad;
}

void Insulator_Zero_Value_Detection_Robot::On_PushRodExtend_Click()
{
	// 选取推杆电机的速度 radioButton是低速，radioButton_2是高速，radioButton_7是中速
	SpeedLevel speed = SpeedLevel::Low;
	if (ui.radioButton_7->isChecked())
	{
		speed = SpeedLevel::Medium;
	}
	else if (ui.radioButton_2->isChecked())
	{
		speed = SpeedLevel::High;
	}
	else
	{
		speed = SpeedLevel::Low;
	}
	if (ui.checkBox->isChecked() && !ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod1Extend(speed);
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox_2->isChecked() && !ui.checkBox->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod2Extend(speed);
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox->isChecked() && ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRodBothExtend(speed);
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
}

void Insulator_Zero_Value_Detection_Robot::On_PushRodRetract_Click()
{
	// 选取推杆电机的速度 radioButton是低速，radioButton_2是高速，radioButton_7是中速
	SpeedLevel speed = SpeedLevel::Low;
	if (ui.radioButton_7->isChecked())
	{
		speed = SpeedLevel::Medium;
	}
	else if (ui.radioButton_2->isChecked())
	{
		speed = SpeedLevel::High;
	}
	else
	{
		speed = SpeedLevel::Low;
	}
	if (ui.checkBox_2->isChecked() && !ui.checkBox->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod1Retract(speed);
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox->isChecked() && !ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod2Extend(speed);
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox->isChecked() && ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRodBothExtend(speed);
		m_pComDevice->Write(cmds.data(), cmds.size());
	}

}

void Insulator_Zero_Value_Detection_Robot::On_PushRodStop_Click()
{
	if (ui.checkBox_2->isChecked() && !ui.checkBox->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod1Stop();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox->isChecked() && !ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod2Stop();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox->isChecked() && ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRodBothStop();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
}

void Insulator_Zero_Value_Detection_Robot::On_PushRodBrake_Click()
{
	if (ui.checkBox_2->isChecked() && !ui.checkBox->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod1Brake();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox->isChecked() && !ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRod2Brake();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
	if (ui.checkBox->isChecked() && ui.checkBox_2->isChecked())
	{
		auto cmds = CWHSDControlBoardProtocol::PushRodBothBrake();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
}

void Insulator_Zero_Value_Detection_Robot::On_MotorStart_Click()
{
	if (ui.checkBox_3->isChecked() && !ui.checkBox_4->isChecked())
	{
        auto cmds = CWHSDControlBoardProtocol::LeftBrushlessOn();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
    if (ui.checkBox_4->isChecked() && !ui.checkBox_3->isChecked())
	{
        auto cmds = CWHSDControlBoardProtocol::RightBrushlessOn();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
    if (ui.checkBox_3->isChecked() && ui.checkBox_4->isChecked())
	{
        auto cmds = CWHSDControlBoardProtocol::BothBrushlessOn();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
}

void Insulator_Zero_Value_Detection_Robot::On_MotorStop_Click()
{
    if (ui.checkBox_3->isChecked() && !ui.checkBox_4->isChecked())
	{
        auto cmds = CWHSDControlBoardProtocol::LeftBrushlessOff();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
    if (ui.checkBox_4->isChecked() && !ui.checkBox_3->isChecked())
	{
        auto cmds = CWHSDControlBoardProtocol::RightBrushlessOff();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
    if (ui.checkBox_3->isChecked() && ui.checkBox_4->isChecked())
	{
        auto cmds = CWHSDControlBoardProtocol::BothBrushlessOff();
		m_pComDevice->Write(cmds.data(), cmds.size());
	}
}

void Insulator_Zero_Value_Detection_Robot::On_EmergencyStop_Click()
{
    auto cmds = CWHSDControlBoardProtocol::EmergencyStop();
}

// 开高速，关低速
void Insulator_Zero_Value_Detection_Robot::On_SpeedChange1_Click(bool checked)
{
    if (checked)
    {
		auto cmds = CWHSDControlBoardProtocol::LowSpeedGearOff();
		m_pComDevice->Write(cmds.data(), cmds.size());

		// 等待 100ms
        Sleep(100);

        cmds = CWHSDControlBoardProtocol::HighSpeedGearOn();
		m_pComDevice->Write(cmds.data(), cmds.size());
    }
}

void Insulator_Zero_Value_Detection_Robot::On_SpeedChange2_Click(bool checked)
{
    if (checked)
    {
		auto cmds = CWHSDControlBoardProtocol::HighSpeedGearOff();
		m_pComDevice->Write(cmds.data(), cmds.size());

		// 等待 100ms
        Sleep(100);

        cmds = CWHSDControlBoardProtocol::LowSpeedGearOn();
		m_pComDevice->Write(cmds.data(), cmds.size());
    }
}

void Insulator_Zero_Value_Detection_Robot::On_DirChange1_Click(bool checked)
{
    if (checked)
    { 
		auto cmds = CWHSDControlBoardProtocol::DirOn();
        m_pComDevice->Write(cmds.data(), cmds.size());
    }
}
void Insulator_Zero_Value_Detection_Robot::On_DirChange2_Click(bool checked)
{
    if (checked)
    {
		auto cmds = CWHSDControlBoardProtocol::DirOff();
        m_pComDevice->Write(cmds.data(), cmds.size());
    }
}

void Insulator_Zero_Value_Detection_Robot::ComDeviceConnectionChanged(const bool connected, int guid, int index)
{
	m_bControlBroadConnected = connected;
}

void Insulator_Zero_Value_Detection_Robot::RefreshControllerState(const ControllerState* p)
{
	// 刷新控制器状态
	std::lock_guard<std::mutex> g(m_mutexXInput);
	memcpy(&m_memControllerState, p, sizeof(ControllerState));
}

void Insulator_Zero_Value_Detection_Robot::CameraConnect()
{
	int initStatus = 0;
	bool needBreak = false;
	while (true)
	{
		if (needBreak)
		{
			break;
		}
		switch (initStatus)
		{
		case 0:
		{
			if (m_pC1->Init({}))
			{
				initStatus = 1;
			}
			break;
		}
		case 1:
		{
			if (m_pC1->Connect(m_strLeftIp, 0, "", ""))
			{
				initStatus = 2;
			}
			break;
		}
		case 2:
		{
			if (m_pC2->Connect(m_strRightIp, 0, "", ""))
			{
				initStatus = 3;
			}
			break;
		}
		//case 3:
		//{
		//	if (m_pC3->Connect(m_strRightIp, 0, "", ""))
		//	{
		//		initStatus = 4;
		//	}
		//	break;
		//}
		default:
		{
			break;
		}
		}
	}
}

void Insulator_Zero_Value_Detection_Robot::NewCameraConnect()
{
	// 使用项目配置管理器获取RTSP参数

	std::string rtsp_url;
	if (m_pConfig->m_memCCameraConfig.m_bUseMainSp)
	{
		rtsp_url = m_pConfig->m_memCCameraConfig.m_strMainRtsp;
	}
	else
	{
		rtsp_url = m_pConfig->m_memCCameraConfig.m_strSubRtsp;
	}

	cv::VideoCapture cap;

	// 设置低延迟参数
	cap.set(cv::CAP_PROP_BUFFERSIZE, 1);  // 最小化缓冲区

	// 尝试打开RTSP流
	bool isOpen = cap.open(rtsp_url, cv::CAP_FFMPEG);

	if (!isOpen)
	{
		qDebug() << "Failed to open RTSP stream: " << rtsp_url;
		return; // 不应该返回-1，因为这不是main函数
	}

	cv::Mat frame;


	while (continueStreaming)
	{
		// 低延迟处理：获取最新帧
		if (cap.grab())
		{
			cap.retrieve(frame);

			if (!frame.empty())
			{
				//cv::imshow("RTSP Low Delay", frame);
				// 将frame 转成QImage显示在lable上
				QImage qImg = Mat2QImage(frame);
				ui.label->setPixmap(QPixmap::fromImage(qImg));

				//// 检查退出条件
				//int key = cv::waitKey(1) & 0xFF;
				//if (key == 27) // ESC键退出
				//{
				//	break;
				//}
			}
			else
			{
				qDebug() << "Frame is empty";
				break;
			}
		}
		else
		{
			qDebug() << "Failed to grab frame";
			break;
		}
	}
	// 清理资源
	cap.release();
}



// 函数实现
QImage Insulator_Zero_Value_Detection_Robot::Mat2QImage(const cv::Mat& mat)
{
	// 处理空矩阵
	if (mat.empty()) {
		return QImage();
	}

	// 如果是彩色图像（BGR -> RGB）
	if (mat.channels() == 3) {
		cv::Mat rgb;
		cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
		return QImage((const unsigned char*)rgb.data,
			rgb.cols,
			rgb.rows,
			rgb.step,
			QImage::Format_RGB888);
	}
	// 如果是灰度图像
	else if (mat.channels() == 1) {
		return QImage((const unsigned char*)mat.data,
			mat.cols,
			mat.rows,
			mat.step,
			QImage::Format_Grayscale8); // Qt 5.13+ 支持，更早版本可用 Format_Indexed8
	}
	// 如果是RGBA图像
	else if (mat.channels() == 4) {
		cv::Mat rgba;
		cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
		return QImage((const unsigned char*)rgba.data,
			rgba.cols,
			rgba.rows,
			rgba.step,
			QImage::Format_RGBA8888);
	}

	// 对于其他通道数，先转换为RGB
	cv::Mat rgb;
	if (mat.channels() == 3) {
		cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
	}
	else {
		cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
	}

	return QImage((const unsigned char*)rgb.data,
		rgb.cols,
		rgb.rows,
		rgb.step,
		QImage::Format_RGB888);
}

void Insulator_Zero_Value_Detection_Robot::Callback_DeviceHeartBeat(const CDeviceHeartBeat& b)
{
	std::lock_guard<std::mutex> lock(m_mutexDeviceInfoLock);
	m_memDeviceHeartBeat = b;
	m_time_LastHeartBeatTime.GetCurTime();
	m_nHeartBeatCount++;
}


//void Insulator_Zero_Value_Detection_Robot::On_ZeroTest_Click()
//{
//	// 零值检测按钮功能待适配新协议
//}
//
//void Insulator_Zero_Value_Detection_Robot::On_Setting_Click()
//{
//	if (xmlManagerWindow == nullptr)
//	{
//		xmlManagerWindow = new XmlManagerWindow();
//	}
//	xmlManagerWindow->show();
//}
//
//void Insulator_Zero_Value_Detection_Robot::captureCurrentWindow()
//{
	// 获取当前窗口的句柄（Windows）/ID（Linux）
	//WId windowId = ui.label_9->winId();

	// 获取当前窗口所在的屏幕
	//QScreen* screen = QApplication::screenAt(ui.label_9->pos());
	//if (!screen) {
	//	screen = QApplication::primaryScreen();
	//}

	//// 截取指定窗口
	//QPixmap pixmap = screen->grabWindow(windowId);

	//// 保存截图
	//savePixmap(pixmap);
//}

//void Insulator_Zero_Value_Detection_Robot::On_SetFileName_Click()
//{
//	// 此处弹出对话框选择一个文件夹
//	QString filePath = QFileDialog::getExistingDirectory(this, "选择文件夹");
//	if (!filePath.isEmpty()) {
//		m_strFileName = filePath;
//		return;
//	}
//	QMessageBox::information(this, "提示", "文件保存失败");
//}

// 保存截图到文件
void Insulator_Zero_Value_Detection_Robot::savePixmap(const QPixmap& pixmap)
{
	if (pixmap.isNull()) {
		QMessageBox::warning(this, "错误", "截图失败，像素数据为空");
		return;
	}

	//// 弹出保存文件对话框
	//QString filePath = QFileDialog::getSaveFileName(
	//	this,
	//	"保存截图",
	//	QString("截图_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss")),
	//	"PNG图片 (*.png);;JPG图片 (*.jpg);;BMP图片 (*.bmp)"
	//);

	QString filePath = QString("%1/%2.png").arg(m_strFileName).arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));

	if (!filePath.isEmpty()) {
		// 保存图片
		bool success = pixmap.save(filePath);
		if (success) {
			QMessageBox::information(this, "成功", QString("截图已保存到：\n%1").arg(filePath));
		}
		else {
			QMessageBox::warning(this, "错误", "截图保存失败");
		}
	}
}
