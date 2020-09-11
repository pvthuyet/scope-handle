#pragma once
#include <Windows.h>

namespace fibo
{
	struct CloseHandleDeleter
	{
		void operator()(HANDLE hdl) const
		{
			::CloseHandle(hdl);
		}
	};

	struct CloseFileDeleter
	{
		void operator()(FILE* file) const
		{
			::fclose(file);
		}
	};

	struct RegCloseKeyDeleter
	{
		void operator()(HKEY hdl) const
		{
			::RegCloseKey(hdl);
		}
	};
}