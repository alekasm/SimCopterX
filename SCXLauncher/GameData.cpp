#include "GameData.h"

std::vector<Instructions> GameData::GenerateData(PEINFO info, GameVersions version)
{
	DetourMaster* master = new DetourMaster(info);
	CreateGlobalInitFunction(master, version);
	CreateResolutionFunction(master, version);
	CreateSleepFunction(master, version);
	CreateCDFunction(master, version);
	CreateChopperUIFunction(master, version);
	CreateFlapUIFunction(master, version);
	CreateChopperClipFunction(master, version);
	CreateScreenClipFunction(master, version);
	CreateDDrawPaletteFunction(master, version);
	CreateHangarMainFunction(master, version);
	CreateMapCheatFunction(master, version);
	RenderSimsFunction(master, version);
	PatchChopperDamageFunction(master, version);
	std::vector<Instructions> ret_ins(master->instructions);	
	delete master;
	return ret_ins;
}

void GameData::CreateDDrawPaletteFunction(DetourMaster *master, GameVersions version)
{
	//Always use GetSystemPaletteEntries flow instead of manually generating the palette entries
	//Previously chose flow based on windowed vs fullscreen (windowed manually generated?)
	DWORD function_entry = Versions[version]->functions.DDRAW_PALETTE;
	DWORD rewrite_start;
	switch (version)
	{
	case GameVersions::V11SC:
	case GameVersions::V11SC_FR:
	case GameVersions::ORIGINAL:
		rewrite_start = function_entry + 0x32;
		break;
	case GameVersions::V102_PATCH:
	case GameVersions::VCLASSICS:
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

void GameData::CreateScreenClipFunction(DetourMaster *master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.SCREEN_CLIP;
	DWORD function_res = Versions[version]->functions.RES_LOOKUP;

	DWORD rewrite_start;
	switch (version)
	{
	case GameVersions::V11SC:
	case GameVersions::V11SC_FR:
	case GameVersions::ORIGINAL:
		rewrite_start = function_entry + 0x151;
		break;
	case GameVersions::V102_PATCH:
	case GameVersions::VCLASSICS:
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

void GameData::CreateChopperClipFunction(DetourMaster *master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.CHOPPER_CLIP;
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

void GameData::CreateFlapUIFunction(DetourMaster *master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.FLAP_UI;
	DWORD rewrite_start;
	switch (version)
	{
	case GameVersions::V11SC:
	case GameVersions::V11SC_FR:
	case GameVersions::ORIGINAL:
		rewrite_start = function_entry + 0xC;
		break;
	case GameVersions::V102_PATCH:
	case GameVersions::VCLASSICS:
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

void GameData::CreateMapCheatFunction(DetourMaster* master, GameVersions version)
{
	DWORD res_dword = Versions[version]->data.RES_TYPE;
	DWORD function_entry = Versions[version]->functions.UNK_RENDER_1;

	Instructions instructions(DWORD(function_entry + 0x1FA));
	instructions.jmp(master->GetNextDetour());
	instructions.cmp(res_dword, 0x1);
	instructions << ByteArray{ 0x75, 0x0F }; //jnz
	instructions << BYTE(0x68);
	instructions << DWORD(0x144);
	instructions << BYTE(0x68);
	instructions << DWORD(0x200);
	instructions.jmp(DWORD(function_entry + 0x204), FALSE);
	instructions << ByteArray{ 0x8B, 0x8E, 0xD4, 0x00, 0x00, 0x00 }; // mov ecx, [esi+0xD4] //Screen height
	instructions << ByteArray{ 0x83, 0xE9, 0x68 }; //sub ecx, 102. Map =  124 x 98, create 4 px buffer	
	instructions << BYTE(0x51); //push ecx
	instructions << ByteArray{ 0x6A, 0x04 }; //push 0x2
	instructions.jmp(DWORD(function_entry + 0x204), FALSE);

	size_t is_size = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size);
	printf("[Map Cheat] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateChopperUIFunction(DetourMaster *master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.CHOPPER_UI;
	DWORD res_dword = Versions[version]->data.RES_TYPE;
	DWORD detour_return;


	//Classics version has a resolution variable check which consistutes a 0xD size difference between the functions
	//Unfortunately the instructions are not after our patch.
	switch (version)
	{
	case GameVersions::V11SC:
	case GameVersions::V11SC_FR:
	case GameVersions::ORIGINAL:
		detour_return = function_entry + 0xF2;
		break;
	case GameVersions::V102_PATCH:
	case GameVersions::VCLASSICS:
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

void GameData::CreateCDFunction(DetourMaster *master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.CD_CHECK;
	Instructions instructions(DWORD(function_entry + 0x171));
	instructions.jmp(DWORD(function_entry + 0x23B));    //jmp <function> (originally jnz)

	size_t is_size = instructions.GetInstructions().size();
	printf("[CD Check Bypass] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateSleepFunction(DetourMaster *master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.MAIN_LOOP;
	DWORD dsfunction_sleep = Versions[version]->functions.DS_SLEEP;
	DWORD sleep_param = master->info.GetDetourVirtualAddress(DetourOffsetType::MY_SLEEP);

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

void GameData::RenderSimsFunction(DetourMaster* master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.RENDER_SIMS;

	//First remove all the instructions which try and store the address 
	//of the element in the array. 
	Instructions instructions(function_entry + 0x35);
	instructions.nop(7); // mov eax, [esp+AC]
	instructions.nop(2); // mov ebp, [ecx]
	instructions.nop(1); // push eax
	instructions.nop(3); //call [ebp+28]
	instructions.relocate(function_entry + 0x46);
	instructions.nop(4); //nop [esp+34], eax

	//Now rewrite the original call, but use a safety length check
	instructions.relocate(function_entry + 0x109);
	DWORD detour = master->GetNextDetour();
	instructions.jmp(detour, FALSE);
	instructions.nop(2); //clean up for debugging
	instructions.relocate(detour);
	instructions << ByteArray{ 0x8B, 0x8C, 0x24, 0xA8, 0x00, 0x00, 0x00 }; //mov ecx, [esp+0xA8]
	instructions << ByteArray{ 0x8B, 0x49, 0x28 }; //mov ecx, [ecx+0x28]
	instructions << ByteArray{ 0x8B, 0x84, 0x24, 0xAC, 0x00, 0x00, 0x00 }; //mov eax, [esp+0xAC] (index arg)
	instructions << ByteArray{ 0x3B, 0x41, 0x14 }; //cmp eax, [ecx+0x14] (compare index arg against length)
	instructions.jge(function_entry + 0x2F0); //jump if outside bounds, dont render
	instructions << ByteArray{ 0x8B, 0x49, 0x04 }; //mov ecx, [ecx+0x4] (the array)
	instructions << ByteArray{ 0x8B, 0x04, 0x81 }; //mov eax, [ecx+eax*4] (the element in the array)
	instructions << ByteArray{ 0x8D, 0x2C, 0xD0 }; //lea ebp, [eax+edx*8] (the instructions we overwrote on detour)
	instructions.jmp(function_entry + 0x110);

	size_t is_size = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size);
	printf("[Render Sims Fix] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateHangarMainFunction(DetourMaster* master, GameVersions version)
{

	DWORD function_entry = Versions[version]->functions.HANGAR_MAIN;
	
	DWORD detour_entry_2;
	DWORD detour_return_2;

	switch (version)
	{
	case GameVersions::V11SC:
	case GameVersions::V11SC_FR:	
	case GameVersions::V102_PATCH:
	case GameVersions::VCLASSICS:
		detour_entry_2 = function_entry + 0x1D5;
		detour_return_2 = function_entry + 0x1E0;
		break;	
	case GameVersions::ORIGINAL:
		detour_entry_2 = function_entry + 0x1DB;
		detour_return_2 = function_entry + 0x1E6;		
		break;
	}

	DWORD viewport_x_offset;
	DWORD offset_bg_ptr;
	switch (version)
	{
	case GameVersions::V11SC:
	case GameVersions::V11SC_FR:
	case GameVersions::V102_PATCH:
	case GameVersions::ORIGINAL:	
		viewport_x_offset = 0x14A;
		offset_bg_ptr = 0x13E;
		break;
	case GameVersions::VCLASSICS:
		viewport_x_offset = 0x152;
		offset_bg_ptr = 0x146;
		break;
	}


	
	Instructions instructions(DWORD(function_entry + 0x41));
	instructions.jmp(master->GetNextDetour());

	//Patches scrolling in the hangar for high resolution
	instructions << ByteArray{ 0x89, 0x86 }; 
	instructions << viewport_x_offset; //mov [esi+0x152], eax
	instructions << ByteArray{ 0x79, 0x0B }; //jns 0xC 
	instructions.jmp(function_entry + 0x61, FALSE);
	instructions << ByteArray{ 0x3B, 0xF9 }; // cmp edi, ecx
	instructions << ByteArray{ 0x7F, 0x02 }; // jg
	instructions << ByteArray{ 0x8B, 0xF9 }; // mov edi, ecx
	instructions << ByteArray{ 0x2B, 0xCF }; // sub ecx, edi
	instructions << ByteArray{ 0x3B, 0xC8 }; // cmp ecx, eax
	instructions << ByteArray{ 0x7D, 0x06 }; // jge
	instructions << ByteArray{ 0x89, 0x8E }; 
	instructions << viewport_x_offset; // mov [esi+152], ecx
	instructions.jmp(function_entry + 0x61, FALSE);
	size_t is_size1 = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size1);

	/*
	End result:
	esi+152 = viewport.x (edx)
    esi+18 = screen width (eax)	
	*/
	instructions.relocate(detour_entry_2);
	instructions.jmp(master->GetNextDetour());

	instructions << BYTE(0x51); //push ecx
	instructions << ByteArray{ 0x8B, 0x8E }; 
	instructions << offset_bg_ptr; //mov ecx, [esi+0x146]
	instructions << ByteArray{ 0x8B, 0x49, 0x08 }; //mov ecx, [ecx+0x8] 

	instructions << ByteArray{ 0x8B, 0x46, 0x18 }; //mov eax, [esi+0x18]
	instructions << ByteArray{ 0x2B, 0x46, 0x10 }; //sub eax, dword ptr ds:[esi+0x10]

	//Check if for some reason edx < 0 (shouldnt be), 
	//should be verified above when we assign in [esi+0x152]
	instructions << ByteArray{ 0x83, 0xFA, 00 }; // cmp edx, 0
	instructions << ByteArray{ 0x7F, 0x02 }; // jg 2
	instructions << ByteArray{ 0x33, 0xD2 }; //xor edx, edx

	instructions << ByteArray{ 0x03, 0xC2 }; //add eax, edx

	instructions << ByteArray{ 0x3B, 0xC1 }; //cmp eax, ecx
	instructions << ByteArray{ 0x7E, 0x0A }; //jle if screen width < background width

	instructions << ByteArray{ 0x2B, 0xC8 }; // sub ecx, eax (creates negative)
	instructions << ByteArray{ 0x03, 0xC1 }; //add eax, ecx (adds negative)
	instructions << ByteArray{ 0x03, 0xD1 }; //add edx, ecx (adds negative)
	instructions << ByteArray{ 0x79, 0x02 }; //jns
	instructions << ByteArray{ 0x33, 0xD2 }; //xor edx, edx
	
    //Continue
	instructions << BYTE(0x59); //pop ecx	
	instructions << ByteArray{ 0x8B, 0x5E, 0x20 }; //mov ebx, [esi + 20h]
	instructions.jmp(detour_return_2, FALSE); //continue back to push eax

	size_t is_size2 = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size2 - is_size1);
	printf("[Hangar Main] Generated a total of %d bytes\n", instructions.GetInstructions().size());
	master->instructions.push_back(instructions);
}

void GameData::PatchChopperDamageFunction(DetourMaster* master, GameVersions version)
{
	/*
	This patch forces the chopper damage calculation to always happen. Previously if max health - current health = 0 (no damage),
	then this calculation would be skipped. If this calculation is skipped, then for some reason the chopper would incorrectly
	sway as if it had taken some damage. Additionally, a chopper is never repaired to exactly its full health - due to division
	its possible to be 1-2 health below max, therefore the "max health sway" bug would only appear after a chopper was purchased
	or after the level switched. This patch only ensures that the calculation is always called regardless of damage taken.

	AOB: B9 14 00 00 00 8B 40 4C 2B 83 D0 00 00 00
	*/

	DWORD function_offset;
	unsigned int nop_count = 6; //different encoding on comparison
	switch (version)
	{
	case V11SC:
	case V11SC_FR:
		function_offset = 0xDB;
		nop_count = 3;
		break;
	case VCLASSICS:
	case V102_PATCH:
		function_offset = 0xD4;
		break;
	case ORIGINAL:
		function_offset = 0xD8;
		break;
	}
	DWORD function_entry = Versions[version]->functions.CHOPPER_RENDER_UNK1;
	Instructions instructions(DWORD(function_entry + function_offset));
	instructions.nop(nop_count); // nops out comparison
	instructions << BYTE(0xEB); //Changes jnz to jmp
	size_t is_size = instructions.GetInstructions().size();
	master->SetLastDetourSize(is_size);
	printf("[Max Health Patch] Generated a total of %d bytes\n", is_size);
	master->instructions.push_back(instructions);
}

void GameData::CreateResolutionFunction(DetourMaster *master, GameVersions version)
{
	DWORD function_entry = Versions[version]->functions.RES_LOOKUP;

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

void GameData::CreateGlobalInitFunction(DetourMaster *master, GameVersions version)
{
	//printf("DetourMaster starting at %x\n", master->current_location);
	DWORD function_entry = Versions[version]->functions.GLOBAL_INIT;
	DWORD function_res = Versions[version]->functions.RES_LOOKUP;

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