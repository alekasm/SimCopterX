#pragma once
#include "Instructions.h"
#include "PEINFO.h"
struct DetourMaster
{
	PEINFO info;
	DWORD current_location;
	DWORD base_location;

	DetourMaster(PEINFO info)
	{
		this->info = info;
		current_location = info.data_map[".detour"].VirtualAddress + 0x400000;
		base_location = current_location;
		current_location += 0x128; //Reserve space for variables
	}

	DWORD GetFileOffset(DWORD address)
	{
		//TODO this function is very inefficient
		for (auto it = info.data_map.begin(); it != info.data_map.end(); ++it)
		{
			DWORD start_address = (0x400000 + it->second.VirtualAddress);
			DWORD end_address = start_address + it->second.VirtualSize;
			if (address >= start_address && address <= end_address)
			{
				//printf("-     %x is in  %s\n", address, it->first.c_str());
				return (address - start_address) + it->second.RawDataPointer;
			}
		}
		printf("Failed to find section where address %x belongs\n", address);
		return NULL;
	}

	DWORD GetNextDetour()
	{
		return current_location;
	}

	void SetLastDetourSize(size_t size)
	{
		//Technically this includes instructions outside of the detour but who cares
		current_location += (size % 4) + size;
	}
	std::vector<Instructions> instructions;
};