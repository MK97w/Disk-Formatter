#include<stdint.h>
#include <Windows.h>
#include <unordered_map>
#include <tchar.h>
#include <type_traits>

class Drive
{
private:
	enum class sizingFormat { PHYSICAL, LOGICAL };

private:
	TCHAR drivePath;
	std::wstring driveName;
	std::wstring filesystem;
	uint64_t size;
	constexpr static int MAX_SIZE_SUFFIXES{ 6 };        		// bytes, KB, MB, GB, TB, PB
	static inline std::unordered_map<int, Drive> driveMap{};


private:
	uint64_t getDriveSize_API(HANDLE&) const;
	std::string logicalDriveSize(uint64_t) const;

public:
	Drive();
	~Drive();

	inline void set_drivePath(TCHAR pth) { drivePath = pth; }
	inline void set_driveName(_TCHAR* drvnm) { driveName = drvnm; }
	inline void set_filesystem(_TCHAR* fs) { filesystem = fs; }
	inline void set_size(uint64_t sz) { size = sz; }

	inline _TCHAR get_drivePath()const { return drivePath; }
	inline std::wstring get_driveName()const { return driveName; }
	inline std::wstring get_filesystem()const { return filesystem; }
	static inline const std::unordered_map<int, Drive>& get_driveMap() { return driveMap; }



	//i also could use a function that returns std::any
	template <sizingFormat szFmt>
	inline auto get_size() const
	{
		if constexpr (szFmt == sizingFormat::PHYSICAL)
			return size;
		else
			return logicalDriveSize(size);
	}

	static void getAllDriveInfo();
	static void printDriveMap();


};