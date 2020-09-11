// scoped_handle.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "scope.hpp"
#include "handle_deleter.hpp"
#include <tchar.h>
#include <strsafe.h>

using std::experimental::make_unique_resource_checked;
using namespace fibo;

void test_file_obj();
void test_gdi_obj();
void test_reg_key();
int test_map_file();

int main()
{
    test_file_obj();
    test_gdi_obj();
    test_reg_key();
    test_map_file();
    return 0;
}

void test_file_obj()
{
    // Handle file
    {
        const TCHAR* strName = _T("data\\create_file.txt");
        auto unr = make_unique_resource_checked(CreateFile(strName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
            INVALID_HANDLE_VALUE,
            CloseHandleDeleter{});
        if (INVALID_HANDLE_VALUE != unr.get())
        {
            // do something
        }
    }

    // Open file
    {
        FILE* fp = nullptr;
        const TCHAR* strName = _T("data\\open_file.txt");
        errno_t err = _tfopen_s(&fp, strName, _T("r"));
        if (0 == err)
        {
            auto unr = make_unique_resource_checked(fp, nullptr, CloseFileDeleter{});
            int c; // note: int, not char, required to handle EOF
            while ((c = fgetc(fp)) != EOF)
            {
                std::wcout << (TCHAR)c;
            }
        }
    }
}

void test_gdi_obj()
{
    auto unr = make_unique_resource_checked(CreatePen(PS_SOLID, 3, RGB(50, 50, 50)),
        (HPEN)NULL,
        DeleteObjectDeleter{});
    if (unr.get())
    {
        // Do something
    }
}

void test_reg_key()
{
    {
        HKEY hKey = nullptr;
        LSTATUS lResult = RegCreateKey(HKEY_CURRENT_USER, TEXT("Software\\TestDir"), &hKey);
        if (ERROR_SUCCESS == lResult)
        {
            auto unr = make_unique_resource_checked(hKey, nullptr, RegCloseKeyDeleter{});
        }
    }
    RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\TestDir"));
}


#define BUFFSIZE 1024 // size of the memory to examine at any one time
#define FILE_MAP_START 138240 // starting point within the file of the data to examine (135K)
int test_map_file()
{
    const TCHAR* lpcTheFile = TEXT("data\\open_file.txt"); // the file to be manipulated
    HANDLE hMapFile;      // handle for the file's memory-mapped region
    HANDLE hFile;         // the file handle
    BOOL bFlag;           // a result holder
    DWORD dBytesWritten;  // number of bytes written
    DWORD dwFileSize;     // temporary storage for file sizes
    DWORD dwFileMapSize;  // size of the file mapping
    DWORD dwMapViewSize;  // the size of the view
    DWORD dwFileMapStart; // where to start the file map view
    DWORD dwSysGran;      // system allocation granularity
    SYSTEM_INFO SysInfo;  // system information; used to get granularity
    LPVOID lpMapAddress;  // pointer to the base address of the
                          // memory-mapped region
    char* pData;         // pointer to the data
    int i;                // loop counter
    int iData;            // on success contains the first int of data
    int iViewDelta;       // the offset into the view where the data
                          //shows up

    // Create the test file. Open it "Create Always" to overwrite any
    // existing file. The data is re-created below
    hFile = CreateFile(lpcTheFile,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        _tprintf(TEXT("hFile is NULL\n"));
        _tprintf(TEXT("Target file is %s\n"),
            lpcTheFile);
        return 4;
    }

    auto unr = make_unique_resource_checked(hFile, INVALID_HANDLE_VALUE, CloseHandleDeleter{});
    {

        // Get the system allocation granularity.
        GetSystemInfo(&SysInfo);
        dwSysGran = SysInfo.dwAllocationGranularity;

        // Now calculate a few variables. Calculate the file offsets as
        // 64-bit values, and then get the low-order 32 bits for the
        // function calls.

        // To calculate where to start the file mapping, round down the
        // offset of the data into the file to the nearest multiple of the
        // system allocation granularity.
        dwFileMapStart = (FILE_MAP_START / dwSysGran) * dwSysGran;
        _tprintf(TEXT("The file map view starts at %ld bytes into the file.\n"),
            dwFileMapStart);

        // Calculate the size of the file mapping view.
        dwMapViewSize = (FILE_MAP_START % dwSysGran) + BUFFSIZE;
        _tprintf(TEXT("The file map view is %ld bytes large.\n"),
            dwMapViewSize);

        // How large will the file mapping object be?
        dwFileMapSize = FILE_MAP_START + BUFFSIZE;
        _tprintf(TEXT("The file mapping object is %ld bytes large.\n"),
            dwFileMapSize);

        // The data of interest isn't at the beginning of the
        // view, so determine how far into the view to set the pointer.
        iViewDelta = FILE_MAP_START - dwFileMapStart;
        _tprintf(TEXT("The data is %d bytes into the view.\n"),
            iViewDelta);

        // Now write a file with data suitable for experimentation. This
        // provides unique int (4-byte) offsets in the file for easy visual
        // inspection. Note that this code does not check for storage
        // medium overflow or other errors, which production code should
        // do. Because an int is 4 bytes, the value at the pointer to the
        // data should be one quarter of the desired offset into the file

        for (i = 0; i < (int)dwSysGran; i++)
        {
            WriteFile(hFile, &i, sizeof(i), &dBytesWritten, NULL);
        }

        // Verify that the correct file size was written.
        dwFileSize = GetFileSize(hFile, NULL);
        _tprintf(TEXT("hFile size: %10d\n"), dwFileSize);

        // Create a file mapping object for the file
        // Note that it is a good idea to ensure the file size is not zero
        hMapFile = CreateFileMapping(hFile,          // current file handle
            NULL,           // default security
            PAGE_READWRITE, // read/write permission
            0,              // size of mapping object, high
            dwFileMapSize,  // size of mapping object, low
            NULL);          // name of mapping object

        if (hMapFile == NULL)
        {
            _tprintf(TEXT("hMapFile is NULL: last error: %d\n"), GetLastError());
            return (2);
        }

        auto unfilemapping = make_unique_resource_checked(hMapFile, (HANDLE)NULL, CloseHandleDeleter{});
        {
            // Map the view and test the results.

            lpMapAddress = MapViewOfFile(hMapFile,            // handle to
                                                              // mapping object
                FILE_MAP_ALL_ACCESS, // read/write
                0,                   // high-order 32
                                     // bits of file
                                     // offset
                dwFileMapStart,      // low-order 32
                                     // bits of file
                                     // offset
                dwMapViewSize);      // number of bytes
                                     // to map
            if (lpMapAddress == NULL)
            {
                _tprintf(TEXT("lpMapAddress is NULL: last error: %d\n"), GetLastError());
                return 3;
            }

            auto unviewfile = make_unique_resource_checked(lpMapAddress, (LPVOID)NULL, UnmapViewOfFileDeleter{});
            {
                // Calculate the pointer to the data.
                pData = (char*)lpMapAddress + iViewDelta;

                // Extract the data, an int. Cast the pointer pData from a "pointer
                // to char" to a "pointer to int" to get the whole thing
                iData = *(int*)pData;

                _tprintf(TEXT("The value at the pointer is %d,\nwhich %s one quarter of the desired file offset.\n"),
                    iData,
                    iData * 4 == FILE_MAP_START ? TEXT("is") : TEXT("is not"));
            }
            // Close the file mapping object and the open file
            //bFlag = UnmapViewOfFile(lpMapAddress);
        }
        //bFlag = CloseHandle(hMapFile); // close the file mapping object
    }
    //bFlag = CloseHandle(hFile);   // close the file itself

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
