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
	static bool Patch(std::vector<Instructions>, std::string exe_fname);
	static bool Patch(Instructions, std::string exe_fname);
	static void SetDetourMaster(DetourMaster* master);

private:
	static DWORD align(DWORD size, DWORD align, DWORD addr);
};
