#include "client.h"
#include <iostream>
#include <queue>
#include "console.h"
#include "function.h"


#define CHATBOX_WIDTH 56

//Things that need to be done
//parsecommand needs a boolean to reupload/sync servers copy of client list
//parsecommand needs its joining/leaving and voice done

//root points
Vec2 chatbox = Vec2(20, 40);
Vec2 messagebox = Vec2(20, 3);
Vec2 lobby = Vec2(2, 2);
Console con;
User serverUser;
User thisUser;
std::vector<User> onlineUsers;

char lobbyname[12] = "DEFAULT    ";

struct Message
{
	Message(short uid, const char* msg) { userID = uid; memcpy(message, msg, 112); }
	short userID;
	char message[112];
};

void parseCommand(std::queue<Message> &q, std::string &msg);

void main()
{
	setup();
	char ip[128];

	do
	{
		std::cout << "Enter the server IP: ";
		std::cin >> ip;

		//connect(ip);
	} while (!connect(ip));

	std::string inputmsg = "";
	con.Initialize(Vec2(80, 45));
	Vec2 origin(0,0);
	std::queue<Message> messages;
	thisUser.colourCode = FG_RED | FG_BLUE | FG_GREEN | FG_INTENSITY;
	thisUser.userID = 0;
	thisUser.status = User::Online;
	memcpy(&serverUser, &thisUser, sizeof(User));
	memcpy(thisUser.name, "Anonymous     ",14);
	onlineUsers.push_back(thisUser);
	memcpy(serverUser.name, "SERVER        ", 14);
	serverUser.userID = -1;

	Packet p = Packet::USER_UPDATE;
	char initsend[32];
	memcpy(initsend, &p, 4);
	memcpy(initsend + 4, &thisUser, sizeof(User));
	nsend(initsend, 32);

	char pkp = '\0';
	while (true)
	{
		//main loop
		//recv server msg
		char recvd[1024];
		memset(recvd, ' ', 1024);
		recvd[1023] = '\0';
		int torecv = nrecv(recvd, 1024, MSG_PEEK);
		//int recvb = nrecv(recvd, sizeof(Packet));//only recv the packet
		if (torecv >= 4)
		{
			//packet extraction
			Packet recvp;
			memcpy(&recvp, recvd, sizeof(Packet));
			//time to get the rest
			int ptotal = 0;
			//int offset = sizeof(Packet);
			if (recvp == Packet::USER_UPDATE)
			{
				//int n = nrecv(recvd + sizeof(Packet), sizeof(User));
				//if (n == -1)
				//	recvp = Packet::GAME_OFF;
				ptotal = sizeof(Packet) + sizeof(User);
			}
			else if (recvp == Packet::LOBBY_CHANGE)
			{
				////retrieve name
				//nrecv(recvd + offset, 12);
				//offset += 12;
				////retrieve num users
				//nrecv(recvd + offset, sizeof(int));
				//offset += sizeof(int);
				//int n;
				//memcpy(&n, recvd + offset - sizeof(int), sizeof(int));
				//nrecv(recvd + offset, n * sizeof(User));


				int n = 0;
				memcpy(&n, recvd + 4 + 12, sizeof(int));
				ptotal = sizeof(User) * n + 12 + sizeof(Packet) + sizeof(int);
			}
			else if (recvp == Packet::MESSAGE)
			{
				//nrecv(recvd + offset, sizeof(Message));

				ptotal = sizeof(Message) + sizeof(Packet);
			}

			int precv = nrecv(recvd, ptotal);

			if (recvp == Packet::MESSAGE)
			{
				Message m(0, " ");
				memcpy(&m, recvd + sizeof(Packet), sizeof(m));
				messages.push(m);
			}
			else if (recvp == Packet::LOBBY_CHANGE)
			{//packet, name, num users, user struct array
				int offset = sizeof(Packet);
				memcpy(lobbyname, recvd + offset, 12);
				offset += 12;
				int numusr;
				memcpy(&numusr, recvd + offset, 4);
				offset += 4;
				onlineUsers.resize(numusr);
				memcpy(&onlineUsers[0], recvd + offset, numusr * sizeof(User));
			}
			else if (recvp == Packet::USER_UPDATE)
			{
				memcpy(&thisUser, recvd + sizeof(Packet), sizeof(User));
			}
		}


		//input
		char kp = keypress();
		if (kp != pkp)
		{
			if (kp > 2)
				inputmsg += kp;
			else if (kp == 0)
				inputmsg = inputmsg.substr(0, inputmsg.length() - 1);
			else if (kp == 1)
			{
				if (inputmsg[0] == '/')
					parseCommand(messages, inputmsg);
				else
				{
					for (int i = inputmsg.length(); i < 112; i++)
						inputmsg += " ";
					char sendd[118];
					Packet p = Packet::MESSAGE;
					Message smesg(thisUser.userID, inputmsg.c_str());
					memcpy(sendd, &p, sizeof(Packet));
					memcpy(sendd + sizeof(Packet), &smesg, sizeof(Message));
					nsend(sendd, 118);
					//messages.push(Message(thisUser.userID, inputmsg.c_str()));
				}
				inputmsg = "";
			}
		}
		pkp = kp;


		std::string printmsg = inputmsg;
		int padding = CHATBOX_WIDTH - inputmsg.length() % CHATBOX_WIDTH;
		char* spaces = new char[padding];
		for (int i = 0; i < padding; i++)
			printmsg += ' ';
		free(spaces);
		//con.Print(&kp, Vec2(0,1), Vec2(1,1), 0x0004 | 0x0008);
		con.Print(printmsg.c_str(), chatbox, Vec2(inputmsg.length() > CHATBOX_WIDTH ? CHATBOX_WIDTH : inputmsg.length(), inputmsg.length() / CHATBOX_WIDTH + 1), thisUser.colourCode);
		
		//chatbox
		std::queue<Message> chat = messages;
		
		int msgqueuesize = messages.size();
		for (int i = 0; i < msgqueuesize; i++)
		{
			User u = chat.front().userID == -1 ? serverUser : chat.front().userID > onlineUsers.size() - 1 ? thisUser : onlineUsers[chat.front().userID];
			con.Print(u.name, Vec2(messagebox.x, messagebox.y + i * 3 - 1), Vec2(14, 1), u.colourCode);
			con.Print(chat.front().message, Vec2(messagebox.x, messagebox.y + i * 3), Vec2(CHATBOX_WIDTH, 2), u.colourCode);
			chat.pop();
		}

		for (int i = msgqueuesize; i > 11; i--)
			messages.pop();

		//con.Print("Hello World.", Vec2(0, 2), Vec2(12, 1), BG_GREEN | BG_BLUE | FG_RED | FG_GREEN | FG_INTENSITY);

		con.Print(lobbyname, lobby, Vec2(12, 1), BG_BLUE | FG_GREEN | FG_INTENSITY);
		for (int i = 0; i < onlineUsers.size(); i++)
		{
			con.Print(onlineUsers[i].name, Vec2(lobby.x, lobby.y + 2 + i * 3), Vec2(14, 1), onlineUsers[i].colourCode);
			con.Print(convertStatus(onlineUsers[i].status), Vec2(lobby.x, lobby.y + 3 + i * 3), Vec2(14, 1), statusColour(onlineUsers[i].status));
		}

		con.Draw();
	}

}

int stringToColour(std::string s)
{
	int c = 0;
	if (!strcmp(s.c_str(), "fgred"))
		c = FG_RED;
	else if (!strcmp(s.c_str(), "fggreen"))
		c = FG_GREEN;
	else if (!strcmp(s.c_str(), "fgblue"))
		c = FG_BLUE;
	else if (!strcmp(s.c_str(), "fgintensity"))
		c = FG_INTENSITY;
	else if (!strcmp(s.c_str(), "bgred"))
		c = BG_RED;
	else if (!strcmp(s.c_str(), "bggreen"))
		c = BG_GREEN;
	else if (!strcmp(s.c_str(), "bgblue"))
		c = BG_BLUE;
	else if (!strcmp(s.c_str(), "bgintensity"))
		c = BG_INTENSITY;
	return c;
}

void parseCommand(std::queue<Message> &q, std::string &msg)
{
	std::istringstream iss(msg);
	std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };
	//tokens now contains a series of space seperated strings

	//0 will be the slash command
	//1 - n are args

	if (strcmp("/help", tokens[0].c_str()) == 0)
	{
		q.push(Message(thisUser.userID, "/join : /game : /status : /colour : /name : /voice : /quit;                                                                 "));
	}
	else if (strcmp("/join", tokens[0].c_str()) == 0)
	{
		if (tokens.size() == 1 || tokens.size() > 1)
		{
			while (!q.empty())
				q.pop();

			Packet p = Packet::LOBBY_JOIN;
			for (int i = tokens[1].length(); i < 12; i++)
				tokens[1].append(" ");
			char msg[44];
			memcpy(msg, &p, sizeof(Packet));
			memcpy(msg + sizeof(Packet), tokens[1].c_str(), 12);
			memcpy(msg + sizeof(Packet) + 12, &thisUser, sizeof(User));

			nsend(msg, 44);


			p = Packet::USER_UPDATE;
			char msg2[32];
			memcpy(msg2, &p, sizeof(Packet));
			memcpy(msg2 + sizeof(Packet), &thisUser, sizeof(User));

			nsend(msg2, 32);
		}
		else
			q.push(Message(thisUser.userID, "Please enter a lobby name to join. If it doesn't exist then it will be created and joined.                                    "));

	}
	else if (strcmp("/game", tokens[0].c_str()) == 0)
	{
		if (tokens.size() == 1 || tokens.size() > 1 && !strcmp(tokens[1].c_str(), "on"))
		{
			Packet p = Packet::GAME_GTN;
			char msg[4];
			memcpy(msg, &p, 4);
			nsend(msg, 4);
		}
		else if (tokens.size() == 1 || tokens.size() > 1 && !strcmp(tokens[1].c_str(), "off"))
		{
			Packet p = Packet::GAME_OFF;
			char msg[4];
			memcpy(msg, &p, 4);
			nsend(msg, 4);
		}
		else
			q.push(Message(thisUser.userID, "/game [on, off] : Begin or end a game of Guess The Number"));
			
	}
	else if (strcmp("/status", tokens[0].c_str()) == 0)
	{
		if (tokens.size() == 1 || tokens.size() > 1 && !strcmp(tokens[1].c_str(), "help"))
			q.push(Message(thisUser.userID, "/status [online, busy] : Change user status                                                                                                "));
		else if (!strcmp(tokens[1].c_str(), "online"))
		{
			thisUser.status = User::Status::Online;
			onlineUsers[thisUser.userID] = thisUser;

			Packet p = Packet::USER_UPDATE;
			char msg[32];
			memcpy(msg, &p, sizeof(Packet));
			memcpy(msg + sizeof(Packet), &thisUser, sizeof(User));

			nsend(msg, 32);
		}
		else if (!strcmp(tokens[1].c_str(), "busy"))
		{
			thisUser.status = User::Status::Busy;
			onlineUsers[thisUser.userID] = thisUser;

			Packet p = Packet::USER_UPDATE;
			char msg[32];
			memcpy(msg, &p, sizeof(Packet));
			memcpy(msg + sizeof(Packet), &thisUser, sizeof(User));

			nsend(msg, 32);
		}
		else
			q.push(Message(thisUser.userID, "Error : Please use /status help                                                                                                        "));
	}
	else if (strcmp("/colour", tokens[0].c_str()) == 0)
	{
		if (tokens.size() == 1 || tokens.size() > 1 && !strcmp(tokens[1].c_str(), "help"))
			q.push(Message(thisUser.userID, "/colour [add, remove, current, list] [colours] : Use modify your user colour.                                                            "));
		else if (tokens.size() >= 2)
		{
			if (!strcmp(tokens[1].c_str(), "list"))
				q.push(Message(thisUser.userID, "Colour options : fgred, fggreen, fgblue, fgintensity, bgred, bggreen, bgblue, bgintensity                                           "));
			else if (!strcmp(tokens[1].c_str(), "current"))
			{
				std::string s;
				if (thisUser.colourCode & FG_RED)
					s += "fgred";
				if (thisUser.colourCode & FG_GREEN)
					s += ", fggreen";
				if (thisUser.colourCode & FG_BLUE)
					s += ", fgblue";
				if (thisUser.colourCode & FG_INTENSITY)
					s += ", fgintensity";
				if (thisUser.colourCode & BG_RED)
					s += ", bgred";
				if (thisUser.colourCode & BG_GREEN)
					s += ", bggreen";
				if (thisUser.colourCode & BG_BLUE)
					s += ", bgblue";
				if (thisUser.colourCode & BG_INTENSITY)
					s += ", bgintensity";

				for (int i = s.length(); i < 112; i++)
					s += ' ';

				q.push(Message(thisUser.userID, s.c_str()));
			}
			else if (!strcmp(tokens[1].c_str(), "add"))
			{
				thisUser.colourCode = thisUser.colourCode | stringToColour(tokens[2]);
				onlineUsers[thisUser.userID] = thisUser;

				Packet p = Packet::USER_UPDATE;
				char msg[32];
				memcpy(msg, &p, 4);
				memcpy(msg + 4, &thisUser, sizeof(User));
				nsend(msg, 32);
			}
			else if (!strcmp(tokens[1].c_str(), "remove"))
			{
				thisUser.colourCode = thisUser.colourCode ^ stringToColour(tokens[2]);
				onlineUsers[thisUser.userID] = thisUser;

				Packet p = Packet::USER_UPDATE;
				char msg[32];
				memcpy(msg, &p, 4);
				memcpy(msg + 4, &thisUser, sizeof(User));
				nsend(msg, 32);
			}
		}
		else
			q.push(Message(thisUser.userID, "Error : Please use /colour help                                                                            "));

	}
	else if (strcmp("/name", tokens[0].c_str()) == 0)
	{
		std::string name;
		name = msg.substr(6, msg.length());
		for (int i = name.length(); i < 14; i++)
			name += ' ';
		name[14] = '\0';
		memcpy(thisUser.name, name.c_str(), 14);
		onlineUsers[thisUser.userID] = thisUser;

		Packet p = Packet::USER_UPDATE;
		char msg[32];
		memcpy(msg, &p, 4);
		memcpy(msg + 4, &thisUser, sizeof(User));
		nsend(msg, 32);
	}
	else if (!strcmp("/voice", tokens[0].c_str()))
	{
		if (tokens.size() == 1 || tokens.size() > 1 && !strcmp(tokens[1].c_str(), "help"))
			q.push(Message(thisUser.userID, "/voice [input, output] [on, off] : Use to toggle sending or playing of voice chat.                                          "));
		else if (tokens.size() >= 3)
		{
			//TODO
		}
		else
			q.push(Message(thisUser.userID, "Error : Please use /voice help                                                                                             "));
	}
	else if (!strcmp("/quit", tokens[0].c_str()))
	{
		nclose();
		WSACleanup();
		exit(0);
	}

	return;
}