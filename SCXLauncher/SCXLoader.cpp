#include "SCXLoader.h"

std::string LastErrorString();
std::string CreateMD5Hash(std::string);
std::vector<std::string> split_string(char delim, std::string split_string);
MessageValue VerifyOriginalGame(std::string source);
std::string FormatDirectoryLocation(std::string full_exe_location);
MessageValue VerifyInstallationDirectory(std::string game_location);
MessageValue VerifyInstallation(GameVersions version);


namespace
{
	const std::string patch_file("SCXPatch.dat");
	const std::string game_file("SimCopter.exe");

	std::map<std::string,  GameVersions> version_hashes = 
	{
		{"6bc646d182ab8625a0d2394112334005", GameVersions::VCLASSICS},
		{"90db54003aa9ba881543c9d2cd0dbfbf", GameVersions::V11SC},
		{"d2f5c5eca71075696964d0f91b1163bf", GameVersions::V102_PATCH},
		{"b296b26e922bc43705b49f7414d7218f", GameVersions::V11SC_FR},
		{"17d5eba3e604229c4b87a68f20520b56", GameVersions::ORIGINAL}
	};

	std::map<GameVersions, std::string> version_description = 
	{
		{GameVersions::VCLASSICS, "Classics Version - February 1998"},
		{GameVersions::V11SC, "Version 1.1SC - 7 November 1996"},
		{GameVersions::ORIGINAL, "Version 1.0 - 7 November 1996"},
		{GameVersions::V102_PATCH, "Version 1.02 Patch - 26 February 1997"},
		{GameVersions::V11SC_FR, "Version 1.1SC (FR) - 7 November 1996"}
	};

	std::string SimCopterXDirectory;
	std::string SimCopterGameLocation = "";
	std::string SimCopterGameInstallDirectory = "";
	bool ValidInstallation = false;

	const std::string version_url("http://simcopter.net/versions.dat");
	std::string patched_hash;
	int patched_scxversion = -1;

	//PEINFO peinfo;
	GameVersions game_version;
	FileVersion fileVersion;
}

bool SCXLoader::GetValidInstallation()
{
	return ValidInstallation;
}

bool SCXLoader::GetFileCompatability(std::string game_location)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, KEY_READ, &hKey);
	CHAR lpData[512];
	DWORD lpcbData = sizeof(lpData);
	std::string format_location(game_location);
	std::replace(format_location.begin(), format_location.end(), '/', '\\');
	LSTATUS status = RegQueryValueEx(hKey, format_location.c_str(), 0, NULL, (LPBYTE)lpData, &lpcbData);
	if (status != ERROR_SUCCESS)
	{
		OutputDebugString(std::string("Registry Error: " + std::to_string((int)status) + "\n").c_str());
		OutputDebugString(std::string(format_location + "\n").c_str());
		return false;
	}
	return std::string(lpData).find("256COLOR") != std::string::npos;
}

int SCXLoader::GetPatchedSCXVersion()
{
	return patched_scxversion;
}

std::string SCXLoader::GetSimCopterGameLocation()
{
	return SimCopterGameLocation;
}

std::string SCXDirectory(std::string sappend)
{
	return std::string(SimCopterXDirectory).append(sappend);
}

MessageValue VerifyOriginalGame(std::string source)
{
	if (!PathFileExistsA(source.c_str()))
	{
		return MessageValue(FALSE, "The game does not exist at " + source + "\n");
	}

	std::string hash_check = CreateMD5Hash(source);
	auto it = version_hashes.find(hash_check);

	StringFileInfoMap sfiMap;
	MessageValue result = fileVersion.GetSCFileVersionInfo(source.c_str(), sfiMap);
	std::string sfi_result = "";
	if (result.Value)
	{
		sfi_result += "\nFile Information\n";
		for (auto sfit = sfiMap.begin(); sfit != sfiMap.end(); sfit++)
		{
			sfi_result += sfit->first + ": " + sfit->second + "\n";
		}
	}
	else
	{
		sfi_result += "\nUnable to query the SimCopter.exe File Information because: \n";
		sfi_result += result.Message;
	}	
	
	if(it == version_hashes.end())
	{
		std::string no_match = "SimCopterX does not have a matching hash stored in the database of patched versions.\n";
		no_match += "Hash: " + hash_check + "\n";
		no_match += sfi_result;
		return MessageValue(FALSE, no_match);
	}

	game_version = it->second;
	return MessageValue(TRUE, sfi_result);
}

bool CreatePatchFile()
{
	std::string hash_reference = CreateMD5Hash(SimCopterGameLocation);
	OutputDebugString(std::string("Created the hash reference: " + hash_reference + "\n").c_str());
	patched_hash = hash_reference;
	std::ofstream patch_stream(SCXDirectory(patch_file));
	if (patch_stream.is_open())
	{
		patch_stream << hash_reference << "," << patched_scxversion << ",";
		patch_stream << SimCopterGameLocation << "," << static_cast<int>(game_version) << ",";
		patch_stream << static_cast<int>(ValidInstallation);
		patch_stream.close();
		return true;
	}
	return false;
}

bool SCXLoader::CreatePatchedGame(std::string game_location, SCXParameters params)
{
	OutputDebugString(std::string("Attempting to patch game at " + game_location + "\n").c_str());

	if (game_location.empty()) //Means nothing was selected
		return false;

	/*
	If the game we selected is not an original, check to see if our directory has an original.
	If the game we selected is an original, copy it to our SimCopterX directory
	*/
	bool using_backup_copy = false;
	MessageValue verifyCurrentValue;

	std::string GameLocation = FormatDirectoryLocation(game_location);

	if (!(verifyCurrentValue = VerifyOriginalGame(GameLocation)).Value)
	{
		if (!params.verify_install)
		{
			std::string message_body = "The SimCopter game you selected isn't supported or is already modified/patched. Not ";
			message_body += "attempting to use a backup copy since 'Verify Install' was not selected.\n";
			ShowMessage("SimCopterX Error", message_body);
			return false;
		}

		if (!VerifyOriginalGame(SCXDirectory("SimCopter.exe")).Value)
		{
			std::string message_body = "The SimCopter game you selected isn't supported or is already modified/patched. SimCopterX ";
			message_body += "could not find an original backup. If this is an official version of the game, please submit the following ";
			message_body += "file information so it can be supported.\n\n";
			message_body += verifyCurrentValue.Message;
			ShowMessage("SimCopterX Error", message_body);
			return false;
		}
		
		using_backup_copy = true;
	}
	else
	{
		BOOL copy_result = CopyFileA(GameLocation.c_str(), SCXDirectory(game_file).c_str(), FALSE);
		if (!copy_result)
		{
			ShowMessage(LastErrorString(), "Failed to copy the original game from the source location to the SimCopterX directory.");
			return false;
		}
	}	

	if (params.verify_install)
	{
		MessageValue msg_verify;
		if (!(msg_verify = VerifyInstallationDirectory(GameLocation)).Value)
		{
			ShowMessage("SimCopterX Installation", msg_verify.Message);
			return false;
		}

		SimCopterGameLocation = SimCopterGameInstallDirectory + "SimCopter/SimCopter.exe";

		MessageValue msg_install;
		if (!(msg_install = VerifyInstallation(game_version)).Value)
		{
			ShowMessage("SimCopterX Installation", msg_install.Message);
			return false;
		}

		ValidInstallation = true;
	}
	else
	{
		SimCopterGameLocation = GameLocation;
	}

	BOOL copy_result2 = CopyFileA(SCXDirectory("SimCopter.exe").c_str(), SCXDirectory("SimCopterX.exe").c_str(), FALSE);
	if (!copy_result2)
	{
		ShowMessage(LastErrorString(), "Failed to create the intermediary SimCopterX patch game file.");
		return false;
	}

	PEINFO info;
	if (!Patcher::CreateDetourSection(SCXDirectory("SimCopterX.exe").c_str(), &info))
	{
		ShowMessage("SimCopterX Patch Failed", "Failed to modify the game's executable file.\n Make sure the game isn't running or opened in another application");
		return false;
	}
		
	std::vector<Instructions> instructions = GameData::GenerateData(info, game_version);
	DWORD sleep_address = info.GetDetourVirtualAddress(DetourOffsetType::MY_SLEEP);
	DWORD res_address = Versions[game_version]->data.RES_TYPE;
	instructions.push_back(DataValue(sleep_address, BYTE(params.sleep_time))); 
	instructions.push_back(DataValue(res_address, BYTE(params.resolution_mode)));

	if (!Patcher::Patch(info, instructions, SCXDirectory("SimCopterX.exe")))
	{
		ShowMessage("SimCopterX Patch Failed", "Failed to patch the game file.\n Make sure the game isn't running or opened in another application");
		return false;
	}

	BOOL copy_result3 = CopyFileA(SCXDirectory("SimCopterX.exe").c_str(), SimCopterGameLocation.c_str(), FALSE);
	if (!copy_result3)
	{
		ShowMessage(LastErrorString(), "Failed to copy the intermediary SimCopterX patch game file to the source directory");
		return false;
	}

	patched_scxversion = SCX_VERSION;

	BOOL delete_result = DeleteFileA(SCXDirectory("SimCopterX.exe").c_str());
	if (!delete_result)
	{
		OutputDebugString(std::string("Failed to delete intermediary SimCopterX game file. Still successful... Error: (" + std::to_string(GetLastError()) + ")").c_str());
	}

	if (params.verify_install && !CreatePatchFile())
	{
		ShowMessage("SimCopterX Error", "Failed to create the patch file which stores information about your specific patch.");
		return false;
	}

	std::string message = "";
	if (using_backup_copy)
		message += "Used Backup: " + version_description[game_version] + "\n";
	else
		message += "Detected Version: " + version_description[game_version] + "\n";

	message += "Patch location : \n" + game_location + "\n\n";
	if (params.verify_install)
		message += "If you modify or move this file, you will need to re-patch again.\n";
	else	
		message += "You patched this game without 'Verify Install', so you can't launch using SCXLauncher\n";
	

	ShowMessage("SimCopterX Patch Successful!", message);
	return true;
}

void ClearPatchFile(std::string reason)
{
	ShowMessage("SimCopterX Error", reason);
	SimCopterGameLocation = "";
	patched_hash = -1;
	patched_scxversion = -1;
	ValidInstallation = false;
	BOOL delete_result = DeleteFileA(SCXDirectory(patch_file).c_str());
	if (!delete_result)
	{
		OutputDebugString("Failed to delete the patch file \n");
	}
}

MessageValue VerifyInstallationDirectory(std::string game_location)
{
	//This is used for installing the game (if verify install is enabled)
	std::vector<std::string> path = split_string('/', game_location);
	SimCopterGameInstallDirectory = "";

	for (size_t i = path.size() - 2; i > 0; i--) //i - 1 = game exe
	{
		std::string path_check = "";
		for (size_t j = 0; j < i + 1; j++)
		{
			path_check += path.at(j) + "/";
		}
		//The top-level directory should have autorun.inf
		std::string auto_run = path_check + "autorun.inf";
		OutputDebugString(std::string("Checking: " + auto_run + "\n").c_str());
		if (PathFileExistsA(auto_run.c_str()))
		{
			SimCopterGameInstallDirectory = path_check;
			OutputDebugString(std::string("Install: " + SimCopterGameInstallDirectory).append("\n").c_str());
			return MessageValue(TRUE);
		}
	}
	return MessageValue(FALSE, "The game does not appear to be on a valid extracted CD, missing autorun.inf");
}

std::string FormatDirectoryLocation(std::string full_exe_location)
{
	std::string format_location(full_exe_location);
	std::replace(format_location.begin(), format_location.end(), '\\', '/');
	return format_location;
}

bool VerifyPatchedGame()
{
	if (!PathFileExistsA(SimCopterGameLocation.c_str()))
	{
		ClearPatchFile(std::string("The game no longer exists where we patched it:\n" + SimCopterGameLocation + "\nPlease try repatching.").c_str());
		return false;
	}

	std::string hash_check = CreateMD5Hash(SimCopterGameLocation);
	printf("Verification: ");
	printf(patched_hash.c_str());
	printf(hash_check.c_str());
	if (hash_check.compare(patched_hash) != 0)
	{
		ClearPatchFile("The patched game doesn't have a matching hash, this can happen if you modified the patched game or restored it back to the original game. Please try repatching.");
		return false;
	}

	if (patched_scxversion != SCXLoader::SCX_VERSION)
	{
		ClearPatchFile(std::string("You currently have SimCopterX Version " + std::to_string(SCXLoader::SCX_VERSION) + " however the game was \npreviously patched using Version " +
			std::to_string(patched_scxversion) + ". Please repatch the game."));
		return false;
	}

	return true;
}

bool SCXLoader::StartSCX(SCXParameters params)
{

	if (patched_scxversion < 0) //This shouldn't happen because the button should not be enabled
	{
		ShowMessage("SimCopterX Error", "You need to patch the game before starting.");
		return false;
	}

	if (!ValidInstallation) //This shouldn't happen because the button should not be enabled
	{
		ShowMessage("SimStreetsX Error", "You need to patch the game using 'Verify Install'");
		return false;
	}

	if (!VerifyPatchedGame())
	{
		return false;
	}
	
	if (!params.fullscreen && !GetFileCompatability(SimCopterGameLocation))
	{
		const char *message =
			"You must run the SimCopter.exe in 8-bit color to use Windowed mode!\n\n"
			"1. Right Click SimCopter.exe -> Properties\n"
			"2. Select the 'Compatibility' tab\n"
			"3. Enable 'Reduced color mode' and select 8-bit/256 color\n"
			"4. Ensure all other options are NOT selected\n"
			"5. Click apply, then try again";
		ShowMessage("SimCopterX", std::string(message));
		return false;
	}	

	PEINFO info;
	if (!Patcher::CreateDetourSection(SimCopterGameLocation.c_str(), &info)) //Should grab detour section
	{
		ShowMessage("SimCopterX Patch Failed", "Failed to modify the game's executable file.\n Make sure the game isn't running or opened in another application");
		return false;
	}

	std::vector<Instructions> instructions;
	DWORD sleep_address = info.GetDetourVirtualAddress(DetourOffsetType::MY_SLEEP);
	DWORD res_address = Versions[game_version]->data.RES_TYPE;
	instructions.push_back(DataValue(sleep_address, BYTE(params.sleep_time)));
	instructions.push_back(DataValue(res_address, BYTE(params.resolution_mode)));

	if (!Patcher::Patch(info, instructions, SimCopterGameLocation.c_str()))
	{
		ShowMessage("SimCopterX Patch Failed", "Failed to patch the game file.\n Make sure the game isn't running or opened in another application");
		return false;
	}

	//Can only get the hash at this point as a reference, can't use it to check for complete validty
	//because changing sleep time and resolution mode dwords will change the hash

	CreatePatchFile();

	std::string parameters = params.fullscreen ? "-f" : "-w";
	HINSTANCE hInstance = ShellExecuteA(NULL, "open", SimCopterGameLocation.c_str(), parameters.c_str(), NULL, SW_SHOWDEFAULT);
	int h_result = reinterpret_cast<int>(hInstance);
	if (h_result <= 31)
	{
		ShowMessage(std::string("SimCopterX Error (" + std::to_string(h_result) + ")"), std::string("Failed to start the patched game at: \n" + SimCopterGameLocation));
		return false;
	}
	return true;
}

bool SCXLoader::LoadFiles()
{
	char* buf = nullptr;
	size_t sz = 0;
	if (_dupenv_s(&buf, &sz, "ProgramFiles") > 0 || buf == nullptr)
	{
		return false;
	}

	SimCopterXDirectory = std::string(buf).append("\\SimCopterX");
	if (!PathFileExistsA(SimCopterXDirectory.c_str()))
	{
		if (!CreateDirectoryA(SimCopterXDirectory.c_str(), NULL))
		{
			//printf("Failed to create directory: %d", GetLastError());
			return false;
		}
	}
	SimCopterXDirectory.append("\\");
	OutputDebugString(std::string("Directory: ").append(std::string(SimCopterXDirectory)).append("\n").c_str());

	if (PathFileExistsA(SCXDirectory(patch_file).c_str()))
	{
		//Reads and verifes that the patchfile is in the correct format
		char szBuff[128];
		bool valid_patchfile = true;
		std::ifstream fin(SCXDirectory(patch_file));
		if (fin.good())
		{
			fin.getline(szBuff, 128);
			fin.close();
			std::vector<std::string> props = split_string(',', std::string(szBuff));
			if (props.size() == 5)
			{
				patched_hash = props.at(0);
				patched_scxversion = std::atoi(props.at(1).c_str());
				SimCopterGameLocation = props.at(2);
				game_version = static_cast<GameVersions>(std::atoi(props.at(3).c_str()));
				ValidInstallation = std::atoi(props.at(4).c_str());
				OutputDebugString(std::string("Game is patched at: " + SimCopterGameLocation + "\n").c_str());	
				OutputDebugString(std::string("Game version enum: " + std::to_string(static_cast<int>(game_version)) + "\n").c_str());
			}
			else
			{
				valid_patchfile = false;
				ClearPatchFile("The patch file does not appear to be valid, please repatch the game");
			}			
		}

		//Verifies that the actual contents of the file match what we have
		if (valid_patchfile)
		{
			if (!VerifyPatchedGame())
			{
				OutputDebugString("Failed to verify patched game files \n");
			}

		}
	}
	else
	{
		OutputDebugString("There currently does not exist a patch file \n");
	}


	OutputDebugString("Succesfully finished loading files \n");
	return true;
}


std::string CreateMD5Hash(std::string filename_string)
{
	std::wstring filename_wstring = std::wstring(filename_string.begin(), filename_string.end());
	LPCWSTR filename = filename_wstring.c_str();

	DWORD cbHash = 16;
	HCRYPTHASH hHash = 0;
	HCRYPTPROV hProv = 0;
	BYTE rgbHash[16];
	CHAR rgbDigits[] = "0123456789abcdef";
	HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);

	BOOL bResult = FALSE;
	DWORD BUFSIZE = 4096;
	BYTE rgbFile[4096];
	DWORD cbRead = 0;
	while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
	{
		if (0 == cbRead)
			break;

		CryptHashData(hHash, rgbFile, cbRead, 0);
	}

	std::string md5_hash = "";
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
	{
		for (DWORD i = 0; i < cbHash; i++)
		{
			char buffer[3]; //buffer needs terminating null
			sprintf_s(buffer, 3, "%c%c", rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]);
			md5_hash.append(buffer);
		}
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		CloseHandle(hFile);
		return std::string(md5_hash);
	}
	else
	{
		OutputDebugString(std::string("CryptGetHashParam Error: " + std::to_string(GetLastError()) + "\n").c_str());
		return std::string("");
	}
}

MessageValue VerifyInstallation(GameVersions version)
{

	//Yeah all of this is inefficient but who cares, you patch once and this is
	//clean/easy to understand

	std::string SimCopterGameLocationDirectory(SimCopterGameLocation);
	auto it = SimCopterGameLocationDirectory.find("SimCopter.exe");
	if (it == std::string::npos)
	{	//If this happens... YIKES
		return MessageValue(FALSE, "Could not find 'SimCopter.exe' in directory location\n"); 
	}
	OutputDebugString(std::string(SimCopterGameLocationDirectory + "\n").c_str());
	OutputDebugString(std::string(std::to_string(it) + "\n").c_str());
	SimCopterGameLocationDirectory.erase(it, SimCopterGameLocationDirectory.size() - 1);

	OutputDebugString(std::string(SimCopterGameLocationDirectory + "\n").c_str());
	OutputDebugString(std::string(SimCopterGameLocation + "\n").c_str());

	std::map<std::string, std::string> dll_source = 
	{
		{"smackw32.dll", "setup/smacker/"},
		{"sst1init.dll", "setup/system/"},
		{"glide.dll", "setup/system/"}
	};

	std::map<GameVersions, std::vector<std::string>> dll_map = 
	{
		{ GameVersions::ORIGINAL,	{"smackw32.dll"}},
		{ GameVersions::V11SC,		{"smackw32.dll"}},
		{ GameVersions::V11SC_FR,	{"smackw32.dll"}},
		{ GameVersions::V102_PATCH, {"sst1init.dll", "glide.dll", "smackw32.dll"}},
		{ GameVersions::VCLASSICS,  {"sst1init.dll", "glide.dll", "smackw32.dll"}}
	};

	for (std::string dll : dll_map[version])
	{
		std::string destination = SimCopterGameLocationDirectory + dll;
		if (PathFileExistsA(destination.c_str()))
		{
			OutputDebugString(std::string(dll + " already exists \n").c_str());
			continue;
		}

		std::string copy_location = SimCopterGameInstallDirectory + dll_source[dll] + dll;
		if (!PathFileExistsA(copy_location.c_str()))
		{
			std::string message = "Your game version is: \n" + version_description[version];
			message += "\n\nWe could not find the required dll: " + dll + "\nExpected location:\n";
			message += copy_location + "\n\n";
			message += "This usually happens if you are trying to mismatch game versions with different CDs. ";
			message += "Try using the 'Classics CD', it should have all the dlls so you can use any game version.\n";
			return MessageValue(FALSE, message);
		}

		BOOL copy_result = CopyFileA(copy_location .c_str(), destination.c_str(), FALSE);
		if (!copy_result)
		{
			std::string message = "Failed to copy file from: \n" + copy_location + "\n\n";
			message += "To destination location: \n" + destination + "\n\n";
			message += "Ensure another program isn't accessing these files.";
			return MessageValue(FALSE, message);
		}
		OutputDebugString(std::string("Copied " + copy_location + " to: " + destination + "\n").c_str());
	}
	return MessageValue(TRUE);
}

std::vector<std::string> split_string(char delim, std::string split_string)
{
	std::vector<std::string> vector;
	std::string splitter(split_string);
	while (splitter.find(delim) != std::string::npos)
	{
		auto sfind = splitter.find(delim);
		vector.push_back(splitter.substr(0, sfind));
		splitter.erase(0, sfind + 1);
	}
	vector.push_back(splitter);
	return vector;
}
