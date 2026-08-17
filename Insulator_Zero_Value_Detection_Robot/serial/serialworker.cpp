// serialworker.cpp
#include "serialworker.h"
#include <QDebug>
#include <QTime>

SerialWorker::SerialWorker(SerialDataQueue *sendQueue, SerialDataQueue *receiveQueue, QObject *parent)
    : QThread(parent)
    , m_stopFlag(false)
    , m_portOpen(0)
    , m_sendQueue(sendQueue)
    , m_receiveQueue(receiveQueue)
    , m_serialPort(nullptr)
{
    // 设置默认串口参数
    m_portSettings.BaudRate = BAUD9600;
    m_portSettings.DataBits = DATA_8;
    m_portSettings.Parity = PAR_NONE;
    m_portSettings.StopBits = STOP_1;
    m_portSettings.FlowControl = FLOW_OFF;
    m_portSettings.Timeout_Millisec = 10;
}

SerialWorker::~SerialWorker()
{
    stop();
    wait();
    
    if (m_serialPort) {
        delete m_serialPort;
    }
}

void SerialWorker::setPortSettings(const QString &portName, BaudRateType baudRate,
                                  DataBitsType dataBits, ParityType parity,
                                  StopBitsType stopBits, FlowType flowControl)
{
    m_portName = portName;
    m_portSettings.BaudRate = baudRate;
    m_portSettings.DataBits = dataBits;
    m_portSettings.Parity = parity;
    m_portSettings.StopBits = stopBits;
    m_portSettings.FlowControl = flowControl;
}

void SerialWorker::stop()
{
    m_stopFlag = true;
}

void SerialWorker::run()
{
    // 支持线程重启（重连串口），复位停止标志
    m_stopFlag = false;

    m_serialPort = new QextSerialPort(m_portName, m_portSettings, QextSerialPort::Polling);
    
    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString("Failed to open port %1: %2")
                          .arg(m_portName).arg(m_serialPort->errorString()));
        delete m_serialPort;
        m_serialPort = nullptr;
        return;
    }
    
    m_portOpen = 1;
    emit portOpened();
    
    // 主循环
    while (!m_stopFlag) {
        // 处理发送数据
        processSending();
        
        // 处理接收数据
        processReceiving();
        
        // 短暂休眠避免CPU占用过高
        msleep(1);
    }
    
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_portOpen = 0;

    // 释放串口对象，便于线程重启时重新创建
    if (m_serialPort) {
        delete m_serialPort;
        m_serialPort = nullptr;
    }
    
    emit portClosed();
}

void SerialWorker::processSending()
{
    if (!m_serialPort || !m_serialPort->isOpen())
        return;
    
    while (!m_sendQueue->isSendQueueEmpty()) {
        QByteArray data = m_sendQueue->dequeueSendData();
        if (!data.isEmpty()) {
            m_serialPort->write(data);
            m_serialPort->flush();
        }
    }
}

void SerialWorker::processReceiving()
{
    if (!m_serialPort || !m_serialPort->isOpen())
        return;
    
    // 检查是否有数据可读
    if (m_serialPort->bytesAvailable() > 0) {
        QByteArray data = m_serialPort->readAll();
        if (!data.isEmpty()) {
            // 透明转发原始字节流，由上层协议层自行组帧解析
            m_receiveQueue->enqueueReceivedData(data);
        }
    }
}
