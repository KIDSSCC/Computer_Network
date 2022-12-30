#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<vector>
#include<fstream>
#include<io.h>
#include<time.h>    
#include<stdlib.h>
#include<iomanip>
#include <Winsock2.h>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int packetLen = 10028;
const int messageLen = 10000;
string serverIp = "127.0.0.1";
string clientIp = "127.0.0.2";
USHORT serverPort = 8888;
USHORT clientPort = 1111;
int seq;
int ack;
int recvSeq;
int recvAck;
int length = 0;

char sendBuffer[packetLen];
char recvBuffer[packetLen];
char filename[100];
string fimenametmp;
vector<char> dataContent;
char timeRecord[20] = { 0 };

WORD wVersionRequested;
WSADATA IpWSAData;
SOCKET sockSrv;

SOCKADDR_IN  Server;
SOCKADDR_IN Client;

void initial();
void setPort();
void setIp();


void calCheckSum()
{
    int sum = 0;
    for (int i = 0; i < packetLen; i += 2)
    {
        if (i == 26)
            continue;
        sum += (sendBuffer[i] << 8) + sendBuffer[i + 1];
        if (sum >= 0x10000)
        {
            sum -= 0x10000;
            sum += 1;
        }
    }
    USHORT checkSum = ~(USHORT)sum;
    sendBuffer[26] = (char)(checkSum >> 8);
    sendBuffer[27] = (char)checkSum;
}
bool checkCheckSum()
{
    int sum = 0;
    for (int i = 0; i < packetLen; i += 2)
    {
        if (i == 26)
            continue;
        sum += (recvBuffer[i] << 8) + recvBuffer[i + 1];
        if (sum >= 0x10000)
        {
            sum -= 0x10000;
            sum += 1;
        }
    }
    USHORT checksum = (recvBuffer[26] << 8) + (unsigned char)recvBuffer[27];
    if (checksum + (USHORT)sum == 0xffff)
    {
        return true;
    }
    {
        cout << "??????§µ????" << endl;
        return false;
    }
}
void getTime()
{
    time_t t = time(0);
    strftime(timeRecord, sizeof(timeRecord), "%H:%M:%S", localtime(&t));
}
void sendlog()
{
    getTime();
    cout << timeRecord << " [send] ";
    int seqtmp = (int)((unsigned char)sendBuffer[12] << 24) + (int)((unsigned char)sendBuffer[13] << 16) + (int)((unsigned char)sendBuffer[14] << 8) + (int)(unsigned char)sendBuffer[15];
    int acktmp = (int)((unsigned char)sendBuffer[16] << 24) + (int)((unsigned char)sendBuffer[17] << 16) + (int)((unsigned char)sendBuffer[18] << 8) + (int)(unsigned char)sendBuffer[19];
    int lengthtmp = (int)((unsigned char)sendBuffer[20] << 8) + (int)((unsigned char)sendBuffer[21] ) ;
    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << seqtmp << "Ack: " << setw(5) << setiosflags(ios::left) << acktmp << "Length: " << setw(5) << setiosflags(ios::left) << lengthtmp;
    int ACKtmp = (sendBuffer[24] & 0xF0) ? 1 : 0;
    int syntmp = (sendBuffer[24] & 0xF) ? 1 : 0;
    int fintmp = (sendBuffer[25] & 0xF0) ? 1 : 0;
    int sttmp = (sendBuffer[25] & 0xC) ? 1 : 0;
    int ovtmp = (sendBuffer[25] & 0x3) ? 1 : 0;
    cout << "ACK: " << ACKtmp << " SYN: " << syntmp << " FIN: " << fintmp << " ST: " << sttmp << " OV: " << ovtmp << endl;
}
void recvlog()
{
    getTime();
    cout << timeRecord << " [recv] ";
    int seqtmp = (int)((unsigned char)recvBuffer[12] << 24) + (int)((unsigned char)recvBuffer[13] << 16) + (int)((unsigned char)recvBuffer[14] << 8) + (int)(unsigned char)recvBuffer[15];
    int acktmp = (int)((unsigned char)recvBuffer[16] << 24) + (int)((unsigned char)recvBuffer[17] << 16) + (int)((unsigned char)recvBuffer[18] << 8) + (int)(unsigned char)recvBuffer[19];
    int lengthtmp = (int)((unsigned char)recvBuffer[20] << 8) + (int)((unsigned char)recvBuffer[21] ) ;
    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << seqtmp << "Ack: " << setw(5) << setiosflags(ios::left) << acktmp << "Length: " << setw(5) << setiosflags(ios::left) << lengthtmp;
    int ACKtmp = (recvBuffer[24] & 0xF0) ? 1 : 0;
    int syntmp = (recvBuffer[24] & 0xF) ? 1 : 0;
    int fintmp = (recvBuffer[25] & 0xF0) ? 1 : 0;
    int sttmp = (recvBuffer[25] & 0xC) ? 1 : 0;
    int ovtmp = (recvBuffer[25] & 0x3) ? 1 : 0;
    cout << "ACK: " << ACKtmp << " SYN: " << syntmp << " FIN: " << fintmp << " ST: " << sttmp << " OV: " << ovtmp << endl;
}

void setSeq(int newSeq)
{
    seq = newSeq;
    sendBuffer[12] = (char)(seq >> 24);
    sendBuffer[13] = (char)(seq >> 16);
    sendBuffer[14] = (char)(seq >> 8);
    sendBuffer[15] = (char)seq;
}
void setAck(int newAck)
{
    ack = newAck;
    sendBuffer[16] = (char)(ack >> 24);
    sendBuffer[17] = (char)(ack >> 16);
    sendBuffer[18] = (char)(ack >> 8);
    sendBuffer[19] = (char)ack;
}
void getSeq()
{
    recvSeq = (int)((unsigned char)recvBuffer[12] << 24) + (int)((unsigned char)recvBuffer[13] << 16) + (int)((unsigned char)recvBuffer[14] << 8) + (int)(unsigned char)recvBuffer[15];
}
void getAck()
{
    recvAck = (int)((unsigned char)recvBuffer[16] << 24) + (int)((unsigned char)recvBuffer[17] << 16) + (int)((unsigned char)recvBuffer[18] << 8) + (int)(unsigned char)recvBuffer[19];
}

void setLength(int len)
{
    length = len;
    sendBuffer[20] = (char)(length >> 8);
    sendBuffer[21] = (char)(length );
}
void clearFlag()
{
    sendBuffer[24] = 0;
    sendBuffer[25] = 0;
}
void setACK()
{
    sendBuffer[24] += 0xF0;
}
void setSYN()
{
    sendBuffer[24] += 0xF;
}
void setFIN()
{
    sendBuffer[25] += 0xF0;
}
void setST()
{
    sendBuffer[25] += 0xC;
}
void setOV()
{
    sendBuffer[25] += 0x3;
}
bool checkACK()
{
    return recvBuffer[24] & 0xF0;
}
bool checkSYN()
{
    return recvBuffer[24] & 0xF;
}
bool checkFIN()
{
    return recvBuffer[25] & 0xF0;
}
bool checkST()
{
    return recvBuffer[25] & 0xC;
}
bool checkOV()
{
    return recvBuffer[25] & 0x3;
}

bool checkAckValue()
{
    getAck();
    if (recvAck == seq)
        return true;
    else
        return false;
}
void connectAckPrepare()
{
    setPort();
    setIp();
    getSeq();
    setSeq(rand());
    setAck(recvSeq);
    setLength(0);
    clearFlag();
    setACK();
    setSYN();
    for (int i = 28; i < packetLen; i++)
        sendBuffer[i] = 0;
    calCheckSum();
}

void transmission(int ctrl)
{
    int len = sizeof(SOCKADDR);
    int start;
    bool whether;
    switch (ctrl)
    {
    case 0:
        sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Client, len);
        sendlog();
        break;
    case 1:
        while (1)
        {
            if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Client, &len) > 0)
            {
                recvlog();
                break;
            }
        }
        break;
    case 2:
        do
        {
            sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Client, len);
            sendlog();
            start = clock();
            whether = false;
            while (clock() - start < 2000)
            {
                if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Client, &len) > 0)
                {
                    whether = true;
                    recvlog();
                    break;
                }
            }

        } while (!checkCheckSum() || (!whether));

        break;
    case 3:
        break;
    default:
        break;
    }
}
bool checkSeq()
{
    getSeq();
    if ((recvSeq == ack+1) || (recvSeq == 0))
        return true;
    else
    {
        cout << "error" << endl;
        cout << recvSeq << " " << ack << endl;
        return false;
    }
}

int main()
{
    initial();
    cout << "Server Service is operating!" << endl;
    cout << "Waiting for connect" << endl;
    while (1)
    {
        transmission(1);
        if (checkCheckSum() && checkSYN())
        {
            connectAckPrepare();
            while (1)
            {
                transmission(2);
                if (checkACK() && checkAckValue())
                {
                    break;
                }
            }
            break;
        }
        else {
            connectAckPrepare();
            clearFlag();
            calCheckSum();
            transmission(0);
        }
    }
    ack = 0;
    
    while (1)
    {
        transmission(1);

        if (checkCheckSum())
        {
            if (checkFIN())
            {
                break;
            }
            if (checkST())
            {
                setSeq(0);
                getSeq();
                setAck(recvSeq);
                setLength(0);
                clearFlag();
                setACK();
                calCheckSum();
                transmission(0);
                int messageLength = (int)(recvBuffer[20] << 8) + (int)(recvBuffer[21] ) ;
                
                for (int i = 0; i < messageLength; i++)
                {
                    filename[i] = recvBuffer[28 + i];
                }
                fimenametmp = "C:\\Users\\lenovo\\Desktop\\NetWork\\" + (string)filename;
                cout << fimenametmp << endl;
            }
            if (checkOV())
            {
                setSeq(0);
                getSeq();
                setAck(recvSeq);
                setLength(0);
                clearFlag();
                setACK();
                calCheckSum();
                transmission(0);
                int messageLength = (int)(recvBuffer[20] << 24) + (int)(recvBuffer[21] << 16) + (int)(recvBuffer[22] << 8) + (int)(recvBuffer[23]);
                for (int i = 0; i < messageLen; i++)
                {
                    dataContent.push_back(recvBuffer[28 + i]);
                }
                ofstream fout(fimenametmp.c_str(), ofstream::binary);
                for (int i = 0; i < dataContent.size(); i++)
                {
                    fout << dataContent[i];
                }
                vector<char>().swap(dataContent);
            }
            if ((recvBuffer[25] == 0) && checkSeq())
            {
                setSeq(0);
                getSeq();
                setAck(recvSeq);
                setLength(0);
                clearFlag();
                setACK();
                calCheckSum();
                transmission(0);
                int messageLength = (int)(recvBuffer[20] << 24) + (int)(recvBuffer[21] << 16) + (int)(recvBuffer[22] << 8) + (int)(recvBuffer[23]);
                for (int i = 0; i < messageLen; i++)
                {
                    dataContent.push_back(recvBuffer[28 + i]);
                }
            }
        }
        else
        {
            setSeq(0);
            setLength(0);
            clearFlag();
            calCheckSum();
            transmission(0);
        }
    }
    setSeq(0);
    getSeq();
    setAck(recvSeq);
    setLength(0);
    clearFlag();
    setFIN();
    setACK();
    calCheckSum();
    transmission(0);
    closesocket(sockSrv);
    WSACleanup();
}



void initial()
{
    wVersionRequested = MAKEWORD(1, 1);
    int error = WSAStartup(wVersionRequested, &IpWSAData);
    if (error != 0)
    {
        cout << "initial error" << endl;
        exit(0);
    }

    sockSrv = socket(AF_INET, SOCK_DGRAM, 0);

    Server.sin_addr.s_addr = inet_addr(serverIp.c_str());
    Server.sin_family = AF_INET;
    Server.sin_port = htons(serverPort);
    /*
    Client.sin_addr.s_addr = inet_addr(clientIp.c_str());
    Client.sin_family = AF_INET;
    Client.sin_port = htons(clientPort);*/

    bind(sockSrv, (SOCKADDR*)&Server, sizeof(SOCKADDR));
    unsigned long ul = 1;
    int ret = ioctlsocket(sockSrv, FIONBIO, (unsigned long*)&ul);
}

void setPort()
{
    sendBuffer[0] = (char)(serverPort >> 8);
    sendBuffer[1] = (char)(serverPort & 0xFF);
    sendBuffer[2] = (char)(clientPort >> 8);
    sendBuffer[3] = (char)(clientPort & 0xFF);
}
void setIp()
{
    int tmp = 0;
    int ctrl = 4;
    for (int i = 0; i < serverIp.length(); i++)
    {
        if (serverIp[i] == '.')
        {
            sendBuffer[ctrl++] = (char)tmp;
            tmp = 0;
        }
        else
        {
            tmp += tmp * 10 + (int)serverIp[i] - 48;
        }
    }
    sendBuffer[ctrl++] = (char)tmp;
    tmp = 0;
    for (int i = 0; i < clientIp.length(); i++)
    {
        if (clientIp[i] == '.')
        {
            sendBuffer[ctrl++] = (char)tmp;
            tmp = 0;
        }
        else
        {
            tmp += tmp * 10 + (int)clientIp[i] - 48;
        }
    }
    sendBuffer[ctrl++] = (char)tmp;
}