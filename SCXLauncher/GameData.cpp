#include "GameData.h"

namespace
{
	struct Game
	{
		std::map<GameData::FunctionType, DWORD> functions;
		std::map<GameData::DWORDType, DWORD> global_dwords;
	};
	std::map<GameData::Version, Game> games;
	DetourMaster *master;
}

bool GameData::PatchGame(std::string game_exe, GameData::Version version)
{
	CreateGlobalInitFunction(master, version);
	CreateResolutionFunction(master, version);
	CreateSleepFunction(master, version);
	CreateCDFunction(master, version);
	CreateChopperUIFunction(master, version);
	CreateFlapUIFunction(master, version);
	CreateChopperClipFunction(master, version);
	CreateScreenClipFunction(master, version);
	CreateDDrawPaletteFunction(master, version);
	return Patcher::Patch(master->instructions, game_exe);
}

DWORD GameData::GetDWORDOffset(GameData::Version version, GameData::DWORDType dword_type)
{
	return master->GetFileOffset(games[version].global_dwords[dword_type]);
}

void GameData::CreateDDrawPaletteFunction(DetourMaster *master, GameData::Version version)
{
	//Always use GetSystemPaletteEntries flow instead of manually generating the palette entries
	//Previously chose flow based on windowed vs fullscreen (windowed manually generated?)
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::DDRAW_PALETTE);
	DWORD rewrite_start;
	switch (version)
	{
	case Version::V11SC:
	case Version::V11SC_FR:
		rewrite_start = function_entry + 0x32;
		break;
	case Version::V102_PATCH:
	case Version::VCLASSICS:
		rewrite_start = function_entry + 0x3F;
		break;
	}

	Instructions instructions(rewrite_start);
	instructions << ByteArray{ 0xEB, 0x07 }; //jmp short 0x07 bytes
	instructions << ByteArray{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }; //Clean up debugging view

	size_t is_size = instructions.GetInstructions().size();
	printf("[DDraw Palette] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateScreenClipFunction(DetourMaster *master, GameData::Version version)
{
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::SCREEN_CLIP);
	DWORD function_res = GameData::GetFunctionAddress(version, GameData::RES_LOOKUP);

	DWORD rewrite_start;
	switch (version)
	{
	case Version::V11SC:
	case Version::V11SC_FR:
		rewrite_start = function_entry + 0x151;
		break;
	case Version::V102_PATCH:
	case Version::VCLASSICS:
		rewrite_start = function_entry + 0x15E;
		break;
	}

	//To make this simple from my IDA conversion, just going to use segment overrides for now..
	Instructions instructions(rewrite_start);		//Start overwriting where the comparison is for res type
	instructions << ByteArray{ 0x36, 0x8D, 0x4C, 0x24, 0x14 };		//lea ecx, [esp+0x14]
	instructions << ByteArray{ 0x36, 0x8D, 0x44, 0x24, 0x18 };		//lea eax, [esp+0x18]
	instructions << BYTE(0x50);										//push eax
	instructions << BYTE(0x51);										//push ecx
	instructions.call(function_res);								//call <resolution>
	instructions << ByteArray{ 0x31, 0xC0 };						//xor eax, eax
	instructions << ByteArray{ 0x36, 0x89, 0x44, 0x24, 0x0C };		//mov [esp+0xC], eax
	instructions << ByteArray{ 0x36, 0x89, 0x44, 0x24, 0x10 };		//mov [esp+0x10], eax
	instructions << ByteArray{ 0xEB, 0x4E };						//jump back to control block

	size_t is_size = instructions.GetInstructions().size();
	printf("[Screen Clip] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateChopperClipFunction(DetourMaster *master, GameData::Version version)
{
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::CHOPPER_CLIP);
	Instructions instructions(function_entry + 0x25);
	instructions << BYTE(0x01);						//Changes cmp <res type>, 0 to 1 (1 being original resolution)
	instructions << ByteArray{ 0x74, 0x7 };			//jz (short) 7 bytes

	//Not Resolution 1, don't subtract
	instructions << ByteArray{ 0x8D, 0x08 };		//lea ecx, [eax]
	instructions << ByteArray{ 0x89, 0x4E, 0x24 };	//mov [esi+0x24], ecx
	instructions << ByteArray{ 0xEB, 0x41 };		//jmp (short) 0x41 bytes		jumps to a block after the subtraction

	//Resolution 1 (original), subtract like original (-80 from width and height)
	instructions << ByteArray{ 0x8D, 0x48, 0xB0 };	//lea ecx [eax-0x50]
	instructions << ByteArray{ 0x83, 0xEA, 0x50 };	//sub edx, 0x50
	instructions << ByteArray{ 0x89, 0x4E, 0x24 };	//mov [esi+24], ecx
	instructions << ByteArray{ 0xEB, 0x36 };		//jmp short 0x36 bytes			jumps to a block after the subtraction

	size_t is_size = instructions.GetInstructions().size();
	printf("[Chopper View Clip] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateFlapUIFunction(DetourMaster *master, GameData::Version version)
{
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::FLAP_UI);
	DWORD rewrite_start;
	switch (version)
	{
	case Version::V11SC:
	case Version::V11SC_FR:
		rewrite_start = function_entry + 0xC;
		break;
	case Version::V102_PATCH:
	case Version::VCLASSICS:
		rewrite_start = function_entry + 0x19;
		break;
	}

	//We're going to be overwriting the instructions to compare resolution type and jnz

	//83 3D D0 17 50 00 01 , comparison
	//75 6A                , jnz

	Instructions instructions(rewrite_start);
	instructions << ByteArray{ 0x8B, 0xF8 };					//mov edi, eax
	instructions << ByteArray{ 0x8B, 0x41, 0x1C };				//mov eax, [ecx+0x1C]		loads screen width into eax
	instructions << ByteArray{ 0x2D, 0x8A, 0x00, 0x00, 0x00 };	//sub eax, 0x8A				subtract 138px (flap img width) from width		
	instructions << BYTE(0x90);									//nop						buffer 
	instructions << ByteArray{ 0x90, 0x90, 0x90, 0x90, 0x90 };  //nops						nops out the 'mov eax, 0x1F6' (original x value)

	size_t is_size = instructions.GetInstructions().size();
	printf("[Chopper Flap UI] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateChopperUIFunction(DetourMaster *master, GameData::Version version)
{
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::CHOPPER_UI);
	DWORD res_dword = GameData::GetDWORDAddress(version, GameData::RES_TYPE);
	DWORD detour_return;


	//Classics version has a resolution variable check which consistutes a 0xD size difference between the functions
	//Unfortunately the instructions are not after our patch.
	switch (version)
	{
	case Version::V11SC:
	case Version::V11SC_FR:
		detour_return = function_entry + 0xF2;
		break;
	case Version::V102_PATCH:
	case Version::VCLASSICS:
		detour_return = function_entry + 0xFF;
		break;
	}

	Instructions instructions(DWORD(function_entry + 0x13));
	instructions << BYTE(0x90); //not necessary, just makes it look nicer in graph views
	instructions.jmp(master->GetNextDetour());

	instructions << ByteArray{ 0xB8, 0x10, 0x00, 0x00, 0x00 };				//mov eax, 10
	instructions << ByteArray{ 0xB9, 0x3E, 0x00, 0x00, 0x00 };				//mov ecx, 3E
	instructions << ByteArray{ 0x89, 0x44, 0x24, 0x1C };					//mov [esp+0x1C], eax
	instructions << ByteArray{ 0x89, 0x44, 0x24, 0x20 };					//mov [esp+0x20], eax
	instructions << ByteArray{ 0xB8, 0x2C, 0x02, 0x00, 0x00 };				//mov eax, 22C
	instructions << ByteArray{ 0xBF, 0x80, 0x02, 0x00, 0x00 };				//mov edi, 280
	instructions << ByteArray{ 0x89, 0x44, 0x24, 0x2C };					//mov [esp+0x2C], eax
	instructions << ByteArray{ 0x89, 0x4C, 0x24, 0x30 };					//mov [esp+0x30], ecx
	instructions << ByteArray{ 0xBB, 0x24, 0x01, 0x00, 0x00 };				//mov ebx, 124
	instructions << ByteArray{ 0x89, 0x7C, 0x24, 0x3C };					//mov [esp+0x3C], edi
	instructions << ByteArray{ 0x89, 0x5C, 0x24, 0x40 };					//mov [esp+0x40], ebx
	instructions << ByteArray{ 0x89, 0x44, 0x24, 0x4C };					//mov [esp+0x4C], eax
	instructions << ByteArray{ 0x89, 0x5C, 0x24, 0x50 };					//mov [esp+0x50], ebx
	instructions << ByteArray{ 0xB8, 0xC8, 0x01, 0x00, 0x00 };				//mov eax, 1C8
	instructions << ByteArray{ 0xB9, 0x8E, 0x01, 0x00, 0x00 };				//mov ecx, 18E
	instructions << ByteArray{ 0x89, 0x44, 0x24, 0x5C };					//mov [esp+0x5C], eax
	instructions << ByteArray{ 0x89, 0x4C, 0x24, 0x60 };					//mov [esp+0x60], ecx
	instructions << ByteArray{ 0x89, 0x7C, 0x24, 0x6C };					//mov [esp+0x6C], edi
	instructions << ByteArray{ 0x89, 0x44, 0x24, 0x7C };					//mov [esp+0x7C], eax
	instructions << ByteArray{ 0x89, 0xBC, 0x24, 0x8C, 0x00, 0x00, 0x00 };	//mov [esp+0x8C], edi
	instructions << ByteArray{ 0xB8, 0xB6, 0x01, 0x00, 0x00 };				//mov eax, 1B6
	instructions << ByteArray{ 0x89, 0x44, 0x24, 0x70 };					//mov [esp+0x70], eax
	instructions << ByteArray{ 0xB8, 0xE0, 0x01, 0x00, 0x00 };				//mov eax, 1E0
	instructions << ByteArray{ 0x89, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00 };	//mov [esp+0x80], eax
	instructions << ByteArray{ 0x89, 0x84, 0x24, 0x90, 0x00, 0x00, 0x00 };	//mov [esp+0x90], eax

	instructions.cmp(res_dword, 0x1);
	instructions.jnz(DWORD(instructions.GetCurrentLocation() + 0xA1));	//Size of original UI = 9B, Size of jnz instruction = 6
																		//9B + 6 = A1
	//0x36 = SS segment override prefix
	//This is the original Chopper UI layout for 640x480
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00 }; //mov dword ptr ss:[esp+14], 0x0
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00 }; //mov dword ptr ss:[esp+18], 0x0
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x24, 0xEE, 0x01, 0x00, 0x00 }; //mov dword ptr ss:[esp+24], 0x1EE
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x28, 0x00, 0x00, 0x00, 0x00 }; //mov dword ptr ss:[esp+28], 0x0
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x34, 0x2C, 0x02, 0x00, 0x00 }; //mov dword ptr ss:[esp+34], 0x22C
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x38, 0x00, 0x00, 0x00, 0x00 }; //mov dword ptr ss:[esp+38], 0x0
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x44, 0x12, 0x02, 0x00, 0x00 }; //mov dword ptr ss:[esp+44], 0x212
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x48, 0x3E, 0x00, 0x00, 0x00 }; //mov dword ptr ss:[esp+48], 0x3E
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x54, 0x00, 0x00, 0x00, 0x00 }; //mov dword ptr ss:[esp+54], 0x0
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x58, 0x63, 0x01, 0x00, 0x00 }; //mov dword ptr ss:[esp+58], 0x163
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x64, 0xC7, 0x01, 0x00, 0x00 }; //mov dword ptr ss:[esp+64], 0x1C7 
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x68, 0x22, 0x01, 0x00, 0x00 }; //mov dword ptr ss:[esp+68], 0x122
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x74, 0x00, 0x00, 0x00, 0x00 }; //mov dword ptr ss:[esp+74], 0x0
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x78, 0x8E, 0x01, 0x00, 0x00 }; //mov dword ptr ss:[esp+78], 0x18E	
	instructions << ByteArray{ 0x36, 0xC7, 0x84, 0x24, 0x84, 0x00, 0x00, 0x00, 0xC6, 0x01, 0x00, 0x00 }; //mov dword ptr ss:[esp+84], 0x1C6 (+0x84, exceeds 8-bit signed)
	instructions << ByteArray{ 0x36, 0xC7, 0x84, 0x24, 0x88, 0x00, 0x00, 0x00, 0xB5, 0x01, 0x00, 0x00 }; //mov dword ptr ss:[esp+88], 0x1B5 
	instructions.jmp(detour_return, FALSE); //Don't switch location yet

	//This is the 0x9B offset
	//If 5017D0 != 1 (not 640x480), we need to write a custom chopper UI layout
	instructions << ByteArray{ 0x36, 0x8B, 0x46, 0x1C };								//mov eax, dword ptr ss:[esi+1C]
	instructions << ByteArray{ 0x36, 0x8B, 0x56, 0x20 };								//mov edx, dword ptr ss:[esi+20]
	instructions << ByteArray{ 0x31, 0xC9 };											//xor ecx, ecx
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00 };	//mov dword ptr ss:[esp+14], 0x0
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00 };	//mov dword ptr ss:[esp+18], 0x0
	instructions << ByteArray{ 0x89, 0xC1 };											//mov ecx, eax
	instructions << ByteArray{ 0x83, 0xE9, 0x3E };										//sub ecx, 3E
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x24 };							//mov dword ptr ss:[esp+24], ecx
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x28, 0x00, 0x00, 0x00, 0x00 };	//mov dword ptr ss:[esp+28], 0
	instructions << ByteArray{ 0x89, 0xC1 };											//mov ecx, eax
	instructions << ByteArray{ 0x81, 0xE9, 0xC6, 0x01, 0x00, 0x00 };					//sub ecx, 1C6
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x74 };							//mov dword ptr ss:[esp+74], ecx
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x54 };							//mov dword ptr ss:[esp+54], ecx
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x34 };							//mov dword ptr ss:[esp+34], ecx
	instructions << ByteArray{ 0x83, 0xE9, 0x1A };										//sub ecx, 1A
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x44 };							//mov dword ptr ss:[esp+44], ecx
	instructions << ByteArray{ 0x81, 0xE9, 0xA0, 0x00, 0x00, 0x00 };					//sub ecx, A0
	instructions << ByteArray{ 0x36, 0x89, 0x8C, 0x24, 0x84, 0x00, 0x00, 0x00 };		//mov dword ptr ss:[esp+84], ecx
	instructions << ByteArray{ 0x89, 0xD1 };											//mov ecx, edx
	instructions << ByteArray{ 0x83, 0xE9, 0x52 };										//sub, ecx, 52
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x78 };							//mov dword ptr ss:[esp+78], ecx
	instructions << ByteArray{ 0x83, 0xE9, 0x2B };										//sub ecx, 2B
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x58 };							//mov dword ptr ss:[esp+58], ecx
	instructions << ByteArray{ 0x83, 0xC1, 0x11 };										//add ecx, 11
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x38 };							//mov dword ptr ss:[esp+38], ecx
	instructions << ByteArray{ 0x83, 0xE9, 0x04 };										//sub ecx, 4
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x48 };							//mov dword ptr ss:[esp+48], ecx
	instructions << ByteArray{ 0x89, 0xD1 };											//mov ecx, edx
	instructions << ByteArray{ 0x83, 0xE9, 0x2B };										//sub ecx, 2B
	instructions << ByteArray{ 0x36, 0x89, 0x8C, 0x24, 0x88, 0x00, 0x00, 0x00 };		//mov dword ptr ss:[esp+88], ecx
	instructions << ByteArray{ 0x36, 0xC7, 0x44, 0x24, 0x64, 0x00, 0x00, 0x00, 0x00 };	//mov dword ptr ss:[esp+64], 0
	instructions << ByteArray{ 0x89, 0xD1 };											//mov ecx, edx
	instructions << ByteArray{ 0x81, 0xE9, 0x93, 0x00, 0x00, 0x00 };					//sub ecx, 93
	instructions << ByteArray{ 0x36, 0x89, 0x4C, 0x24, 0x68 };							//mov dword ptr ss:[esp+68], ecx
	instructions.jmp(detour_return);	//Now we can jump back

	size_t is_size = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size);
	printf("[Chopper UI] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateCDFunction(DetourMaster *master, GameData::Version version)
{
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::CD_CHECK);
	Instructions instructions(DWORD(function_entry + 0x171));
	instructions.jmp(DWORD(function_entry + 0x23B));    //jmp <function> (originally jnz)

	size_t is_size = instructions.GetInstructions().size();
	printf("[CD Check Bypass] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateSleepFunction(DetourMaster *master, GameData::Version version)
{
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::MAIN_LOOP);
	DWORD dsfunction_sleep = GameData::GetFunctionAddress(version, GameData::DS_SLEEP);
	DWORD sleep_param = master->base_location + 0x4;

	Instructions instructions(DWORD(function_entry + 0x11E));
	instructions.jmp(master->GetNextDetour());                      //jmp <detour> 
	instructions << ByteArray{ 0xFF, 0xD6 };						//call esi
	instructions << BYTE(0x50);										//push eax
	instructions.push_rm32(sleep_param);                            //push param millis
	instructions.call_rm32(dsfunction_sleep);						//call Sleep
	instructions << BYTE(0x58);										//pop eax
	instructions << ByteArray{ 0x85, 0xC0 };						//test eax, eax
	instructions.jnz(DWORD(function_entry + 0x124));				//jnz <function>
	instructions.jmp(DWORD(function_entry + 0x148));				//jmp <function>

	size_t is_size = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size);
	printf("[Main Loop Sleep] Generated a total of %d bytes\n", is_size);
	//printf("DetourMaster now points to address starting at %x\n", master->current_location);
	master->instructions.push_back(instructions);

}

void GameData::CreateResolutionFunction(DetourMaster *master, GameData::Version version)
{
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::RES_LOOKUP);

	Instructions instructions(DWORD(function_entry + 0x13));
	instructions << DWORD(0x500);
	instructions.relocate(function_entry + 0x19);
	instructions << DWORD(0x320);

	instructions.relocate(function_entry + 0x53);
	instructions << DWORD(0x500);
	instructions.relocate(function_entry + 0x59);
	instructions << DWORD(0x2D0);

	instructions.relocate(function_entry + 0x73);
	instructions << DWORD(0x400);
	instructions.relocate(function_entry + 0x79);
	instructions << DWORD(0x300);

	size_t is_size = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size);
	printf("[Resolution Lookup] Generated a total of %d bytes\n", is_size);
	//printf("DetourMaster now points to address starting at %x\n", master->current_location);
	master->instructions.push_back(instructions);

}
void GameData::CreateGlobalInitFunction(DetourMaster *master, GameData::Version version)
{
	//printf("DetourMaster starting at %x\n", master->current_location);
	DWORD function_entry = GameData::GetFunctionAddress(version, GameData::GLOBAL_INIT);
	DWORD function_res = GameData::GetFunctionAddress(version, GameData::RES_LOOKUP);

	/*
	lea edi, [esi+4040h]     +0x66             <<used in detour
	mov eax, 1E0h            +0x6C             <<overwritten in detour
	mov [esi+40ACh], ebp     +0x71 (1.1)       <<preserved since its different between versions
	mov [esi+40B0h], ebp     +0x71 (Classics)  <<preserved since its different between versions
	mov [esi+4044h], eax     +0x77             <<this is where we start our detour, very next instruction after jump is this
	*/

	//The detour function:
	Instructions instructions(DWORD(function_entry + 0x77));               //The detour jmp will overwrite mov[esi+4044h],  eax  
	instructions.jmp(master->GetNextDetour());                             //jmp <detour> 
	instructions << ByteArray{ 0x3E, 0x8D, 0x86, 0x44, 0x40, 0x00, 0x00 }; //lea eax, [esi+4044h]
	instructions << BYTE(0x50);                                            //push eax
	instructions << BYTE(0x57);                                            //push edi
	instructions.call(function_res);                                       //call <resolution>
	instructions << ByteArray{ 0x3E, 0x8B, 0xBE, 0x40, 0x40, 0x00, 0x00 }; //mov edi, [esi+4040h]
	instructions << ByteArray{ 0x3E, 0x8B, 0x86, 0x44, 0x40, 0x00, 0x00 }; //mov eax, [esi+4044h]
	instructions << ByteArray{ 0x3E, 0x89, 0xBE, 0x4C, 0x40, 0x00, 0x00 }; //mov [esi+404Ch], edi
	instructions << ByteArray{ 0x3E, 0x89, 0x86, 0x50, 0x40, 0x00, 0x00 }; //mov [esi+4050h], eax
	instructions << ByteArray{ 0x3E, 0x8D, 0xBE, 0x40, 0x40, 0x00, 0x00 }; //lea edi, [esi+4040h]
	instructions << ByteArray{ 0xB9, 0x08, 0x00, 0x00, 0x00 };             //mov ecx, 8
	instructions << ByteArray{ 0x3E, 0x89, 0x8E, 0x48, 0x40, 0x00, 0x00 }; //mov [esi+4048h], ecx
	instructions.jmp(function_entry + 0xA0);                               //jmp <function>	
	//push ebp                 +0xA0             <<the instruction we jump back to
	
	instructions.relocate(function_entry + 0xC7);						//We're going to nop out the manual res type setter
	instructions << ByteArray{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };	//mov [5017D0], 1

	size_t is_size = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size);
	printf("[Global Initialization] Generated a total of %d bytes\n", is_size);
	//printf("DetourMaster now points to address starting at %x\n", master->current_location);
	master->instructions.push_back(instructions);
}

DWORD GameData::GetFunctionAddress(Version version, FunctionType ftype)
{
	return games[version].functions[ftype];
}

DWORD GameData::GetDWORDAddress(Version version, DWORDType dtype)
{
	return games[version].global_dwords[dtype];
}

void GameData::initialize(PEINFO info)
{	
	if (master)
	{
		delete master;
		master = nullptr;
	}

	master = new DetourMaster(info);
	Patcher::SetDetourMaster(master);

	Game version_classics; //February 1998
	Game version_11sc; //7 November 1996
	Game version_102patch; // 26 February 1997
	Game version_11scfr; //7 November 1996 (FR)

	version_classics.global_dwords[RES_TYPE] = 0x5017D0;
	version_11sc.global_dwords[RES_TYPE] = 0x4F9798;
	version_11scfr.global_dwords[RES_TYPE] = 0x4F9778;
	version_102patch.global_dwords[RES_TYPE] = 0x4FE7A0;

	version_classics.functions[GLOBAL_INIT] = 0x45DDB0; 
	version_11sc.functions[GLOBAL_INIT] = 0x45ADE0;  
	version_11scfr.functions[GLOBAL_INIT] = 0x459B30;
	version_102patch.functions[GLOBAL_INIT] = 0x45B540;

	version_classics.functions[MAIN_LOOP] = 0x4308B0;   
	version_11sc.functions[MAIN_LOOP] = 0x42DBB0;   
	version_11scfr.functions[MAIN_LOOP] = 0x42C900;
	version_102patch.functions[MAIN_LOOP] = 0x42E0A0;

	version_classics.functions[DS_SLEEP] = 0x62B624;
	version_11sc.functions[DS_SLEEP] = 0x61D594;
	version_11scfr.functions[DS_SLEEP] = 0x61D594;
	version_102patch.functions[DS_SLEEP] = 0x62560C;

	version_classics.functions[CD_CHECK] = 0x435840;   
	version_11sc.functions[CD_CHECK] = 0x432B20;	
	version_11scfr.functions[CD_CHECK] = 0x431870;
	version_102patch.functions[CD_CHECK] = 0x433030;

	version_classics.functions[CHOPPER_UI] = 0x4124C0;  
	version_11sc.functions[CHOPPER_UI] = 0x412440;  
	version_11scfr.functions[CHOPPER_UI] = 0x411190;
	version_102patch.functions[CHOPPER_UI] = 0x4124F0;

	version_classics.functions[FLAP_UI] = 0x412860;    
	version_11sc.functions[FLAP_UI] = 0x4127D0;   
	version_11scfr.functions[FLAP_UI] = 0x411520;
	version_102patch.functions[FLAP_UI] = 0x412890;

	version_classics.functions[CHOPPER_CLIP] = 0x413170; 
	version_11sc.functions[CHOPPER_CLIP] = 0x413090;    
	version_11scfr.functions[CHOPPER_CLIP] = 0x411DE0;
	version_102patch.functions[CHOPPER_CLIP] = 0x4131A0;

	version_classics.functions[RES_LOOKUP] = 0x4641C0;
	version_11sc.functions[RES_LOOKUP] = 0x4610A0;
	version_11scfr.functions[RES_LOOKUP] = 0x45FDF0;
	version_102patch.functions[RES_LOOKUP] = 0x461930;

	version_classics.functions[SCREEN_CLIP] = 0x430E40; 
	version_11sc.functions[SCREEN_CLIP] = 0x42E130; 
	version_11scfr.functions[SCREEN_CLIP] = 0x42CE80;
	version_102patch.functions[SCREEN_CLIP] = 0x42E630;
	
	version_classics.functions[DDRAW_PALETTE] = 0x41CD40;
	version_11sc.functions[DDRAW_PALETTE] = 0x41C9E0;
	version_11scfr.functions[DDRAW_PALETTE] = 0x41B730;
	version_102patch.functions[DDRAW_PALETTE] = 0x41CD60;


	//---------- IN PROGRESS
	version_classics.functions[BITDEPTH_CHECK] = 0x45E870;	//+0x170
	version_classics.functions[VERSIONS] = 0x45F210;		//Sets the versions for random things like glide, OS, game?
	version_11sc.functions[VERSIONS] = 0x45C0F0;
	version_classics.functions[GRAPHICS_INIT] = 0x41C2E0;	//Conditional on whether to initialize glide or DDraw, use to patch DDraw
	version_classics.functions[ARG_PARSER] = 0x45EB10;		//All command-line arguments processed here
	version_classics.functions[GFX_SOUND_INIT] = 0x45DEC0;	//+36F (45E22F), if not windowed mode - calls DirectX fullscreen
	version_classics.functions[ADJUST_WINDOW] = 0x421400;	//Positions the window (LPRECT) in the center of your screen	



	//---------- Below here contains completely new functions/variables
	version_classics.global_dwords[MY_SLEEP] = master->base_location + 0x4;
	version_11sc.global_dwords[MY_SLEEP] = master->base_location + 0x4;
	version_11scfr.global_dwords[MY_SLEEP] = master->base_location + 0x4;
	version_102patch.global_dwords[MY_SLEEP] = master->base_location + 0x4;

	games[VCLASSICS] = version_classics;
	games[V11SC] = version_11sc;
	games[V11SC_FR] = version_11scfr;
	games[V102_PATCH] = version_102patch;


}