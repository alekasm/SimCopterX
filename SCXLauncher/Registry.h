#pragma once
#include <windows.h>
#include <string>
#include <minwindef.h>

struct RegistryValue {
  const DWORD dwType;
  const DWORD Size;
  const std::wstring wstring;
  LPBYTE Data;

  explicit RegistryValue(const std::wstring& value) :
    dwType(REG_SZ), Size((value.size() + 1) * sizeof(wchar_t)),
    wstring(value)
  {
    Data = (LPBYTE)malloc(Size);
    if (Data == nullptr) throw std::bad_alloc();
    memcpy(Data, value.c_str(), Size);
  }

  explicit RegistryValue(const DWORD& value) :
    dwType(REG_DWORD), Size(sizeof(DWORD)),
    wstring(std::to_wstring(value))
  {
    Data = (LPBYTE)malloc(Size);
    if (Data == nullptr) throw std::bad_alloc();
    memcpy(Data, &value, Size);
  }

  RegistryValue(const RegistryValue& v) :
    dwType(v.dwType), Size(v.Size), wstring(v.wstring)
  {
    Data = (LPBYTE)malloc(Size);
    if (Data == nullptr) throw std::bad_alloc();
    memcpy(Data, v.Data, Size);
  }

  ~RegistryValue()
  {
    free(Data);
    Data = nullptr;
  }

};

struct RegistryEntry {
  const std::wstring Name;
  RegistryValue* Value;
  RegistryEntry(std::wstring Name, RegistryValue* Value) :
    Name(Name), Value(Value)
  {
  }
  RegistryEntry(std::wstring Name) : 
    Name(Name), Value(nullptr)
  {
  }
  RegistryEntry() = delete;
  ~RegistryEntry()
  {
    delete Value;
    Value = nullptr;
  }
};

struct RegistryKey {
  HKEY hKey;
  std::wstring SubKey;
  RegistryKey(HKEY hKey, std::wstring SubKey)
  {
    this->hKey = hKey;
    this->SubKey = SubKey;
  }
  RegistryKey() = default;
};

namespace Registry {
  BOOL SetValue(const RegistryKey, const RegistryEntry&);
  BOOL GetValue(const RegistryKey, RegistryEntry&);
  BOOL DeleteValue(const RegistryKey, const RegistryEntry&);
}