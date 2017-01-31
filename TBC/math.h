#pragma once

#include <Windows.h>


class Vec2
{
public:
	int x, y;
	Vec2() { x = y = 0; }
	Vec2(int _x, int _y) { x = _x; y = _y; }

	COORD coord()
	{
		COORD c;
		c.X = x;
		c.Y = y;
		return c;
	}

	Vec2 operator+ (const Vec2 &r)
	{
		return Vec2(x + r.x, y + r.y);
	}
	Vec2 operator+= (const Vec2 &r)
	{
		x += r.x; y += r.y;
	}
	Vec2 operator- (const Vec2 &r)
	{
		return Vec2(x - r.x, y - r.y);
	}
	Vec2 operator-= (const Vec2 &r)
	{
		x -= r.x; y -= r.y;
	}
	Vec2 operator* (const int f)
	{
		return Vec2(x*f, y*f);
	}
	Vec2 operator*= (const int f)
	{
		x *= f;
		y *= f;
	}
};