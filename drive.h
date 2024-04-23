#include<stdint.h>
#include <Windows.h>
#include <unordered_map>
#include <tchar.h>

class Drive
{
public:
	Drive();
	PCWSTR get_drivePath();
	PCWSTR get_driveName();
	PCWSTR get_filesystem();
	uint64_t get_size();

	void set_drivePath(PCWSTR);
	void set_driveName(PCWSTR);
	void set_filesystem(PCWSTR);
	void set_size(uint64_t);

	void getAllDriveInfo();

private:
	PCWSTR drivePath;
	PCWSTR driveName;
	PCWSTR filesystem;
	uint64_t size;
   std::unordered_map<int, Drive> driveMap;

private:
	uint64_t getDriveSize_API(HANDLE&);
	uint16_t logicalDriveSize(uint64_t);

};