#pragma once
#include <Windows.h>
#include <map>
#include <string>

static const DWORD WIN32_PE_ENTRY = 0x400000;
struct PEINFO
{
	struct PEDATA {
		DWORD RealVirtualAddress;
		DWORD VirtualAddress;
		DWORD RawDataPointer;
		DWORD VirtualSize;
	};

	DWORD GetDetourVirtualAddress(DWORD offset = 0x0)
	{
		return data_map[".detour"].RealVirtualAddress + offset;
	}

	std::map<std::string, PEDATA> data_map;
};