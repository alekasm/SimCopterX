#pragma once
#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <stdlib.h>
#include <string>
#include <map>
#include "Message.h"

typedef DWORD(CALLBACK* GetFileVersionInfoSizeExA2)(DWORD, LPCSTR, LPDWORD);
typedef DWORD(CALLBACK* GetFileVersionInfoExA2)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
typedef DWORD(CALLBACK* VerQueryValueA2)(LPCVOID, LPCSTR, LPVOID*, PUINT);
typedef std::map<std::string, std::string> StringFileInfoMap;

class FileVersion
{
public:
  MessageValue GetSCFileVersionInfo(LPCSTR, StringFileInfoMap&);

private:
  BOOL Initialize();
  GetFileVersionInfoSizeExA2 _GetFileVersionInfoSizeExA2;
  GetFileVersionInfoExA2 _GetFileVersionInfoExA2;
  VerQueryValueA2 _VerQueryValueA2;
  HINSTANCE hInst = NULL;
};