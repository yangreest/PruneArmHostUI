# serial.pri - 独立串口通信模块
# 仅依赖 QtCore 和 qextserialport 第三方库，不依赖任何 GUI 组件
# 其他设备项目可直接 include 本文件来构建串口模块

HEADERS += \
    $$PWD/serialdataqueue.h \
    $$PWD/serialworker.h \
    $$PWD/serialportenumerator.h

SOURCES += \
    $$PWD/serialdataqueue.cpp \
    $$PWD/serialworker.cpp
