#include "drive.h"
#include <sstream> 
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

    std::wstring _API_CompatablePath(TCHAR tchar)
    {
        std::wstring path = L"\\\\.\\";
        path += tchar;
        path += L":";
        return path;
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

    std::string addSuffix(int suffix, std::string str)
    {
      if (suffix == 1)
            str += " KB";
      else if (suffix == 2)
            str += " MB";
      else if (suffix == 3)
            str += " GB";
      else if (suffix == 4)
            str += " TB";

      return str;

    }
}
Drive::Drive() :
    drivePath{},
    driveName{},
    filesystem{},
    size{}
{
    //std::cout << "live!";
}
Drive::~Drive()
{/*
    drivePath = '\0';   
    driveName = nullptr; 
    delete[] driveName;
    filesystem = nullptr; 
    delete[] filesystem;
    size = 0;
*/
    //std::cout << "died!";
}

uint64_t Drive::getDriveSize_API(HANDLE& hDrive) const
{
    BOOL r;
    DWORD size;
    BYTE geometry[256];
    PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;

    r = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
        NULL, 0, geometry, sizeof(geometry), &size, NULL);
    return DiskGeometry->DiskSize.QuadPart;
}

std::string Drive::logicalDriveSize(uint64_t physicalSize) const
{
    int suffix{ 0 };
    std::string res{ };
    double hr_size = static_cast<double>(physicalSize);
    double t;
    uint16_t i_size;
    const double divider = 1000.0;

    for (suffix = 0; suffix < MAX_SIZE_SUFFIXES - 1 ; suffix++)
    {
        if (hr_size < divider)
            break;
        hr_size /= divider;
    }
    if (hr_size > 0)
    {
        t = static_cast<double>(helperFunction::_nextPowerOfTwo(static_cast<uint16_t>(hr_size)));
        i_size = (uint16_t)((fabs(1.0f - (hr_size / t)) < 0.05f) ? t : hr_size);
        auto tmp = helperFunction::_toString(static_cast<int>(i_size));
        res = helperFunction::addSuffix(suffix ,tmp);
    }
    return res;
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
            _TCHAR volumeName[MAX_PATH];
            _TCHAR fileSystem[MAX_PATH];
            DWORD serialNumber;
            DWORD maxComponentLength;
            DWORD fileSystemFlags;

            if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
            {
       
                hDrive = CreateFile(helperFunction::_API_CompatablePath(drivePath[0]).c_str(),
                    0,FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                
                if (hDrive == INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hDrive); 
                    break;
                }
                Drive drive;
                drive.set_drivePath(drivePath[0]);
                drive.set_driveName(volumeName);
                drive.set_size(drive.getDriveSize_API((hDrive)));
                drive.set_filesystem(fileSystem);
                driveMap.emplace(idCounter++,std::move(drive));
                CloseHandle(hDrive);
            }
        }
    }
}

void Drive::printDriveMap()
{
    for ( const auto& pair : driveMap )
    {
        std::wcout << "Drive ID: " << pair.first << '\n';
        std::wcout << "Drive Path: " << pair.second.get_drivePath() << '\n';
        std::wcout << "Drive Name: " << pair.second.get_driveName() << '\n';
        std::wcout << "File System: " << pair.second.get_filesystem() << '\n';
        std::cout << "Drive Size: " << pair.second.get_size<sizingFormat::LOGICAL>()<< '\n';
        std::wcout << "==============================\n";
    }
}