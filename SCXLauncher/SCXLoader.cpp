#include "SCXLoader.h"
#include "Settings.h"

std::string LastErrorString();
MessageValue CreateMD5Hash(std::wstring);
MessageValue VerifyOriginalGame(std::string, GameVersions&);
MessageValue VerifyInstallationDirectory(std::wstring, std::filesystem::path&);
MessageValue VerifyInstallation(GameVersions version, std::filesystem::path);
MessageValue CopyFileSafe(std::string source, std::string destination);

namespace
{
  const std::string GAME_FILE = "SimCopter.exe";
  const std::string BACKUP_FILE = "SimCopter(Backup).exe";
  const std::string PATCH_NAME = "SimCopterX";
  const std::wstring PATCH_NAMEW = L"SimCopterX";
  const std::string GAME_NAME = "SimCopter";

  std::map<std::string, GameVersions> version_hashes =
  {
    {"6bc646d182ab8625a0d2394112334005", GameVersions::VCLASSICS},
    {"90db54003aa9ba881543c9d2cd0dbfbf", GameVersions::V11SC},
    {"d2f5c5eca71075696964d0f91b1163bf", GameVersions::V102_PATCH},
    {"b296b26e922bc43705b49f7414d7218f", GameVersions::V11SC_FR},
    {"17d5eba3e604229c4b87a68f20520b56", GameVersions::ORIGINAL},
    {"1ea2ece4cf9b4e0ed3da217a31426795", GameVersions::V10_JP}
  };

  //The advertisted dates in techtips is wrong
  std::map<GameVersions, std::string> version_description =
  {
    {GameVersions::VCLASSICS, "Classics Version (1.0.1.4) - 1 April 1997"},
    {GameVersions::V11SC, "Version 1.1SC (1.0.1.0) - 8 December 1996"},
    {GameVersions::ORIGINAL, "Version 1.0 (1.0.0.0) - 14 November 1996"},
    {GameVersions::V102_PATCH, "Version 1.02 Patch (1.0.1.3) - 26 February 1997"},
    {GameVersions::V11SC_FR, "Version 1.1SC (French) (1.0.1.0) - 9 December 1996"},
    {GameVersions::V10_JP, "Version 1.0 (Japanese) (1.0.0.0) - 7 December 1996"}
  };
  FileVersion fileVersion;
  PatchInfo patch_info; 
}

bool SCXLoader::GetValidInstallation()
{
  return patch_info.PatchedGameIsInstalled;
}

int SCXLoader::GetPatchedSCXVersion()
{
  return patch_info.PatcherVersion;
}

bool SCXLoader::GetFileCompatability(std::wstring game_location)
{
  RegistryKey rkey;
  rkey.hKey = HKEY_CURRENT_USER;
  rkey.SubKey = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";

  HKEY hKey;
  LONG lRes = RegOpenKeyExW(rkey.hKey, rkey.SubKey.c_str(), 0, KEY_READ, &hKey);
  WCHAR lpData[512];
  DWORD lpcbData = sizeof(lpData);
  std::wstring format_location(game_location);
  std::replace(format_location.begin(), format_location.end(), '/', '\\');
  LSTATUS status = RegQueryValueExW(hKey, format_location.c_str(), 0, NULL, (LPBYTE)lpData, &lpcbData);
  if (status != ERROR_SUCCESS)
  {
    OutputDebugString(std::string("Registry Error: " + std::to_string((int)status) + "\n").c_str());
    OutputDebugStringW(std::wstring(format_location + L"\n").c_str());
    return false;
  }

  if (std::wstring(lpData).find(L"~ 256COLOR") == std::wstring::npos)
  {
    return Registry::SetValue(rkey, RegistryEntry(format_location,
     new RegistryValue(L"~ 256COLOR")));
  }
  return true;
}

MessageValue VerifyOriginalGame(std::string source, GameVersions& version)
{
  if (!PathFileExistsA(source.c_str()))
  {
    return MessageValue(FALSE, "The game does not exist at " + source + "\n");
  }

  StringFileInfoMap sfiMap;
  MessageValue result = fileVersion.GetSCFileVersionInfo(source.c_str(), sfiMap);
  std::string sfi_result = "";
  if (result.Value)
  {
    sfi_result += "\nFile Information\n";
    for (auto sfit = sfiMap.begin(); sfit != sfiMap.end(); sfit++)
    {
      sfi_result += sfit->first + ": " + sfit->second + "\n";
    }
  }
  else
  {
    sfi_result += "\nUnable to query the SimCopter.exe File Information because: \n";
    sfi_result += result.Message;
  }

  MessageValue hash_check = CreateMD5Hash(std::wstring(source.begin(), source.end()));
  auto it = version_hashes.find(hash_check.Message);
  if (!hash_check.Value || it == version_hashes.end())
  {
    std::string reason;
    if (hash_check.Value)
    {
      reason = PATCH_NAME + " does not have a matching hash stored in the database of patched versions.\n";
      reason += "Hash: " + hash_check.Message + "\n";
    }
    else
    {
      reason = hash_check.Message + "\n";
    }
    reason += sfi_result;
    return MessageValue(FALSE, reason);
  }

  version = it->second;
  return MessageValue(TRUE, sfi_result);
}

bool CreatePatchFile()
{
  MessageValue hash_check = CreateMD5Hash(patch_info.PatchedGameLocation);
  if (!hash_check.Value)
  {
    ShowMessage(PATCH_NAME + " Error", hash_check.Message);
    return false;
  }
  patch_info.PatchedGameHash = std::wstring(hash_check.Message.begin(), hash_check.Message.end());
  return Settings::SetPatchInfo(patch_info);
}

bool SCXLoader::InstallGame()
{
  if (patch_info.PatcherVersion < 1 || patch_info.PatchedGameLocation.empty())
  {
    return false;
  }

  MessageValue msg_verify;
  std::filesystem::path install_dir;
  if (!(msg_verify = VerifyInstallationDirectory(patch_info.PatchedGameLocation, install_dir)).Value)
  {
    ShowMessage(PATCH_NAME + " Installation", msg_verify.Message);
    return false;
  }

  MessageValue msg_install;
  if (!(msg_install = VerifyInstallation(patch_info.PatchedGameVersion, install_dir)).Value)
  {
    ShowMessage(PATCH_NAME + " Installation", msg_install.Message);
    return false;
  }

  patch_info.PatchedGameIsInstalled = true;
  return Settings::SetPatchInfo(patch_info);
}

bool SCXLoader::CreatePatchedGame(std::string game_location, SCXParameters params)
{

  if (game_location.empty()) //Means nothing was selected
    return false;

  /*
  If the game we selected is not an original, check to see if our directory has an original.
  If the game we selected is an original, copy it to our SimCopterX directory
  */
  bool using_backup_copy = false;
  MessageValue verifyCurrentValue;
  GameVersions version;

  std::filesystem::path exe_path;
  std::filesystem::path backup_exe_path;
  std::filesystem::path exe_parent_path;


  exe_path = std::filesystem::path(game_location);
  try
  {
    exe_path = std::filesystem::canonical(exe_path);
    exe_parent_path = exe_path.parent_path();
    backup_exe_path = exe_parent_path;
    backup_exe_path.append(BACKUP_FILE);
  }
  catch (const std::exception& e)
  {
    printf("%s\n", e.what());
    return false;
  }

  if (!(verifyCurrentValue = VerifyOriginalGame(exe_path.string(), version)).Value)
  {
    if (!VerifyOriginalGame(backup_exe_path.string(), version).Value)
    {
      std::string message_body = "The SimCopter game you selected isn't supported or is already modified/patched. " + PATCH_NAME + " ";
      message_body += "could not find an original backup. If this is an official version of the game, please submit the following ";
      message_body += "file information so it can be supported.\n\n";
      message_body += verifyCurrentValue.Message;
      ShowMessage(PATCH_NAME + " Error", message_body);
      return false;
    }
    MessageValue copy_result = CopyFileSafe(backup_exe_path.string(), exe_path.string());
    if (!copy_result.Value)
    {
      ShowMessage(LastErrorString(), copy_result.Message);
      return false;
    }
    using_backup_copy = true;
  }
  else
  {
    MessageValue copy_result = CopyFileSafe(exe_path.string(), backup_exe_path.string());
    if (!copy_result.Value)
    {
      ShowMessage(LastErrorString(), copy_result.Message);
      return false;
    }
  }

  PEINFO info;
  if (!Patcher::CreateDetourSection(exe_path.string().c_str(), &info))
  {
    ShowMessage(PATCH_NAME + " Patch Failed", "Failed to modify the game's executable file.\n Make sure the game isn't running or opened in another application");
    return false;
  }

  std::vector<Instructions> instructions = GameData::GenerateData(info, version);
  DWORD sleep_address = info.GetDetourVirtualAddress(DetourOffsetType::MY_SLEEP);
  DWORD res_address = Versions[version]->data.RES_TYPE;
  instructions.push_back(DataValue(sleep_address, BYTE(params.sleep_time)));
  instructions.push_back(DataValue(res_address, BYTE(params.resolution_mode)));

  if (!Patcher::Patch(info, instructions, exe_path.string()))
  {
    ShowMessage(PATCH_NAME + " Patch Failed", "Failed to patch the game file.\n Make sure the game isn't running or opened in another application");
    return false;
  }

  MessageValue hash_check = CreateMD5Hash(exe_path.wstring());
  if (!hash_check.Value)
  {
    ShowMessage(PATCH_NAME + " Error", hash_check.Message);
    return false;
  }

  patch_info.PatchedGameHash = std::wstring(hash_check.Message.begin(), hash_check.Message.end());
  patch_info.PatchedGameLocation = exe_path.wstring();
  patch_info.PatcherVersion = SCX_VERSION;
  patch_info.PatchedGameVersion = version;
  bool reg_result = Settings::SetPatchInfo(patch_info);

  std::string message = "";
  if (using_backup_copy)
    message += "Used Backup:\n" + version_description[version] + "\n\n";
  else
    message += "Detected Version:\n" + version_description[version] + "\n\n";

  message += "Patch location: \n" + game_location + "\n\n";
  if (reg_result)
    message += "Patch was successful!\nIf you move or modify this file, then you cannot install.\n";
  else
    message += "Patch was successful!\nThe patch info couldn't be stored to your registry.\n"
               "You will not be able to install the game.\n";

  ShowMessage(PATCH_NAME + " Patch Successful!", message);
  return true;
}

void ClearPatchInfo(std::wstring reason)
{
  std::string title = PATCH_NAME + " Error";
  ShowMessage(std::wstring(title.begin(), title.end()), reason);
  patch_info = PatchInfo();
  Settings::ClearPatchInfo();
}

MessageValue VerifyInstallationDirectory(std::wstring game_location, std::filesystem::path& install_dir)
{
  std::filesystem::path exe_path;
  std::filesystem::path root_path;
  std::filesystem::path autorun_path;
  try
  {
    exe_path = std::filesystem::path(game_location);
    root_path = exe_path.parent_path().parent_path();
    root_path = std::filesystem::canonical(root_path);
    autorun_path = root_path;
    autorun_path.append("autorun.inf");
  }
  catch (const std::exception& e)
  {
    return MessageValue(FALSE, "The game does not appear to be on a valid extracted CD");
    printf("%s\n", e.what());
  }

  OutputDebugString(std::string("Checking: " + autorun_path.string() + "\n").c_str());
  if (!PathFileExistsA(autorun_path.string().c_str()))
  {
    return MessageValue(FALSE, "The game does not appear to be on a valid extracted CD, missing autorun.inf");
  }
  install_dir = root_path;
  return MessageValue(TRUE);
}


bool VerifyPatchedGame()
{
  if (!PathFileExistsW(patch_info.PatchedGameLocation.c_str()))
  {
    ClearPatchInfo(std::wstring(L"The game no longer exists where we patched it:\n" + 
      patch_info.PatchedGameLocation + L"\nPlease try repatching.").c_str());
    return false;
  }

  MessageValue hash_check = CreateMD5Hash(patch_info.PatchedGameLocation);
  if (!hash_check.Value)
  {
    ShowMessage(PATCH_NAME + " Error", hash_check.Message);
    return false;
  }

  std::wstring hash_checkw(hash_check.Message.begin(), hash_check.Message.end());

  if (hash_checkw.compare(patch_info.PatchedGameHash) != 0)
  {
    ClearPatchInfo(L"The patched game doesn't have a matching hash, this can happen"
                    "if you modified the patched game or restored it back to the"
                    "original game. Please try repatching.");
    return false;
  }

  if (patch_info.PatcherVersion != SCXLoader::SCX_VERSION)
  {
    ClearPatchInfo(std::wstring(L"You currently have " + PATCH_NAMEW + 
      L" Version " + std::to_wstring(SCXLoader::SCX_VERSION) +
      L" however the game was \npreviously patched using Version " +
      std::to_wstring(patch_info.PatcherVersion) + L". Please repatch the game."));
    return false;
  }

  return true;
}

bool SCXLoader::StartSCX(SCXParameters params)
{

  if (patch_info.PatcherVersion < 0)
  { //This shouldn't happen because the button should not be enabled
    ShowMessage(PATCH_NAME + " Error", "You need to patch the game before starting.");
    return false;
  }

  if (!patch_info.PatchedGameIsInstalled) 
  { //This shouldn't happen because the button should not be enabled
    ShowMessage(PATCH_NAME + " Error", "You need to patch the game using 'Verify Install'");
    return false;
  }

  if (!VerifyPatchedGame())
  {
    return false;
  }

  if (!params.fullscreen && !GetFileCompatability(patch_info.PatchedGameLocation))
  {
    const char* message =
      "You must run the SimCopter.exe in 8-bit color to use Windowed mode!\n\n"
      "1. Right Click SimCopter.exe -> Properties\n"
      "2. Select the 'Compatibility' tab\n"
      "3. Enable 'Reduced color mode' and select 8-bit/256 color\n"
      "4. Ensure all other options are NOT selected\n"
      "5. Click apply, then try again";
    ShowMessage(PATCH_NAME, std::string(message));
    return false;
  }

  std::string game_location = std::filesystem::path(patch_info.PatchedGameLocation).string();
  PEINFO info;
  if (!Patcher::CreateDetourSection(game_location.c_str(), &info))
  { //Should grab detour section
    ShowMessage(PATCH_NAME + " Patch Failed", "Failed to modify the game's executable file.\n Make sure the game isn't running or opened in another application");
    return false;
  }

  std::vector<Instructions> instructions;
  DWORD sleep_address = info.GetDetourVirtualAddress(DetourOffsetType::MY_SLEEP);
  DWORD res_address = Versions[patch_info.PatchedGameVersion]->data.RES_TYPE;
  instructions.push_back(DataValue(sleep_address, BYTE(params.sleep_time)));
  instructions.push_back(DataValue(res_address, BYTE(params.resolution_mode)));

  if (!Patcher::Patch(info, instructions, game_location.c_str()))
  {
    ShowMessage(PATCH_NAME + " Patch Failed", "Failed to patch the game file.\n Make sure the game isn't running or opened in another application");
    return false;
  }

  //Can only get the hash at this point as a reference, can't use it to check for complete validty
  //because changing sleep time and resolution mode dwords will change the hash

  CreatePatchFile();

  std::string parameters = params.fullscreen ? "-f" : "-w";
  HINSTANCE hInstance = ShellExecuteA(NULL, "open", game_location.c_str(), parameters.c_str(), NULL, SW_SHOWDEFAULT);
  int h_result = reinterpret_cast<int>(hInstance);
  if (h_result <= 31)
  {
    ShowMessage(std::string(PATCH_NAME + " Error (" + std::to_string(h_result) + ")"), std::string("Failed to start the patched game at: \n" + game_location));
    return false;
  }
  return true;
}

bool SCXLoader::FixMaxisHelpViewer(std::filesystem::path path)
{
  std::string webste32 = path.string() + "/setup/webste32/";
  std::vector<std::string> webste_files = { "regsvr32.exe", "webster.ocx" };
  for (std::string file : webste_files)
  {
    if (!PathFileExistsA(std::string(webste32 + file).c_str()))
    {
      return false;
    }
  }
  std::string executable = webste32 + "regsvr32.exe";
  std::string parameters = "/s /i " + webste32 + "webster.ocx";
  HINSTANCE hInstance = ShellExecuteA(NULL, "open", executable.c_str(), parameters.c_str(), NULL, SW_HIDE);
  return reinterpret_cast<int>(hInstance) > 32;
}

void SCXLoader::LoadSettings()
{
  patch_info = Settings::GetPatchInfo();
  if (patch_info.PatcherVersion > 0)
  {
    VerifyPatchedGame();
  }
}

MessageValue CreateMD5Hash(std::wstring filename_wstring)
{
  //LPCWSTR filename = filename_wstring.c_str();

  DWORD cbHash = 16;
  HCRYPTHASH hHash = 0;
  HCRYPTPROV hProv = 0;
  BYTE rgbHash[16];
  CHAR rgbDigits[] = "0123456789abcdef";
  HANDLE hFile = CreateFileW(filename_wstring.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  int v = GetLastError();
  printf("%d\n", v);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    std::string error_message = "Failed to retrieve the MD5 Hash of the program:\n";
    error_message += "CreateFileW has an invalid handle.\n";
    return MessageValue(FALSE, error_message);
  }

  CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);

  BOOL bResult = FALSE;
  DWORD BUFSIZE = 4096;
  BYTE rgbFile[4096];
  DWORD cbRead = 0;
  while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
  {
    if (0 == cbRead)
      break;

    CryptHashData(hHash, rgbFile, cbRead, 0);
  }

  std::string md5_hash = "";
  if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
  {
    for (DWORD i = 0; i < cbHash; i++)
    {
      char buffer[3]; //buffer needs terminating null
      sprintf_s(buffer, 3, "%c%c", rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]);
      md5_hash.append(buffer);
    }
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);
    return MessageValue(TRUE, md5_hash);
  }
  else
  {
    CloseHandle(hFile);
    std::string error_message = "Failed to retrieve the MD5 Hash of the program:\n";
    error_message += "CryptGetHashParam returned false.\n";
    return MessageValue(FALSE, error_message);
  }
}

MessageValue VerifyInstallation(GameVersions version, std::filesystem::path install_dir)
{

  std::filesystem::path game_dir = std::filesystem::path(patch_info.PatchedGameLocation);
  try
  {
    game_dir = game_dir.parent_path();
    game_dir = std::filesystem::canonical(game_dir);
  }
  catch (const std::exception& e)
  {
    return MessageValue(FALSE, std::string("Failed to copy:\n") + e.what());
  }

  std::map<std::string, std::string> dll_source =
  {
    {"smackw32.dll", "setup/smacker/"},
    {"sst1init.dll", "setup/system/"},
    {"glide.dll", "setup/system/"}
  };

  std::map<GameVersions, std::vector<std::string>> dll_map =
  {
    { GameVersions::ORIGINAL,	{"smackw32.dll"}},
    { GameVersions::V10_JP,	  {"smackw32.dll"}},
    { GameVersions::V11SC,		{"smackw32.dll"}},
    { GameVersions::V11SC_FR,	{"smackw32.dll"}},
    { GameVersions::V102_PATCH, {"sst1init.dll", "glide.dll", "smackw32.dll"}},
    { GameVersions::VCLASSICS,  {"sst1init.dll", "glide.dll", "smackw32.dll"}}
  };

  for (std::string dll : dll_map[version])
  {
    std::filesystem::path dll_path = install_dir;
    std::filesystem::path dll_dest = game_dir;
    try
    {      
      dll_path.append(dll_source[dll] + dll);
      dll_dest.append(dll);
      dll_path = std::filesystem::canonical(dll_path);
    }
    catch (const std::exception& e)
    {
      return MessageValue(FALSE, std::string("Failed to copy:\n") + e.what());
    }


    if (!PathFileExistsA(dll_path.string().c_str()))
    {
      std::string message = "Your game version is: \n" + version_description[version];
      message += "\n\nWe could not find the required dll: " + dll + "\nExpected location:\n";
      message += dll_path.string() + "\n\n";
      message += "This usually happens if you are trying to mismatch game versions with different CDs. ";
      message += "Try using the 'Classics CD', it should have all the dlls so you can use any game version.\n";
      return MessageValue(FALSE, message);
    }

    MessageValue copy_result = CopyFileSafe(dll_path.string(), dll_dest.string());
    if (!copy_result.Value)
    {
      return copy_result;
    }
  }
  SCXLoader::FixMaxisHelpViewer(install_dir);

  return MessageValue(TRUE);
}

MessageValue CopyFileSafe(std::string source, std::string destination)
{
  DWORD attributes = GetFileAttributes(source.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES)
  {
    return MessageValue(FALSE, "Failed to GET file attributes for:\n" + source + "\n");
  }

  if (attributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN))
  {
    if (SetFileAttributes(source.c_str(), FILE_ATTRIBUTE_NORMAL) == 0)
    {
      return MessageValue(FALSE, "Failed to SET(" + std::to_string(GetLastError()) +
        ") file attributes for:\n" + source + "\n");
    }
    OutputDebugString(std::string("Reset file attributes for: " + source + "\n").c_str());
  }

  if (!CopyFileA(source.c_str(), destination.c_str(), FALSE))
  {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "Failed to copy:\n%s\n\nDestination:\n%s\n\nError Code: %d\n\n",
      source.c_str(), destination.c_str(), GetLastError());
    return MessageValue(FALSE, std::string(buffer));
  }
  OutputDebugString(std::string("Copied " + source + " to: " + destination + "\n").c_str());
  return MessageValue(TRUE);
}

