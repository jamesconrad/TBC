#include <iostream>
#include "console.h"




void main()
{

	Console con;
	con.Initialize(Vec2(80, 45));

	Vec2 origin(0,0);

	while (true)
	{
		con.Print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", Vec2(4,4), Vec2(5, 5), 0x0004 | 0x0008);
		origin.y++;
		if (origin.y >= 45)
			origin.y = 0;
		con.Draw();
	}
}