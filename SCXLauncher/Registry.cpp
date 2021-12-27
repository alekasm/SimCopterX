#include "Registry.h"
#include <winreg.h>

BOOL Registry::GetValue(const RegistryKey key, RegistryEntry& entry)
{
  HKEY hKey;
  DWORD disposition;
  LSTATUS status_createkeyex = RegCreateKeyExW(key.hKey, key.SubKey.c_str(),
    0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disposition);
  if (status_createkeyex != ERROR_SUCCESS)
  {
    printf("[Registry::SetValues] RegCreateKeyExW (%ls) = %d\n",
      key.SubKey.c_str(), status_createkeyex);
    return FALSE;
  }

  DWORD queryType;
  WCHAR queryData[256] = { 0 };
  DWORD cbData = sizeof(queryData) - 1;
  LSTATUS status_queryvalue = RegQueryValueExW(hKey,
    entry.Name.c_str(), NULL, &queryType,
    (LPBYTE)queryData, &cbData);
  if (status_queryvalue != ERROR_SUCCESS) return FALSE;

  const std::wstring wdata(queryData);
  if (entry.Value)
  {
    delete entry.Value;
    entry.Value = nullptr;
  }
  switch (queryType)
  {
  case REG_SZ:
    entry.Value = new RegistryValue(wdata);
    break;
  case REG_DWORD:
    DWORD value;
    memcpy(&value, queryData, sizeof(DWORD));
    entry.Value = new RegistryValue(value);
    break;
  }

  RegCloseKey(hKey);
  return TRUE;
}

BOOL DeleteValue(const RegistryKey key, const RegistryEntry& value)
{
  HKEY hKey;
  DWORD disposition;
  LSTATUS status_createkeyex = RegCreateKeyExW(key.hKey, key.SubKey.c_str(),
    0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disposition);
  if (status_createkeyex != ERROR_SUCCESS)
  {
    printf("[Registry::SetValues] RegCreateKeyExW (%ls) = %d\n",
      key.SubKey.c_str(), status_createkeyex);
    return FALSE;
  }
  BOOL result = RegDeleteKeyW(hKey, value.Name.c_str());
  RegCloseKey(hKey);
  return result;
}


BOOL Registry::SetValue(const RegistryKey key, const RegistryEntry& value)
{
  HKEY hKey;
  DWORD disposition;
  LSTATUS status_createkeyex = RegCreateKeyExW(key.hKey, key.SubKey.c_str(),
    0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disposition);
  if (status_createkeyex != ERROR_SUCCESS)
  {
    printf("[Registry::SetValues] RegCreateKeyExW (%ls) = %d\n",
      key.SubKey.c_str(), status_createkeyex);
    return FALSE;
  }

  LSTATUS status_setvalue = RegSetValueExW(hKey,
    value.Name.c_str(), NULL,
    value.Value->dwType,
    value.Value->Data,
    value.Value->Size);

  if (status_setvalue != ERROR_SUCCESS)
  {
    printf("Unable to set the registry key. LSTATUS=%d\n", status_setvalue);
    return FALSE;
  }  
  RegCloseKey(hKey);
  return TRUE;
}
