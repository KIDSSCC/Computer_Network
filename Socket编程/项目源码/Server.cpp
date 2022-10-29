#include<iostream>
#include<winsock2.h>
#include<string.h>
#include<time.h>

//加载链接库
#pragma comment(lib,"ws2_32.lib")
using namespace std;

const int waitTime = 10;//等待事件的时间
const int maxLink = 10;//最大链接数
SOCKET cliSock[maxLink];
SOCKADDR_IN cliAddr[maxLink];
string clientName[maxLink];
WSAEVENT cliEvent[maxLink];
int total = 0;

char timeRecord[20] = { 0 };
char recvBuffer[1044] = { 0 };
char sendBuffer[1044] = { 0 };
char messagebuffer[1024] = { 0 };
string leftTime;
DWORD WINAPI eventReceiveThread(LPVOID IpParameter);//服务器端处理线程

/*系统时间获取--start*/
void getTime()
{
	time_t t = time(0);
	strftime(timeRecord, sizeof(timeRecord), "%Y-%m-%d %H:%M:%S", localtime(&t));
}
/*系统时间获取--end*/
int main() 
{
	string title[] = {
"                                                                                                                                                                    \n",
"                                                                                                                                                                    \n",
"           CCCCCCCCCCCCChhhhhhh                                       tttt               RRRRRRRRRRRRRRRRR                                                             \n",
"        CCC::::::::::::Ch:::::h                                    ttt:::t               R::::::::::::::::R                                                            \n",
"      CC:::::::::::::::Ch:::::h                                    t:::::t               R::::::RRRRRR:::::R                                                           \n",
"     C:::::CCCCCCCC::::Ch:::::h                                    t:::::t               RR:::::R     R:::::R                                                          \n",
"    C:::::C       CCCCCC h::::h hhhhh         aaaaaaaaaaaaa  ttttttt:::::ttttttt           R::::R     R:::::R   ooooooooooo      ooooooooooo      mmmmmmm    mmmmmmm   \n",
"   C:::::C               h::::hh:::::hhh      a::::::::::::a t:::::::::::::::::t           R::::R     R:::::R oo:::::::::::oo  oo:::::::::::oo  mm:::::::m  m:::::::mm \n",
"   C:::::C               h::::::::::::::hh    aaaaaaaaa:::::at:::::::::::::::::t           R::::RRRRRR:::::R o:::::::::::::::oo:::::::::::::::om::::::::::mm::::::::::m\n",
"   C:::::C               h:::::::hhh::::::h            a::::atttttt:::::::tttttt           R:::::::::::::RR  o:::::ooooo:::::oo:::::ooooo:::::om::::::::::::::::::::::m\n",
"   C:::::C               h::::::h   h::::::h    aaaaaaa:::::a      t:::::t                 R::::RRRRRR:::::R o::::o     o::::oo::::o     o::::om:::::mmm::::::mmm:::::m\n",
"   C:::::C               h:::::h     h:::::h  aa::::::::::::a      t:::::t                 R::::R     R:::::Ro::::o     o::::oo::::o     o::::om::::m   m::::m   m::::m\n",
"   C:::::C               h:::::h     h:::::h a::::aaaa::::::a      t:::::t                 R::::R     R:::::Ro::::o     o::::oo::::o     o::::om::::m   m::::m   m::::m\n",
"    C:::::C       CCCCCC h:::::h     h:::::ha::::a    a:::::a      t:::::t    tttttt       R::::R     R:::::Ro::::o     o::::oo::::o     o::::om::::m   m::::m   m::::m\n",
"     C:::::CCCCCCCC::::C h:::::h     h:::::ha::::a    a:::::a      t::::::tttt:::::t     RR:::::R     R:::::Ro:::::ooooo:::::oo:::::ooooo:::::om::::m   m::::m   m::::m\n",
"      CC:::::::::::::::C h:::::h     h:::::ha:::::aaaa::::::a      tt::::::::::::::t     R::::::R     R:::::Ro:::::::::::::::oo:::::::::::::::om::::m   m::::m   m::::m\n",
"        CCC::::::::::::C h:::::h     h:::::h a::::::::::aa:::a       tt:::::::::::tt     R::::::R     R:::::R oo:::::::::::oo  oo:::::::::::oo m::::m   m::::m   m::::m\n",
"           CCCCCCCCCCCCC hhhhhhh     hhhhhhh  aaaaaaaaaa  aaaa         ttttttttttt       RRRRRRRR     RRRRRRR   ooooooooooo      ooooooooooo   mmmmmm   mmmmmm   mmmmmm\n"
	};
	for (int i = 0; i < 18; i++)
	{
		cout << title[i];
	}

	/*Socket服务初始化--start*/
	//异步套接字启动 MAKEWORD声明版本号
	WSADATA wsaData;
	int error=WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (error != 0)
	{
		exit(0);
	}
	//服务端套接字，声明地址类型，套接字服务类型，通信协议
	SOCKET ServerSocket;
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN ServerAddr;
	//IP格式
	ServerAddr.sin_family = AF_INET;
	//监听端口号
	USHORT uPort = 8888;
	ServerAddr.sin_port = htons(uPort);
	ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	//建立捆绑  
	bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	WSAEVENT servEvent = WSACreateEvent();//创建事件对象
	WSAEventSelect(ServerSocket, servEvent, FD_ALL_EVENTS);//绑定事件，并且监听所有事件
	cliSock[0] = ServerSocket;
	cliEvent[0] = servEvent;
	//服务端姓名为ChatServer
	cout << "Welcome to chatroom,this is ChatServer" << endl;
	cout << "The Server is Online" << endl;
	/*Socket服务初始化--end*/

	/*事件监听--start*/
	listen(ServerSocket, 10);
	//创建事件处理线程
	CreateThread(NULL, 0, eventReceiveThread, (LPVOID)&ServerSocket, 0, 0);
	/*事件监听--start*/

	/*主线程信息发送--start*/
	while (1)
	{
		memset(sendBuffer, 0, sizeof(sendBuffer));
		memset(messagebuffer, 0, sizeof(messagebuffer));
		cin.getline(messagebuffer, 1024);
		getTime();
		string messageToSend = messagebuffer;
		if (messageToSend.find("over") == 0)
		{
			string buffertmp = "This chat room will close";
			strcpy(sendBuffer, timeRecord);
			strcat(sendBuffer, "3");
			strcat(sendBuffer, buffertmp.c_str());
		}
		else
		{
			strcpy(sendBuffer, timeRecord);
			strcat(sendBuffer, "1");
			strcat(sendBuffer, messagebuffer);
		}
		for (int j = 1; j <= total; j++)
		{
			send(cliSock[j], sendBuffer, sizeof(sendBuffer), 0);
		}
		if (messageToSend.find("over") == 0)
		{
			cout << "This chat room will close in 5 sec" << endl;
			Sleep(5000);
			return 0;
		}
	}
	/*主线程信息发送--end*/
}
DWORD WINAPI eventReceiveThread(LPVOID IpParameter) //服务器端线程
{
	SOCKET servSock = *(SOCKET*)IpParameter;
	while (1)
	{
		for (int i = 0; i <= total; i++)
		{
			int index = WSAWaitForMultipleEvents(1, &cliEvent[i], false, waitTime, 0);
			if (index == 0)
			{
				WSANETWORKEVENTS networkEvents;
				WSAEnumNetworkEvents(cliSock[i], cliEvent[i], &networkEvents);//查看是什么事件
				if (networkEvents.lNetworkEvents & FD_ACCEPT)
				{
					if (total < maxLink - 1)
					{
						int nextIndex = total + 1;
						int addrLen = sizeof(SOCKADDR);
						SOCKET newSock = accept(servSock, (SOCKADDR*)&cliAddr[nextIndex], &addrLen);
						cliSock[nextIndex] = newSock;
						WSAEVENT newEvent = WSACreateEvent();
						WSAEventSelect(cliSock[nextIndex], newEvent, FD_CLOSE | FD_READ );
						cliEvent[nextIndex] = newEvent;
						total++;//客户端连接数增加
					}
				}
				else if(networkEvents.lNetworkEvents & FD_CLOSE)
				{
					//i表示已关闭的客户端下标
					getTime();
					strcpy(sendBuffer, timeRecord);
					strcat(sendBuffer, "1");
					string buffertmp = "user:" + clientName[i] + " left the room";
					strcat(sendBuffer, buffertmp.c_str());
					cout << "[state]# " << timeRecord << " " << buffertmp << ",Now " << total-1 << " people in" << endl;
					//释放这个客户端的资源
					closesocket(cliSock[i]);
					WSACloseEvent(cliEvent[i]);
					//数组调整,用顺序表删除元素
					for (int j = i; j < total; j++)
					{
						cliSock[j] = cliSock[j + 1];
						cliEvent[j] = cliEvent[j + 1];
						cliAddr[j] = cliAddr[j + 1];
						clientName[j] = clientName[j + 1];
					}
					for (int k = 1; k < total; k++)
					{
						send(cliSock[k], sendBuffer, sizeof(sendBuffer), 0);
					}
					total--;
				}
				else if (networkEvents.lNetworkEvents & FD_READ)
				{
					for (int j = 1; j <= total; j++)
					{
						memset(recvBuffer, 0, sizeof(recvBuffer));
						memset(sendBuffer, 0, sizeof(sendBuffer));
						int nrecv = recv(cliSock[j], recvBuffer, sizeof(recvBuffer), 0);
						if (nrecv > 0)
						{
							string receMessage = recvBuffer;
							string recetime = receMessage.substr(0, 19);
							char order = receMessage[19];
							string message = receMessage.substr(20, receMessage.length() - 20);
							if (order == '1')
							{
								//收到了新用户发来的用户名 message
								clientName[j] = message;
								cout << "[state]# " << recetime << " user:" << clientName[j] << " enter the chat room,Now " << total << " people in " << endl;
								//准备向各个用户分发
								strcpy(sendBuffer, recetime.c_str());
								strcat(sendBuffer, "1");
								string buffertmp = "Welcome " + message + " enter the room";
								strcat(sendBuffer, buffertmp.c_str());
								for (int k = 1; k <= total; k++)
								{
									send(cliSock[k], sendBuffer, sizeof(sendBuffer), 0);
								}
							}
							else if (order == '0')
							{
								//收到了用户消息
								strcpy(sendBuffer, recetime.c_str());
								strcat(sendBuffer, "0");
								string buffertmp = clientName[j]+" :" + message;
								strcat(sendBuffer, buffertmp.c_str());
								cout << "[info ]# " << recetime << " " << buffertmp << endl;
								for (int k = 1; k <= total; k++)
								{
									if (k != j)
									{
										send(cliSock[k], sendBuffer, sizeof(sendBuffer), 0);
									}
								}
							}
							else if (order == '2')
							{
								strcpy(sendBuffer, recetime.c_str());
								strcat(sendBuffer, "2");
								string userToSend = message.substr(message.find(">>") + 2, message.length() - 1);
								message = message.substr(0, message.find(">>"));
								string buffertmp = clientName[j] + " (only to you): " + message;
								strcat(sendBuffer, buffertmp.c_str());
								cout << "[info ]# " << recetime << " " << clientName[j]<<" only to "<< userToSend <<' ' << message << endl;
								for (int k = 1; k <= total; k++)
								{
									if (strcmp(clientName[k].c_str(), userToSend.c_str())==0)
									{
										send(cliSock[k], sendBuffer, sizeof(sendBuffer), 0);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}