#pragma once
#include <Windows.h>
#include "Shlwapi.h"
#include "shellapi.h"
#include <fstream>
#include <vector>
#include <wincrypt.h>
#include <Windows.h>
#include <algorithm>
#include <regex>
#include "Patcher.h"
#include "GameData.h"
#include "FileVersion.h"
#include "Message.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Advapi32.lib" )
class SCXLoader
{
public:
	static bool LoadFiles();	
	static int GetPatchedSCXVersion();
	static std::string GetSimCopterGameLocation();
	static bool CreatePatchedGame(std::string, bool);
	static bool StartSCX(int sleep_time, int resolution_mode, bool fullscreen);	
	static bool GetValidInstallation();
	static const unsigned int SCX_VERSION = 7;
private:
	static bool InitializeGameData(std::string);
	static bool GetFileCompatability(std::string);
};