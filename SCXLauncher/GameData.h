#pragma once
#include <Windows.h>
#include <map>
#include <vector>
#include <string>
#include "Patcher.h"




class GameData
{
public:
	enum FunctionType { MAIN_LOOP, GLOBAL_INIT, FLAP_UI, CHOPPER_UI, 
						CD_CHECK, CHOPPER_CLIP, RES_LOOKUP, DS_SLEEP, 
						SCREEN_CLIP, BITDEPTH_CHECK, VERSIONS, GRAPHICS_INIT, 
						ARG_PARSER, GFX_SOUND_INIT, ADJUST_WINDOW, DDRAW_PALETTE };

	enum DWORDType { RES_TYPE, MY_SLEEP };
	enum Version { VCLASSICS, V11SC };
	static void initialize(PEINFO info);
	static bool PatchGame(std::string game_exe, GameData::Version version);
	static DWORD GetDWORDOffset(GameData::Version version, GameData::DWORDType dword_type);
private:
	static DWORD GetFunctionAddress(Version, FunctionType);
	static DWORD GetDWORDAddress(Version version, DWORDType dtype);
	static void CreateSleepFunction(DetourMaster *master, GameData::Version version);
	static void CreateResolutionFunction(DetourMaster *master, GameData::Version version);
	static void CreateGlobalInitFunction(DetourMaster *master, GameData::Version version);
	static void CreateCDFunction(DetourMaster *master, GameData::Version version);
	static void CreateChopperUIFunction(DetourMaster *master, GameData::Version version);
	static void CreateFlapUIFunction(DetourMaster *master, GameData::Version version);
	static void CreateChopperClipFunction(DetourMaster *master, GameData::Version version);
	static void CreateScreenClipFunction(DetourMaster *master, GameData::Version version);
	static void CreateDDrawPaletteFunction(DetourMaster *master, GameData::Version version);
};