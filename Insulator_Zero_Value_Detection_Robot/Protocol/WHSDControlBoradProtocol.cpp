#include "WHSDControlBoradProtocol.h"

#include <thread>
#include <Windows.h>

// 心跳帧总长度：FF FE 12 <计数> 00 [10字节] <校验> FD FC = 18字节
#define HEART_BEAT_FRAME_LEN 18

uint8_t CWHSDControlBoardProtocol::cPackNumber = 0;

// ======================== CDeviceHeartBeat ========================

CDeviceHeartBeat::CDeviceHeartBeat()
{
	m_wBusVoltage = 0;
	m_cFaultCode = 0;
	m_cSpeedGear = 0;
	m_cOutputStatus = 0;
	m_cPushRod1Status = 0;
	m_cPushRod2Status = 0;
	m_cRSSI = 0;
	m_cFirmwareMajor = 0;
	m_cFirmwareMinor = 0;
}

// ======================== CWHSDControlBoardProtocol 构造与工作控制 ========================

CWHSDControlBoardProtocol::CWHSDControlBoardProtocol(uint16_t wHeartBeatTime)
{
	m_cPackNumber = 0;
	m_function_Send = nullptr;
	m_function_DeviceHeartBeat = nullptr;
	m_cCmd = 0;
	m_nIsNeedExit = false;
	m_function_WriteLogCallBack = nullptr;
	m_wHeartBeatTime = wHeartBeatTime;
	m_bPauseHeartBeat = false;
	m_function_FaultNotification = nullptr;
	m_function_AnswerCallBack = nullptr;
}

bool CWHSDControlBoardProtocol::BeginWork()
{
	m_nIsNeedExit = false;
	//std::thread td(&CWHSDControlBoardProtocol::LoopSendHeartBeat, this);
	//td.detach();
	return true;
}

bool CWHSDControlBoardProtocol::EndWork()
{
	m_nIsNeedExit = true;
	return true;
}

// ======================== 数据接收与解析 ========================

void CWHSDControlBoardProtocol::ReceiveNewData(const uint8_t* p, int len)
{
	auto oldSize = m_vectorDataBuffer.size();
	m_vectorDataBuffer.resize(oldSize + len);
	memcpy(m_vectorDataBuffer.data() + oldSize, p, len);
	while (true)
	{
		if (!Parse())
		{
			break;
		}
	}
}

bool CWHSDControlBoardProtocol::Parse()
{
	while (m_vectorDataBuffer.size() > 4)
	{
		// 查找帧头 FF FE
		if (m_vectorDataBuffer[0] == 0xff && m_vectorDataBuffer[1] == 0xfe)
		{
			auto packLen = m_vectorDataBuffer[2];
			if (packLen < 9) // 最小帧长度：FF FE len cnt cmd checksum FD FC = 8字节，有效数据至少1字节
			{
				Erase(1);
				continue;
			}
			if (m_vectorDataBuffer.size() < packLen)
			{
				return false; // 数据不够，等待更多数据
			}
			// 检查帧尾 FD FC
			if (m_vectorDataBuffer[packLen - 2] == 0xfd && m_vectorDataBuffer[packLen - 1] == 0xfc)
			{
				// 校验和验证
				auto checkSum = m_vectorDataBuffer[packLen - 3];
				uint8_t checkSum2 = 0;
				for (int i = 0; i < packLen - 3; i++)
				{
					checkSum2 = checkSum2 + m_vectorDataBuffer[i];
				}
				if (checkSum == checkSum2)
				{
					m_cPackNumber = m_vectorDataBuffer[3];
					auto cmd = m_vectorDataBuffer[4];
					// 提取指令内容（去掉帧头4字节 + 帧尾3字节）
					int cmdDataLen = packLen - 7;
					m_vectorCmdData.resize(cmdDataLen);
					if (cmdDataLen > 0)
					{
						memcpy(m_vectorCmdData.data(), m_vectorDataBuffer.data() + 5, cmdDataLen);
					}
					Erase(packLen);

					// 根据指令号分发处理
					switch (cmd)
					{
					case 0x00:
					{
						// 心跳帧（cmd=0x00，帧内容首字节为0x00）
						DealHeartBeat();
						break;
					}
					case 0x06:
					{
						// 故障反馈帧
						DealFaultNotification();
						break;
					}
					case 0x21:
					{
						// 控制指令应答帧
						DealAnswer();
						break;
					}
					case 0x07:
					{
						// 日志数据
						if (m_function_WriteLogCallBack != nullptr)
						{
							m_function_WriteLogCallBack(std::string(
								m_vectorCmdData.data(), m_vectorCmdData.data() + m_vectorCmdData.size()));
						}
						break;
					}
					default:
					{
						break;
					}
					}
					return true;
				}
				Erase(1);
			}
			else
			{
				Erase(1);
			}
		}
		else
		{
			Erase(1);
		}
	}
	return false;
}

// ======================== 回调注册 ========================

void CWHSDControlBoardProtocol::RegisterSendFunction(const std::function<bool(uint8_t*, int)>& f)
{
	m_function_Send = f;
}

void CWHSDControlBoardProtocol::RegisterDeviceHeartBeat(const std::function<void(const CDeviceHeartBeat&)>& f)
{
	m_function_DeviceHeartBeat = f;
}

void CWHSDControlBoardProtocol::RegisterDeviceLog(const std::function<void(const std::string&)>& f)
{
	m_function_WriteLogCallBack = f;
}

void CWHSDControlBoardProtocol::RegisterFaultNotification(const std::function<void(uint8_t, bool)>& f)
{
	m_function_FaultNotification = f;
}

void CWHSDControlBoardProtocol::RegisterAnswerCallback(const std::function<void(uint8_t, uint8_t)>& f)
{
	m_function_AnswerCallBack = f;
}

// ======================== 一、推杆电机指令（指令号 0x05，电机组=01）========================

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRodControl(PushRodSelect rod, PushRodAction action,
                                                                SpeedLevel speed)
{
	// 帧内容：电机组(01) | 推杆选择 | 动作 | 速度档位
	return GetCmdData(0x05, { 0x01, (uint8_t)rod, (uint8_t)action, (uint8_t)speed });
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod1Extend(SpeedLevel speed)
{
	return PushRodControl(PushRodSelect::Rod1, PushRodAction::Extend, speed);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod1Retract(SpeedLevel speed)
{
	return PushRodControl(PushRodSelect::Rod1, PushRodAction::Retract, speed);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod2Extend(SpeedLevel speed)
{
	return PushRodControl(PushRodSelect::Rod2, PushRodAction::Extend, speed);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod2Retract(SpeedLevel speed)
{
	return PushRodControl(PushRodSelect::Rod2, PushRodAction::Retract, speed);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRodBothExtend(SpeedLevel speed)
{
	return PushRodControl(PushRodSelect::Both, PushRodAction::Extend, speed);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRodBothRetract(SpeedLevel speed)
{
	return PushRodControl(PushRodSelect::Both, PushRodAction::Retract, speed);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod1Stop()
{
	return PushRodControl(PushRodSelect::Rod1, PushRodAction::Stop, SpeedLevel::High);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod1Brake()
{
	return PushRodControl(PushRodSelect::Rod1, PushRodAction::Brake, SpeedLevel::High);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod2Stop()
{
	return PushRodControl(PushRodSelect::Rod2, PushRodAction::Stop, SpeedLevel::High);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRod2Brake()
{
	return PushRodControl(PushRodSelect::Rod2, PushRodAction::Brake, SpeedLevel::High);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRodBothStop()
{
	return PushRodControl(PushRodSelect::Both, PushRodAction::Stop, SpeedLevel::High);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::PushRodBothBrake()
{
	return PushRodControl(PushRodSelect::Both, PushRodAction::Brake, SpeedLevel::High);
}

// ======================== 二、无刷电机指令（指令号 0x05，电机组=02）========================

std::vector<uint8_t> CWHSDControlBoardProtocol::BrushlessControl(BrushlessSelect sel, bool on)
{
	// 帧内容：电机组(02) | 电机选择 | 开(01)/关(00) | 00
	return GetCmdData(0x05, { 0x02, (uint8_t)sel, (uint8_t)(on ? 0x01 : 0x00), 0x00 });
}

std::vector<uint8_t> CWHSDControlBoardProtocol::LeftBrushlessOn()
{
	return BrushlessControl(BrushlessSelect::Left, true);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::LeftBrushlessOff()
{
	return BrushlessControl(BrushlessSelect::Left, false);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::RightBrushlessOn()
{
	return BrushlessControl(BrushlessSelect::Right, true);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::RightBrushlessOff()
{
	return BrushlessControl(BrushlessSelect::Right, false);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::BothBrushlessOn()
{
	return BrushlessControl(BrushlessSelect::Both, true);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::BothBrushlessOff()
{
	return BrushlessControl(BrushlessSelect::Both, false);
}

// ======================== 三、开关量指令（指令号 0x08）========================

std::vector<uint8_t> CWHSDControlBoardProtocol::SetSwitch(SwitchIndex sw, bool on)
{
	// 帧内容：开关索引 | 开(01)/关(00)
	return GetCmdData(0x08, { (uint8_t)sw, (uint8_t)(on ? 0x01 : 0x00) });
}

std::vector<uint8_t> CWHSDControlBoardProtocol::LowSpeedGearOn()
{
	return SetSwitch(SwitchIndex::LowSpeed, true);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::LowSpeedGearOff()
{
	return SetSwitch(SwitchIndex::LowSpeed, false);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::HighSpeedGearOn()
{
	return SetSwitch(SwitchIndex::HighSpeed, true);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::HighSpeedGearOff()
{
	return SetSwitch(SwitchIndex::HighSpeed, false);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::DirOn()
{
	return SetSwitch(SwitchIndex::Dir, true);
}

std::vector<uint8_t> CWHSDControlBoardProtocol::DirOff()
{
	return SetSwitch(SwitchIndex::Dir, false);
}

// ======================== 四、系统指令 ========================

std::vector<uint8_t> CWHSDControlBoardProtocol::EmergencyStop()
{
	// 指令号 0x20，不受故障锁限制，全部输出切断
	return GetCmdData(0x20, { 0x00 });
}

std::vector<uint8_t> CWHSDControlBoardProtocol::QueryRSSI()
{
	// 指令号 0x21，回0x21应答帧
	return GetCmdData(0x21, { 0x00 });
}

std::vector<uint8_t> CWHSDControlBoardProtocol::SetParameter(uint8_t underVoltageX10, uint8_t heartbeatIntervalX100)
{
	// 指令号 0x22
	// Byte0=欠压阈值×10（范围100~250，即10.0~25.0V）
	// Byte1=心跳周期×100ms（范围0~20，0=关闭心跳）
	if (underVoltageX10 < 100) underVoltageX10 = 100;
	if (underVoltageX10 > 250) underVoltageX10 = 250;
	if (heartbeatIntervalX100 > 20) heartbeatIntervalX100 = 20;
	return GetCmdData(0x22, { underVoltageX10, heartbeatIntervalX100 });
}

// ======================== 帧构建与发送 ========================

void CWHSDControlBoardProtocol::Erase(int en)
{
	if (en < m_vectorDataBuffer.size())
	{
		m_vectorDataBuffer.erase(m_vectorDataBuffer.begin(), m_vectorDataBuffer.begin() + en);
	}
	else
	{
		m_vectorDataBuffer.clear();
	}
}

std::vector<uint8_t> CWHSDControlBoardProtocol::GetCmdData(uint8_t cmd, const std::vector<uint8_t>& cmdData)
{
	// 包计数逐包+1，不为0（协议要求）
	if (++cPackNumber == 0)
	{
		cPackNumber = 1;
	}
	uint8_t packLen = cmdData.size() + 8;
	std::vector<uint8_t> result(packLen);
	result[0] = 0xff;
	result[1] = 0xfe;
	result[2] = packLen;
	result[3] = cPackNumber;
	result[4] = cmd;
	memcpy(result.data() + 5, cmdData.data(), cmdData.size());
	uint8_t checkSum2 = 0;
	for (int i = 0; i < packLen - 3; i++)
	{
		checkSum2 = checkSum2 + result[i];
	}
	result[packLen - 3] = checkSum2;
	result[packLen - 2] = 0xfd;
	result[packLen - 1] = 0xfc;
	return result;
}

void CWHSDControlBoardProtocol::SendData(uint8_t* p, int len)
{
	if (m_function_Send != nullptr)
	{
		m_function_Send(p, len);
	}
}

void CWHSDControlBoardProtocol::LoopSendHeartBeat()
{
	while (!m_nIsNeedExit)
	{
		Sleep(m_wHeartBeatTime);
		if (!m_bPauseHeartBeat)
		{
			// 心跳查询帧：FF FE 09 <计数> 00 <校验> FD FC
			auto needSendData = GetCmdData(0x00, { m_cPackNumber });
			SendData(needSendData.data(), needSendData.size());
		}
	}
}

// ======================== 接收帧解析处理 ========================

void CWHSDControlBoardProtocol::DealHeartBeat()
{
	// 心跳帧内容（10字节）：
	// Byte0~1 = 母线电压×100，小端
	// Byte2   = 故障码（bit0=左无刷，bit1=右无刷）
	// Byte3   = 速度档位（0=无 1=低速 2=高速）
	// Byte4   = 输出状态位（bit0=DIR bit1=左无刷 bit2=右无刷 bit3=低速档 bit4=高速档）
	// Byte5   = 推杆1状态（0=停止 1=伸出中 2=收回中）
	// Byte6   = 推杆2状态（同上）
	// Byte7   = RSSI绝对值（当前固定0）
	// Byte8   = 固件主版本号
	// Byte9   = 固件次版本号
	if (m_vectorCmdData.size() >= 10 && m_function_DeviceHeartBeat != nullptr)
	{
		m_memDeviceHeartBeat.m_wBusVoltage = m_vectorCmdData[0] | (m_vectorCmdData[1] << 8);
		m_memDeviceHeartBeat.m_cFaultCode = m_vectorCmdData[2];
		m_memDeviceHeartBeat.m_cSpeedGear = m_vectorCmdData[3];
		m_memDeviceHeartBeat.m_cOutputStatus = m_vectorCmdData[4];
		m_memDeviceHeartBeat.m_cPushRod1Status = m_vectorCmdData[5];
		m_memDeviceHeartBeat.m_cPushRod2Status = m_vectorCmdData[6];
		m_memDeviceHeartBeat.m_cRSSI = m_vectorCmdData[7];
		m_memDeviceHeartBeat.m_cFirmwareMajor = m_vectorCmdData[8];
		m_memDeviceHeartBeat.m_cFirmwareMinor = m_vectorCmdData[9];
		m_function_DeviceHeartBeat(m_memDeviceHeartBeat);
	}
}

void CWHSDControlBoardProtocol::DealAnswer()
{
	// 应答帧内容：[指令号] [结果码]
	// 结果码：0x00=执行失败 0x01=执行成功
	//   0x02=0x05时表示故障锁定拒绝；0x08时表示档位切换中请稍候
	//   0x03=0x08时表示故障锁定拒绝
	if (m_vectorCmdData.size() >= 2 && m_function_AnswerCallBack != nullptr)
	{
		m_function_AnswerCallBack(m_vectorCmdData[0], m_vectorCmdData[1]);
	}
}

void CWHSDControlBoardProtocol::DealFaultNotification()
{
	// 故障反馈帧内容：[故障码] [01=发生/00=解除]
	// 故障发生时自动切断全部输出并锁定，运动类指令将被拒绝；
	// 故障解除后自动解锁，恢复正常控制。
	if (m_vectorCmdData.size() >= 2 && m_function_FaultNotification != nullptr)
	{
		uint8_t faultCode = m_vectorCmdData[0];
		bool isOccurring = m_vectorCmdData[1] == 0x01;
		m_function_FaultNotification(faultCode, isOccurring);
	}
}
