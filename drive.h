#include<stdint.h>
#include <Windows.h>
#include <unordered_map>
#include <tchar.h>

class Drive
{
public:
	Drive();
	~Drive();
	inline TCHAR get_drivePath() { return drivePath; }
	inline PCWSTR get_driveName() { return driveName; }
	inline PCWSTR get_filesystem() { return filesystem; }
	inline uint64_t get_size() { return size; }

	inline void set_drivePath(TCHAR pth) { drivePath = pth; }
	inline void set_driveName(PCWSTR drvnm) { driveName = drvnm; }
	inline void set_filesystem(PCWSTR fs) { filesystem = fs; }
	inline void set_size(uint64_t sz) { size = sz; }

	static void getAllDriveInfo();


	

private:
	TCHAR drivePath;
	PCWSTR driveName;
	PCWSTR filesystem;
	uint64_t size;
	static inline std::unordered_map<int, Drive> driveMap{};
    

private:
	uint64_t getDriveSize_API(HANDLE&);
	uint16_t logicalDriveSize(uint64_t);

};