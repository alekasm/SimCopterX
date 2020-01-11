#pragma once
#include <Windows.h>

struct FunctionType
{
	DWORD MAIN_LOOP;
	DWORD GLOBAL_INIT;
	DWORD FLAP_UI;
	DWORD CHOPPER_UI;
	DWORD CD_CHECK;
	DWORD CHOPPER_CLIP;
	DWORD RES_LOOKUP;
	DWORD DS_SLEEP;
	DWORD SCREEN_CLIP;
	DWORD BITDEPTH_CHECK;
	DWORD VERSIONS; 
	DWORD GRAPHICS_INIT;
	DWORD ARG_PARSER;
	DWORD GFX_SOUND_INIT;
	DWORD ADJUST_WINDOW;
	DWORD DDRAW_PALETTE;
	DWORD CHEAT;
	DWORD HANGAR_MAIN;
};

struct DataType
{
	DWORD RES_TYPE;
};

struct DetourOffsetType
{
	static const DWORD MY_SLEEP = 0x0;
};

struct GameVersion
{	
	FunctionType functions;
	DataType data;
};

const struct VersionClassics : GameVersion
{
	VersionClassics()
	{
		functions.GLOBAL_INIT = 0x45DDB0;
		functions.MAIN_LOOP = 0x4308B0;
		functions.DS_SLEEP = 0x62B624;
		functions.CD_CHECK = 0x435840;
		functions.CHOPPER_UI = 0x4124C0;
		functions.FLAP_UI = 0x412860;
		functions.CHOPPER_CLIP = 0x413170;
		functions.RES_LOOKUP = 0x4641C0;
		functions.SCREEN_CLIP = 0x430E40;
		functions.DDRAW_PALETTE = 0x41CD40;
		functions.HANGAR_MAIN = 0x43ECC0;

		functions.BITDEPTH_CHECK = 0x45E870;	//+0x170
		functions.VERSIONS = 0x45F210;			//Contains versions for everything (game, glide, os, etc)
		functions.GRAPHICS_INIT = 0x41C2E0;		//Conditional on whether to initialize glide or DDraw = use to patch DDraw
		functions.ARG_PARSER = 0x45EB10;		//All command-line arguments processed here
		functions.GFX_SOUND_INIT = 0x45DEC0;	//+36F (45E22F) = if not windowed mode - calls DirectX fullscreen
		functions.ADJUST_WINDOW = 0x421400;		//Positions the window (LPRECT) in the center of your screen
		functions.CHEAT = 0x438370;				//This function will get rewritten, space: 0x240D (9229 bytes)

		data.RES_TYPE = 0x5017D0;
	}
} version_classics;

const struct Version11SC : GameVersion
{
	Version11SC()
	{		
		functions.GLOBAL_INIT = 0x45ADE0;
		functions.MAIN_LOOP = 0x42DBB0;
		functions.DS_SLEEP = 0x61D594;
		functions.CD_CHECK = 0x432B20;
		functions.CHOPPER_UI = 0x412440;
		functions.FLAP_UI = 0x4127D0;
		functions.CHOPPER_CLIP = 0x413090;
		functions.RES_LOOKUP = 0x4610A0;
		functions.SCREEN_CLIP = 0x42E130;
		functions.DDRAW_PALETTE = 0x41C9E0;
		functions.HANGAR_MAIN = 0x43BFA0;

		functions.VERSIONS = 0x45C0F0;
		
		data.RES_TYPE = 0x4F9798;		
	}
} version_11sc;

const struct Version11SCFR : GameVersion
{
	Version11SCFR()
	{		
		functions.GLOBAL_INIT = 0x459B30;
		functions.MAIN_LOOP = 0x42C900;
		functions.DS_SLEEP = 0x61D594;
		functions.CD_CHECK = 0x431870;
		functions.CHOPPER_UI = 0x411190;
		functions.FLAP_UI = 0x411520;
		functions.CHOPPER_CLIP = 0x411DE0;
		functions.RES_LOOKUP = 0x45FDF0;
		functions.SCREEN_CLIP = 0x42CE80;
		functions.DDRAW_PALETTE = 0x41B730;
		functions.HANGAR_MAIN = 0x43ACF0;
		
		data.RES_TYPE = 0x4F9778;		
	}
} version_11scfr;

const struct Version102Patch : GameVersion
{
	Version102Patch()
	{
		functions.GLOBAL_INIT = 0x45B540;
		functions.MAIN_LOOP = 0x42E0A0;
		functions.DS_SLEEP = 0x62560C;
		functions.CD_CHECK = 0x433030;
		functions.CHOPPER_UI = 0x4124F0;
		functions.FLAP_UI = 0x412890;
		functions.CHOPPER_CLIP = 0x4131A0;
		functions.RES_LOOKUP = 0x461930;
		functions.SCREEN_CLIP = 0x42E630;
		functions.DDRAW_PALETTE = 0x41CD60;
		functions.HANGAR_MAIN = 0x43C4F0;

		functions.BITDEPTH_CHECK = 0x45C000; //+0x170

		data.RES_TYPE = 0x4FE7A0;
	}
} version_102patch;

const struct VersionOriginal : GameVersion
{
	VersionOriginal()
	{
		functions.GLOBAL_INIT = 0x45AC60;
		functions.MAIN_LOOP = 0x42DA10;
		functions.DS_SLEEP = 0x61B588;
		functions.CD_CHECK = 0x432900;
		functions.CHOPPER_UI = 0x412440;
		functions.FLAP_UI = 0x4127D0;
		functions.CHOPPER_CLIP = 0x413090;
		functions.RES_LOOKUP = 0x460F20;
		functions.SCREEN_CLIP = 0x42DF90;
		functions.DDRAW_PALETTE = 0x41CA10;
		functions.HANGAR_MAIN = 0x43BD40;
		

		data.RES_TYPE = 0x4F8798;
	}
} version_original;


//The order of this matters
enum GameVersions { VCLASSICS, V11SC, V102_PATCH, V11SC_FR, ORIGINAL };
static const GameVersion* const Versions[5] =
{
	&version_classics,
	&version_11sc,
	&version_102patch,
	&version_11scfr,
	&version_original
};
