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
#include<thread>
#include<mutex>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int packetLen = 4096;
const int messageLen = 4068;



WORD wVersionRequested;
WSADATA IpWSAData;
SOCKET sockSrv;

SOCKADDR_IN  Server;
string serverIp = "127.0.0.3";
string clientIp = "127.0.0.2";
USHORT serverPort = 4444;
USHORT clientPort = 1111;
int len = sizeof(SOCKADDR);



char sendBuffer[packetLen];
char recvBuffer[packetLen];
char resendBuffer[packetLen];
char timeRecord[20] = { 0 };
int length = 0;
int seq;
int ack;
int recvseq;
int recvack;
int window;
int recvWindowLen;


vector<char> dataContent;
vector<string> files;
string path("E:\\mycode\\testfiles");


struct DataInWindow {
    bool ack = false;
    char buffer[packetLen];
    int seq;
    int clock;
};
vector<DataInWindow> slidingWindow;
int windowLength = 30;
std::mutex mtx;

int globalPackCount = 0;

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
    unsigned long ul = 1;
    int ret = ioctlsocket(sockSrv, FIONBIO, (unsigned long*)&ul);
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
    else
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
    sendBuffer[0] = (char)(clientPort >> 8);
    sendBuffer[1] = (char)(clientPort & 0xFF);
    sendBuffer[2] = (char)(serverPort >> 8);
    sendBuffer[3] = (char)(serverPort & 0xFF);
}
void setIP()
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
int getWindow()
{
    return (int)((unsigned char)recvBuffer[22] << 8) + (int)(unsigned char)recvBuffer[23];
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

void sendlog()
{
    getTime();
    cout << timeRecord << " [send] ";
    int seqtmp = (int)((unsigned char)sendBuffer[12] << 24) + (int)((unsigned char)sendBuffer[13] << 16) + (int)((unsigned char)sendBuffer[14] << 8) + (int)(unsigned char)sendBuffer[15];

    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << seqtmp << "Ack: " << setw(5) << setiosflags(ios::left) << ack;
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
void resendlog()
{
    getTime();
    cout << timeRecord << " [send] ";
    int seqtmp = (int)((unsigned char)resendBuffer[12] << 24) + (int)((unsigned char)resendBuffer[13] << 16) + (int)((unsigned char)resendBuffer[14] << 8) + (int)(unsigned char)resendBuffer[15];

    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << seqtmp << "Ack: " << setw(5) << setiosflags(ios::left) << ack;
    int ACKtmp = (resendBuffer[24] & 0xF0) ? 1 : 0;
    int syntmp = (resendBuffer[24] & 0xF) ? 1 : 0;
    int fintmp = (resendBuffer[25] & 0xF0) ? 1 : 0;
    int sttmp = (resendBuffer[25] & 0xC) ? 1 : 0;
    int ovtmp = (resendBuffer[25] & 0x3) ? 1 : 0;
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
    int windowLentmp = getWindow();
    cout << "  Seq: " << setw(5) << setiosflags(ios::left) << recvseq << "Ack: " << setw(5) << setiosflags(ios::left) << recvack;
    int ACKtmp = (recvBuffer[24] & 0xF0) ? 1 : 0;
    int syntmp = (recvBuffer[24] & 0xF) ? 1 : 0;
    int fintmp = (recvBuffer[25] & 0xF0) ? 1 : 0;
    int sttmp = (recvBuffer[25] & 0xC) ? 1 : 0;
    int ovtmp = (recvBuffer[25] & 0x3) ? 1 : 0;
    cout << "ACK: " << ACKtmp << " SYN: " << syntmp << " FIN: " << fintmp << " ST: " << sttmp << " OV: " << ovtmp;
    cout << "\trecvWin:" << windowLentmp << endl;
}

void connectEstablishment()
{
    setPort();
    setIP();
    setseq(rand());
    setack(0);
    setLength(0);
    setWindow(0);
    clearFlag();
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
        cout << "ackvalue error" << endl;
        cout << (int)sendBuffer[12] << " " << (int)sendBuffer[13] << " " << (int)sendBuffer[14] << " " << (int)sendBuffer[15] << endl;
        cout << (unsigned int)recvBuffer[16] << " " << (unsigned int)recvBuffer[17] << " " << (unsigned int)recvBuffer[18] << " " << (int)recvBuffer[19] << endl;
        cout << recvack << endl;
        return false;
    }
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
            while (clock() - start < 2000)
            {
                if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Server, &len) > 0)
                {
                    recvlog();
                    whether = true;
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

void recvthread()
{

    while (1)
    {
        mtx.lock();
        if (recvfrom(sockSrv, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&Server, &len) > 0)
        {
            if (checkCheckSum() && checkACK)
            {
                //recvlog();
                getack();

                if (recvack < slidingWindow[0].seq)
                {
                }
                else
                {
                    recvWindowLen += recvack - slidingWindow[0].seq + 1;
                    while (slidingWindow[0].seq <= recvack)
                    {
                        slidingWindow.erase(slidingWindow.begin());
                        if (slidingWindow.size() == 0)
                            break;
                    }
                }

            }
        }
        mtx.unlock();
    }
}
void retransmission()
{
    while (1)
    {
        mtx.lock();
        if (slidingWindow.size() > 0)
        {
            if (clock() - slidingWindow[0].clock > 1000)
            {
                slidingWindow[0].clock = clock();
                sendto(sockSrv, slidingWindow[0].buffer, sizeof(slidingWindow[0].buffer), 0, (SOCKADDR*)&Server, len);
                //resendlog();
                cout << "timeout error" << endl;
                globalPackCount++;
            }
        }
        mtx.unlock();
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
        if (checkACK() && checkSYN() && checkackValue())
        {

            recvWindowLen = getWindow();
            getseq();
            setseq(0);
            setack(recvseq);
            clearFlag();
            setAck();
            calCheckSum();
            transmission(0);
            cout << "**********end   connect**********" << endl;
            break;

        }
    }



    getFiles(path, files);
    int num = files.size();
    cout << "**********begin send files*******" << endl;
    std::thread t1(recvthread);
    std::thread t2(retransmission);

    int globalStart = clock();
    for (int i = 0; i < num; i++)
    {
        bool whethersend = false;
        while (!whethersend)
        {
            mtx.lock();
            if ((slidingWindow.size() < windowLength) && (recvWindowLen > 0))
            {
                whethersend = true;
                recvWindowLen--;
                char fileName[100];
                strcpy(fileName, files[i].substr(20, files[i].length() - 20).c_str());
                setseq(seq + 1);
                setack(0);
                setLength(strlen(fileName));
                clearFlag();
                setST();
                for (int j = 0; j < length; j++)
                {
                    sendBuffer[j + 28] = fileName[j];
                }
                calCheckSum();
                DataInWindow message;
                for (int x = 0; x < packetLen; x++)
                    message.buffer[x] = sendBuffer[x];
                message.seq = seq;
                message.clock = clock();
                slidingWindow.push_back(message);
                sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Server, len);
                globalPackCount++;
                //sendlog();
            }
            mtx.unlock();
        }

        loadFile(i);
        cout << "+++++send file " << i + 1 << " total length is " << dataContent.size() << "++++++" << endl;
        for (int j = 0; j < dataContent.size(); j++)
        {
            sendBuffer[28 + (j % messageLen)] = dataContent[j];
            if ((j % messageLen == messageLen - 1) && (j != dataContent.size() - 1))
            {
                whethersend = false;
                while (!whethersend)
                {
                    mtx.lock();
                    if ((slidingWindow.size() < windowLength) && recvWindowLen > 0)
                    {
                        whethersend = true;
                        recvWindowLen--;
                        setseq(seq + 1);
                        setack(0);
                        setLength(messageLen);
                        clearFlag();
                        calCheckSum();
                        DataInWindow message;
                        for (int x = 0; x < packetLen; x++)
                            message.buffer[x] = sendBuffer[x];
                        message.seq = seq;
                        message.clock = clock();
                        slidingWindow.push_back(message);
                        sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Server, len);
                        globalPackCount++;
                        //sendlog();
                    }
                    mtx.unlock();
                }
            }
            if (j == dataContent.size() - 1)
            {
                whethersend = false;
                while (!whethersend)
                {
                    mtx.lock();
                    if ((slidingWindow.size() < windowLength) && (recvWindowLen > 0))
                    {
                        whethersend = true;
                        recvWindowLen--;
                        setseq(seq + 1);
                        setack(0);
                        setLength(j % messageLen + 1);
                        clearFlag();
                        setOV();
                        calCheckSum();
                        DataInWindow message;
                        for (int x = 0; x < packetLen; x++)
                            message.buffer[x] = sendBuffer[x];
                        message.seq = seq;
                        message.clock = clock();
                        slidingWindow.push_back(message);
                        sendto(sockSrv, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&Server, len);
                        globalPackCount++;
                        //sendlog();
                    }
                    mtx.unlock();
                }

            }
        }
    }
    cout << "**********end   send files*******" << endl;
    while (1)
    {
        mtx.lock();
        if (slidingWindow.size() == 0)
        {
            break;
        }
        mtx.unlock();
    }
    int globalEnd = clock();
    cout << "**********begin disconnect*******" << endl;
    setseq(rand());
    setack(0);
    setLength(0);
    clearFlag();
    setFIN();
    calCheckSum();
    do
    {
        transmission(2);
    } while (!(checkCheckSum() && checkACK() && checkackValue()));
    closesocket(sockSrv);
    WSACleanup();
    cout << "**********end   disconnect*******" << endl;
    cout << globalPackCount * packetLen << endl;
    cout << globalEnd - globalStart << endl;
}



