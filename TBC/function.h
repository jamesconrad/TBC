#pragma once
#include <Windows.h>
#include <sstream>
#include <iterator>

enum Colour
{
	FG_BLUE = 0x0001,
	FG_GREEN = 0x0002,
	FG_RED = 0x0004,
	FG_INTENSITY = 0x0008,
	BG_BLUE = 0x0010,
	BG_GREEN = 0x0020,
	BG_RED = 0x0040,
	BG_INTENSITY = 0x0080,
	UNDERSCORE = 0x8000
};

struct User
{
	short userID;
	int colourCode;
	char name[14];

	enum Status { Online, Busy, InGame };
	Status status;
};

char* convertStatus(User::Status s)
{
	if (s == User::Status::Online)
		return "        Online";
	else if (s == User::Status::Busy)
		return "          Busy";
	else
		return "       In Game";
}

int statusColour(User::Status s)
{
	if (s == User::Status::Online)
		return FG_GREEN | FG_INTENSITY;
	else if (s == User::Status::Busy)
		return FG_RED | FG_INTENSITY;
	else
		return FG_GREEN | BG_GREEN | FG_INTENSITY;
}

char keypress()
{
	bool kp[256];
	for (int i = 0; i < 256; i++)
	{
		if (GetAsyncKeyState(i))
			kp[i] = true;
		else
			kp[i] = false;
	}

	short caps = 0;

	if (kp[VK_RETURN])
		return 1;
	else if (kp[VK_BACK])
		return 0;

	if (kp[VK_SHIFT])
		caps = 1;

	for (int i = 0; i < 26; i++)
		if (kp[0x41 + i])
			return !caps ? 'a' + i : 'a' + i - 32;

	//why cant ascii values just play nice like alpha characters
	if (kp[0x30])
		return !caps ? '0' : ')';
	else if (kp[0x31])
		return !caps ? '1' : '!';
	else if (kp[0x32])
		return !caps ? '2' : '@';
	else if (kp[0x33])
		return !caps ? '3' : '#';
	else if (kp[0x34])
		return !caps ? '4' : '$';
	else if (kp[0x35])
		return !caps ? '5' : '%';
	else if (kp[0x36])
		return !caps ? '6' : '^';
	else if (kp[0x37])
		return !caps ? '7' : '&';
	else if (kp[0x38])
		return !caps ? '8' : '*';
	else if (kp[0x39])
		return !caps ? '9' : '(';//numbers over
	else if (kp[VK_OEM_4])//[
		return !caps ? '[' : '{';
	else if (kp[VK_OEM_6])//]
		return !caps ? ']' : '}';
	else if (kp[VK_OEM_5])//'\'
		return !caps ? '\\' : '|';
	else if (kp[VK_OEM_1])//;
		return !caps ? ';' : ':';
	else if (kp[VK_OEM_7])//'
		return !caps ? '\'' : '\"';
	else if (kp[VK_OEM_COMMA])//,
		return !caps ? ',' : '<';
	else if (kp[VK_OEM_PERIOD])//.
		return !caps ? '.' : '>';
	else if (kp[VK_OEM_2])///
		return !caps ? '/' : '?';
	else if (kp[VK_OEM_MINUS])
		return !caps ? '-' : '_';//-
	else if (kp[VK_OEM_PLUS])
		return !caps ? '=' : '+';//=
	else if (kp[VK_OEM_3])
		return !caps ? '`' : '~';
	else if (kp[VK_SPACE])
		return ' ';


	return 2;
}