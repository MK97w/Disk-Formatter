#include<stdint.h>
#include <Windows.h>
#include <unordered_map>

class Drive
{
public:
	Drive();
	PCWSTR get_drivePath();
	PCWSTR get_driveName();
	PCWSTR get_filesystem();
	uint16_t get_size();

	void set_drivePath(PCWSTR);
	void set_driveName(PCWSTR);
	void set_filesystem(PCWSTR);
	void set_size(uint16_t);

	static void getAllDriveInfo();

private:
	PCWSTR drivePath;
	PCWSTR driveName;
	PCWSTR filesystem;
	uint16_t size;
	static std::unordered_map<int, Drive> driveMap;
};