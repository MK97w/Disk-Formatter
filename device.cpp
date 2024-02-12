#include "device.h"
#include <devguid.h>
#pragma comment(lib, "Setupapi.lib")


const GUID GUID_DEVINTERFACE_USB_HUB =
{ 0xf18a0e88L, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8} };

BOOL DeviceManager::getDeviceInfo()
{
	deviceInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_USB_HUB, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	return TRUE;
}