// Fat32 formatter version 1.05
// (c) Tom Thornhill 2007,2008,2009
// This software is covered by the GPL. 
// By using this tool, you agree to absolve Ridgecrop of an liabilities for lost data.
// Please backup any data you value before using this tool.

// |                      ALIGNING_SIZE * N                      |
// | BPB,FSInfo,Reserved | FAT1              | FAT2              | Cluster0




#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winioctl.h>
#include <versionhelpers.h>



/*
28.2  CALCULATING THE VOLUME SERIAL NUMBER

For example, say a disk was formatted on 26 Dec 95 at 9:55 PM and 41.94
seconds.  DOS takes the date and time just before it writes it to the
disk.

Low order word is calculated:               Volume Serial Number is:
	Month & Day         12/26   0c1ah
	Sec & Hundrenths    41:94   295eh               3578:1d02
								-----
								3578h

High order word is calculated:
	Hours & Minutes     21:55   1537h
	Year                1995    07cbh
								-----
								1d02h
*/

int formatLarge_FAT32(_In_z_ LPCWSTR vol, _In_z_ LPCWSTR label);

