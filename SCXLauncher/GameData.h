#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include "Patcher.h"
#include "GameVersion.h"


class GameData
{
public:
	static std::vector<Instructions> GenerateData(PEINFO, GameVersions);
private:
	static void CreateSleepFunction(DetourMaster*, GameVersions);
	static void CreateResolutionFunction(DetourMaster*, GameVersions);
	static void CreateGlobalInitFunction(DetourMaster*, GameVersions);
	static void CreateCDFunction(DetourMaster*, GameVersions);
	static void CreateChopperUIFunction(DetourMaster*, GameVersions);
	static void CreateFlapUIFunction(DetourMaster*, GameVersions);
	static void CreateChopperClipFunction(DetourMaster*, GameVersions);
	static void CreateScreenClipFunction(DetourMaster*, GameVersions);
	static void CreateDDrawPaletteFunction(DetourMaster*, GameVersions);
	static void CreateHangarMainFunction(DetourMaster*, GameVersions);
	static void CreateMapCheatFunction(DetourMaster*, GameVersions);
	static void RenderSimsFunction(DetourMaster*, GameVersions);
	static void PatchChopperDamageFunction(DetourMaster*, GameVersions);
};