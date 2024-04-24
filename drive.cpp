#include "drive.h"
#include <iostream>

#define STATIC

namespace helperFunction
{
    template <typename T>
    std::string _toString(T val)
    {
        std::stringstream stream;
        stream << val;
        return stream.str();
    }

    LPCWSTR _API_CompatablePath(_TCHAR tchar)
    {
        std::string path = "\\\\.\\";
        path.append(1, static_cast<char>(tchar));
        path += ":";
        std::wstring temp = std::wstring(path.begin(), path.end());
        LPCWSTR wideString = temp.c_str();
        return wideString;
    }

    uint16_t _nextPowerOfTwo(uint16_t val) 
    {
        val--;
        val |= val >> 1;
        val |= val >> 2;
        val |= val >> 4;
        val |= val >> 8;
        val++;
        return val;
    }
}
Drive::Drive() :
    drivePath{},
    driveName{},
    filesystem{},
    size{}
{

}

uint64_t Drive::getDriveSize_API(HANDLE& hDrive)
{
    BOOL r;
    DWORD size;
    BYTE geometry[256];
    PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;

    r = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
        NULL, 0, geometry, sizeof(geometry), &size, NULL);
    return DiskGeometry->DiskSize.QuadPart;
}

uint16_t Drive::logicalDriveSize(uint64_t size)
{
    //implement
    return 0;
}
void Drive::getAllDriveInfo()
{
    HANDLE hDrive;
    _TCHAR buffer[MAX_PATH];
    DWORD drives = GetLogicalDriveStrings(MAX_PATH, buffer);

    if (drives == 0)
    {
        std::cerr << "Error getting logical drives. Error code: " << GetLastError() << std::endl;
        return;
    }
    driveMap.clear();

    int idCounter = 1;

    for (DWORD i = 0; i < drives; i += 4)
    {
        _TCHAR drivePath[4] = { buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3] };
        UINT driveType = GetDriveType(drivePath);

        if (driveType == DRIVE_REMOVABLE && driveType != DRIVE_FIXED)
        {
            Drive drive;
            int idCounter = 0;

            _TCHAR volumeName[MAX_PATH];
            _TCHAR fileSystem[MAX_PATH];
            DWORD serialNumber;
            DWORD maxComponentLength;
            DWORD fileSystemFlags;

            if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
            {
                LPCWSTR wideString = helperFunction::_API_CompatablePath(drivePath[0]);
                hDrive = CreateFile( L"\\\\.\\F:",
                    0,FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                
                if (hDrive == INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hDrive); 
                    break;
                }
                CloseHandle(hDrive);
                drive.set_drivePath(drivePath[0]);
                drive.set_driveName(volumeName);
                drive.set_size(drive.getDriveSize_API((hDrive)));
                drive.set_filesystem(fileSystem);
                driveMap.emplace(idCounter++,drive);
            }
        }
    }
}


