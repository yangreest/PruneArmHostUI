// serialdataqueue.h
#ifndef SERIALDATAQUEUE_H
#define SERIALDATAQUEUE_H

#include <QMutex>
#include <QQueue>
#include <QByteArray>
#include <QWaitCondition>

class SerialDataQueue
{
public:
    SerialDataQueue();
    
    // 发送数据到队列
    void enqueueSendData(const QByteArray &data);
    
    // 从队列获取待发送数据
    QByteArray dequeueSendData();
    
    // 添加接收到的数据到队列
    void enqueueReceivedData(const QByteArray &data);
    
    // 从接收队列获取数据
    QByteArray dequeueReceivedData();
    
    // 检查队列是否为空
    bool isSendQueueEmpty() const;
    bool isReceiveQueueEmpty() const;
    
    // 等待数据到达
    bool waitForReceivedData(int timeoutMs = 30000);

private:
    mutable QMutex mutex;
    QQueue<QByteArray> sendQueue;
    QQueue<QByteArray> receiveQueue;
    QWaitCondition dataAvailable;
    
};

#endif // SERIALDATAQUEUE_H
