
#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <fileapi.h>
#include "device.h"


void listAllVolumeInfo() 
{
    _TCHAR buffer[MAX_PATH];
    DWORD drives = GetLogicalDriveStrings(MAX_PATH, buffer);

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

            if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
            {
                std::wcout << "Volume Name: " << volumeName << '\n';
                std::wcout << "Serial Number: " << serialNumber << '\n';
                std::wcout << "File System: " << fileSystem << '\n';
                std::wcout << "Device Index: " << i <<'\n';
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
