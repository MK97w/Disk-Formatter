#include "format.h"



int main()
{
   Drive::getAllDriveInfo();
   Drive::printDriveMap();
   auto m = Drive::get_driveMap();
   auto selectedDrive = m[1];
   std::string s = "\\\\.\\E:\\";
   std::string sL = "aaaa";
   std::wstring ws = L"\\\\.\\E:\\";
   std::wstring wsL = L"vvvv";


   VolumeFormatter formatter;
   formatter.formatDrive(selectedDrive,L"FAT32");

   Drive::getAllDriveInfo();
   Drive::printDriveMap();
  

    return 0;
}