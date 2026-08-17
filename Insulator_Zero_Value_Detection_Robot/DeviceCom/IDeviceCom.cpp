#include "IDeviceCom.h"
#include "TcpClient.h"
#include "SerialCom.h"

IDeviceCom* IDeviceCom::GetIDeviceCom(int nComType)
{
	switch (nComType)
	{
	case 1:
		return new CTcpClientCom();
	case 2:
		// 串口通讯（pComName为串口名，nComPort为波特率）
		return new CSerialCom();
	default:
		return nullptr;
	}
}