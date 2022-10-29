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
/*系统时间获取--start*/
void getTime()
{
	time_t t = time(0);
	strftime(timeRecord, sizeof(timeRecord), "%Y-%m-%d %H:%M:%S", localtime(&t));
}
/*系统时间获取--end*/
int main()
{
	system("mode con cols=70 lines=15  ");
	/*Socket服务初始化--start*/
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET ClientSocket;
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	USHORT uPort = 8888;
	ServerAddr.sin_port = htons(uPort);
	ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	/*Socket服务初始化--end*/

	/*连接模块--start*/
	cout << "Please input your name to start connect and chat" << endl;
	cout << "If you want to exit ,please input \"over\"" << endl;
	cout << "If you want to chat with someone only,please input \">>\" and uesrname" << endl;
	char client_name[32] = { 0 };
	cout << "input your name：";
	cin.getline(client_name, 32);
	connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	cout << "connect success!" << endl;
	/*连接模块--end*/

	/*准备发送用户名 --start*/
	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(messagebuffer, 0, sizeof(messagebuffer));
	getTime();
	strcpy(sendBuffer, timeRecord);
	strcat(sendBuffer, "1");
	strcat(sendBuffer, client_name);
	send(ClientSocket, sendBuffer, strlen(sendBuffer), 0);
	/*准备发送用户名 --end*/
	
	/*信息接收监听线程--start*/
	CreateThread(NULL, 0, recvMsgThread, (LPVOID)&ClientSocket, 0, 0);
	/*信息接收监听线程--end*/

	/*主线程信息发送--start*/
	while (1)
	{
		memset(messagebuffer, 0, sizeof(messagebuffer));
		memset(sendBuffer, 0, sizeof(sendBuffer));
		cin.getline(messagebuffer, sizeof(messagebuffer));
		string messageToSend = messagebuffer;
		if (messageToSend.find("over") == 0)
		{
			//客户端申请退出
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
	/*主线程信息发送--end*/
	closesocket(ClientSocket);
	WSACleanup();
}
DWORD WINAPI recvMsgThread(LPVOID IpParameter)
{
	SOCKET cliSock = *(SOCKET*)IpParameter;//获取客户端的SOCKET参数
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
				//用户消息
				cout << "[info  ]# " << recetime << ' ' << message << endl;
			}
			if (order == '1')
			{
				//客户端消息
				cout << "[server]# " << recetime << ' ' << message << endl;
			}
			if (order == '2')
			{
				cout << "[info  ]# " << recetime << ' ' << message << endl;
			}
			if (order == '3')
			{
				//退出命令
				cout << "[server]# " << recetime << ' ' << message << endl;
				cout << "You will exit in 3 sec,Welcome to come again" << endl;
				Sleep(3000);
				exit(0);
			}
		}
	}
	return 0;

}