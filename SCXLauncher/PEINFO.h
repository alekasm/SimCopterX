#pragma once
#include <Windows.h>
#include <map>

struct PEINFO
{
	struct PEDATA {
		DWORD VirtualAddress;
		DWORD RawDataPointer;
		DWORD VirtualSize;
	};
	std::map<std::string, PEDATA> data_map;
};