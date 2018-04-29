#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

#pragma comment (lib, "ws2_32.lib")

#include <queue>

#include <vector>
#include <time.h>
#include <random>
#include <string>

#define PORT 8888

struct User
{
	short userID;//0
	int colourCode;//8
	char name[14];//12

	enum Status { Online, Busy, InGame };
	Status status;//24
};

struct Client
{
	SOCKET soc;
	int lobby;
	int index;
};

struct Lobby
{
	std::vector<User> users;
	bool isplaying;
	bool gamestart;
	unsigned int number;
	char name[13];
};

struct Message
{
	Message(short uid, const char* msg) { userID = uid; memcpy(message, msg, 112); }
	short userID;
	char message[112];
};

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
enum Packet { USER_UPDATE, MESSAGE, LOBBY_CHANGE, LOBBY_JOIN, GAME_GTN, GAME_OFF, LOBBY_UPDATE};


struct sendstruct
{
	sendstruct(SOCKET soc, char* data, int len)
	{
		s = soc;
		d = (char*)malloc(len);
		memcpy(d, data, len);
		l = len;
	}
	SOCKET s;
	char* d;
	int l;
};

std::queue<sendstruct> sq;

std::vector<Lobby> lobbies;
std::vector<Client> clients;
SOCKET connectSoc;



void QueueSend(sendstruct ss)
{
	sq.emplace(ss);
}

//if ar is true we add user, else subtract
int UpdateLobby(int lobby, bool ar, User &c)
{
	if (ar)
	{
		c.userID = lobbies[lobby].users.size();
		lobbies[lobby].users.push_back(c);
		memcpy(lobbies[lobby].users.back().name, c.name, 14);
		printf("client moved to %i.%i\n", lobby, c.userID);
		printf("user data: \n     %i\n     %i\n     %s\n     %i\n",
			c.userID,
			c.colourCode,
			c.name,
			c.status);
		return lobbies[lobby].users.size() - 1;
	}
	else
	{
		lobbies[lobby].users.erase(lobbies[lobby].users.begin() + c.userID);
		lobbies[lobby].users.shrink_to_fit();
		for (int i = 0; i < lobbies[lobby].users.size(); i++)
		{
			lobbies[lobby].users[i].userID = i;
			for (int c = 0; c < clients.size(); c++)
			{
				if (clients[c].lobby == lobby && clients[c].index == i)
				{
					Packet p = Packet::USER_UPDATE;
					char tmp[32];//sizeof usr+ packet
					memcpy(tmp, &p, sizeof(Packet));
					memcpy(tmp + sizeof(Packet), &lobbies[lobby].users[i], sizeof(User));
					QueueSend(sendstruct(clients[c].soc, tmp, 32));
				}
			}
		}
		return -1;
	}
}

void TrySend()
{
	while (!sq.empty())
	{
		int ls = 0;
		int limit = 0;
		sendstruct ss = sq.front();
		do
		{
			limit++;
			printf("sending %i bytes of data... ", ss.l);
			ls = send(ss.s, ss.d, ss.l, 0);
		} while (ls != ss.l && limit < 1);
		if (limit > 1)
		{
			printf("sending failed. dropping client.\n");
			for (int i = 0; i < clients.size(); i++)
			{
				if (clients[i].soc == ss.s)
				{
					UpdateLobby(clients[i].lobby, false, lobbies[clients[i].lobby].users[clients[i].index]);
					closesocket(ss.s);
					clients.erase(clients.begin() + i);
					clients.shrink_to_fit();
				}
			}
		}
		else
			printf("send complete.\n");
		free(ss.d);
		sq.pop();
	}
}

//The length is only the length of the data not the data+packet
void Broadcast(Packet p, char* data, int len, int lobby)
{
	char* s = (char*)malloc(len + sizeof(Packet));
	memcpy(s, &p, sizeof(Packet));
	memcpy(s + sizeof(Packet), data, len);
	if (lobby == -1)
	{
		for (int i = 0; i < clients.size(); i++)
		{
			QueueSend(sendstruct(clients[i].soc, s, len + sizeof(Packet)));
			printf("send: p%i to all\n", p);
		}
	}
	else
	{
		for (int i = 0; i < clients.size(); i++)
		{
			if (lobby == clients[i].lobby)
			{
				QueueSend(sendstruct(clients[i].soc, s, len + sizeof(Packet)));
				printf("send: p%i to user: %i.%i \n", p, lobby, clients[i].index);
			}
		}
	}
	free(s);
}

void main()
{
	srand(time(NULL));

	//init server
	WSADATA wsa;

	//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		exit(EXIT_FAILURE);

	sockaddr_in sockAddr;

	//create socket
	connectSoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSoc == SOCKET_ERROR)
		exit(EXIT_FAILURE);

	

	//setup address structure
	memset((char *)&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = INADDR_ANY;
	sockAddr.sin_port = htons(PORT);


	//Bind
	if (bind(connectSoc, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
		exit(EXIT_FAILURE);

	// If iMode!=0, non-blocking mode is enabled.
	u_long iMode = 1;
	ioctlsocket(connectSoc, FIONBIO, &iMode);
	char v = 1;
	setsockopt(connectSoc, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));


	listen(connectSoc, 1);

	//SOCKET tempsock = SOCKET_ERROR;
	//while (tempsock == SOCKET_ERROR)
	//{
	//	tempsock = accept(connectSoc, NULL, NULL);
	//	int e = WSAGetLastError();
	//}


	Lobby defaultlob;
	memcpy(defaultlob.name, "DEFAULT      ", 14);
	defaultlob.gamestart = defaultlob.isplaying = defaultlob.number = 0;
	lobbies.push_back(defaultlob);

	//connectSoc = tempsock;

	while (true)
	{
		TrySend();
		//check for a pending connection
		SOCKET tempsoc = accept(connectSoc, NULL, NULL);
		if (tempsoc != SOCKET_ERROR)
		{
			//we have a new user
			Packet tempp = Packet::MESSAGE;
			Message tempm = Message(-1, "Welcome to TBC                                                                                                       ");
			char msg[118];
			memcpy(msg, &tempp, sizeof(Packet));
			memcpy(msg + sizeof(Packet), &tempm, sizeof(Message));
			printf("sending MOTD\n");
			//QueueSend(sendstruct(tempsoc, msg, 118));
			
			Client c;
			c.soc = tempsoc;
			c.lobby = 0;
			c.index = lobbies[0].users.size();


			User u;
			u.colourCode = FG_RED | FG_BLUE | FG_GREEN | FG_INTENSITY;
			u.userID = 0;
			memcpy(u.name, "Anonymous     ", 14);

			clients.push_back(c);
			UpdateLobby(0, true, u);


			int numusers = lobbies[0].users.size();
			int basedatasize = sizeof(char) * 12 + sizeof(int);
			char* oldlobbydata = (char*)malloc(basedatasize + sizeof(User) * numusers);

			memcpy(oldlobbydata, lobbies[0].name, 12);
			memcpy(oldlobbydata + 12, &numusers, 4);
			memcpy(oldlobbydata + 16, &lobbies[0].users[0], sizeof(User) * numusers);
			//for (int i = 0; i < lobbies[0].users.size(); i++)
			//	memcpy(oldlobbydata + 16 + 8 + (i * 28), &lobbies[0].users[i].name, 14);
			Broadcast(Packet::LOBBY_CHANGE, oldlobbydata, basedatasize + sizeof(User) * numusers, 0);
		}

		//recv any messages from lobby
		for (int cid = 0; cid < clients.size(); cid++)
		{
			char buffer[1024];
			memset(buffer, 0, 1024);
			int r = recv(clients[cid].soc, buffer, 4, 0);

			if (r >= 4)
			{
				//we recvd somthing

				//check the packet
				Packet p;
				memcpy(&p, buffer, sizeof(p));

				//time to get the rest of only that packet
				if (p == Packet::USER_UPDATE)
					recv(clients[cid].soc, buffer + 4, sizeof(User), 0);
				else if (p == Packet::MESSAGE)
					recv(clients[cid].soc, buffer + 4, sizeof(Message), 0);
				else if (p == LOBBY_JOIN)
					recv(clients[cid].soc, buffer + 4, 12 + sizeof(User), 0);
				//nothing else should be recvd
				


				int lob = clients[cid].lobby;
				
				bool lobbysend = false;
				bool broadcast = false;

				if (p == Packet::USER_UPDATE)
				{
					printf("recv: USER_UPDATE\n");
					//packet,user struct
					//update user, trigger a lobby_change
					int uid = lobbies[clients[cid].lobby].users[clients[cid].index].userID;
					memcpy(&lobbies[clients[cid].lobby].users[clients[cid].index], buffer + 4, sizeof(User));
					lobbies[clients[cid].lobby].users[clients[cid].index].userID = uid;

					char tmp[32];//sizeof usr+ packet
					memcpy(tmp, &p, sizeof(Packet));
					memcpy(tmp + sizeof(Packet), buffer + sizeof(Packet), sizeof(User));
					printf("send: USER_UPDATE\n");
					printf("user data: \n     %i\n     %i\n     %s\n     %i\n",
						lobbies[clients[cid].lobby].users[clients[cid].index].userID,
						lobbies[clients[cid].lobby].users[clients[cid].index].colourCode,
						lobbies[clients[cid].lobby].users[clients[cid].index].name,
						lobbies[clients[cid].lobby].users[clients[cid].index].status);
					QueueSend(sendstruct(clients[cid].soc, tmp, 32));

					p = Packet::LOBBY_CHANGE;
					lobbysend = true;
				}
				else if (p == Packet::MESSAGE)
				{
					printf("recv: MESSAGE\n");
					//packet, message struct
					broadcast = true;
				}
				else if (p == Packet::LOBBY_CHANGE)
				{
					printf("recv: LOBBY_CHANGE\n");
					//packet, name, num users, user struct array
					lobbysend = true;
				}
				else if (p == Packet::LOBBY_JOIN)
				{
					printf("recv: LOBBY_JOIN\n");
					//packet, lobbyname, user struct
					lobbysend = true;
				}
				else if (p == Packet::GAME_GTN)
				{
					printf("recv: GAME_GTN\n");
					//packet
					lobbies[lob].isplaying = true;
					lobbies[lob].gamestart = true;

					for (int i = 0; i < lobbies[lob].users.size(); i++)
						lobbies[lob].users[i].status = User::InGame;

					int numusers = lobbies[lob].users.size();
					int basedatasize = sizeof(char) * 12 + sizeof(int);
					char* oldlobbydata = (char*)malloc(basedatasize + sizeof(User) * numusers);

					memcpy(oldlobbydata, lobbies[lob].name, 12);
					memcpy(oldlobbydata + 12, &numusers, 4);
					memcpy(oldlobbydata + 16, &lobbies[lob].users[0], sizeof(User) * numusers);

					Broadcast(Packet::LOBBY_CHANGE, oldlobbydata, basedatasize + sizeof(User) * numusers, lob);
					free(oldlobbydata);
				}
				else if (p == Packet::GAME_OFF)
				{
					printf("recv: GAME_OFF\n");
					//packet
					lobbies[lob].isplaying = false;
					Message m(-1, "Game has been turned off.                                                                                              ");
					Broadcast(Packet::MESSAGE, (char*)&m, sizeof(Message), lob);

					for (int i = 0; i < lobbies[lob].users.size(); i++)
						lobbies[lob].users[i].status = User::Online;

					int numusers = lobbies[lob].users.size();
					int basedatasize = sizeof(char) * 12 + sizeof(int);
					char* oldlobbydata = (char*)malloc(basedatasize + sizeof(User) * numusers);

					memcpy(oldlobbydata, lobbies[lob].name, 12);
					memcpy(oldlobbydata + 12, &numusers, 4);
					memcpy(oldlobbydata + 16, &lobbies[lob].users[0], sizeof(User) * numusers);

					Broadcast(Packet::LOBBY_CHANGE, oldlobbydata, basedatasize + sizeof(User) * numusers, lob);
					free(oldlobbydata);
				}




				if (broadcast)
				{
					//broadcast to group
					Broadcast(p, buffer + sizeof(Packet), sizeof(Message), lob);
				}

				if (lobbies[lob].gamestart == true)
				{
					lobbies[lob].gamestart = false;
					lobbies[lob].number = rand() % 10;
					//send helptext
					Message m(-1, "The lobby is in game mode. A number has been chosen between 0 and 9. Please enter guesses now.                                                    ");
					Broadcast(Packet::MESSAGE, (char*)&m, sizeof(Message), lob);
				}
				
				if (lobbies[lob].isplaying == true)
				{
					if (p == Packet::MESSAGE)
					{
						//check if its the winning number
						Message m(0, "");
						memcpy(&m, buffer + sizeof(Message), sizeof(Message));
						int geuss = buffer[6] - 48;
						geuss < 0 || geuss > 9 ? geuss = -1 : geuss;
						if (geuss == lobbies[lob].number)
						{
							lobbies[lob].gamestart = true;
							std::string str = "The number: ";
							str += buffer[6];
							str += " was guessed correctly by: ";
							str += lobbies[clients[cid].lobby].users[clients[cid].index].name;
							str += ".                                                                                                                       ";

							Message s(-1, str.c_str());
							Broadcast(Packet::MESSAGE, (char*)&s, sizeof(Message), lob);
						}
						else
						{
							std::string str = "The number: ";
							str += buffer[6];
							str += " was guessed incorrectly by: ";
							str += lobbies[clients[cid].lobby].users[clients[cid].index].name;
							str += ".                                                                                                                       ";

							Message s(-1, str.c_str());
							Broadcast(Packet::MESSAGE, (char*)&s, sizeof(Message), lob);
						}
					}
				}
				
				if (lobbysend)
				{
					if (p == Packet::LOBBY_JOIN)
					{
						char newlob[13];
						User usr;
						int newlobid;
						memcpy(newlob, buffer + sizeof(Packet), 12);
						newlob[12] = '\0';
						memcpy(&usr, buffer + sizeof(Packet) + 12, sizeof(User));

						bool found = false;
						for (int i = 0; i < lobbies.size(); i++)
						{
							if (!strcmp(newlob, lobbies[i].name))
							{
								UpdateLobby(lob, false, usr);
								int id = UpdateLobby(i, true, usr);
								clients[cid].lobby = i;
								clients[cid].index = id;
								found = true;
								newlobid = i;
							}
						}
						if (!found)
						{
							Lobby newlobby;

							memcpy(newlobby.name, newlob, 13);
							newlobby.gamestart = newlobby.isplaying = newlobby.number = 0;

							lobbies.push_back(newlobby);

							UpdateLobby(lob, false, usr);
							int id = UpdateLobby(lobbies.size() - 1, true, usr);
							clients[cid].lobby = lobbies.size() - 1;
							clients[cid].index = id;

							newlobid = lobbies.size() - 1;
						}

						//packet, name, num users, user struct array
						//sizeof(char) * 12 + sizeof(int)
						int numusers = lobbies[lob].users.size();
						int basedatasize = sizeof(char) * 12 + sizeof(int);
						if (numusers != 0)
						{
							char* oldlobbydata = (char*)malloc(basedatasize + sizeof(User) * numusers);

							memcpy(oldlobbydata, lobbies[lob].name, 12);
							memcpy(oldlobbydata + 12, &numusers, 4);
							memcpy(oldlobbydata + 16, &lobbies[lob].users[0], sizeof(User) * numusers);
							//for (int i = 0; i < lobbies[lob].users.size(); i++)
							//	memcpy(oldlobbydata + 16 + 8 + (i * 28), &lobbies[lob].users[i].name, 14);
							Broadcast(Packet::LOBBY_CHANGE, oldlobbydata, basedatasize + sizeof(User) * numusers, lob);
							free(oldlobbydata);
						}
						
						
						numusers = lobbies[newlobid].users.size();
						char* newlobbydata = (char*)malloc(basedatasize + sizeof(User) * numusers);

						memcpy(newlobbydata, lobbies[newlobid].name, 12);
						memcpy(newlobbydata + 12, &numusers, 4);
						memcpy(newlobbydata + 16, &lobbies[newlobid].users[0], sizeof(User) * numusers);
						//for (int i = 0; i < lobbies[lob].users.size(); i++)
						//	memcpy(newlobbydata + 16 + 8 + (i * 28), &lobbies[lob].users[i].name, 14);
						Broadcast(Packet::LOBBY_CHANGE, newlobbydata, basedatasize + sizeof(User) * numusers, newlobid);
						free(newlobbydata);
					}
					//send the new lobby info
					else
					{
						int numusers = lobbies[lob].users.size();
						int basedatasize = sizeof(char) * 12 + sizeof(int);
						char* oldlobbydata = (char*)malloc(basedatasize + sizeof(User) * numusers);

						memcpy(oldlobbydata, lobbies[lob].name, 12);
						memcpy(oldlobbydata + 12, &numusers, 4);
						memcpy(oldlobbydata + 16, &lobbies[lob].users[0], sizeof(User) * numusers);
						const int usrsz = sizeof(User);
						//name starts at 12 offset, repeats every 28th
						//for (int i = 0; i < lobbies[lob].users.size(); i++)
						//	memcpy(oldlobbydata + 16 + 8 + (i * 28), &lobbies[lob].users[i].name, 14);
						Broadcast(Packet::LOBBY_CHANGE, oldlobbydata, basedatasize + sizeof(User) * numusers, lob);
						free(oldlobbydata);

						//send just the user
						for (int u = 0; u < lobbies[lob].users.size(); u++)
						{
							int cli = 0;
							for (int c = 0; c < clients.size(); c++)
							{
								if (u == clients[c].index && lob == clients[c].lobby)
								{
									cli = c;
								}
							}
							Packet pack = Packet::USER_UPDATE;
							char tmp[32];//sizeof usr+ packet
							memcpy(tmp, &pack, sizeof(Packet));
							User *usr = &lobbies[lob].users[u];
							memcpy(tmp + sizeof(Packet), &lobbies[lob].users[u], sizeof(User));
							printf("send: USER_UPDATE\n");
							printf("user data: \n     %i\n     %i\n     %s\n     %i\n",
								usr->userID,
								usr->colourCode,
								usr->name,
								usr->status);
							QueueSend(sendstruct(clients[cli].soc, tmp, 32));
						}
					}
				}
			}		
		}
	}
}