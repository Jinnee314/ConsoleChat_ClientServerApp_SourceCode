#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

bool flagStopThreads = false;
mutex m, addClient, forStopServ, forAllText;
vector<string> allText;

struct Client {
	string name;
	sockaddr_in clientInfo;
	SOCKET in;
	SOCKET out;
	bool connected = false;
	int numberThread;
};

vector<Client> vecClient;

void rec(Client cl)
{
	////////////////////////////////////////////////////////////
	string sendAll = "All Users:\n";
	addClient.lock();
	for (int i = 0; i < vecClient.size(); i++)
	{
		sendAll += vecClient[i].name + "\n";
	}
	addClient.unlock();
	send(cl.out, sendAll.c_str(), sendAll.length(), 0);
	sendAll = "Last 50 messages:\n";

	forAllText.lock();
	for (int i = 0; i < allText.size(); i++)
	{
		sendAll += allText[i] + "\n";
	}
	forAllText.unlock();
	send(cl.out, sendAll.c_str(), sendAll.length(), 0);
	sendAll = "";
	////////////////////////////////////////////////////////////////////
	unique_lock<mutex> uStopServ(forStopServ, defer_lock);
	char buf[2048] = {};
	char size[10] = {};
	char forExit[14] = "";
	strcat(forExit, cl.name.c_str());
	strcat(forExit, ": EXIT");
	bool proof = false;
	int control = 0;

	while (true)
	{
		uStopServ.lock();
		if (flagStopThreads)
		{
			break;
		}
		uStopServ.unlock();
		strcpy(buf, "\0");
		control = recv(cl.in, buf, 2048, 0);
		if (control == SOCKET_ERROR)
		{
			cout << "Error recv" << endl;
			break;
		}
		else
		{
			buf[control] = '\0';

			if (!strcmp(buf, forExit))
			{
				strcpy(buf, "");
				strcat(buf, cl.name.c_str());
				strcat(buf, " out chat\n");
				proof = true;
			}

			forAllText.lock();
			allText.push_back(buf);
			if (allText.size() > 50)
			{
				allText.erase(allText.begin());
			}
			forAllText.unlock();
		}

		addClient.lock();
		for (int i = 0; i < vecClient.size(); i++)
		{
			if (vecClient[i].name == cl.name)
			{
				if (proof)
				{
					send(vecClient[i].out, "Bye", 3, 0);
					vecClient[i].connected = false;
				}
			}
			else
			{
				send(vecClient[i].out, buf, strlen(buf), 0);
			}
		}
		addClient.unlock();

		if (proof)
		{
			break;
		}
	}
	return;
}

void runServer()
{
	WSADATA wsData;

	int erStat = WSAStartup(MAKEWORD(2, 2), &wsData);

	if (erStat != 0)
	{
		cout << "Error WinSock version initializain # ";
		cout << WSAGetLastError() << endl;
		return;
	}
	else
		cout << "WinSock initialization is OK" << endl;

	SOCKET ServSock = socket(AF_INET, SOCK_STREAM, 0);

	if (ServSock == INVALID_SOCKET)
	{
		cout << "Error initialization socket # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return;
	}
	else
		cout << "Server socket initialization is OK" << endl;

	in_addr ip_to_num;
	erStat = inet_pton(AF_INET, "127.0.0.1", &ip_to_num);

	if (erStat <= 0)
	{
		cout << "Error in IP translation to special numeric format" << endl;
		return;
	}

	sockaddr_in servInfo;
	ZeroMemory(&servInfo, sizeof(servInfo));

	servInfo.sin_family = AF_INET;
	servInfo.sin_addr = ip_to_num;
	servInfo.sin_port = htons(1234);

	erStat = bind(ServSock, (sockaddr*)&servInfo, sizeof(servInfo));
	if (erStat != 0)
	{
		cout << "Error Socket binding to server info. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return;
	}
	else
		cout << "Binding socket to Server info is OK" << endl;

	erStat = listen(ServSock, SOMAXCONN);

	if (erStat != 0) {
		cout << "Can't start to listen to. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return;
	}
	else
		cout << "Listening..." << endl;

	Client newClient;
	const int sizeName = 30;
	char name[sizeName] = {};
	sockaddr_in clientInfo;
	int clientInfo_size = sizeof(clientInfo);
	ZeroMemory(&clientInfo, clientInfo_size);
	SOCKET ClientConnOut, ClientConnIn;
	int control = 0;
	bool checkReg = false;
	int posReg = 0;
	vector<thread> vecThread;
	unique_lock<mutex> uStopServ(forStopServ, defer_lock);

	
	while (true)
	{
		uStopServ.lock();
		if (flagStopThreads)
		{
			break;
		}
		uStopServ.unlock();

		ClientConnIn = accept(ServSock, (sockaddr*)&clientInfo, &clientInfo_size);

		if (ClientConnIn == INVALID_SOCKET)
		{
			cout << "Client detected, but can't connect to a client. (for IN) Error # " << WSAGetLastError() << endl;
			continue;
		}
		else
			cout << "Connection to a client established successfuly (for IN)" << endl;

		control = recv(ClientConnIn, name, sizeName, 0);
		name[control] = '\0';

		if (!strcmp(name, "STOP"))
		{
			break;
		}

		ClientConnOut = accept(ServSock, (sockaddr*)&clientInfo, &clientInfo_size);

		if (ClientConnOut == INVALID_SOCKET)
		{
			cout << "Client detected, but can't connect to a client. (for OUT) Error # " << WSAGetLastError() << endl;
			continue;
		}
		else
			cout << "Connection to a client established successfuly(for OUT)" << endl;

		
		

		send(ClientConnOut, "Welcom to the server", strlen("Welcom to the server"), 0);
		cout << name << " connected " << strlen(name) << endl;

		addClient.lock();
		for (int i = 0; i < vecClient.size(); i++)
		{
			if (!strcmp(vecClient[i].name.c_str(), name))
			{
				checkReg = true;
				posReg = i;
				break;
			}
		}
		addClient.unlock();

		if (checkReg)
		{
			cout << "check reg = " << checkReg << endl;
			vecClient[posReg].connected = true;
			vecClient[posReg].clientInfo = clientInfo;
			vecClient[posReg].out = ClientConnOut;
			vecClient[posReg].in = ClientConnIn;
			vecClient[posReg].numberThread = vecThread.size();
			vecThread.push_back(thread{ rec, vecClient[posReg] });
		}
		else
		{
			newClient.name = name;
			newClient.clientInfo = clientInfo;
			newClient.out = ClientConnOut;
			newClient.in = ClientConnIn;
			newClient.connected = true;
			newClient.numberThread = vecThread.size();
			vecClient.push_back(newClient);
			vecThread.push_back(thread{ rec, vecClient.back() });
		}


		strcpy(name, "");
		checkReg = false;
		posReg = -1;
	}
	

	for (int i = 0; i < vecThread.size(); i++)
	{
		vecThread[i].join();
	}

	for (int i = 0; i < vecClient.size(); i++)
	{
		shutdown(vecClient[i].out, SD_BOTH);
		shutdown(vecClient[i].in, SD_BOTH);
		closesocket(vecClient[i].out);
		closesocket(vecClient[i].in);
	}
	shutdown(ServSock, SD_BOTH);
	closesocket(ServSock);
	shutdown(ClientConnOut, SD_BOTH);
	closesocket(ClientConnOut);
	shutdown(ClientConnIn, SD_BOTH);
	closesocket(ClientConnIn);
	WSACleanup();

	return;
}

SOCKET connectSock(sockaddr_in& servInfoRes)
{
	SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, 0);
	int erStat = 0;

	if (ClientSock == INVALID_SOCKET)
	{
		cout << "Error initialization inSocket # " << WSAGetLastError() << endl;
		closesocket(ClientSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Client inSocket initialization is OK" << endl;

	in_addr ip_to_num;
	erStat = inet_pton(AF_INET, "127.0.0.1", &ip_to_num);

	if (erStat <= 0)
	{
		cout << "Error in IP translation to special numeric format" << endl;
		return 1;
	}

	sockaddr_in servInfo;
	ZeroMemory(&servInfo, sizeof(servInfo));

	servInfo.sin_family = AF_INET;
	servInfo.sin_addr = ip_to_num;	  // Server's IPv4 after inet_pton() function
	servInfo.sin_port = htons(1234);

	erStat = connect(ClientSock, (sockaddr*)&servInfo, sizeof(servInfo));

	if (erStat != 0) {
		cout << "Connection to Server for In is FAILED. Error # " << WSAGetLastError() << endl;
		closesocket(ClientSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Connection for In established SUCCESSFULLY. Ready to send a message to Server"
		<< endl;
	servInfoRes = servInfo;
	return ClientSock;
}

int main()
{
	thread threadServer(runServer);
	string stopServ = "";
	unique_lock<mutex> uStopServ(forStopServ, defer_lock);

	cin >> stopServ;
	while (stopServ != "STOP")
	{
		cin >> stopServ;
	}
	uStopServ.lock();
	flagStopThreads = true;
	uStopServ.unlock();

	addClient.lock();
	for (int i = 0; i < vecClient.size(); i++)
	{
		send(vecClient[i].out, "STOP SERVER", strlen("STOP SERVER"), 0);
	}
	addClient.unlock();
	sockaddr_in servInfo;
	Sleep(1000);
	SOCKET gouste = connectSock(servInfo);

	send(gouste, "STOP", 4, 0);

	threadServer.join();
	shutdown(gouste, SD_BOTH);
	closesocket(gouste);
	return 0;
}