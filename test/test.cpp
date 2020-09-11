// scoped_handle.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "scope.hpp"
#include "handle_deleter.hpp"
#include <tchar.h>
#include <iostream>

using std::experimental::make_unique_resource_checked;
using namespace fibo;

int main()
{
    // Handle file
    {
        const TCHAR* strName = _T("data\\create_file.txt");
        auto unr = make_unique_resource_checked(CreateFile(strName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
            INVALID_HANDLE_VALUE, 
            CloseHandleDeleter{});
    }

    // Open file
    {
        FILE* fp = nullptr;
        const TCHAR* pFileName = _T("data\\open_file.txt");
        errno_t err = _tfopen_s(&fp, pFileName, _T("r"));
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
