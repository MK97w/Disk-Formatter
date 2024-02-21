
#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <fileapi.h>
#include <devguid.h>
#include <SetupAPI.h>
#include <stdint.h>
#include <cfgmgr32.h>

#pragma comment(lib, "Setupapi.lib")


const GUID GUID_DEVINTERFACE_USB_HUB =
{ 0xf18a0e88L, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8} };

typedef struct {
    DWORD DeviceType;
    ULONG DeviceNumber;
    ULONG PartitionNumber;
} STORAGE_DEVICE_NUMBER_REDEF;

typedef struct {
    DWORD NumberOfDiskExtents;
    // The one from MS uses ANYSIZE_ARRAY, which can lead to all kind of problems
    DISK_EXTENT Extents[8];
} VOLUME_DISK_EXTENTS_REDEF;


BOOL getDeviceInterfaceInfo(HDEVINFO& hdevInfo, SP_DEVINFO_DATA& spDevInfoData, 
    SP_DEVICE_INTERFACE_DATA& spDeviceInterfaceData, PSP_DEVICE_INTERFACE_DETAIL_DATA_A& pspDeviceInterfaceDetailData, DWORD memIndex)
{
    BOOL res{ false };
    DWORD size;
    if( SetupDiEnumDeviceInterfaces( hdevInfo, &spDevInfoData, &GUID_DEVINTERFACE_DISK, memIndex/4, &spDeviceInterfaceData ) )
    {
        if ( SetupDiGetDeviceInterfaceDetailA(hdevInfo, &spDeviceInterfaceData, NULL, 0, &size, NULL))
        {
            if ( SetupDiGetDeviceInterfaceDetailA( hdevInfo, &spDeviceInterfaceData, pspDeviceInterfaceDetailData, size, &size, NULL ) )
            {
                res = TRUE;
            }
        }
    }
    return res;
}

int getDriveNumber(HANDLE hDrive)
{
    STORAGE_DEVICE_NUMBER_REDEF DeviceNumber;
    VOLUME_DISK_EXTENTS_REDEF DiskExtents;
    DWORD size = 0;
    BOOL s;
    int r = -1;

    if (!DeviceIoControl(hDrive, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, 
        &DiskExtents, sizeof(DiskExtents), &size, NULL) || (size <= 0) || (DiskExtents.NumberOfDiskExtents < 1))
    {
        // DiskExtents are NO_GO (which is the case for external USB HDDs...)
        s = DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &DeviceNumber, sizeof(DeviceNumber),&size, NULL);
        if ((!s) || (size == 0)) 
        {   
            return -1;
        }
        r = (int)DeviceNumber.DeviceNumber;
    }
    else if (DiskExtents.NumberOfDiskExtents >= 2) 
    {
        return -1;
    }
    else
    {
        r = (int)DiskExtents.Extents[0].DiskNumber;
    }
   /* if (r >= MAX_DRIVES)
    {
        uprintf("Device Number for device %s is too big (%d) - ignoring device", path, r);
        uprintf("NOTE: This may be due to an excess of Virtual Drives, such as hidden ones created by the XBox PC app");
        return -1;
    }*/
    return r;
}


void listAllVolumeInfo() 
{
    _TCHAR buffer[MAX_PATH];
    DWORD drives = GetLogicalDriveStrings(MAX_PATH, buffer);
    HDEVINFO dev_info = NULL;
    SP_DEVINFO_DATA dev_info_data;
    SP_DEVICE_INTERFACE_DATA devint_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A devint_detail_data;
    HANDLE hDrive;

    if (drives == 0)
    {
        std::cerr << "Error getting logical drives. Error code: " << GetLastError() << std::endl;
    }
    std::cout << "==========================================================\n";
    // Iterate through each drive
    for (DWORD i = 0; i < drives; i += 4)
    {

        _TCHAR drivePath[4] = { buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3] };

        // Check if the drive is a removable drive (e.g., SD card)
        UINT driveType = GetDriveType(drivePath);

        if (driveType == DRIVE_REMOVABLE)
        {

            std::wcout << "Removable drive found: " << drivePath << '\n';

            // Get volume information
            _TCHAR volumeName[MAX_PATH];
            _TCHAR fileSystem[MAX_PATH];
            DWORD serialNumber;
            DWORD maxComponentLength;
            DWORD fileSystemFlags; 

            dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
            dev_info_data.cbSize = sizeof(dev_info_data);

            if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
            {
                if (getDeviceInterfaceInfo(dev_info, dev_info_data, devint_data, devint_detail_data, i))
                {
                    hDrive = CreateFileA(devint_detail_data->DevicePath, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    std::wcout << "Volume Name: " << volumeName << '\n';
                    std::wcout << "Serial Number: " << serialNumber << '\n';
                    std::wcout << "File System: " << fileSystem << '\n';
                    std::wcout << "Device Index: " << getDriveNumber(hDrive) << '\n';
                    
                }
               
            }
            else
                std::cerr << "Error getting volume information. Error code: " << GetLastError() << std::endl;
            std::cout << "==========================================================\n";
        }

    }

}


int main()
{
    listAllVolumeInfo();
}
