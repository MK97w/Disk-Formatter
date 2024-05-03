#include "drive.h"
#include <iostream>
#include <iomanip>
#include "helper_functions.h"


Drive::Drive() :
    drivePath{},
    driveName{},
    filesystem{},
    size{}
{
    //std::cout << "live!"<<'\n';
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
    //std::cout << "died!"<<'\n';
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

std::string Drive::printableLogicalSize(uint64_t physicalSize) const
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
        res = helperFunction::addSuffix(suffix , helperFunction::_toString(static_cast<int>(i_size)));
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

    int idCounter = 0;

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
    std::cout 
        << std::setw(6) << std::left << "ID "
        << std::setw(9) << "Path"
        << std::setw(13) << std::left << "Drive Name"
        << std::setw(13) << "Drive Size"
        << std::setw(12) << "Filesystem"
        << std::endl;

    std::cout << std::setfill('-') << std::setw(52) << "-" << std::endl;
    std::cout << std::setfill(' ');
    
    for (const auto& pair : driveMap)
    {
        std::cout << std::internal <<'['<< pair.first + 1<< ']';
        std::wcout << std::internal << std::setw(5) << pair.second.get_drivePath()<<":\\";
        std::wcout << std::internal << std::setw(15) << pair.second.get_driveName();
        std::cout << std::internal << std::setw(11) << pair.second.printableLogicalSize(pair.second.get_size());
        std::wcout << std::internal << std::setw(12) << pair.second.get_filesystem()<< std::endl;
    }
    std::cout << '\n';
}