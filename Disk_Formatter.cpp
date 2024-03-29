﻿
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
#include <sstream>

#pragma comment(lib, "Setupapi.lib")

#define safe_free(p) do {free((void*)p); p = NULL;} while(0)
#define safe_sprintf(dst, count, ...) do {_snprintf(dst, count, __VA_ARGS__); (dst)[(count)-1] = 0; } while(0)
#define static_sprintf(dst, ...) safe_sprintf(dst, sizeof(dst), __VA_ARGS__)
#define CheckDriveIndex(DriveIndex) do {                                            \
	if ((int)DriveIndex < 0) goto out;                                              \
	assert((DriveIndex >= DRIVE_INDEX_MIN) && (DriveIndex <= DRIVE_INDEX_MAX));     \
	if ((DriveIndex < DRIVE_INDEX_MIN) || (DriveIndex > DRIVE_INDEX_MAX)) goto out; \
	DriveIndex -= DRIVE_INDEX_MIN; } while (0)

#define wchar_to_utf8_no_alloc(wsrc, dest, dest_size) \
	WideCharToMultiByte(CP_UTF8, 0, wsrc, -1, dest, dest_size, NULL, NULL)

#define STR_NO_LABEL                "NO_LABEL"


#define LEFT_TO_RIGHT_MARK          "‎"
#define MSG_020                         3020
#define MSG_000                         3000
#define MSG_MAX                         3351

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


const GUID GUID_DEVINTERFACE_USB_HUB =
{ 0xf18a0e88L, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8} };

typedef struct {
    DWORD DeviceType;
    ULONG DeviceNumber;
    ULONG PartitionNumber;
} STORAGE_DEVICE_NUMBER_REDEF;

typedef struct {
    DWORD NumberOfDiskExtents;
    // The one from MS uses ANYSIZE_ARRAY, which can lead to all kind of problems
    DISK_EXTENT Extents[8];
} VOLUME_DISK_EXTENTS_REDEF;

class Device
{
    Device() :
        id{}, name{}, label{},
        index{}, size{}, is_USB{ false },
        is_SCSI{ false }, is_CARD{ false }
    {};

    std::string id;
    std::string name;
    std::string label;
    DWORD index;
    uint64_t size;
    BOOLEAN   is_USB;
    BOOLEAN   is_SCSI;
    BOOLEAN   is_CARD;
};

inline BOOL IsRemovable(const char* buffer)
{
    switch (*((DWORD*)buffer)) {
    case CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL:
    case CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL:
        return TRUE;
    default:
        return FALSE;
    }
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


int GetDriveNumber(HANDLE hDrive, char* path)
{
    STORAGE_DEVICE_NUMBER_REDEF DeviceNumber;
    VOLUME_DISK_EXTENTS_REDEF DiskExtents;
    DWORD size = 0;
    BOOL s;
    int r = -1;

    if (!DeviceIoControl(hDrive, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
        &DiskExtents, sizeof(DiskExtents), &size, NULL) || (size <= 0) || (DiskExtents.NumberOfDiskExtents < 1)) {
        // DiskExtents are NO_GO (which is the case for external USB HDDs...)
        s = DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &DeviceNumber, sizeof(DeviceNumber),
            &size, NULL);
        if ((!s) || (size == 0)) {

            return -1;
        }
        r = (int)DeviceNumber.DeviceNumber;
    }
    else if (DiskExtents.NumberOfDiskExtents >= 2) {

        return -1;
    }
    else {
        r = (int)DiskExtents.Extents[0].DiskNumber;
    }
    if (r >= 64) {
        r = -1;
    }
    return r;
}


void listAllVolumeInfo()
{
    _TCHAR buffer[MAX_PATH];
    _TCHAR buffer2[MAX_PATH];
    DWORD drives = GetLogicalDriveStrings(MAX_PATH, buffer), data_type, size;
    HDEVINFO dev_info = NULL;
    SP_DEVINFO_DATA dev_info_data;
    SP_DEVICE_INTERFACE_DATA devint_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A devint_detail_data;
    HANDLE hDrive;

    if (drives == 0)
    {
        std::cerr << "Error getting logical drives. Error code: " << GetLastError() << std::endl;
    }

    dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE)
    {
        std::cout << "aa";
    }
    dev_info_data.cbSize = sizeof(dev_info_data);

    std::cout << "==========================================================\n";

    // Iterate through each drive
    for (DWORD i = 0; i < drives && SetupDiEnumDeviceInfo(dev_info, i/4, &dev_info_data); i += 4)
    {
        std::cout << i;
        _TCHAR drivePath[4] = { buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3] };

        UINT driveType = GetDriveType(drivePath);

        if (driveType == DRIVE_REMOVABLE)
        {
            // Get volume information
            _TCHAR volumeName[MAX_PATH];
            _TCHAR fileSystem[MAX_PATH];
            DWORD serialNumber;
            DWORD maxComponentLength;
            DWORD fileSystemFlags;


            if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
            {
                memset(buffer2, 0, sizeof(buffer2));
                devint_data.cbSize = sizeof(devint_data);
                devint_detail_data = NULL;

                if (!SetupDiEnumDeviceInterfaces(dev_info, &dev_info_data, &GUID_DEVINTERFACE_DISK, 0, &devint_data))
                {
                    if (GetLastError() != ERROR_NO_MORE_ITEMS)
                    {
                    }
                    else
                    {
                    }
                    //break;
                    std::cout << "brrt";
                }

                if (!SetupDiGetDeviceInterfaceDetailA(dev_info, &devint_data, NULL, 0, &size, NULL)) {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        devint_detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)calloc(1, size);
                        if (devint_detail_data == NULL)
                        {
                            continue;
                        }
                        devint_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
                    }
                    else {
                        continue;
                    }
                }
                if (devint_detail_data == NULL) {
                    continue;
                }
                if (!SetupDiGetDeviceInterfaceDetailA(dev_info, &devint_data, devint_detail_data, size, &size, NULL)) {

                    continue;
                }
                hDrive = CreateFileA(devint_detail_data->DevicePath, 0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hDrive == INVALID_HANDLE_VALUE) {
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
                    if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
                    {
                        CloseHandle(hDrive);
                        std::wcout <<"Drive Path: " << drivePath << '\n';
                        std::wcout << "Drive Name: " << volumeName << '\n';
                        std::cout << "Serial Number: " << serialNumber << '\n';
                        std::wcout << "File System: " << fileSystem << '\n';
                        // std::cout << "Drive Index: " << drive_index << '\n';
                        std::cout << "Drive Size: " << SizeToHumanReadable(drive_size, 0, 1) << '\n';
                    }
                    else
                        std::cerr << "Error getting volume information. Error code: " << GetLastError() << std::endl;
                    safe_free(devint_detail_data);
                    std::cout << "==========================================================\n";
                }
            }

        }

    }
}



int main()
{
   listAllVolumeInfo();
   
   /*  setlocale(LC_ALL, "Turkish");
     int prefix = 0;

        // List of USB storage drivers we know - list may be incomplete!
    const char* usbstor_name[] = {
        // Standard MS USB storage driver
        "USBSTOR",
        // USB card readers, with proprietary drivers (Realtek,etc...)
        // Mostly "guessed" from http://www.carrona.org/dvrref.php
        "RTSUER", "CMIUCR", "EUCR",
        // UASP Drivers *MUST* be listed after this, starting with "UASPSTOR"
        // (which is Microsoft's native UASP driver for Windows 8 and later)
        // as we use "UASPSTOR" as a delimiter
        "UASPSTOR", "VUSBSTOR", "ETRONSTOR", "ASUSSTPT"
    };
    // These are the generic (non USB) storage enumerators we also test
    const char* genstor_name[] = {
        // Generic storage drivers (Careful now!)
        "SCSI", // "STORAGE",	// "STORAGE" is used by 'Storage Spaces" and stuff => DANGEROUS!
        // Non-USB card reader drivers - This list *MUST* start with "SD" (delimiter)
        // See http://itdoc.hitachi.co.jp/manuals/3021/30213B5200e/DMDS0094.HTM
        // Also  http://www.carrona.org/dvrref.php. NB: All members from this list should have
        // been reported as enumerators by Rufus, when Enum Debug is enabled.
        "SD", "PCISTOR", "RTSOR", "JMCR", "JMCF", "RIMMPTSK", "RIMSPTSK", "RISD", "RIXDPTSK",
        "TI21SONY", "ESD7SK", "ESM7SK", "O2MD", "O2SD", "VIACR", "GLREADER"
    };
    // Oh, and we also have card devices (e.g. 'SCSI\DiskO2Micro_SD_...') under the SCSI enumerator...
    const char* scsi_disk_prefix = "SCSI\\Disk";
    const char* scsi_card_name[] = {
        "_SD_", "_SDHC_", "_SDXC_", "_MMC_", "_MS_", "_MSPro_", "_xDPicture_", "_O2Media_"
    };
    //const char* usb_speed_name[USB_SPEED_MAX] = { "USB", "USB 1.0", "USB 1.1", "USB 2.0", "USB 3.0", "USB 3.1" };
    const char* windows_sandbox_vhd_label = "PortableBaseLayer";
    // Hash table and String Array used to match a Device ID with the parent hub's Device Interface Path
    char* drive, drives[26 * 4 + 1];
    char letter_name[] = " (?:)";
    char drive_name[] = "?:\\";
    char setting_name[32];
    char uefi_togo_check[] = "?:\\EFI\\Rufus\\ntfs_x64.efi";
    char scsi_card_name_copy[16];
    BOOL r = FALSE, found = FALSE, post_backslash;
    HDEVINFO dev_info = NULL;
    SP_DEVINFO_DATA dev_info_data;
    SP_DEVICE_INTERFACE_DATA devint_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A devint_detail_data;
    DEVINST parent_inst, grandparent_inst, device_inst;
    DWORD size, i, j, k, l, data_type, drive_index;
    DWORD uasp_start = ARRAYSIZE(usbstor_name), card_start = ARRAYSIZE(genstor_name);
    ULONG list_size[ARRAYSIZE(usbstor_name)] = { 0 }, list_start[ARRAYSIZE(usbstor_name)] = { 0 }, full_list_size, ulFlags;
    HANDLE hDrive;
    LONG maxwidth = 0;
    int s, u, v, score, drive_number, remove_drive, num_drives = 0;
    char drive_letters[27], * device_id, * devid_list = NULL, display_msg[128];
    char* p, * label, * display_name, buffer[MAX_PATH], str[MAX_PATH], device_instance_id[MAX_PATH], * method_str, * hub_path;
    //uint32_t ignore_vid_pid[MAX_IGNORE_USB];
    uint64_t drive_size = 0;
    //usb_device_props props;


    _TCHAR volumeName[MAX_PATH];
    _TCHAR fileSystem[MAX_PATH];
    DWORD serialNumber;
    DWORD maxComponentLength;
    DWORD fileSystemFlags;

    //get all drives first
    GetLogicalDriveStringsA(sizeof(drives), drives);

    device_id = (char*)malloc(MAX_PATH);
    if (device_id == NULL)
        std::cout << "fail";

    // Now use SetupDi to enumerate all our disk storage devices
    dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    dev_info_data.cbSize = sizeof(dev_info_data);
    for (i = 0; num_drives < 64 && SetupDiEnumDeviceInfo(dev_info, i, &dev_info_data); i++)
    {
            
            memset(buffer, 0, sizeof(buffer));
            if (!SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_ENUMERATOR_NAME,
                &data_type, (LPBYTE)buffer, sizeof(buffer), &size)) {
                continue;
            }
            if (SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_REMOVAL_POLICY,
                &data_type, (LPBYTE)buffer, sizeof(buffer), &size) && IsRemovable(buffer))
            {

                memset(buffer, 0, sizeof(buffer));
                devint_data.cbSize = sizeof(devint_data);
                devint_detail_data = NULL;

                for (j = 0; ; j++)
                {
                    safe_free(devint_detail_data);

                    if (!SetupDiEnumDeviceInterfaces(dev_info, &dev_info_data, &GUID_DEVINTERFACE_DISK, j, &devint_data)) {
                        if (GetLastError() != ERROR_NO_MORE_ITEMS)
                        {
                        }
                        else {
                        }
                        break; // E den sonra buraya geldik! i değerim hala 3
                    }

                    if (!SetupDiGetDeviceInterfaceDetailA(dev_info, &devint_data, NULL, 0, &size, NULL)) {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                            devint_detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)calloc(1, size);
                            if (devint_detail_data == NULL)
                            {
                                continue;
                            }
                            devint_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
                        }
                        else {
                            continue;
                        }
                    }
                    if (devint_detail_data == NULL) {
                        continue;
                    }
                    if (!SetupDiGetDeviceInterfaceDetailA(dev_info, &devint_data, devint_detail_data, size, &size, NULL)) {

                        continue;
                    }

                    hDrive = CreateFileA(devint_detail_data->DevicePath, 0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hDrive == INVALID_HANDLE_VALUE) {
                        const DWORD error = GetLastError();
                        std::cout << error << '\n';
                        std::cout << j << " invalid" << '\n';
                        break;
                    }
                    uint64_t drive_size = GetDriveSize(hDrive);
                    drive_number = GetDriveNumber(hDrive, devint_detail_data->DevicePath);
                    if (drive_number < 0)
                        continue;
                    drive_index = drive_number + 128;
                    BYTE geometry[128];
                    if (!DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                        NULL, 0, geometry, sizeof(geometry), &size, NULL) && (size > 0))
                    {
                        CloseHandle(hDrive);
                        break;
                    }
                    else
                    {
                        drive_name[0] = drives[i * 4];
                        // drive_name[0] = drives[8];

                        _TCHAR drivePath[4];

                        int written = MultiByteToWideChar(CP_ACP, 0, drive_name, 4, drivePath, 4);


                        if (GetVolumeInformation(drivePath, volumeName, MAX_PATH, &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystem, MAX_PATH))
                        {
                            CloseHandle(hDrive);
                            std::cout << "==========================================================\n";
                            std::cout << "Drive Label : " << drive_name << '\n';
                            std::wcout << "Drive Name: " << volumeName << '\n';
                            std::cout << "Serial Number: " << serialNumber << '\n';
                            std::wcout << "File System: " << fileSystem << '\n';
                            // std::cout << "Drive Index: " << drive_index << '\n';
                            std::cout << "Drive Size: " << SizeToHumanReadable(drive_size, 0, 1) << '\n';
                            std::cout << "==========================================================\n";
                        }
                    }
                }

            }
            else
            {
                continue;
            }
    }*/
    
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
*    Note to self 25 / 03
*
*     Still polling and opening handle to wrong device. i+=4 probably causes error
*
*
*/