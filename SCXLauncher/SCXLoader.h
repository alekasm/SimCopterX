#pragma once
#include <Windows.h>
#include "Shlwapi.h"
#include "shellapi.h"
#include <urlmon.h>
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

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Advapi32.lib" )
class SCXLoader
{
public:
	static bool VerifyInstallation();
	static bool LoadFiles();	
	static void CheckForUpdates();
	static int GetPatchedSCXVersion();
	static std::string GetSimCopterGameLocation();
	static bool CreatePatchedGame(std::string game_location);
	static bool StartSCX(int sleep_time, int resolution_mode, bool fullscreen);	
private:
	static bool InitializeGameData(std::string);
	static bool GetFileCompatability(std::string game_location);
};