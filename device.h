#pragma once

#include <windows.h>
#include <SetupAPI.h>
#include <stdint.h>
#include <cfgmgr32.h>


class DeviceManager
{
public:
	struct DeviceProps
	{
		uint32_t  vid;
		uint32_t  pid;
		uint32_t  speed;
		uint32_t  lower_speed;
		uint32_t  port;
		BOOLEAN   is_USB;
		BOOLEAN   is_SCSI;
		BOOLEAN   is_CARD;
		BOOLEAN   is_UASP;
		BOOLEAN   is_VHD;
		BOOLEAN   is_Removable;
	};


	HANDLE hDrive;
	HDEVINFO deviceInfo{};
	SP_DEVINFO_DATA deviceInfoData{};
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA_A deviceInterfaceDetailData{};
	

public:
	BOOL getDeviceInfo();


};

