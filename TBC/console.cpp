#include "console.h"
#include <iostream>

Console::Console()
{
}

//Used to easily fill CHAR_INFO arrays
void ConvertString(const char * text, CHAR_INFO * result, int hexAttribute)
{
	wchar_t* converted = new wchar_t[strlen(text)];
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, strlen(text) * sizeof(char), converted, strlen(text));
	for (int i = 0; i < strlen(text); i++)
	{
		result[i].Char.UnicodeChar = text[i];
		result[i].Attributes = hexAttribute;
	}
	delete[]converted;
}

int Console::Initialize(Vec2 Screen)
{
	//Get Handle
	hConsole = (HANDLE)GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTitle("The Best Chat");

	screenSize.Left = 0;
	screenSize.Right = Screen.x - 1;
	screenSize.Top = 0;
	screenSize.Bottom = Screen.y - 1;

	//Resize screen buffer
	SetConsoleWindowInfo(hConsole, true, &screenSize);
	SetConsoleScreenBufferSize(hConsole, Screen.coord());

	hConsoleBack = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleWindowInfo(hConsoleBack, true, &screenSize);
	SetConsoleScreenBufferSize(hConsoleBack, Screen.coord());
	
	return 0;
}

void Console::Print(char* text, Vec2 topLeftCorner, Vec2 bottomRightCorner, int hexColour)
{
	CHAR_INFO* res = new CHAR_INFO[strlen(text)];
	ConvertString(text, res, hexColour);
	SMALL_RECT frame = screenSize;
	frame.Right += topLeftCorner.x + bottomRightCorner.x;
	frame.Left += topLeftCorner.x;
	frame.Bottom += topLeftCorner.y + bottomRightCorner.x;
	frame.Top += topLeftCorner.y;
	WriteConsoleOutput(hConsoleBack, res, bottomRightCorner.coord(), { 0,0 }, &frame);
	delete[] res;
}

void Console::Print(CHAR_INFO* text, Vec2 topLeftCorner, Vec2 bottomRightCorner)
{
	SMALL_RECT frame = screenSize;
	WriteConsoleOutput(hConsoleBack, text, bottomRightCorner.coord(), topLeftCorner.coord(), &frame);
}

void Console::Clear()
{
	COORD coordScreen = { 0, 0 };    // home for the cursor
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	// Get the number of character cells in the current buffer. 

	GetConsoleScreenBufferInfo(hConsoleBack, &csbi);

	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.

	FillConsoleOutputCharacter(hConsoleBack, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten);
	FillConsoleOutputAttribute(hConsoleBack, 0, dwConSize, coordScreen, &cCharsWritten);

	// Set the buffer's attributes accordingly.

	//FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);

	// Put the cursor at its home coordinates.
	//SetConsoleCursorPosition(hConsole, coordScreen);
}

void Console::Draw()
{
	SetConsoleActiveScreenBuffer(hConsoleBack);
	HANDLE temp = hConsoleBack;
	hConsoleBack = hConsole;
	hConsole = temp;
	Clear();
}