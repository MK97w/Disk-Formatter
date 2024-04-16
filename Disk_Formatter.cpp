
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

enum CALLBACKCOMMAND {
    PROGRESS,
    DONEWITHSTRUCTURE,
    UNKNOWN2,
    UNKNOWN3,
    UNKNOWN4,
    UNKNOWN5,
    INSUFFICIENTRIGHTS,
    UNKNOWN7,
    UNKNOWN8,
    UNKNOWN9,
    UNKNOWNA,
    DONE, // format OK!
    UNKNOWNC,
    UNKNOWND,
    OUTPUT,
    STRUCTUREPROGRESS
};

// 
// FMIFS callback definition 
// 
typedef BOOLEAN(WINAPI* PFMIFSCALLBACK)(CALLBACKCOMMAND Command, DWORD SubAction, PVOID ActionInfo);

enum class FMIFS_MEDIA_TYPE
{
    Unknown = MEDIA_TYPE::Unknown,                   // Format is unknown
    RemovableMedia = MEDIA_TYPE::RemovableMedia,     // Removable media other than floppy
    FixedMedia = MEDIA_TYPE::FixedMedia,             // Fixed hard disk media
};

ULONG g_dwTlsIndex;

struct FORMAT_DATA
{
    BOOLEAN fOk;
};

BOOLEAN FormatCb(CALLBACKCOMMAND Command, DWORD SubAction, PVOID ActionInfo)
{
    FORMAT_DATA* fd = (FORMAT_DATA*)TlsGetValue(g_dwTlsIndex);
    if (Command == DONE)
    {
        fd->fOk = TRUE;
    }
    return TRUE;
}

BOOL TryFormat()
{
    FORMAT_DATA fd;
    fd.fOk = FALSE;
    const wchar_t* str = L"E:\\";

    if ((g_dwTlsIndex = TlsAlloc()) != TLS_OUT_OF_INDEXES)
    {
        if (HMODULE hmod = LoadLibrary(L"fmifs"))
        {
            VOID(WINAPI * FormatEx)(
                PCWSTR DriveRoot,
                FMIFS_MEDIA_TYPE MediaType,
                PCWSTR FileSystemName,
                PCWSTR VolumeLabel,
                BOOL QuickFormat,
                DWORD ClusterSize,
                PFMIFSCALLBACK Callback);

            *(void**)&FormatEx = GetProcAddress(hmod, "FormatEx");

            if (FormatEx)
            {
                TlsSetValue(g_dwTlsIndex, &fd);
                auto a = GetVolumeGuid(str);
                FormatEx(str, FMIFS_MEDIA_TYPE::RemovableMedia, L"FAT32", L"Mert32", TRUE, 8192, FormatCb);
            }

            FreeLibrary(hmod);
        }

        TlsFree(g_dwTlsIndex);
    }

    return fd.fOk;
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
    listAllVolumeInfo();
    auto res = TryFormat();
   //char volume[8] = R"(\\.\?:)";
   //volume[4] = 'E';
   //format_params p;
   //format_volume(volume,&p);
   //strcat_s(volume, "\\");
   //SetVolumeLabelA(volume, "32GB_SDCard");
   listAllVolumeInfo();
}


/*
* Note to self:
*
* Rufus lists devices starting from 130 to 131
*
* My app finds 128 and 129 find out why
*/

/*
    Note to self: 29/02

   1- Rufus first SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_ENUMERATOR_NAME) to get the name

    if USBSTOR (and variants) -> sets device props to USB
                    -> Then looks for if generic storage names to find out if its CARD or SCSI

   2-  SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_HARDWAREID,
            &data_type, (LPBYTE)buffer, sizeof(buffer), &size) && IsVHD(buffer);  to find if its VHD

            Buffer becomes : USBSTOR\Disk________MassStorageClass____
                  ->If in step1 wont set the parameter is_CARD true
                    ->Inside this buffer again we search for SD Card related information
                        "_SD_", "_SDHC_", "_SDXC_", "_MMC_", "_MS_", "_MSPro_", "_xDPicture_", "_O2Media_"
                        = "SCSI\\Disk";
                        and _SD&", "_SDHC&", "_SDXC&", "_MMC&", "_MS&", "_MSPro&", "_xDPicture&", "_O2Media&

    3- Checkes for removal policy with  SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_REMOVAL_POLICY,
            &data_type, (LPBYTE)buffer, sizeof(buffer), &size) && IsRemovable(buffer);

    4- IsMediaPresent(drive_index)) where we eliminate the device if there is no media
*/

/*Note to self
*
* wcout part didnt work when turkish char encountered and broke.
*/

/*Note to self
*
* Probably i will not need device id -> evint_detail_data->DevicePath, should be enough. I think i should only keep this and open deivce using it

*/


/*Note to self 17/03
*
* For right formatting function i will need to keep the storage size of the device
*    --> implement: GetDriveSize and get value  then send it to
*                   SizeToHumanReadable ->from this we get smth like 16GB
*
*
* -----> Formatting:
*
*   1. Delete partition
*           implement : GetVdsDiskInterface(DriveIndex, &IID_IVdsAdvancedDisk, (void**)&pAdvancedDisk, bSilent))
*   2.	AnalyzeMBR(hPhysicalDrive, "Drive", FALSE);
*
*   3. if ((!ClearMBRGPT(hPhysicalDrive, SelectedDrive.DiskSize, SelectedDrive.SectorSize, use_large_fat32)) ||
            (!InitializeDisk(hPhysicalDrive)))

    4. CreatePartition(hPhysicalDrive, partition_type, fs_type, (partition_type == PARTITION_STYLE_MBR)
        && (target_type == TT_UEFI), extra_partitions)) {

    5. Format partition
        implement: FormatLargeFAT32

   6. !RemountVolume(drive_name, FALSE)

*/

/*
*    Note to self 18 / 03
*  -
*
*   Bug on Terrabyte
*   Bug on multiple devices on SD Card reader
*
*
*/

/*
*    Note to self 19 / 03
*  -
*
*   Terrabyte storage cannot be detected by its own
*     Bug on multiple devices caused by mislabeling while polling through whole drive names.
*
*
*/

/*
*    Note to self 20 / 03
*
*     Bug on multiple devices caused by mislabeling while polling through whole drive names. add i+4 if disk is eliminated
*
*
*/

/*
*    Note to self 02 / 04
*
*     SD Card reader USB devices listed as unknown
*
*/
/*
*    Note to self 04 / 04
*
*     Okay so back to stealing from rufus hehe :)
*     I must implement FormatNativeVds for GB < 64 -> for fat32
*     otherwise implement FormatLargeFAT32
*
Note to self 08 / 04
*
*     FormatLargeFAT32 -> is mandatory for 64 gb FAT32 otherwise it fails
*     FormatNativeVds -> handles anything under 32GB
* 
*
*/