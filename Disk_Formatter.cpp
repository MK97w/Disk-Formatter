
#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <fileapi.h>


HWND hDevice;

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
                std::wcout << "Device No: " << (DWORD)((LRESULT)(ULONG_PTR)SNDMSG((hDevice), CB_GETITEMDATA, (WPARAM)(int)(i), 0L)) << std::endl;;
            }
            else
                std::cerr << "Error getting volume information. Error code: " << GetLastError() << std::endl;
            std::cout << "==========================================================\n";
        }

    }

}

/*
int GetSDDiskNumber() 
{
    // Enumerate through drive letters
    for (WCHAR drive = L'A'; drive <= L'Z'; ++drive) 
    {
        std::wstring driveLetter = std::wstring(1, drive) + L":\\";
        std::wstring volumePath = driveLetter + L"\\";
        WCHAR volumeDevicePath[MAX_PATH];
        if (GetVolumePathNamesForVolumeNameW(volumePath.c_str(), volumeDevicePath, MAX_PATH, NULL)) 
        {
           std::wstring devicePath = volumeDevicePath;
           size_t found = devicePath.find(L"\\", 0);
           if (found != std::wstring::npos) 
           {
                std::wstring diskNumberStr = devicePath.substr(found + 1);
                return _wtoi(diskNumberStr.c_str());
           }
        }
    }
    return -1; // SD card not found
}
*/

int main()
{
    listAllVolumeInfo();

 /*   auto driveLayoutInfo = std::make_unique<DRIVE_LAYOUT_INFORMATION_EX>;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    hDevice = CreateFile(L"\\\\.\\PhysicalDrive2",
        GENERIC_READ | GENERIC_WRITE,
        0,              // Only we can access 
        NULL,           // Default security
        OPEN_EXISTING,  // For hardware, open existing 
        0,              // File attributes
        NULL);          //Do not copy attributes 
   
    CREATE_DISK dsk;
    memset(&dsk, 0, sizeof(dsk));
    CREATE_DISK_MBR dskmbr = { 0 };
    dskmbr.Signature = 1;
    dsk.PartitionStyle = PARTITION_STYLE_MBR;
    dsk.Mbr = dskmbr;
*/
}
