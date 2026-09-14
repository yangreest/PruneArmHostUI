#include "SerialCom.h"

#include "../serial/serialdataqueue.h"
#include "../serial/serialworker.h"

#include <chrono>

CSerialCom::CSerialCom()
	: m_nBaudRate(115200)
	, m_pSendQueue(nullptr)
	, m_pReceiveQueue(nullptr)
	, m_pWorker(nullptr)
	, m_running(false)
	, m_connected(false)
{
	m_function_ReadDataCallBack = nullptr;
	m_connectCallback = nullptr;
}

CSerialCom::~CSerialCom()
{
	EndWork();
}

void CSerialCom::SetParam(const char* pComName, int nBaudRate)
{
	m_strPortName = QString::fromLocal8Bit(pComName);
	//m_nBaudRate = nBaudRate;
}

void CSerialCom::RegisterReadDataCallBack(const std::function<void(uint8_t*, int, uint64_t)>& f)
{
	m_function_ReadDataCallBack = f;
}

void CSerialCom::RegisterConnectStatusCallBack(const std::function<void(bool, int)>& f)
{
	m_connectCallback = f;
}

bool CSerialCom::Write(uint8_t* data, size_t  len)
{
	if (!m_running || !m_connected || m_pSendQueue == nullptr)
	{
		return false;
	}

	m_pSendQueue->enqueueSendData(QByteArray(reinterpret_cast<const char*>(data), (int)len));

	return true;
}

bool CSerialCom::BeginWork()
{
	if (m_running)
	{
		return true;
	}

	if (m_strPortName.isEmpty())
	{
		return false;
	}

	m_pSendQueue = new SerialDataQueue();
	m_pReceiveQueue = new SerialDataQueue();
	m_pWorker = new SerialWorker(m_pSendQueue, m_pReceiveQueue);
	m_pWorker->setPortSettings(m_strPortName, static_cast<BaudRateType>(m_nBaudRate),
		DATA_8, PAR_NONE, STOP_1, FLOW_OFF);

	m_running = true;
	m_connected = false;

	// 启动串口收发线程
	m_pWorker->start();

	// 启动监控线程（接收回调、连接状态、自动重连）
	m_monitorThread = std::thread(&CSerialCom::MonitorThread, this);

	return true;
}

bool CSerialCom::EndWork()
{
	if (!m_running)
	{
		return true;
	}

	m_running = false;

	if (m_monitorThread.joinable())
	{
		m_monitorThread.join();
	}

	if (m_pWorker != nullptr)
	{
		m_pWorker->stop();
		m_pWorker->wait(3000);
		delete m_pWorker;
		m_pWorker = nullptr;
	}

	if (m_pSendQueue != nullptr)
	{
		delete m_pSendQueue;
		m_pSendQueue = nullptr;
	}

	if (m_pReceiveQueue != nullptr)
	{
		delete m_pReceiveQueue;
		m_pReceiveQueue = nullptr;
	}

	SetConnected(false);

	return true;
}

void CSerialCom::MonitorThread()
{
	int nRetryWaitCount = 0;

	while (m_running)
	{
		// 串口线程结束（打开失败或断开）则延时重启，实现自动重连
		if (!m_pWorker->isRunning())
		{
			if (nRetryWaitCount++ >= 200) // 约2秒重试一次
			{
				nRetryWaitCount = 0;
				m_pWorker->start();
			}
		}
		else
		{
			nRetryWaitCount = 0;
		}

		// 更新连接状态
		SetConnected(m_pWorker->isPortOpen());

		// 取出接收数据回调上层协议
		while (m_running && !m_pReceiveQueue->isReceiveQueueEmpty())
		{
			QByteArray data = m_pReceiveQueue->dequeueReceivedData();
			if (!data.isEmpty() && m_function_ReadDataCallBack != nullptr)
			{
				m_function_ReadDataCallBack(reinterpret_cast<uint8_t*>(data.data()), data.size(), 0);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

// 设置连接状态
void CSerialCom::SetConnected(bool connected)
{
	if (m_connected != connected)
	{
		m_connected = connected;

		// 调用连接状态回调
		if (m_connectCallback != nullptr)
		{
			m_connectCallback(connected, 0);
		}
	}
}
