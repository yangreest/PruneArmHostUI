// serialdataqueue.cpp
#include "serialdataqueue.h"
#include <QDateTime>

SerialDataQueue::SerialDataQueue()
{
}

void SerialDataQueue::enqueueSendData(const QByteArray &data)
{
    QMutexLocker locker(&mutex);
    sendQueue.enqueue(data);
}

QByteArray SerialDataQueue::dequeueSendData()
{
    QMutexLocker locker(&mutex);
    if (sendQueue.isEmpty())
        return QByteArray();
    QByteArray data = sendQueue.dequeue();

    return data;
}

void SerialDataQueue::enqueueReceivedData(const QByteArray &data)
{
    QMutexLocker locker(&mutex);
    
    // 将修改后的数据加入队列
    receiveQueue.enqueue(data);
    dataAvailable.wakeAll(); // 唤醒等待的线程
}

QByteArray SerialDataQueue::dequeueReceivedData()
{
    QMutexLocker locker(&mutex);
    if (receiveQueue.isEmpty())
        return QByteArray();
    return receiveQueue.dequeue();
}

bool SerialDataQueue::isSendQueueEmpty() const
{
    QMutexLocker locker(&mutex);
    return sendQueue.isEmpty();
}

bool SerialDataQueue::isReceiveQueueEmpty() const
{
    QMutexLocker locker(&mutex);
    return receiveQueue.isEmpty();
}

bool SerialDataQueue::waitForReceivedData(int timeoutMs)
{
    QMutexLocker locker(&mutex);
    if (!receiveQueue.isEmpty())
        return true;
    
    return dataAvailable.wait(&mutex, timeoutMs);
}
