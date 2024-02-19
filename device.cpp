#include "device.h"
#include <iostream>
#include <devguid.h>

#pragma comment(lib, "Setupapi.lib")


const GUID GUID_DEVINTERFACE_USB_HUB =
{ 0xf18a0e88L, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8} };


int DeviceManager::getDriveNumber(HANDLE hDrive, char* path)
{
	return 32;
}









BOOL DeviceManager::getDeviceInfo()
{
	DWORD size;
	int drive_number = 0;
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
				hDrive = CreateFileA(deviceInterfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hDrive == INVALID_HANDLE_VALUE) 
				{
					//invald handle
					continue;

				}
				DevTypeNumPartition deviceNumber;
				VolumeDiskExtents diskExtent;
				DWORD size = 0;
				BOOL s;
				int r = -1;
				/*DWORD size;
				deviceInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_USB_HUB, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
				deviceInfoData.cbSize = sizeof(deviceInfoData);
				*/
				if (!DeviceIoControl(hDrive, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
					&diskExtent, sizeof(diskExtent), &size, NULL) || (size <= 0) || (diskExtent.NumberOfDiskExtents < 1)) {
					// DiskExtents are NO_GO (which is the case for external USB HDDs...)
					s = DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &deviceNumber, sizeof(deviceNumber),
						&size, NULL);
					if ((!s) || (size == 0)) {
						
					}
					r = (int)deviceNumber.DeviceNumber;
				}
				else if (diskExtent.NumberOfDiskExtents >= 2) {
				}
				else {
					r = (int)diskExtent.Extents[0].DiskNumber;
				}
				if (r >= MAX_DRIVES) {;
				}
				std::cout << r;
				
	
			}
			//free(devint_detail_data);
		}
	}
	return TRUE;
}