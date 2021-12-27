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
#include <filesystem>
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
  unsigned int sleep_time;
  unsigned int resolution_mode;
  bool fullscreen;
};

class SCXLoader
{
public:
  static void LoadSettings();
  static int GetPatchedSCXVersion();
  static bool CreatePatchedGame(std::string, SCXParameters);
  static bool InstallGame();
  static bool StartSCX(SCXParameters);
  static bool GetValidInstallation();
  static bool FixMaxisHelpViewer(std::filesystem::path);
  static constexpr unsigned int SCX_VERSION = 15;
private:
  static bool GetFileCompatability(std::wstring);
};

