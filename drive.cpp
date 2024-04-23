#include "drive.h"
#define STATIC

STATIC void Drive::getAllDriveInfo()
{
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
            drive.set_drivePath(drivePath);
            
            driveMap[idCounter++] = drive;
        }
    }
}
