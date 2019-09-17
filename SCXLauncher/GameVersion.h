#pragma once
#include <Windows.h>
#include <map>

struct GameVersion
{
	enum Version { VCLASSICS, V11SC, V102_PATCH, V11SC_FR, V1 };

	enum FunctionType {
		MAIN_LOOP, GLOBAL_INIT, FLAP_UI, CHOPPER_UI,
		CD_CHECK, CHOPPER_CLIP, RES_LOOKUP, DS_SLEEP,
		SCREEN_CLIP, BITDEPTH_CHECK, VERSIONS, GRAPHICS_INIT,
		ARG_PARSER, GFX_SOUND_INIT, ADJUST_WINDOW, DDRAW_PALETTE,
		CHEAT
	};

	enum DataType { RES_TYPE, MY_SLEEP };

	void SetDataTypeLocation(DataType Type, DWORD Location)
	{
		DataMap[Type] = Location;
	}

	void SetFunctionTypeLocation(FunctionType Type, DWORD Location)
	{
		FunctionMap[Type] = Location;
	}

	std::map<FunctionType, DWORD> FunctionMap;	
	std::map<DataType, DWORD> DataMap;
};

struct VersionClassics : GameVersion
{
	std::map<FunctionType, DWORD> GameVersion::FunctionMap =
	{
		{GLOBAL_INIT, 0x45DDB0},
		{MAIN_LOOP, 0x4308B0},
		{DS_SLEEP, 0x62B624},
		{CD_CHECK, 0x435840},
		{CHOPPER_UI, 0x4124C0},
		{FLAP_UI, 0x412860},
		{CHOPPER_CLIP, 0x413170},
		{RES_LOOKUP, 0x4641C0},
		{SCREEN_CLIP, 0x430E40},
		{DDRAW_PALETTE, 0x41CD40},

		{BITDEPTH_CHECK, 0x45E870},		//+0x170
		{VERSIONS, 0x45F210},			//Contains versions for everything (game, glide, os, etc)
		{GRAPHICS_INIT, 0x41C2E0},		//Conditional on whether to initialize glide or DDraw, use to patch DDraw
		{ARG_PARSER, 0x45EB10},			//All command-line arguments processed here
		{GFX_SOUND_INIT, 0x45DEC0},		//+36F (45E22F), if not windowed mode - calls DirectX fullscreen
		{ADJUST_WINDOW, 0x421400},		//Positions the window (LPRECT) in the center of your screen
		{CHEAT, 0x438370}				//This function will get rewritten, space: 0x240D (9229 bytes)
	};

	std::map<DataType, DWORD> GameVersion::DataMap =
	{
		{RES_TYPE, 0x5017D0}
	};
};


struct Version11SC : GameVersion
{
	std::map<FunctionType, DWORD> GameVersion::FunctionMap =
	{
		{GLOBAL_INIT, 0x45ADE0},
		{MAIN_LOOP, 0x42DBB0},
		{DS_SLEEP, 0x61D594},
		{CD_CHECK, 0x432B20},
		{CHOPPER_UI, 0x412440},
		{FLAP_UI, 0x4127D0},
		{CHOPPER_CLIP, 0x413090},
		{RES_LOOKUP, 0x4610A0},
		{SCREEN_CLIP, 0x42E130},
		{DDRAW_PALETTE, 0x41C9E0},

		{VERSIONS, 0x45C0F0}
	};

	std::map<DataType, DWORD> GameVersion::DataMap =
	{
		{RES_TYPE, 0x4F9798}
	};
};


struct Version11SCFR : GameVersion
{
	std::map<FunctionType, DWORD> GameVersion::FunctionMap =
	{
		{GLOBAL_INIT, 0x459B30},
		{MAIN_LOOP, 0x42C900},
		{DS_SLEEP, 0x61D594},
		{CD_CHECK, 0x431870},
		{CHOPPER_UI, 0x411190},
		{FLAP_UI, 0x411520},
		{CHOPPER_CLIP, 0x411DE0},
		{RES_LOOKUP, 0x45FDF0},
		{SCREEN_CLIP, 0x42CE80},
		{DDRAW_PALETTE, 0x41B730}
	};

	std::map<DataType, DWORD> GameVersion::DataMap =
	{
		{RES_TYPE, 0x4F9778}
	};
};

struct Version102Patch : GameVersion
{
	std::map<FunctionType, DWORD> GameVersion::FunctionMap =
	{
		{GLOBAL_INIT, 0x45B540},
		{MAIN_LOOP, 0x42E0A0},
		{DS_SLEEP, 0x62560C},
		{CD_CHECK, 0x433030},
		{CHOPPER_UI, 0x4124F0},
		{FLAP_UI, 0x412890},
		{CHOPPER_CLIP, 0x4131A0},

		{RES_LOOKUP, 0x461930},
		{SCREEN_CLIP, 0x42E630},
		{DDRAW_PALETTE, 0x41CD60}
	};

	std::map<DataType, DWORD> GameVersion::DataMap =
	{
		{RES_TYPE, 0x4FE7A0}
	};
};

struct VersionOriginal : GameVersion
{
	std::map<FunctionType, DWORD> GameVersion::FunctionMap =
	{
		
		{MAIN_LOOP, 0x42DA10},
		{DS_SLEEP, 0x61B588},
		{CD_CHECK, 0x432900}
	};

	std::map<DataType, DWORD> GameVersion::DataMap =
	{
		
	};
};