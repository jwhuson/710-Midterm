#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <iostream>
#include <filesystem>
#include "accctrl.h"
#include "aclapi.h"
#include <atlbase.h>
#include <string>
#include <iomanip>
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "advapi32.lib")


namespace fs = std::filesystem;
using namespace std;
void printFileOwner(const std::string& filePath)
{
    PSID pSidOwner;
    PSECURITY_DESCRIPTOR pSD;
    if (GetNamedSecurityInfo(filePath.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
        &pSidOwner, nullptr, nullptr, nullptr, &pSD) == ERROR_SUCCESS)
    {
        TCHAR ownerName[256];
        DWORD ownerNameSize = sizeof(ownerName) / sizeof(TCHAR);
        TCHAR domainName[256];
        DWORD domainNameSize = sizeof(domainName) / sizeof(TCHAR);
        SID_NAME_USE eUse;
        if (LookupAccountSid(nullptr, pSidOwner, ownerName, &ownerNameSize, domainName, &domainNameSize, &eUse))
        {
            _tprintf(_T(" +-------------------> owned by %s\\\\%s\n"), domainName, ownerName);
        }
        LocalFree(pSD);
    }
}

void searchDirectory(const std::string& path, int level = 0) 
{
    for (auto& entry : fs::directory_iterator(path)) 
    {
        if (entry.is_directory()) 
        {
            cout << string(level, '\t') << "<DIR> " << entry.path().filename().string() << " __________________]" << endl;
            searchDirectory(entry.path().string(), level + 1);

        }
        else 
        {
            cout << string(level, '\t') << "+-----------> " << entry.path().filename().string() << " (" << fs::file_size(entry) << std::dec << " bytes)" << endl;
        }
    }
 }

int main(int argc, char* argv[])
{
    DWORD serialNumber = 0;
    WCHAR volumeName[MAX_PATH + 1] = { 0 };
    WCHAR fileSystemName[MAX_PATH + 1] = { 0 };
    if (GetVolumeInformationW(
        L"C:\\",
        volumeName,
        MAX_PATH,
        &serialNumber,
        NULL,
        NULL,
        NULL,
        sizeof(fileSystemName)) == TRUE)
    {
        wprintf(L"Volume in drive C is: %ls\n", volumeName);
        std::cout << "Volume Serial Number is: " << std::hex
            << serialNumber << std::endl;
    }
    else
    {
        wprintf(L"GetVolumeInformation() failed, error %u\n", GetLastError());
    }

    _tprintf(TEXT("\nDirectory of %s\n\n"), argv[1]);

    bool showOwner = false;
    bool recursive = false;
    bool showAll = false;

    int dirArgIndex = 1;

    if (argc < 2 || argc > 3)
    {
        cout << "Invalid number of arguments. Please enter one or two arguments." << endl;
        return 1;
    }

    if (argc == 3)
    {
        string arg2 = argv[2];
        if (arg2 == "/a")
        {
            showAll = true;
            dirArgIndex = 2;
        }
        else if (arg2 == "/q")
        {
            showOwner = true;
            dirArgIndex = 2;
        }
        else if (arg2 == "/s")
        {
            recursive = true;
            dirArgIndex = 2;
        }
        else
        {
            cout << "Invalid second argument. Please enter one of the following: a/, q/, or s/." << endl;
            return 1;

        }
    }

    std::string path = std::string(argv[1]) + "\\*";

    WIN32_FIND_DATA FindFileData;
    LARGE_INTEGER filesize;
    HANDLE hFind = FindFirstFile(path.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::cerr << "FindFirstFile failed\n";
        return -1;
    }
    do
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            FILETIME ftCreate = FindFileData.ftCreationTime;
            SYSTEMTIME stUTC, stLocal;
            FileTimeToSystemTime(&ftCreate, &stUTC);
            printf("%02d/%02d/%d %02d:%02d   ", stUTC.wMonth, stUTC.wDay, stUTC.wYear, stUTC.wHour, stUTC.wMinute);
            _tprintf(TEXT("<DIR>                             %s\n"), FindFileData.cFileName);
        }

        else
        {
            FILETIME ftCreate = FindFileData.ftCreationTime;
            SYSTEMTIME stUTC, stLocal;
            FileTimeToSystemTime(&ftCreate, &stUTC);
            printf("%02d/%02d/%d %02d:%02d   ", stUTC.wMonth, stUTC.wDay, stUTC.wYear, stUTC.wHour, stUTC.wMinute);
            filesize.LowPart = FindFileData.nFileSizeLow;
            filesize.HighPart = FindFileData.nFileSizeHigh;
            _tprintf(TEXT("          %lld bytes              %s\n"), filesize.QuadPart, FindFileData.cFileName);
        }
        if (showOwner && !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::string filePath = std::string(argv[1]) + "\\" + std::string(FindFileData.cFileName);
            printFileOwner(filePath);
            _tprintf(_T("\n"));
        }
        if (recursive && !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::string path = argv[1];
            searchDirectory(path);
            _tprintf(_T("\n"));

            return 0;

        }
    } while (FindNextFile(hFind, &FindFileData));

    int file_count = 0;
    int dir_count = 0;
    uintmax_t total_size = 0;
    for (const auto& entry : fs::directory_iterator(argv[1]))
    {
        if (entry.is_regular_file()) {
            ++file_count;
            total_size += entry.file_size();
        }
        else if (entry.is_directory()) {
            ++dir_count;
        }
    }
    auto free_space = fs::space(argv[1]).free;
    
    printf(("                 Total files: %d\n"), file_count);
    
    printf(("                 Total directories: %d\n"), dir_count);
    
    printf(("                 Total size in bytes: %lld\n"), total_size);
    
    printf(("                 Free space in bytes: %lld\n"), free_space);


    FindClose(hFind);

    return 0;
}
