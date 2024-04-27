#include<stdint.h>
#include <Windows.h>
#include <unordered_map>
#include <tchar.h>

class Drive
{
public:
	Drive();
	~Drive();
	inline _TCHAR get_drivePath()const { return drivePath; }
	inline std::wstring get_driveName()const { return driveName; }
	inline std::wstring get_filesystem()const { return filesystem; }
	inline uint64_t get_size()const { return size; }

	inline void set_drivePath(TCHAR pth) { drivePath = pth; }
	inline void set_driveName(_TCHAR* drvnm) { driveName = drvnm; }
	inline void set_filesystem(_TCHAR* fs) { filesystem = fs; }
	inline void set_size(uint64_t sz) { size = sz; }

	static void getAllDriveInfo();
	static void printDriveMap();

	

private:
	TCHAR drivePath;
	std::wstring driveName;
	std::wstring filesystem;
	uint64_t size;
	static inline std::unordered_map<int, Drive> driveMap{};
    

private:
	uint64_t getDriveSize_API(HANDLE&);
	uint16_t logicalDriveSize(uint64_t);

};