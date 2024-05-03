#include "format.h"
#include <iostream>
#include <functional>
#include <limits>

int getDriveIndex(size_t mapSize)
{
    std::cout << "Please select the ID of the Removable Drive you want to format:";
    auto index = -1;
    while (-1 == index)
    {
        auto ch = getchar();
        if (isdigit(ch))
        {
            int selected = ch - '0';
            if (selected >= 1 && selected <= static_cast<int>(mapSize))
            {
                index = selected;
                std::cout << "Removable Drive [" << index << "] is selected\n";
            }
            else
            {
                std::cout << "Please select a valid option!\n";
            }
        }
        else
        {
            std::cout << "Please enter a numeric value!\n";
            while (getchar() != '\n');
        }
    }
    return index;
}
int main()
{
   Drive::getAllDriveInfo();
   Drive::printDriveMap();
   auto dmap = Drive::get_driveMap();
   auto index = getDriveIndex(dmap.size()) -1;
   
   std::cout << index<<"\n";
   //auto selectedDrive = dmap[index];

#if 0 
   std::string s = "\\\\.\\E:\\";
   std::string sL = "aaaaaa";
   std::wstring ws = L"\\\\.\\E:\\";
   std::wstring wsL = L"vvvvvv";
   SetVolumeLabelW(ws.c_str(), wsL.c_str());
#endif
  // VolumeFormatter formatter;
  // formatter.formatDrive(selectedDrive,L"FAT32");

   Drive::getAllDriveInfo();
   Drive::printDriveMap();
  

    return 0;
}