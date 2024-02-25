
#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <fileapi.h>
#include <devguid.h>
#include <SetupAPI.h>
#include <stdint.h>
#include <cfgmgr32.h>

#pragma comment(lib, "Setupapi.lib")

#define safe_free(p) do {free((void*)p); p = NULL;} while(0)

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


BOOL getDeviceInterfaceInfo(HDEVINFO& hdevInfo, SP_DEVINFO_DATA& spDevInfoData, 
    SP_DEVICE_INTERFACE_DATA& spDeviceInterfaceData, PSP_DEVICE_INTERFACE_DETAIL_DATA_A& pspDeviceInterfaceDetailData, DWORD memIndex)
{
    BOOL res{ false };
    DWORD size;
    if( SetupDiEnumDeviceInterfaces( hdevInfo, &spDevInfoData, &GUID_DEVINTERFACE_DISK, memIndex/4, &spDeviceInterfaceData ) )
    {
        if ( SetupDiGetDeviceInterfaceDetailA(hdevInfo, &spDeviceInterfaceData, NULL, 0, &size, NULL))
        {
            if ( SetupDiGetDeviceInterfaceDetailA( hdevInfo, &spDeviceInterfaceData, pspDeviceInterfaceDetailData, size, &size, NULL ) )
            {
                res = TRUE;
            }
        }
    }
    return res;
}

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
    DWORD drives = GetLogicalDriveStrings(MAX_PATH, buffer) , data_type, size;
    HDEVINFO dev_info = NULL;
    SP_DEVINFO_DATA dev_info_data;
    SP_DEVICE_INTERFACE_DATA devint_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A devint_detail_data;
    HANDLE hDrive;

    if (drives == 0)
    {
        std::cerr << "Error getting logical drives. Error code: " << GetLastError() << std::endl;
    }
    std::cout << "==========================================================\n";
    // Iterate through each drive
    for (DWORD i = 0; i < drives; i ++)
    {

        _TCHAR drivePath[4] = { buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3] };

        // Check if the drive is a removable drive (e.g., SD card)
        UINT driveType = GetDriveType(drivePath);

        if (driveType == DRIVE_REMOVABLE)
        {
            dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
            dev_info_data.cbSize = sizeof(dev_info_data);
            SetupDiEnumDeviceInfo(dev_info, i, &dev_info_data);
            memset(buffer2, 0, sizeof(buffer2));
            SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_ENUMERATOR_NAME, &data_type, (LPBYTE)buffer2, sizeof(buffer2), &size);

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
                   //td::wcout << "Device Index: " << getDriveNumber(hDrive) << '\n'
               
            }
            else
                std::cerr << "Error getting volume information. Error code: " << GetLastError() << std::endl;
            std::cout << "==========================================================\n";
        }

    }

}


int main()
{
    //listAllVolumeInfo();

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

    device_id = (char*)malloc(MAX_PATH);
    if (device_id == NULL)
        std::cout << "fail";

    // Now use SetupDi to enumerate all our disk storage devices
    dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    dev_info_data.cbSize = sizeof(dev_info_data);
    for (i = 0; num_drives < 64 && SetupDiEnumDeviceInfo(dev_info, i, &dev_info_data); i++)
    {
        std::cout << "==========================================================\n";
        memset(buffer, 0, sizeof(buffer));
        if (!SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_ENUMERATOR_NAME,
            &data_type, (LPBYTE)buffer, sizeof(buffer), &size)) {
            continue;
        }
        if (SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_info_data, SPDRP_REMOVAL_POLICY,
            &data_type, (LPBYTE)buffer, sizeof(buffer), &size) && IsRemovable(buffer))
            std::cout << "removable" << '\n';

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
                break;
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
                std::cout << error<<'\n';
                std::cout <<j <<" invalid" << '\n';
            }

            drive_number = GetDriveNumber(hDrive, devint_detail_data->DevicePath);
            CloseHandle(hDrive);
            if (drive_number < 0)
                continue;

            drive_index = drive_number + 128;
            std::cout << "Drive Index: " << drive_index << '\n';
            std::cout << "==========================================================\n";


        }
    }
}