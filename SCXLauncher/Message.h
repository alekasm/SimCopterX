#pragma once
#include <Windows.h>
#include <string>

static void ShowMessage(std::string title, std::string message)
{
	MessageBox(NULL, message.c_str(), title.c_str(), MB_OK);
	OutputDebugString(message.c_str());
}

static std::string LastErrorString()
{
	return std::string("SimCopterX Error (" + std::to_string(GetLastError()) + ")");
}

struct MessageValue
{
	BOOL Value;
	std::string Message;
	MessageValue(BOOL value, std::string message)
	{
		Value = value;
		Message = message;
	}

	MessageValue(BOOL value)
	{
		Value = value;
		Message = "";
	}

	MessageValue()
	{
		Value = FALSE;
		Message = "";
	}
};

