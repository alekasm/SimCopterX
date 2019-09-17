#pragma once
#include <Windows.h>
#include <map>
#include <vector>
#include <string>
#include "Patcher.h"
#include "GameVersion.h"





class GameData
{
public:
	static std::vector<Instructions> GenerateData(PEINFO, GameVersion::Version);
	static DWORD GetDWORDAddress(GameVersion::Version, GameVersion::DataType);
private:
	static void CreateRelativeData(DetourMaster*, GameVersion::Version);
	static DWORD GetFunctionAddress(GameVersion::Version, GameVersion::FunctionType);	
	static void CreateSleepFunction(DetourMaster*, GameVersion::Version);
	static void CreateResolutionFunction(DetourMaster*, GameVersion::Version);
	static void CreateGlobalInitFunction(DetourMaster*, GameVersion::Version);
	static void CreateCDFunction(DetourMaster*, GameVersion::Version);
	static void CreateChopperUIFunction(DetourMaster*, GameVersion::Version);
	static void CreateFlapUIFunction(DetourMaster*, GameVersion::Version);
	static void CreateChopperClipFunction(DetourMaster*, GameVersion::Version);
	static void CreateScreenClipFunction(DetourMaster*, GameVersion::Version);
	static void CreateDDrawPaletteFunction(DetourMaster*, GameVersion::Version);
};