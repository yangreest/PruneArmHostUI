#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/// <summary>
/// 除树障机器人 控制板协议（V1.0）
/// 帧格式：FF FE | 包长度 | 包计数 | 指令号 | 指令内容 | 校验和 | FD FC
/// 串口：LoRa 无线链路（USART3，115200 8N1）
/// </summary>

/// <summary>
/// 推杆动作枚举
/// </summary>
enum class PushRodAction : uint8_t
{
	Stop = 0x00,    // 停止
	Extend = 0x01,  // 伸出
	Retract = 0x02, // 收回
	Brake = 0x03    // 刹车
};

/// <summary>
/// 速度档位枚举（对应推杆 PWM 值）
/// </summary>
enum class SpeedLevel : uint8_t
{
	Low = 0x00,    // 低速 PWM500
	Medium = 0x01, // 中速 PWM750
	High = 0x02    // 高速 PWM999
};

/// <summary>
/// 推杆选择枚举
/// </summary>
enum class PushRodSelect : uint8_t
{
	Rod1 = 0x01,
	Rod2 = 0x02,
	Both = 0x03
};

/// <summary>
/// 无刷电机选择枚举
/// </summary>
enum class BrushlessSelect : uint8_t
{
	Left = 0x01,
	Right = 0x02,
	Both = 0x03
};

/// <summary>
/// 开关量索引枚举（指令号 0x08）
/// </summary>
enum class SwitchIndex : uint8_t
{
	LowSpeed = 0x01,  // 低速档
	HighSpeed = 0x02, // 高速档
	Dir = 0x03        // DIR方向
};

/// <summary>
/// 心跳帧数据结构（模块→上位机，默认500ms一帧）
/// FF FE 12 <计数> 00 [10字节内容] <校验> FD FC
/// </summary>
class CDeviceHeartBeat
{
public:
	CDeviceHeartBeat();

	uint16_t m_wBusVoltage;       // Byte0~1 母线电压×100，小端（例 0x0998 = 2456 = 24.56V）
	uint8_t m_cFaultCode;         // Byte2 故障码（bit0=左无刷，bit1=右无刷）
	uint8_t m_cSpeedGear;         // Byte3 速度档位（0=无 1=低速 2=高速）
	uint8_t m_cOutputStatus;      // Byte4 输出状态位（bit0=DIR bit1=左无刷 bit2=右无刷 bit3=低速档 bit4=高速档）
	uint8_t m_cPushRod1Status;    // Byte5 推杆1状态（0=停止 1=伸出中 2=收回中）
	uint8_t m_cPushRod2Status;    // Byte6 推杆2状态（同上）
	uint8_t m_cRSSI;              // Byte7 RSSI绝对值（当前固定0）
	uint8_t m_cFirmwareMajor;     // Byte8 固件主版本号
	uint8_t m_cFirmwareMinor;     // Byte9 固件次版本号
};

class CWHSDControlBoardProtocol
{
public:
	CWHSDControlBoardProtocol(uint16_t wHeartBeatTime);

	bool BeginWork();

	bool EndWork();

	void ReceiveNewData(const uint8_t* p, int len);

	bool Parse();

	uint8_t m_cPackNumber;

	// ======================== 一、推杆电机（指令号 0x05，电机组=01）========================

	/// <summary>推杆1 伸出（低/中/高速）</summary>
	static std::vector<uint8_t> PushRod1Extend(SpeedLevel speed);

	/// <summary>推杆1 收回（低/中/高速）</summary>
	static std::vector<uint8_t> PushRod1Retract(SpeedLevel speed);

	/// <summary>推杆2 伸出（低/中/高速）</summary>
	static std::vector<uint8_t> PushRod2Extend(SpeedLevel speed);

	/// <summary>推杆2 收回（低/中/高速）</summary>
	static std::vector<uint8_t> PushRod2Retract(SpeedLevel speed);

	/// <summary>两推杆 伸出（低/中/高速）</summary>
	static std::vector<uint8_t> PushRodBothExtend(SpeedLevel speed);

	/// <summary>两推杆 收回（低/中/高速）</summary>
	static std::vector<uint8_t> PushRodBothRetract(SpeedLevel speed);

	/// <summary>推杆1 停止</summary>
	static std::vector<uint8_t> PushRod1Stop();

	/// <summary>推杆1 刹车</summary>
	static std::vector<uint8_t> PushRod1Brake();

	/// <summary>推杆2 停止</summary>
	static std::vector<uint8_t> PushRod2Stop();

	/// <summary>推杆2 刹车</summary>
	static std::vector<uint8_t> PushRod2Brake();

	/// <summary>两推杆 停止</summary>
	static std::vector<uint8_t> PushRodBothStop();

	/// <summary>两推杆 刹车</summary>
	static std::vector<uint8_t> PushRodBothBrake();

	// ======================== 二、无刷电机（指令号 0x05，电机组=02）========================

	/// <summary>左无刷 开/关</summary>
	static std::vector<uint8_t> LeftBrushlessOn();
	static std::vector<uint8_t> LeftBrushlessOff();

	/// <summary>右无刷 开/关</summary>
	static std::vector<uint8_t> RightBrushlessOn();
	static std::vector<uint8_t> RightBrushlessOff();

	/// <summary>左右无刷 开/关</summary>
	static std::vector<uint8_t> BothBrushlessOn();
	static std::vector<uint8_t> BothBrushlessOff();

	// ======================== 三、开关量（指令号 0x08）========================

	/// <summary>开关量控制（低速档/高速档/DIR方向 开/关）</summary>
	static std::vector<uint8_t> SetSwitch(SwitchIndex sw, bool on);

	/// <summary>低速档 开/关</summary>
	static std::vector<uint8_t> LowSpeedGearOn();
	static std::vector<uint8_t> LowSpeedGearOff();

	/// <summary>高速档 开/关</summary>
	static std::vector<uint8_t> HighSpeedGearOn();
	static std::vector<uint8_t> HighSpeedGearOff();

	/// <summary>DIR方向 开/关</summary>
	static std::vector<uint8_t> DirOn();
	static std::vector<uint8_t> DirOff();

	// ======================== 四、系统指令 ========================

	/// <summary>紧急停止（不受故障锁限制，全部输出切断）指令号 0x20</summary>
	static std::vector<uint8_t> EmergencyStop();

	/// <summary>RSSI查询 指令号 0x21</summary>
	static std::vector<uint8_t> QueryRSSI();

	/// <summary>
	/// 参数配置 指令号 0x22
	/// </summary>
	/// <param name="underVoltageX10">欠压阈值×10（范围100~250，即10.0~25.0V）</param>
	/// <param name="heartbeatIntervalX100">心跳周期×100ms（范围0~20，0=关闭心跳）</param>
	static std::vector<uint8_t> SetParameter(uint8_t underVoltageX10, uint8_t heartbeatIntervalX100);

	// ======================== 回调注册 ========================

	/// <summary>注册发送函数（用于发送数据到串口）</summary>
	void RegisterSendFunction(const std::function<bool(uint8_t*, int)>& f);

	/// <summary>注册心跳数据回调</summary>
	void RegisterDeviceHeartBeat(const std::function<void(const CDeviceHeartBeat&)>& f);

	/// <summary>注册设备日志回调</summary>
	void RegisterDeviceLog(const std::function<void(const std::string&)>& f);

	/// <summary>
	/// 注册故障反馈回调（指令号 0x06）
	/// 参数：faultCode 故障码, isOccurring true=发生 false=解除
	/// </summary>
	void RegisterFaultNotification(const std::function<void(uint8_t, bool)>& f);

	/// <summary>
	/// 注册指令应答回调（指令号 0x21）
	/// 参数：cmdId 被应答的指令号, resultCode 结果码
	///   结果码：0x00=执行失败 0x01=执行成功
	///   0x02=故障锁定拒绝(0x05)或档位切换中(0x08)
	///   0x03=故障锁定拒绝(0x08)
	/// </summary>
	void RegisterAnswerCallback(const std::function<void(uint8_t, uint8_t)>& f);

private:
	static std::vector<uint8_t> GetCmdData(uint8_t cmd, const std::vector<uint8_t>& cmdData);

	/// <summary>推杆电机通用控制（指令号 0x05，电机组=01）</summary>
	static std::vector<uint8_t> PushRodControl(PushRodSelect rod, PushRodAction action, SpeedLevel speed);

	/// <summary>无刷电机通用控制（指令号 0x05，电机组=02）</summary>
	static std::vector<uint8_t> BrushlessControl(BrushlessSelect sel, bool on);

	static uint8_t cPackNumber;

	void SendData(uint8_t* p, int len);

	void LoopSendHeartBeat();

	void DealHeartBeat();

	void DealAnswer();

	void DealFaultNotification();

	CDeviceHeartBeat m_memDeviceHeartBeat;

	std::vector<uint8_t> m_vectorDataBuffer;

	void Erase(int en);

	std::function<bool(uint8_t*, int)> m_function_Send;

	std::function<void(const CDeviceHeartBeat&)> m_function_DeviceHeartBeat;

	std::function<void(const std::string&)> m_function_WriteLogCallBack;

	std::function<void(uint8_t, bool)> m_function_FaultNotification;

	std::function<void(uint8_t, uint8_t)> m_function_AnswerCallBack;

	std::vector<uint8_t> m_vectorCmdData;

	int m_cCmd;

	bool m_nIsNeedExit;

	uint16_t m_wHeartBeatTime;

	bool m_bPauseHeartBeat;
};
