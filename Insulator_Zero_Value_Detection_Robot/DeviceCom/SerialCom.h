#pragma once
#include <atomic>
#include <thread>

#include "IDeviceCom.h"
#include <QString>

class SerialWorker;
class SerialDataQueue;

/// <summary>
/// 串口通讯实现（替代 TCP），接口与 CTcpClientCom 完全一致
/// SetParam: pComName = 串口名（如 "COM3"），nComPort = 波特率（如 115200）
/// 数据帧固定 8 数据位、无校验、1 停止位、无流控
/// </summary>
class CSerialCom : public IDeviceCom
{
public:
	CSerialCom();
	~CSerialCom();
	void SetParam(const char* pComName, int nBaudRate) override;
	void RegisterReadDataCallBack(const std::function<void(uint8_t*, int, uint64_t)>& f) override;
	void RegisterConnectStatusCallBack(const std::function<void(bool, int)>& f)override;
	bool Write(uint8_t* data, size_t  len) override;
	bool BeginWork() override;
	bool EndWork() override;
private:
	// 监控线程：取接收数据回调上层、监控连接状态、串口断开后自动重连
	void MonitorThread();

	void SetConnected(bool b);

	QString m_strPortName;

	int m_nBaudRate;

	SerialDataQueue* m_pSendQueue;

	SerialDataQueue* m_pReceiveQueue;

	SerialWorker* m_pWorker;

	std::thread m_monitorThread;

	std::atomic<bool> m_running;

	std::atomic<bool> m_connected;

	std::function<void(bool, int)> m_connectCallback;

	std::function<void(uint8_t*, int, uint64_t)> m_function_ReadDataCallBack;
};
