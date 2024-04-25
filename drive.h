#include<stdint.h>
#include <Windows.h>
#include <unordered_map>
#include <tchar.h>

class Drive
{
public:
	Drive();
	~Drive();
	inline TCHAR get_drivePath()const { return drivePath; }
	inline TCHAR* get_driveName()const { return driveName; }
	inline TCHAR* get_filesystem()const { return filesystem; }
	inline uint64_t get_size()const { return size; }

	inline void set_drivePath(TCHAR pth) { drivePath = pth; }
	inline void set_driveName(TCHAR* drvnm) { driveName = drvnm; }
	inline void set_filesystem(TCHAR* fs) { filesystem = fs; }
	inline void set_size(uint64_t sz) { size = sz; }

	static void getAllDriveInfo();
	static void printDriveMap();

	

private:
	TCHAR drivePath;
	TCHAR* driveName;
	TCHAR* filesystem;
	uint64_t size;
	static inline std::unordered_map<int, Drive> driveMap{};
    

private:
	uint64_t getDriveSize_API(HANDLE&);
	uint16_t logicalDriveSize(uint64_t);

};