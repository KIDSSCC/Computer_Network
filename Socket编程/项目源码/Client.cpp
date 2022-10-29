#include<iostream>
#include<winsock.h>
#include<string.h>
#include<time.h>
#include<stdlib.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;
char timeRecord[20] = { 0 };
char messagebuffer[1024] = { 0 };
char recvBuffer[1044] = { 0 };
char sendBuffer[1044] = { 0 };
DWORD WINAPI recvMsgThread(LPVOID IpParameter);
/*ϵͳʱ���ȡ--start*/
void getTime()
{
	time_t t = time(0);
	strftime(timeRecord, sizeof(timeRecord), "%Y-%m-%d %H:%M:%S", localtime(&t));
}
/*ϵͳʱ���ȡ--end*/
int main()
{
	system("mode con cols=70 lines=15  ");
	/*Socket�����ʼ��--start*/
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET ClientSocket;
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	USHORT uPort = 8888;
	ServerAddr.sin_port = htons(uPort);
	ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	/*Socket�����ʼ��--end*/

	/*����ģ��--start*/
	cout << "Please input your name to start connect and chat" << endl;
	cout << "If you want to exit ,please input \"over\"" << endl;
	cout << "If you want to chat with someone only,please input \">>\" and uesrname" << endl;
	char client_name[32] = { 0 };
	cout << "input your name��";
	cin.getline(client_name, 32);
	connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	cout << "connect success!" << endl;
	/*����ģ��--end*/

	/*׼�������û��� --start*/
	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(messagebuffer, 0, sizeof(messagebuffer));
	getTime();
	strcpy(sendBuffer, timeRecord);
	strcat(sendBuffer, "1");
	strcat(sendBuffer, client_name);
	send(ClientSocket, sendBuffer, strlen(sendBuffer), 0);
	/*׼�������û��� --end*/
	
	/*��Ϣ���ռ����߳�--start*/
	CreateThread(NULL, 0, recvMsgThread, (LPVOID)&ClientSocket, 0, 0);
	/*��Ϣ���ռ����߳�--end*/

	/*���߳���Ϣ����--start*/
	while (1)
	{
		memset(messagebuffer, 0, sizeof(messagebuffer));
		memset(sendBuffer, 0, sizeof(sendBuffer));
		cin.getline(messagebuffer, sizeof(messagebuffer));
		string messageToSend = messagebuffer;
		if (messageToSend.find("over") == 0)
		{
			//�ͻ��������˳�
			cout << "You will exit in 3 sec" << endl;
			Sleep(3000);
			return 0;
		}
		else if (messageToSend.find(">>")<1044)
		{
			getTime();
			strcpy(sendBuffer, timeRecord);
			strcat(sendBuffer, "2");
			strcat(sendBuffer, messagebuffer);
			send(ClientSocket, sendBuffer, strlen(sendBuffer), 0);
		}
		else
		{
			getTime();
			strcpy(sendBuffer, timeRecord);
			strcat(sendBuffer, "0");
			strcat(sendBuffer, messagebuffer);
			send(ClientSocket, sendBuffer, strlen(sendBuffer), 0);
		}
	}
	/*���߳���Ϣ����--end*/
	closesocket(ClientSocket);
	WSACleanup();
}
DWORD WINAPI recvMsgThread(LPVOID IpParameter)
{
	SOCKET cliSock = *(SOCKET*)IpParameter;//��ȡ�ͻ��˵�SOCKET����
	while (1)
	{
		memset(recvBuffer, 0, sizeof(recvBuffer));
		int nrecv = recv(cliSock, recvBuffer, sizeof(recvBuffer), 0);
		if (nrecv > 0)
		{
			string receMessage = recvBuffer;
			string recetime = receMessage.substr(0, 19);
			char order = receMessage[19];
			string message = receMessage.substr(20, receMessage.length() - 20);
			if (order == '0')
			{
				//�û���Ϣ
				cout << "[info  ]# " << recetime << ' ' << message << endl;
			}
			if (order == '1')
			{
				//�ͻ�����Ϣ
				cout << "[server]# " << recetime << ' ' << message << endl;
			}
			if (order == '2')
			{
				cout << "[info  ]# " << recetime << ' ' << message << endl;
			}
			if (order == '3')
			{
				//�˳�����
				cout << "[server]# " << recetime << ' ' << message << endl;
				cout << "You will exit in 3 sec,Welcome to come again" << endl;
				Sleep(3000);
				exit(0);
			}
		}
	}
	return 0;

}