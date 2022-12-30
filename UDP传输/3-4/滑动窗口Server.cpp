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
#include<thread>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int packetLen = 4096;
const int messageLen = 4068;


WORD wVersionRequested;
WSADATA IpWSAData;
SOCKET sockSrv;

SOCKADDR_IN  Server;
SOCKADDR_IN Client;
string serverIp = "127.0.0.1";
string clientIp = "127.0.0.2";
USHORT serverPort = 8888;
USHORT clientPort = 1111;
int len = sizeof(SOCKADDR);
char sendBuffer[packetLen];
char recvBuffer[packetLen];
char timeRecord[20] = { 0 };
int seq;
int ack;
int recvseq;
int recvack;
int length = 0;
int window;
int recvWindow;


char filename[100];
string fimenametmp;
vector<char> dataContent;

struct DataInWindow {
    bool ack = false;
    char buffer[packetLen];
    int length;
    int seq;
};
vector<DataInWindow> slidingWindow;
int windowLength = 50;

void initial()
{
    wVersionRequested = MAKEWORD(1, 1);
    int error = WSAStartup(wVersionRequested, &IpWSAData);
    if (error != 0)
    {
        cout << "��ʼ������" << endl;
        exit(0);
    }

    sockSrv = socket(AF_INET, SOCK_DGRAM, 0);
    Server.sin_addr.s_addr = inet_addr(serverIp.c_str());
    Server.sin_family = AF_INET;
    Server.sin_port = htons(serverPort);

    bind(sockSrv, (SOCKADDR*)&Server, sizeof(SOCKADDR));//�᷵��һ��SOCKET_ERROR
    /*unsigned long ul = 1;
    int ret = ioctlsocket(sockSrv, FIONBIO, (unsigned long*)&ul);*/
}
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
        cout << "checksum error" << endl;
        return false;
    }
}
void getTime()
{
    time_t t = time(0);
    strftime(timeRecord, sizeof(timeRecord), "%H:%M:%S", localtime(&t));
}




void setPort()
{
    sendBuffer[0] = (char)(serverPort >> 8);
    sendBuffer[1] = (char)(serverPort & 0xFF);
    sendBuffer[2] = (char)(clientPort >> 8);
    sendBuffer[3] = (char)(clientPort & 0xFF);
}
void setIP()
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
            tmp += tmp * 10 + (int)clientIp[i] - 48;
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
            tmp += tmp * 10 + (int)serverIp[i] - 48;
        }
    }
    sendBuffer[ctrl++] = (char)tmp;
}
void setseq(int newSeq)
{
    seq = newSeq;
    sendBuffer[12] = (char)(seq >> 24);
    sendBuffer[13] = (char)(seq >> 16);
    sendBuffer[14] = (char)(seq >> 8);
    sendBuffer[15] = (char)seq;
}
void getseq()
{
    recvseq = (int)((unsigned char)recvBuffer[12] << 24) + (int)((unsigned char)recvBuffer[13] << 16) + (int)((unsigned char)recvBuffer[14] << 8) + (int)(unsigned char)recvBuffer[15];
}
void setack(int newack)
{
    ack = newack;
    sendBuffer[16] = (char)(ack >> 24);
    sendBuffer[17] = (char)(ack >> 16);
    sendBuffer[18] = (char)(ack >> 8);
    sendBuffer[19] = (char)ack;
}
void getack()
{
    recvack = (int)((unsigned char)recvBuffer[16] << 24) + (int)((unsigned char)recvBuffer[17] << 16) + (int)((unsigned char)recvBuffer[18] << 8) + (int)(unsigned char)recvBuffer[19];
}
void setLength(int len)
{
    length = len;
    sendBuffer[20] = (char)(length >> 8);
    sendBuffer[21] = (char)(length);
}
void getLength()
{
    length = (int)((unsigned char)recvBuffer[20] << 8) + (int)(unsigned char)recvBuffer[21];
}
void setWindow(int newWindow)
{
    window = newWindow;
    sendBuffer[22] = (char)(window >> 8);
    sendBuffer[23] = (char)(window);
}
void getWindow()
{
    recvWindow = (int)((unsigned char)recvBuffer[22] << 8) + (int)(unsigned char)recvBuffer[23];
}
void clearFlag()
{
    sendBuffer[24] = 0;
    sendBuffer[25] = 0;
}
void setAck()
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


void connectAckPrepare()
{
    setPort();
    setIP();
    getseq();
    setseq(rand());
    setack(recvseq);
    setLength(0);
    setWindow(windowLength - slidingWindow.size());
    clearFlag();
    setAck();
    setSYN();
    for (int i = 28; i < packetLen; i++)
        sendBuffer[i] = 0;
    calCheckSum();
}

bool checkackValue()
{
    getack();
    if (recvack == seq)
        return true;
    else
    {
        cout << "ackValue error" << endl;
        cout << (int)sendBuffer[12] << " " << (int)sendBuffer[13] << " " << (int)sendBuffer[14] << " " << (int)sendBuffer[15] << endl;
        cout << (unsigned int)recvBuffer[16] << " " << (unsigned int)recvBuffer[17] << " " << (unsigned int)recvBuffer[18] << " " << (int)recvBuffer[19] << endl;
        cout << recvack << endl;
        return false;
    }
}
void sendlog()
{
    getTime();
    cout << timeRecord << " [send] ";
    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << seq << "Ack: " << setw(5) << setiosflags(ios::left) << ack;
    int ACKtmp = (sendBuffer[24] & 0xF0) ? 1 : 0;
    int syntmp = (sendBuffer[24] & 0xF) ? 1 : 0;
    int fintmp = (sendBuffer[25] & 0xF0) ? 1 : 0;
    int sttmp = (sendBuffer[25] & 0xC) ? 1 : 0;
    int ovtmp = (sendBuffer[25] & 0x3) ? 1 : 0;
    cout << "ACK: " << ACKtmp << " SYN: " << syntmp << " FIN: " << fintmp << " ST: " << sttmp << " OV: " << ovtmp;
    cout << "\tsendWin:";
    for (int i = 0; i < slidingWindow.size(); i++)
    {
        cout << slidingWindow[i].seq << " ";
    }
    cout << endl;
}
void recvlog()
{
    getTime();
    cout << timeRecord << " [recv] ";
    getseq();
    getack();
    getLength();
    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << recvseq << "Ack: " << setw(5) << setiosflags(ios::left) << recvack;
    int ACKtmp = (recvBuffer[24] & 0xF0) ? 1 : 0;
    int syntmp = (recvBuffer[24] & 0xF) ? 1 : 0;
    int fintmp = (recvBuffer[25] & 0xF0) ? 1 : 0;
    int sttmp = (recvBuffer[25] & 0xC) ? 1 : 0;
    int ovtmp = (recvBuffer[25] & 0x3) ? 1 : 0;
    cout << "ACK: " << ACKtmp << " SYN: " << syntmp << " FIN: " << fintmp << " ST: " << sttmp << " OV: " << ovtmp << endl;

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
        //sendlog();
        break;
    case 1:
        while (1)
        {
            if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Client, &len) > 0)
            {
                //recvlog();
                break;
            }
        }
        break;
    case 2:
        do
        {
            sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Client, len);
            //sendlog();
            start = clock();
            whether = false;
            while (clock() - start < 2000)
            {
                if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Client, &len) > 0)
                {
                    whether = true;
                    //recvlog();
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
    if (recvseq == ack + 1)
        return true;
    else
    {
        cout << "error" << endl;
        cout << "last ack is " << ack << " and now recv seq is " << recvseq << endl;
        return false;
    }
}
void messageProcessing()
{
    if (checkST())
    {
        getLength();
        cout << "this" << endl;
        for (int i = 0; i < length; i++)
        {
            filename[i] = recvBuffer[28 + i];
        }
        fimenametmp = "C:\\Users\\lenovo\\Desktop\\NetWork\\" + (string)filename;
        cout << fimenametmp << endl;
    }
    if (checkOV())
    {
        getLength();
        for (int i = 0; i < length; i++)
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
    if (recvBuffer[25] == 0)
    {
        getLength();
        for (int i = 0; i < length; i++)
        {
            dataContent.push_back(recvBuffer[28 + i]);
        }
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
                if (checkACK() && checkackValue())
                {
                    getseq();
                    setack(recvseq);
                    break;
                }
            }
            break;
        }
        else {

        }
    }
    while (1)
    {

        recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Client, &len);
        //recvlog();
        if (checkCheckSum())
        {
            if (checkFIN())
            {
                break;
            }
            getseq();
            if (checkSeq())
            {
                messageProcessing();
                setack(recvseq);
                while ((slidingWindow.size() > 0) && (slidingWindow[0].seq == ack + 1))
                {
                    for (int j = 0; j < packetLen; j++)
                    {
                        recvBuffer[j] = slidingWindow[0].buffer[j];
                    }
                    slidingWindow.erase(slidingWindow.begin());
                    getseq();
                    setack(recvseq);
                    messageProcessing();
                }
                setseq(0);
                setLength(0);
                setWindow(windowLength - slidingWindow.size());
                clearFlag();
                setAck();
                calCheckSum();
                sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Client, len);
                //sendlog();
            }
            else
            {
                if (recvseq <= ack)
                {

                }
                else if (recvseq <= ack + windowLength)
                {
                    DataInWindow message;
                    message.seq = recvseq;
                    for (int i = 0; i < packetLen; i++)
                        message.buffer[i] = recvBuffer[i];
                    slidingWindow.push_back(message);
                    setWindow(windowLength - slidingWindow.size());
                    calCheckSum();
                    sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Client, len);
                    //sendlog();
                }
            }
        }
        else
        {
            clearFlag();
        }
    }
    getseq();
    setseq(rand());
    setack(recvseq);
    setLength(0);
    clearFlag();
    setFIN();
    setAck();
    calCheckSum();
    transmission(0);
    closesocket(sockSrv);
    WSACleanup();
}



