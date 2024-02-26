#pragma once

#include <windows.h>
#include <SetupAPI.h>
#include <stdint.h>
#include <cfgmgr32.h>
#include <vector>
#include <string>

class Device
{
	Device(): 
		id{},	name{},	label{},
		index{},size{}, is_USB{ false },
		is_SCSI{ false },is_CARD{ false }, 
		is_Removable{ false }
		{};
		
		std::string id;
		std::string name;
		std::string label;
		DWORD index;
		uint64_t size;
		BOOLEAN   is_USB;
		BOOLEAN   is_SCSI;
		BOOLEAN   is_CARD;
		BOOLEAN   is_Removable;
};



