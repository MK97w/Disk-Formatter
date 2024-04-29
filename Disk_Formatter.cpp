
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
//#include "drive.h"
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

int main()
{
   Drive::getAllDriveInfo();
   Drive::printDriveMap();
   auto m = Drive::get_driveMap();

   
   
   VolumeFormatter formatter;
   //istAllVolumeInfo();

   std::wstring b = L"FAT32";
   formatter.FMIFS_Format(L"D:\\",b.c_str(), L"mmmmm", 8192);
   Drive::getAllDriveInfo();
   Drive::printDriveMap();
   //listAllVolumeInfo();
   //formatter.Large_FAT32_Format(R"(\\.\D:)");
  // SetVolumeLabelA(R"(\\.\D:\\)", "bidid");
//   listAllVolumeInfo();
  

    return 0;
}
/*
MUST IMPLEMENT CLUSTER SIZES


The default cluster sizes for FAT32 are as follows: -> maybe fix it to 4KB

    < 64 MB: 512 bytes.
    64 MB—128 MB: 1 KB.
    128 MB—256 MB: 2 KB.
    256 MB—8 GB: 4 KB.
    8 GB—16 GB: 8 KB.
    16 GB—32 GB: 16 KB.
    32 GB—2 TB: 32 KB.


 The default cluster size for exFAT is:

    7MB – 256MB: 4KB
    256MB – 32GB: 32KB
    32GB – 256TB: 128KB
    >256TB: Not supported


the common default NTFS cluster sizes as follows:

    ≤512MB: 512 bytes
    513MB ~ 1GB: 1KB
    1GB ~ 2GB: 2KB
    2GB ~ 4GB: 4KB
    4GB ~ 8GB: 8KB
    8GB ~ 16GB: 16KB
    16GB ~ 32GB: 32KB
    ＞32GB: 64KB


    https://www.easeus.com/partition-master/allocation-unit-size-fat32.html
    https://support.microsoft.com/en-au/topic/description-of-default-cluster-sizes-for-fat32-file-system-905ea1b1-5c4e-a03f-3863-e4846a878d31
    https://www.partitionwizard.com/partitionmagic/fat32-allocation-unit-size.html
    https://www.partitionwizard.com/partitionmagic/ntfs-cluster-size.html
    https://www.partitionwizard.com/partitionmagic/allocation-unit-size-exfat.html



*/