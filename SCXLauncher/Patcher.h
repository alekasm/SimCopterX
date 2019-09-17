#pragma once
#include "PEINFO.h"
#include "DetourMaster.h"
#include <utility>
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <Windows.h>
#include <map>


class Patcher
{
public:
	static bool CreateDetourSection(const char *filepath, PEINFO *info);
	static bool Patch(PEINFO, std::vector<Instructions>, std::string exe_fname);
	static DWORD GetFileOffset(PEINFO info, DWORD address);

private:
	static DWORD align(DWORD size, DWORD align, DWORD addr);
};
