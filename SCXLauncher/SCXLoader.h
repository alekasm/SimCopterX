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
#include "FileVersion.h"
#include "Message.h"
#include "GameData.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Advapi32.lib" )

struct SCXParameters
{
	bool verify_install;
	unsigned int sleep_time;
	unsigned int resolution_mode;
	bool fullscreen;
};

class SCXLoader
{
public:
	static bool LoadFiles();	
	static int GetPatchedSCXVersion();
	static bool CreatePatchedGame(std::string, SCXParameters);
	static bool StartSCX(SCXParameters);	
	static bool GetValidInstallation();
	static const unsigned int SCX_VERSION = 11;	
private:
	static bool GetFileCompatability(std::string);
};

