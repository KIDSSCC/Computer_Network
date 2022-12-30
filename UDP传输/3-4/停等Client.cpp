#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 
#include<iostream>
#include<vector>
#include<fstream>
#include<time.h> 
#include<io.h>
#include<string>
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
int length = 0;
int seq;
int ack;
int recvSeq;
int recvAck;
vector<string> files;
string path("E:\\mycode\\testfiles");

char sendBuffer[packetLen];
char recvBuffer[packetLen];
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
bool getFiles(string path, vector<string>& files);
void loadFile(int number);

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
        cout << "ʧ��У��" << endl;
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
    int lengthtmp = (int)((unsigned char)sendBuffer[20] << 8) + (int)((unsigned char)sendBuffer[21]);
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
    int lengthtmp = (int)((unsigned char)recvBuffer[20] << 8) + (int)((unsigned char)recvBuffer[21]) ;
    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << seqtmp << "Ack: " << setw(5) << setiosflags(ios::left) << acktmp << "Length: " << setw(5) << setiosflags(ios::left) << lengthtmp;
    int ACKtmp = (recvBuffer[24] & 0xF0) ? 1 : 0;
    int syntmp = (recvBuffer[24] & 0xF) ? 1 : 0;
    int fintmp = (recvBuffer[25] & 0xF0) ? 1 : 0;
    int sttmp = (recvBuffer[25] & 0xC) ? 1 : 0;
    int ovtmp = (recvBuffer[25] & 0x3) ? 1 : 0;
    cout << "ACK: " << ACKtmp << " SYN: " << syntmp << " FIN: " << fintmp << " ST: " << sttmp << " OV: " << ovtmp << endl;
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
void setSeq(int newSeq)
{
    seq = newSeq;
    sendBuffer[12] = (char)(newSeq >> 24);
    sendBuffer[13] = (char)(newSeq >> 16);
    sendBuffer[14] = (char)(newSeq >> 8);
    sendBuffer[15] = (char)newSeq;
}
void setAck(int newAck)
{
    ack = newAck;
    sendBuffer[16] = (char)(ack >> 24);
    sendBuffer[17] = (char)(ack >> 16);
    sendBuffer[18] = (char)(ack >> 8);
    sendBuffer[19] = (char)ack;
}
void setLength(int newLen)
{
    length = newLen;
    sendBuffer[20] = (char)(length >> 8);
    sendBuffer[21] = (char)(length );
}
void getSeq()
{
    recvSeq = (int)((unsigned char)recvBuffer[12] << 24) + (int)((unsigned char)recvBuffer[13] << 16) + (int)((unsigned char)recvBuffer[14] << 8) + (int)(unsigned char)recvBuffer[15];
}
void getAck()
{
    recvAck = (int)((unsigned char)recvBuffer[16] << 24) + (int)((unsigned char)recvBuffer[17] << 16) + (int)((unsigned char)recvBuffer[18] << 8) + (int)(unsigned char)recvBuffer[19];

}

bool checkAckValue()
{
    getAck();
    if (recvAck == seq)
        return true;
    else
    {
        cout << "check Ack Value error" << endl;
        return false;
    }
}
void connectEstablishment()
{
    setPort();
    setIp();
    setSeq(rand());
    setAck(0);
    setLength(0);
    clearFlag();

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
        whether = false;
        do
        {
            sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Server, len);
            sendlog();
            whether = false;
            start = clock();
            while (clock() - start < 4000)
            {
                if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Server, &len) > 0)
                {
                    recvlog();
                    whether = true;
                    break;
                }
            }
        } while (whether);

        break;
    case 1:
        break;
    case 2:
        do
        {
            sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Server, len);
            sendlog();
            start = clock();
            whether = false;
            while (clock() - start < 5000)
            {
                if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Server, &len) > 0)
                {
                    recvlog();
                    whether = true;
                    break;
                }
            }
            if (!whether)
            {
                cout << "time out errer" << endl;
            }
        } while (!checkCheckSum() || (!whether));

        break;
    case 3:
        break;
    default:
        break;
    }
}
int main()
{
    initial();
    cout << "Client Service is operating!" << endl;
    cout << "**********begin connect**********" << endl;
    connectEstablishment();
    while (1)
    {
        transmission(2);
        if (checkACK() && checkSYN() && checkAckValue())
        {
            getSeq();
            setSeq(rand());
            setAck(recvSeq);
            clearFlag();
            setACK();
            calCheckSum();
            transmission(0);
            cout << "**********end   connect**********" << endl;
            break;

        }
    }
    getFiles(path, files);
    int num = files.size();
    cout << "**********begin send files*******" << endl;
    setSeq(0);
    for (int i = 0; i < num; i++)
    {
        char fileName[100];
        strcpy(fileName, files[i].substr(20, files[i].length() - 20).c_str());
        setSeq(seq+1);
        setAck(0);
        setLength(strlen(fileName));
        clearFlag();
        setST();
        for (int j = 0; j < length; j++)
        {
            sendBuffer[j + 28] = fileName[j];
        }
        calCheckSum();
        do
        {
            transmission(2);
        } while (!(checkACK() && checkAckValue()));

        loadFile(i);
        for (int j = 0; j < dataContent.size(); j++)
        {
            sendBuffer[28 + (j % messageLen)] = dataContent[j];
            if ((j % messageLen == messageLen - 1) && (j != dataContent.size() - 1))
            {
                setSeq(seq + 1);
                setAck(0);
                setLength(messageLen);
                clearFlag();
                calCheckSum();
                do
                {
                    transmission(2);
                } while (!(checkACK() && checkAckValue()));
            }
            if (j == dataContent.size() - 1)
            {
                setSeq(seq + 1);
                setAck(0);
                setLength(j % messageLen + 1);
                clearFlag();
                setOV();
                calCheckSum();
                do
                {
                    transmission(2);
                } while (!(checkACK() && checkAckValue()));
            }
        }
    }
    cout << "**********end   send files*******" << endl;
    cout << "**********begin disconnect*******" << endl;
    setSeq(rand());
    setAck(0);
    setLength(0);
    clearFlag();
    setFIN();
    calCheckSum();
    do
    {
        transmission(2);
    } while (!(checkCheckSum() && checkACK() && checkAckValue()));
    closesocket(sockSrv);
    WSACleanup();
    cout << "**********end   disconnect*******" << endl;
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
    /*****************************************/
    Server.sin_addr.s_addr = inet_addr(serverIp.c_str());
    Server.sin_family = AF_INET;
    Server.sin_port = htons(serverPort);
    /*****************************************/
    Client.sin_addr.s_addr = inet_addr(clientIp.c_str());
    Client.sin_family = AF_INET;
    Client.sin_port = htons(clientPort);
    /*****************************************/
    bind(sockSrv, (SOCKADDR*)&Client, sizeof(SOCKADDR));
    unsigned long ul = 1;
    int ret = ioctlsocket(sockSrv, FIONBIO, (unsigned long*)&ul);
}
void setPort()
{
    sendBuffer[0] = (char)(clientPort >> 8);
    sendBuffer[1] = (char)(clientPort & 0xFF);
    sendBuffer[2] = (char)(serverPort >> 8);
    sendBuffer[3] = (char)(serverPort & 0xFF);
}
void setIp()
{
    int tmp = 0;
    int ctrl = 4;
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
    tmp = 0;
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
}
bool getFiles(string path, vector<string>& files)
{
    _int64 hFile = 0;
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
        if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
        {
            files.push_back(p.assign(path).append("\\").append(fileinfo.name));

        }
        while (_findnext(hFile, &fileinfo) == 0)
        {
            if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
            {
                files.push_back(p.assign(path).append("\\").append(fileinfo.name));
            }
        }
        _findclose(hFile);
    }
    return 1;
}
void loadFile(int number)
{
    vector<char>().swap(dataContent);
    ifstream fin(files[number].c_str(), ifstream::binary);
    char t = fin.get();
    while (fin)
    {
        dataContent.push_back(t);
        t = fin.get();
    }
}
