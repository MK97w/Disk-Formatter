#include "format.h"
#include <iostream>
#include <functional>
#include <limits>


int getDriveIndex(size_t mapSize)
{
    std::cout << "Please select the ID of the Removable Drive you want to format:";
    int index = -1;
    while (index == -1)
    {
        std::string input;
        std::cin >> input;

        auto validInput = true;
        for (const char ch : input)
        {
            if (!isdigit(ch))
            {
                std::cout << "Please enter a numeric value!\n";
                validInput = false;
                break; 
            }
        }

        if (validInput)
        {
            int selected = std::stoi(input);
            if (selected >= 1 && selected <= static_cast<int>(mapSize))
            {
                index = selected;
                std::cout << "Removable Drive [" << index << "] is selected\n"<<std::endl;
            }
            else
            {
                std::cout << "Please select a valid option!\n";
            }
        }
    }
    return index;
}

std::wstring getDesiredFileSystem()
{
    std::wstring fs{};
    std::unordered_map<int, std::wstring> fileSystemTypes = 
    {
        {1, L"FAT32"},
        {2, L"NTFS"},
        {3, L"exFAT"},
  
    };

    std::cout << "Filesystems" << "\n"<<"------------"<<'\n';
    for (const auto& pair : fileSystemTypes)
    {
        std::wcout << "[" << pair.first << "] "<< pair.second << "\n";
    }
    std::cout << "\nPlease select the Filesystem:";
    int index = -1;
    while (index == -1)
    {
        std::string input;
        std::cin >> input;

        auto validInput = true;
        for (const char ch : input)
        {
            if (!isdigit(ch))
            {
                std::cout << "Please enter a numeric value!\n";
                validInput = false;
                break;
            }
        }
        if (validInput)
        {
            int selected = std::stoi(input);
            if (selected >= 1 && selected <= static_cast<int>(fileSystemTypes.size()))
            {
                index = selected;
                std::wcout << fileSystemTypes[index]<<" is selected\n"<<std::endl;
            }
            else
            {
                std::cout << "Please select a valid option!\n";
            }
        }
    }
    return fileSystemTypes[index];
}



bool proceed()
{
    std::cout << "Warning! ALL data on drive will be lost irretrievably, are you sure\n(y / n) :";
    char input;
    std::cin >> input;
    input = std::toupper(input); 

    if (input == 'Y')
    {
        return true;
    }
    else if (input == 'N')
    {
        std::cout << "Operation canceled by user.\n";
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << "Invalid input. Please enter 'y' or 'n'.\n";
        return proceed();
    }
}


int main()
{
   Drive::getAllDriveInfo();
   Drive::printDriveMap();
   auto dmap = Drive::get_driveMap();
   auto index = getDriveIndex( dmap.size() ) -1;
   
   auto selectedDrive = dmap[index];
   auto selectedFileSystem = getDesiredFileSystem();

   if (proceed())
   {
        VolumeFormatter formatter;
        formatter.formatDrive(selectedDrive, selectedFileSystem);
   }

   Drive::getAllDriveInfo();
   Drive::printDriveMap();
  

   return 0;
}