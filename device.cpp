#include "device.h"
#include <devguid.h>
#pragma comment(lib, "Setupapi.lib")


const GUID GUID_DEVINTERFACE_USB_HUB =
{ 0xf18a0e88L, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8} };

BOOL DeviceManager::getDeviceInfo()
{
	DWORD size;
	deviceInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_USB_HUB, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	deviceInfoData.cbSize = sizeof(deviceInfoData);

	for (auto i = 0; SetupDiEnumDeviceInfo(deviceInfo, i, &deviceInfoData); i++) 
	{
		deviceInterfaceDetailData = NULL;
		deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
		// Only care about the first interface (MemberIndex 0)
		if ((SetupDiEnumDeviceInterfaces(deviceInfo, &deviceInfoData, &GUID_DEVINTERFACE_USB_HUB, 0, &deviceInterfaceData))
			&& (!SetupDiGetDeviceInterfaceDetailA(deviceInfo, &deviceInterfaceData, NULL, 0, &size, NULL))
			&& (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			&& ((deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)calloc(1, size)) != NULL))
		{
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
			if (SetupDiGetDeviceInterfaceDetailA(deviceInfo, &deviceInterfaceData, deviceInterfaceDetailData, size, &size, NULL))
			{

	
			}
			//free(devint_detail_data);
		}
	}

	
	return TRUE;
}
}