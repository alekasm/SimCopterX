#include "FileVersion.h"

BOOL FileVersion::Initialize()
{
	if (hInst)
		return TRUE;

	//Version.lib does not contain GetFileVersionInfoSizeExA or GetFileVersionInfoExA, must be 
	//retrieved from Version.dll. All functions are now instead loaded from dll, including
	//VerQueryValueA
	if ((hInst = LoadLibrary("Version.dll")) == NULL)
	{
		return FALSE;
	}

	FARPROC _GetFileVersionInfoSizeExA2Address = GetProcAddress(hInst, "GetFileVersionInfoSizeExA");
	FARPROC _GetFileVersionInfoExA2Address = GetProcAddress(hInst, "GetFileVersionInfoExA");
	FARPROC _VerQueryValueA2Address = GetProcAddress(hInst, "VerQueryValueA");

	if (_GetFileVersionInfoSizeExA2Address == NULL ||
		_GetFileVersionInfoExA2Address  == NULL ||
		_VerQueryValueA2Address == NULL)
	{
		return FALSE;
	}

	_GetFileVersionInfoSizeExA2 = (GetFileVersionInfoSizeExA2)_GetFileVersionInfoSizeExA2Address;
	_GetFileVersionInfoExA2 = (GetFileVersionInfoExA2)_GetFileVersionInfoExA2Address;
	_VerQueryValueA2 = (VerQueryValueA2)_VerQueryValueA2Address;

	return TRUE;
}

MessageValue FileVersion::GetSCFileVersionInfo(LPCSTR filename, StringFileInfoMap& out)
{
	if (!Initialize())
		return MessageValue(FALSE, "Could not load Version.dll (System32)!\n");

	DWORD flags = FILE_VER_GET_NEUTRAL | FILE_VER_GET_LOCALISED;
	DWORD size = _GetFileVersionInfoSizeExA2(flags, filename, NULL);
	if (size == 0)
	{
		return MessageValue(FALSE, "Failed to get the file version info size! Error Code: " + std::to_string(GetLastError()) + "\n");
	}

	LPVOID pVersionInfo = new BYTE[size];
	BOOL result = _GetFileVersionInfoExA2(flags, filename, NULL, size, pVersionInfo);
	if (!result)
	{
		return MessageValue(FALSE, "Failed to get the file version info! Error Code: " + std::to_string(GetLastError()) + "\n");
	}

	UINT puLen2;
	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	BOOL t_result = _VerQueryValueA2(pVersionInfo, "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &puLen2);
	if (!t_result)
	{
		return MessageValue(FALSE, "Failed to get the file translation! Error Code: " + std::to_string(GetLastError()) + "\n");
	}
	printf("0x%x, 0x%x\n", lpTranslate->wLanguage, lpTranslate->wCodePage);

	std::map<std::string, std::string> stringFileInfo =
	{
		{"FileVersion", ""},
		{"CompanyName", ""},
		{"FileDescription", ""},
		{"OriginalFilename", ""}
	};

	for (auto it = stringFileInfo.begin(); it != stringFileInfo.end(); it++)
	{
		UINT puLen;
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "\\StringFileInfo\\%04lx%04lx\\%s", lpTranslate->wLanguage, lpTranslate->wCodePage, it->first.c_str());
		printf("Quering Value: %s\n", buffer);

		LPCSTR lpBuffer;
		BOOL result3 = _VerQueryValueA2(pVersionInfo, buffer, (LPVOID*)&lpBuffer, &puLen);
		if (!result3)
		{
			return MessageValue(FALSE, "Failed to get the StringFileInfo("+ it->first + ") + Error Code: " + std::to_string(GetLastError()) + "\n");
		}
		stringFileInfo[it->first] = lpBuffer;
		printf("%s: %s\n", it->first.c_str(), it->second.c_str());
	}

	out = stringFileInfo;
	return MessageValue(TRUE);
}
