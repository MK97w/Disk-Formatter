
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
//#pragma comment(lib, "vds.lib") 



#define safe_sprintf(dst, count, ...) do {_snprintf(dst, count, __VA_ARGS__); (dst)[(count)-1] = 0; } while(0)
#define static_sprintf(dst, ...) safe_sprintf(dst, sizeof(dst), __VA_ARGS__)

#define STR_NO_LABEL                "NO_LABEL"
#define MSG_020                         3020
#define MSG_000                         3000
#define MSG_MAX                         3351

std::unordered_map<int, LPCWSTR>devices;

template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}

static __inline uint16_t upo2(uint16_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v++;
    return v;
}



uint64_t GetDriveSize(HANDLE& hDrive)
{
    BOOL r;
    DWORD size;
    BYTE geometry[256];
    PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;

    r = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
        NULL, 0, geometry, sizeof(geometry), &size, NULL);

    if (!r || size <= 0)
        return 0;
    return DiskGeometry->DiskSize.QuadPart;
}


std::string SizeToHumanReadable(uint64_t size, BOOL copy_to_log, BOOL fake_units)
{

    const char* default_msg_table[MSG_MAX - MSG_000] = { "%s", 0 };
    int suffix;
    static char str_size[32];
    std::string res{ };
    const char* dir = "";
    double hr_size = (double)size;
    double t;
    uint16_t i_size;
    const char** _msg_table = default_msg_table;
    const double divider = fake_units ? 1000.0 : 1024.0;

    for (suffix = 0; suffix < 6 - 1; suffix++) {
        if (hr_size < divider)
            break;
        hr_size /= divider;
    }
    if (suffix == 0) {
        static_sprintf(str_size, "%s%d%s %s", dir, (int)hr_size, dir, _msg_table[MSG_020 - MSG_000]);
    }
    else if (fake_units)
    {
        if (hr_size < 8) // 1-8 terrabyte a kadar buraya girecek 
        {
            //static_sprintf(str_size, (fabs((hr_size * 10.0) - (floor(hr_size + 0.5) * 10.0)) < 0.5) ? "%0.0f%s" : "%0.1f%s",
              //  hr_size, _msg_table[MSG_020 + suffix - MSG_000]);

            t = (double)upo2((uint16_t)hr_size);
            i_size = (uint16_t)((fabs(1.0f - (hr_size / t)) < 0.05f) ? t : hr_size);
            res = std::to_string(static_cast<int>(i_size));
            if (suffix == 3)
                res += " GB";
            else if (suffix == 4)
                res += " TB";
            else if (suffix == 2)
                res += " MB";

        }
        else {
            t = (double)upo2((uint16_t)hr_size);
            i_size = (uint16_t)((fabs(1.0f - (hr_size / t)) < 0.05f) ? t : hr_size);
            res = std::to_string(static_cast<int>(i_size));
            if (suffix == 3)
                res += " GB";
            else if (suffix == 4)
                res += " TB";
            else if (suffix == 2)
                res += " MB";
        }
    }
    else {
        static_sprintf(str_size, (hr_size * 10.0 - (floor(hr_size) * 10.0)) < 0.5 ?
            "%s%0.0f%s %s" : "%s%0.1f%s %s", dir, hr_size, dir, _msg_table[MSG_020 + suffix - MSG_000]);
    }
    return res;
}

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
                uint64_t drive_size = GetDriveSize(hDrive);

                BYTE geometry[128];
                if (!DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                    NULL, 0, geometry, sizeof(geometry), &size, NULL) && (size > 0))
                {
                    CloseHandle(hDrive);
                }
                else
                {
                    CloseHandle(hDrive);
                    devices[foundDevices] = wideString;
                    std::cout << "==========================================================\n";
                    std::wcout << "Drive Path: " << drivePath << '\n';
                    std::wcout << "GUID: " << GetVolumeGuid(drivePath) << '\n';
                    std::wcout << "Drive Name: " << volumeName << '\n';
                    std::wcout << "File System: " << fileSystem << '\n';
                    std::cout << "Drive Size: " << SizeToHumanReadable(drive_size, 0, 1) << '\n';
                    std::cout << "==========================================================\n";

                }

            }

        }

    }
}

int main()
{
   VolumeFormatter formatter;
   listAllVolumeInfo();
   formatter.FMIFS_Format(L"D:\\",L"NTFS",L"mert32",8192);
   //formatLarge_FAT32(R"(\\.\D:)");
   //SetVolumeLabelA(R"(\\.\D:\\)", "mertmert");
   listAllVolumeInfo();

}

