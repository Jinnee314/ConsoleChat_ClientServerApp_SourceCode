#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <vector>
#include <mutex>
#include <thread>
#include <cstring>
#include <fstream>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

bool flagStopThreads = false;
mutex m;
WSADATA wsData;

//создаёт сокет, подключает его к серверу и возвращает его для использования
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

//функция приёма сообщений
void receive()
{
	sockaddr_in servInfo;
	SOCKET from = connectSock(servInfo);
	char buf[10000] = {};
	int control = 0;
	/*control = recv(from, buf, 10000, 0);
	buf[control] = '\0';*/
	while (true)
	{
		strcpy(buf, "");
		control = recv(from, buf, 10000, 0);
		if (control == SOCKET_ERROR)
		{
			cout << "Error recv" << endl;
		}
		else
		{
			buf[control] = '\0';
			if (!strcmp("STOP SERVER", buf))
			{
				m.lock();
				flagStopThreads = true;
				m.unlock();
				cout << "Server stoped, sorry\nEnter any symbols\n";
				break;
			}
			cout << buf << endl;
		}
		m.lock();
		if (flagStopThreads)
		{
			m.unlock();
			break;
		}
		else
		{
			m.unlock();
		}

	}
	return;
}

//функция отправления сообщений
void sendMes(char* name)
{
	sockaddr_in servInfo;
	SOCKET whom = connectSock(servInfo);
	send(whom, name, strlen(name), 0);
	char buf[2048] = {};
	char* outBuf = new char[2148];
	char* forExit = new char[100];
	strcpy(forExit, name);
	strcat(forExit, ": EXIT");
	int err = 0;
	string str;
	while (true)
	{
		m.lock();
		if (flagStopThreads)
		{
			m.unlock();
			break;
		}
		m.unlock();
		strcpy(buf, "\0");
		strcpy(outBuf, name);
		strcat(outBuf, ": ");
		cin.getline(buf, 2048);
		strcat(outBuf, buf);
		str = outBuf;
		err = send(whom, str.c_str(), str.length(), 0);
		if (err == SOCKET_ERROR)
		{
			cout << "Error send" << endl;
		}
		else
		{
			if (!strcmp(outBuf, forExit))
			{
				m.lock();
				flagStopThreads = true;
				m.unlock();
				cout << "You exit from chat" << endl;
				break;
			}
		}

	}

	delete[] outBuf;
	delete[] forExit;
	return;
}

//main
int main()
{
	char* name = new char[20];

	cout << "Enter name for reg on server(10 symbols)\n";
	cin.getline(name, 20);

	int erStat = WSAStartup(MAKEWORD(2, 2), &wsData);

	if (erStat != 0)
	{
		cout << "Error WinSock version initializain # ";
		cout << WSAGetLastError() << endl;
		return 1;
	}
	else
		cout << "WinSock initialization is OK" << endl;



	thread sThread(sendMes, name);
	Sleep(100);
	thread rThread(receive);

	sThread.join();
	rThread.join();

	WSACleanup();
	delete[] name;

	return 0;
}