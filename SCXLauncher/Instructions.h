#pragma once
#include <Windows.h>
#include <vector>

typedef std::initializer_list<BYTE> ByteArray;


struct Instruction {
	BYTE byte;
	DWORD address;
	Instruction(DWORD address, BYTE byte)
	{
		this->address = address;
		this->byte = byte;
	}
};

struct StringValue {
	std::string string;
	unsigned int size_alignment;
	StringValue(std::string string, unsigned int size_alignment)
	{
		this->string = string;
		this->size_alignment = size_alignment;
	}
};

class Instructions
{
public:

	void operator<<(BYTE b)
	{
		container.push_back(Instruction(current_location++, b));
	}

	void operator<<(std::initializer_list<BYTE> bytes)
	{
		for (BYTE b : bytes)
		{
			container.push_back(Instruction(current_location++, b));
		}
	}

	void operator<<(DWORD address)
	{
		BYTE byte[sizeof(DWORD)];
		memcpy(byte, &address, sizeof(DWORD));
		for (int i = 0; i < sizeof(DWORD); i++)
		{
			container.push_back(Instruction(current_location++, byte[i]));
		}
	}

	void operator<<(StringValue string_value)
	{
		unsigned int track_address = 0;
		for (char character : string_value.string)
		{
			operator<<(BYTE(character));
			track_address++;
		}
		if (track_address < string_value.size_alignment)
			relocate(current_location + (string_value.size_alignment - track_address));
	}

	Instructions(DWORD address)
	{
		current_location = address;
	}

	void relocate(DWORD address)
	{
		current_location = address;
	}

	void nop(size_t amount)
	{
		for (size_t i = 0; i < amount; i++)
			operator<<(BYTE(0x90));
	}

	void jmp(DWORD address, BOOL change_location)
	{
		DWORD next_address = current_location + 0x5;
		DWORD encoding = address - next_address;
		operator<<(BYTE(0xE9));
		operator<<(encoding);
		if (change_location)
			current_location = address;
	}

	void jmp(DWORD address)
	{
		jmp(address, TRUE);
	}

	void jnz(DWORD address)
	{
		DWORD next_address = current_location + 0x6;
		DWORD encoding = address - next_address;
		operator<<(BYTE(0x0F));
		operator<<(BYTE(0x85));
		operator<<(encoding);
	}

	void jge(DWORD address)
	{
		DWORD next_address = current_location + 0x6;
		DWORD encoding = address - next_address;
		operator<<(BYTE(0x0F));
		operator<<(BYTE(0x8D));
		operator<<(encoding);
	}

	void cmp(DWORD address, BYTE value)
	{
		//operator<<(BYTE(0x3E)); //segment overload
		operator<<(ByteArray{ 0x83, 0x3D }); //cmp
		operator<<(address);
		operator<<(value);
	}

	void call(DWORD address)
	{
		DWORD next_address = current_location + 0x5;
		DWORD encoding = address - next_address;
		operator<<(BYTE(0xE8));
		operator<<(encoding);
	}

	//FF /2	CALL r/m32
	//https://c9x.me/x86/html/file_module_x86_id_26.html
	//http://ref.x86asm.net/coder32.html#modrm_byte_32
	//https://stackoverflow.com/questions/15017659/how-to-read-the-intel-opcode-notation/41616657#41616657
	void call_disp32(DWORD address)
	{
		//3E = DS segment override prefix
		//FF 15 = call near absolute indirect, /2 disp32 = 0x15
		operator<<(ByteArray{ 0xFF, 0x15 });
		operator<<(address);
	}

	void call_rm32(DWORD address)
	{
		//3E = DS segment override prefix (unused)
		//FF 15 = call near absolute indirect, /2 disp32 = 0x15
		operator<<(ByteArray{ 0xFF, 0x15 });
		operator<<(address);
	}

	void push_rm32(DWORD address)
	{
		//3E = DS segment override prefix (unused)
		//FF 35 = push near absolute indirect, /6 disp32 = 0x35
		operator<<(ByteArray{ 0xFF, 0x35 });
		operator<<(address);
	}

	std::vector<Instruction> GetInstructions()
	{
		return container;
	}

	DWORD GetCurrentLocation()
	{
		return current_location;
	}

private:
	std::vector<Instruction> container;
	DWORD current_location;
};

class DataValue : public Instructions
{
public:
	DataValue(DWORD address, BYTE value) : Instructions(address)
	{
		operator<<(value);
	}	
};