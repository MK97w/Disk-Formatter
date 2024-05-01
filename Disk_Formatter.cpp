#include "format.h"



int main()
{
   Drive::getAllDriveInfo();
   Drive::printDriveMap();
   auto m = Drive::get_driveMap();
   auto selectedDrive = m[1];
#if 0 
   std::string s = "\\\\.\\E:\\";
   std::string sL = "aaaaaa";
   std::wstring ws = L"\\\\.\\E:\\";
   std::wstring wsL = L"vvvvvv";
   SetVolumeLabelW(ws.c_str(), wsL.c_str());
#endif
   VolumeFormatter formatter;
   formatter.formatDrive(selectedDrive,L"FAT32");

   Drive::getAllDriveInfo();
   Drive::printDriveMap();
  

    return 0;
}