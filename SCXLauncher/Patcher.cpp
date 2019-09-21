#include "Patcher.h"

bool Patcher::Patch(PEINFO info, std::vector<Instructions> instructions, std::string exe_fname)
{	
	FILE* efile;
	int result = fopen_s(&efile, exe_fname.c_str(), "r+");

	if (efile == NULL)
	{
		OutputDebugString(std::string("Failed to load exe file: " + exe_fname + "\n").c_str());
		OutputDebugString(std::string("Reason: fopen_s returns error code: " + std::to_string(result) + "\n").c_str());
		return false;
	}

	unsigned int bytes_written = 0;
	for (Instructions is : instructions)
	{
		for (Instruction instruction : is.GetInstructions())
		{
			DWORD address = GetFileOffset(info, instruction.address);
			/*
			char buffer[256];
			sprintf_s(buffer, sizeof(buffer), "VA = %x, FO = %x, BYTE: %x \n", instruction.address, address, instruction.byte);
			OutputDebugString(buffer);
			*/
			fseek(efile, address, SEEK_SET);
			fprintf(efile, "%c", instruction.byte);
			bytes_written++;
		}
	}

	OutputDebugString(std::string("Total bytes patched: " + std::to_string(bytes_written) + "\n").c_str());

	int close_result = fclose(efile);
	OutputDebugString(std::string("Patched file closed successfully: " + close_result == 0 ? "true\n" : "false\n").c_str());
	return true;
}

DWORD Patcher::GetFileOffset(PEINFO info, DWORD address)
{
	//TODO this function is very inefficient
	for (auto it = info.data_map.begin(); it != info.data_map.end(); ++it)
	{
		DWORD start_address = it->second.RealVirtualAddress;
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

DWORD Patcher::align(DWORD size, DWORD align, DWORD addr)
{
	if (!(size % align))
		return addr + size;
	return addr + (size / align + 1) * align;
}

bool Patcher::CreateDetourSection(const char *filepath, PEINFO *info)
{
	const char* section_name = ".detour";
	DWORD section_size = 2048;

	HANDLE file = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		OutputDebugString(std::string("Invalid handle when attempting to check detour sections: \n" + std::string(filepath) + "\n").c_str());
		OutputDebugString(std::string("Reason: " + std::to_string(GetLastError()) + "\n").c_str());
		return false;
	}

	DWORD fileSize = GetFileSize(file, NULL);
	BYTE *pByte = new BYTE[fileSize];
	DWORD dw;
	ReadFile(file, pByte, fileSize, &dw, NULL);

	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)pByte;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
	{
		OutputDebugString("Error with IMAGE_DOS_SIGNATURE when creating detour section\n");
		return false;
	}

	PIMAGE_FILE_HEADER FH = (PIMAGE_FILE_HEADER)(pByte + dos->e_lfanew + sizeof(DWORD));
	PIMAGE_OPTIONAL_HEADER OH = (PIMAGE_OPTIONAL_HEADER)(pByte + dos->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
	PIMAGE_SECTION_HEADER SH = (PIMAGE_SECTION_HEADER)(pByte + dos->e_lfanew + sizeof(IMAGE_NT_HEADERS));

	bool detour_section_exists = false;
	OutputDebugString(std::string("Current total sections: " + std::to_string(FH->NumberOfSections) + "\n").c_str());
	for (unsigned int i = 0; i < FH->NumberOfSections; i++)
	{
		std::string name(reinterpret_cast<char const*>(SH[i].Name));
		info->data_map[name].VirtualAddress = SH[i].VirtualAddress;
		info->data_map[name].RealVirtualAddress = SH[i].VirtualAddress + WIN32_PE_ENTRY;
		info->data_map[name].RawDataPointer = SH[i].PointerToRawData;
		info->data_map[name].VirtualSize = SH[i].Misc.VirtualSize;
		if (name.compare(section_name) == 0)
			detour_section_exists = true;
	}

	if (detour_section_exists)
	{
		char buffer[256];
		snprintf(buffer, sizeof(buffer), ".detour section already exists at 0x%x\n", info->data_map[".detour"].VirtualAddress);
		OutputDebugString(std::string(buffer).c_str());
		CloseHandle(file);
		return true;
	}

	ZeroMemory(&SH[FH->NumberOfSections], sizeof(IMAGE_SECTION_HEADER));
	CopyMemory(&SH[FH->NumberOfSections].Name, section_name, 8);

	SH[FH->NumberOfSections].Misc.VirtualSize = align(section_size, OH->SectionAlignment, 0);
	SH[FH->NumberOfSections].VirtualAddress = align(SH[FH->NumberOfSections - 1].Misc.VirtualSize, OH->SectionAlignment, SH[FH->NumberOfSections - 1].VirtualAddress);
	SH[FH->NumberOfSections].SizeOfRawData = align(section_size, OH->FileAlignment, 0);
	SH[FH->NumberOfSections].PointerToRawData = align(SH[FH->NumberOfSections - 1].SizeOfRawData, OH->FileAlignment, SH[FH->NumberOfSections - 1].PointerToRawData);// right here ptr to raw data
	SH[FH->NumberOfSections].Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

	SetFilePointer(file, SH[FH->NumberOfSections].PointerToRawData + SH[FH->NumberOfSections].SizeOfRawData, NULL, FILE_BEGIN);
	SetEndOfFile(file);
	OH->SizeOfImage = SH[FH->NumberOfSections].VirtualAddress + SH[FH->NumberOfSections].Misc.VirtualSize;
	FH->NumberOfSections += 1;
	SetFilePointer(file, 0, NULL, FILE_BEGIN);
	WriteFile(file, pByte, fileSize, &dw, NULL);

	unsigned int new_size = FH->NumberOfSections - 1;	//Size not length
	OutputDebugString(std::string("Added .detour section, total sections: " + std::to_string(FH->NumberOfSections) + "\n").c_str());
	std::string name(reinterpret_cast<char const*>(SH[new_size].Name));
	info->data_map[name].VirtualAddress = SH[new_size].VirtualAddress;
	info->data_map[name].RawDataPointer = SH[new_size].PointerToRawData;
	info->data_map[name].VirtualSize = SH[new_size].Misc.VirtualSize;

	char buffer[256];
	sprintf_s(buffer, sizeof(buffer), "Created .detour section at 0x%x\n", info->data_map[".detour"].VirtualAddress);
	OutputDebugString(std::string(buffer).c_str());

	BOOL result = CloseHandle(file);
	if (!result)
	{
		OutputDebugString(std::string("Error closing the file, error code: " + std::to_string(GetLastError()) + "\n").c_str());
	}

	return true;
}