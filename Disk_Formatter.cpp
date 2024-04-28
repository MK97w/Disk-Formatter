
#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <fileapi.h>
#include <devguid.h>
#include <SetupAPI.h>
#include <stdint.h>
#include <cfgmgr32.h>
#include <array>
#include <io.h> 
#include <fcntl.h> 
#include <string>
#include <windowsx.h>
#include <unordered_map>
#include <sstream>
#include <winioctl.h>
#include <Vds.h>
#include "fat32_format.h"
#include "format.h"
#include "drive.h"
//#pragma comment(lib, "vds.lib") 

std::wstring GetVolumeGuid(const std::wstring& mountPoint)
{
    DWORD bufferLen = MAX_PATH;
    wchar_t volumeName[MAX_PATH] = { 0 };

    if (GetVolumeNameForVolumeMountPoint(mountPoint.c_str(), volumeName, bufferLen))
    {
        return std::wstring(volumeName);
    }
    return std::wstring();
}





void listAllVolumeInfo()
{
    _TCHAR buffer[MAX_PATH];
    DWORD drives = GetLogicalDriveStrings(MAX_PATH, buffer), data_type, size;
    HANDLE hDrive;
    int foundDevices{ 0 };

    if (drives == 0)
    {
        std::cerr << "Error getting logical drives. Error code: " << GetLastError() << std::endl;
    }

    // Iterate through each drive
    for (DWORD i = 0; i < drives; i += 4)
    {
        _TCHAR drivePath[4] = { buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3] };

        UINT driveType = GetDriveType(drivePath);

        if (driveType == DRIVE_REMOVABLE && driveType != DRIVE_FIXED)
        {
            // Get volume information
            _TCHAR volumeName[MAX_PATH];
            _TCHAR fileSystem[MAX_PATH];
            DWORD serialNumber;
            DWORD maxComponentLength;
            DWORD fileSystemFlags;


            if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
            {
                foundDevices++;
                std::string path = "\\\\.\\";
                path.append(1, static_cast<char>(drivePath[0]));
                path += ":";
                std::wstring temp = std::wstring(path.begin(), path.end());
                LPCWSTR wideString = temp.c_str();
                hDrive = CreateFile(wideString, 0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hDrive == INVALID_HANDLE_VALUE)
                {
                    const DWORD error = GetLastError();
                    std::cout << error << '\n';
                    break;
                }
                //uint64_t drive_size = GetDriveSize(hDrive);

                BYTE geometry[128];
                if (!DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                    NULL, 0, geometry, sizeof(geometry), &size, NULL) && (size > 0))
                {
                    CloseHandle(hDrive);
                }
                else
                {
                    CloseHandle(hDrive);
                    //devices[foundDevices] = wideString;
                    std::cout << "==========================================================\n";
                    std::wcout << "Drive Path: " << drivePath << '\n';
                    //std::wcout << "GUID: " << GetVolumeGuid(drivePath) << '\n';
                    std::wcout << "Drive Name: " << volumeName << '\n';
                    std::wcout << "File System: " << fileSystem << '\n';
                   // std::cout << "Drive Size: " << drive_size<< '\n';
                    std::cout << "==========================================================\n";

                }

            }

        }

    }
}

int main()
{
   Drive::getAllDriveInfo();
   Drive::printDriveMap();
   auto m = Drive::get_driveMap();
   std::cout << m[0].get_logicalSize();

   
    /*
   VolumeFormatter formatter;
   //istAllVolumeInfo();
   formatter.FMIFS_Format(L"D:\\",L"NTFS",L"mert32",8192);
   listAllVolumeInfo();
   formatter.Large_FAT32_Format(R"(\\.\D:)");
   SetVolumeLabelA(R"(\\.\D:\\)", "bidid");
   listAllVolumeInfo();
  
*/
    return 0;
}
