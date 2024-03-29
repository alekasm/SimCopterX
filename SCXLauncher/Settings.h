#pragma once
#include "Registry.h"
#include "GameVersion.h"

#define REGISTRY_SUBKEY L"Software\\SimCopterX"
#define REGISTRY_PATCHEDHASH L"PatchedHash"
#define REGISTRY_GAMELOCATION L"GameLocation"
#define REGISTRY_PATCHERVER L"PatcherVersion"
#define REGISTRY_INSTALLED L"Installed"
#define REGISTRY_GAMEVER L"GameVersion"
#define REGISTRY_SCREENMODE L"ScreenMode"
#define REGISTRY_RESOLUTION L"Resolution"
#define REGISTRY_SLEEPTIME L"SleepTime"


struct PatchInfo
{
  std::wstring PatchedGameLocation;
  std::wstring PatchedGameHash;
  GameVersions PatchedGameVersion;
  bool PatchedGameIsInstalled = false;
  int PatcherVersion = -1;
  const bool IsPatched() const { return PatcherVersion > 0; };
};

struct SettingsInfo
{
  int ScreenMode = 0;
  int Resolution = 0;
  int SleepTime = 5;
};

struct Settings
{
  static const SettingsInfo GetSettingsInfo()
  {
    RegistryEntry re_screenmode(REGISTRY_SCREENMODE);
    RegistryEntry re_resolution(REGISTRY_RESOLUTION);
    RegistryEntry re_sleeptime(REGISTRY_SLEEPTIME);

    RegistryKey rkey;
    rkey.hKey = HKEY_CURRENT_USER;
    rkey.SubKey = REGISTRY_SUBKEY;
    SettingsInfo info;

    if (Registry::GetValue(rkey, re_screenmode))
    {
      info.ScreenMode = std::stoi(re_screenmode.Value->wstring);
    }

    if (Registry::GetValue(rkey, re_resolution))
    {
      info.Resolution = std::stoi(re_resolution.Value->wstring);
    }

    if (Registry::GetValue(rkey, re_sleeptime))
    {
      info.SleepTime = std::stoi(re_sleeptime.Value->wstring);
    }
    return info;
  }

  static BOOL SetSettingsInfo(const SettingsInfo& info)
  {
    RegistryKey rkey;
    rkey.hKey = HKEY_CURRENT_USER;
    rkey.SubKey = REGISTRY_SUBKEY;

    BOOL resultScreenMode =
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_SCREENMODE,
        new RegistryValue(info.ScreenMode)));

    BOOL resultResolution =
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_RESOLUTION,
        new RegistryValue(info.Resolution)));

    BOOL resultSleepTime =
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_SLEEPTIME,
        new RegistryValue(info.SleepTime)));

    return resultResolution && resultScreenMode && resultSleepTime;
  }


  static const PatchInfo GetPatchInfo()
  {
    RegistryEntry re_hash(REGISTRY_PATCHEDHASH);
    RegistryEntry re_location(REGISTRY_GAMELOCATION);
    RegistryEntry re_scxversion(REGISTRY_PATCHERVER);
    RegistryEntry re_installed(REGISTRY_INSTALLED);
    RegistryEntry re_gameversion(REGISTRY_GAMEVER);

    RegistryKey rkey;
    rkey.hKey = HKEY_CURRENT_USER;
    rkey.SubKey = REGISTRY_SUBKEY;
    PatchInfo info;

    if (Registry::GetValue(rkey, re_hash))
    {
      info.PatchedGameHash = re_hash.Value->wstring;
    }

    if (Registry::GetValue(rkey, re_location))
    {
      info.PatchedGameLocation = re_location.Value->wstring;
    }

    if (Registry::GetValue(rkey, re_scxversion))
    {
      info.PatcherVersion = std::stoi(re_scxversion.Value->wstring);
    }

    if (Registry::GetValue(rkey, re_installed))
    {
      info.PatchedGameIsInstalled = std::stoi(re_installed.Value->wstring);
    }

    if (Registry::GetValue(rkey, re_gameversion))
    {
      info.PatchedGameVersion = (GameVersions)std::stoi(re_gameversion.Value->wstring);
    }
    return info;
  }

  static BOOL SetPatchInfo(const PatchInfo& info)
  {
    RegistryKey rkey;
    rkey.hKey = HKEY_CURRENT_USER;
    rkey.SubKey = REGISTRY_SUBKEY;

    BOOL resultPatchedHash =
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_PATCHEDHASH,
        new RegistryValue(info.PatchedGameHash)));

    BOOL resultGameLocation = 
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_GAMELOCATION,
      new RegistryValue(info.PatchedGameLocation)));

    BOOL resultPatcherVersion =
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_PATCHERVER,
        new RegistryValue(info.PatcherVersion)));

    BOOL resultInstalled =
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_INSTALLED,
        new RegistryValue(info.PatchedGameIsInstalled)));

    BOOL resultGameVersion =
      Registry::SetValue(rkey, RegistryEntry(REGISTRY_GAMEVER,
        new RegistryValue(info.PatchedGameVersion)));

    return resultPatchedHash && resultGameLocation &&
           resultPatcherVersion && resultInstalled &&
           resultGameVersion;
  }

  static BOOL ClearPatchInfo()
  {
    RegistryKey rkey;
    rkey.hKey = HKEY_CURRENT_USER;
    rkey.SubKey = REGISTRY_SUBKEY;

    BOOL resultPatchedHash = Registry::DeleteValue(rkey, RegistryEntry(REGISTRY_PATCHEDHASH));
    BOOL resultGameLocation = Registry::DeleteValue(rkey, RegistryEntry(REGISTRY_GAMELOCATION));
    BOOL resultPatcherVersion = Registry::DeleteValue(rkey, RegistryEntry(REGISTRY_PATCHERVER));
    BOOL resultInstalled = Registry::DeleteValue(rkey, RegistryEntry(REGISTRY_INSTALLED));
    BOOL resultGameVersion = Registry::DeleteValue(rkey, RegistryEntry(REGISTRY_GAMEVER));

    return resultPatchedHash && resultGameLocation &&
      resultPatcherVersion && resultInstalled &&
      resultGameVersion;
  }

};