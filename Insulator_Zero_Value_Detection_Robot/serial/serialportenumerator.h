// serialportenumerator.h
#ifndef SERIALPORTENUMERATOR_H
#define SERIALPORTENUMERATOR_H

#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include "qextserialport.h"

/**
 * 串口枚举工具类
 * 提供跨平台的串口枚举功能，可被任何需要串口发现的项目复用
 */
class SerialPortEnumerator
{
public:
    /**
     * 枚举系统中所有可用的串口
     * @return 可用串口名称列表
     */
    static QStringList enumerate()
    {
        QStringList portList;

#ifdef Q_OS_WIN
        // Windows下通过试探法获取串口列表
        for (int i = 1; i <= 10; i++) {
            QString portName = QString("COM%1").arg(i);
            QextSerialPort port(portName, QextSerialPort::Polling);
            // 尝试打开端口来检测是否存在
            if (port.open(QIODevice::ReadWrite)) {
                portList.append(portName);
                port.close();
                port.deleteLater();
            }
        }
#else
        // Linux/macOS下扫描设备文件
        QDir dir("/dev");
        QStringList nameFilters;
        nameFilters << "ttyS*" << "ttyUSB*" << "ttyACM*" << "cu.*" << "tty.*";

        QFileInfoList fileInfos = dir.entryInfoList(nameFilters, QDir::Files | QDir::System);
        foreach(const QFileInfo & fileInfo, fileInfos) {
            QString fileName = fileInfo.fileName();
            if (!fileName.startsWith("ttyprintk")) { // 排除内核打印端口
                portList.append(fileInfo.absoluteFilePath());
            }
        }
#endif

        return portList;
    }
};

#endif // SERIALPORTENUMERATOR_H
