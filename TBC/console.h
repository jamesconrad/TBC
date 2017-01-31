#pragma once

#include "math.h"
#include <Windows.h>

class Console
{
public:
	HANDLE hConsole;
	HANDLE hConsoleBack;

	Console();
	int Initialize(Vec2 screen);

	void Print(char* text, Vec2 topLeftCorner, Vec2 bottomRightCorner, int hexColour);
	void Print(CHAR_INFO* text, Vec2 topLeftCorner, Vec2 bottomRightCorner);

	void Draw();
	void Clear();
private:
	SMALL_RECT screenSize;

	wchar_t* wconverted;
};