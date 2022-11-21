# 编程作业三：基于UDP服务设计可靠传输协议并编程实现

## 实验3-1：

​		利用数据报套接字在用户空间实现面向连接的可靠数据传输，功能包括：建立连接、差错检测、确认重传等。流量控制采用停等机制，完成给定测试文件的传输。

### 一.作业要求

​		数据包套接字采用UDP协议，通过实现类似于TCP协议的握手挥手机制来建立连接。计算校验和进行差错检测。实现rdt可靠传输协议或自行设计协议，完成由客户端到服务端的单向文件传输，输出日志。

### 二.协议设计

![UDP传输-第 1 页](C:\Users\lenovo\Desktop\UDP传输\3-1\UDP传输-第 1 页.png)

​		UDP传输协议的设计如图，根据查阅相关资料，UDP报文长度的最大值为65507字节。UDP传输的通常实现其报文长度均小于这一最大值。在本次实验中，设置UDP报文大小为32768字节。其中头部信息占28字节，数据段信息占32740字节。

​		报文的头部包含的信息如下：

| 标识             | 含义                                         |
| ---------------- | -------------------------------------------- |
| 源端口号         | 发送此报文的终端所对应的端口号               |
| 目的端口号       | 接收此报文的终端所对应的端口号               |
| 源IP             | 发送此报文的终端IP地址                       |
| 目的IP           | 接收此报文的终端IP地址                       |
| 初始序号seq      | 初始序号值，代表当前报文状态，也用于去除冗余 |
| 确认序号ack      | 与seq相应，用以进行回复                      |
| 数据段长度Length | 代表本次报文中数据段有效信息的长度           |
| ACK              | 标志位，接收到上一次报文                     |
| SYN              | 标志位，申请建立连接                         |
| FIN              | 标志位，申请断开连接                         |
| ST               | 标志位，本次报文为一个文件的首个报文         |
| OV               | 标志位，本次报文为当前文件的最后一个报文     |
| 校验和           | 存放校验和。                                 |

### 三.传输过程

​		整个传输过程的流程示意如图。

![UDP传输-第 2 页](C:\Users\lenovo\Desktop\UDP传输\3-1\UDP传输-第 2 页.png)

**握手阶段：**

​		在握手阶段，由Client向server端发起连接请求，将SYN位置一并随机初始化seq值。Server端收到报文后，回复ACK与SYN，并将ack值根据第一次握手时的seq加一，同时也随机初始化自己的seq值。Client收到二次握手的消息后，回复ACK并将ack值根据二次握手时的seq值加一。至此，完成Client与Server的连接建立过程。

**文件传输过程**

​		在文件传输过程中。由Client开始向Server发送文件报文。在发送第一个报文时，会将Client端的seq与Server端的ack同步为0.随后Server端每收到一个报文，便会将ack值加一后在ACK报文中进行回复。在此处引入**额外的校验机制**，由Client端发出的报文seq值会有序递增（为0时为初始化）。Server端接收时会将自己的ack值与接收到的seq值进行比对。二者相等说明本次收到的报文与上一次收到的报文时连续的。即可正确接收，否则不予接收。每个文件的第一个报文均为文件名。且报文中ST位置一。Server端会做好接收此文件的准备。最后一个报文其OV位会置一。Server端收到OV标志的报文后会着手进行文件的输出。

**两次挥手过程**

​		发送完所有文件后，Client会向Server端发送带有FIN标志的报文，申请断开连接。Server端接收到带有FIN信号的报文后，会退出文件接收状态。进入挥手阶段，回复ACK与FIN的报文。Client接收后，完成挥手过程。Client与Server端各自进行关闭。

### 四.差错检测

​		在本次实验中最主要的差错检测机制即校验和检验。大致流程如下：

![UDP传输-第 3 页](C:\Users\lenovo\Desktop\UDP传输\3-1\UDP传输-第 3 页.png)

在发送报文前，发送报文的一侧会进行校验和的计算，并填入到发送缓冲区的对应位置。接收一方接收到报文后会重新计算一遍校验和并与随报文一同发送过来的校验和进行求和检验。当求和结果全1时说明数据是一致的，在传输过程没有发生比特位的偏转。

### 五.可靠传输

​	本次实验中可靠传输协议仿照rdt3.0进行设计，在rdt3.0的基础上，仍保留了差错重传并针对超时重传进行了冗余控制。

**差错重传**

​		在接收一方接收到报文时，会进行校验码的检查。若得到全1的检查结果，说明在传输过程中没有发生比特位的翻转，可以正常进行下一个环节。而当校验结果错误时。会进行回复，在回复的报文中，ACK并不会被置位。当发送的一方收到ACK未置位的报文时，说明上一条报文在传输中发生了错误，会重新进行传送。差错重传同样适用于ACK报文出错的情况。

**超时重传**

​		当发送一方发送出需要接收方进行回复的报文时，会开始计时，在指定时间内未收到回复时，会将报文进行重传。

**冗余控制**

![UDP传输-第 4 页](C:\Users\lenovo\Desktop\UDP传输\3-1\UDP传输-第 4 页.png)

流程如图所示，当因为超时将数据包进行重传后，原数据包到达Server，此时报文本身并不存在问题，但在Server端形成了冗余，Server会继续接收。因此，在接收数据包时，添加了序列号的检测。对于相同的数据包，只会接收一次。从而解决了冗余问题。

​		**注：在11月18日检查中。超时重传的机制并未完成。至11月19日提交报告前，完成了完整了可靠的传输协议（包括差错重传，超时重传）。具体演示将在下文进行。**

### 六.代码分析

```C++
//计算校验和
void calCheckSum()
{
    int sum = 0;
    //计算除校验码位的全部字节
    for (int i = 0; i < packetLen; i += 2)
    {
        if (i == 26)
            continue;
        //每次往上加16位
        sum += (sendBuffer[i] << 8) + sendBuffer[i + 1];
        //出现进位,移至最低位
        if (sum >= 0x10000)
        {
            sum -= 0x10000;
            sum += 1;
        }
    }
    //取反
    USHORT checkSum = ~(USHORT)sum;
    sendBuffer[26] = (char)(checkSum >> 8);
    sendBuffer[27] = (char)checkSum;
}
```

```C++
//校验码检查
bool checkCheckSum()
{
    int sum = 0;
    //计算除校验码位的全部字节
    for (int i = 0; i < packetLen; i += 2)
    {
        if (i == 26)
            continue;
        //每次往上加16位
        sum += (recvBuffer[i] << 8) + recvBuffer[i + 1];
        //出现进位,移至最低位
        if (sum >= 0x10000)
        {
            sum -= 0x10000;
            sum += 1;
        }
    }
    //checksum计算
    //取反
    USHORT checksum = (recvBuffer[26] << 8) + (unsigned char)recvBuffer[27];
    if (checksum + (USHORT)sum == 0xffff)
    {
        return true;
    }
    {
        cout << "失败校验" << endl;
        return false;
    }
}
```

SeqAndAckSet(int,bool)：用以进行seq与ack值得设定。第一个参数为需要设定得seq值。第二个参数为对ack值得处理方法。为1时，将根据收到得seq值+1，为0时，将进行初始化，将ack设置为0

```C++
void SeqAndAckSet(int newSeq, bool newAck)
{
    seq = newSeq;
    int recvSeq = (int)((unsigned char)recvBuffer[12] << 24) + (int)((unsigned char)recvBuffer[13] << 16) + (int)((unsigned char)recvBuffer[14] << 8) + (int)(unsigned char)recvBuffer[15];
    ack = newAck ? recvSeq + 1 : 0;
    sendBuffer[12] = (char)(seq >> 24);
    sendBuffer[13] = (char)(seq >> 16);
    sendBuffer[14] = (char)(seq >> 8);
    sendBuffer[15] = (char)seq;
    sendBuffer[16] = (char)(ack >> 24);
    sendBuffer[17] = (char)(ack >> 16);
    sendBuffer[18] = (char)(ack >> 8);
    sendBuffer[19] = (char)ack;
}
```

lengthSet(int):对于length段的设定，参数为本次报文数据段的长度

```C++
void lengthSet(int len)
{
    length = len;
    sendBuffer[20] = (char)(length >> 24);
    sendBuffer[21] = (char)(length >> 16);
    sendBuffer[22] = (char)(length >> 8);
    sendBuffer[23] = (char)length;
}
```

标志位的清除，设置于检查

```C++
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
```

ackValueCheck()与checkSeq():在文件传输部分所提到的序列号校验机制，会将收到的ack与seq与已有信息进行比对，对于ack的比对用于Client判断上一次报文是否发生错误，对于seq的比对将用于Server判断是否发生乱序或者冗余。

```C++
bool ackValueCheck()
{
     int recvack = (int)((unsigned char)recvBuffer[16] << 24) +(int)((unsigned char)recvBuffer[17] << 16) +(int) ((unsigned char)recvBuffer[18] << 8) + (int)(unsigned char)recvBuffer[19];
    if (recvack == seq + 1)
        return true;
    else
    {
        cout << "错误" << endl;
        return false;
    }
}
bool checkSeq()
{
    int recvSeq = (int)((unsigned char)recvBuffer[12] << 24) + (int)((unsigned char)recvBuffer[13] << 16) + (int)((unsigned char)recvBuffer[14] << 8) + (int)(unsigned char)recvBuffer[15];
    if ((recvSeq == ack)||(recvSeq==0))
        return true;
    else
    {
        cout << "error" << endl;
        cout << recvSeq << " " << ack << endl;
        return false;
    }
}
```

initial():连接的初始化工作，用以完成socket的配置。

```C++
void initial()
{
    //MAKEWORD，第一个参数为低位字节，主版本号，第二个参数为高位字节，副版本号
    wVersionRequested = MAKEWORD(1, 1);
    //进行socket库的绑定
    int error = WSAStartup(wVersionRequested, &IpWSAData);
    if (error != 0)
    {
        cout << "初始化错误" << endl;
        exit( 0);
    }

    //创建用于监听的socket
    //AF_INET：TCP/IP&IPv4，SOCK_DGRAM：UDP数据报
    sockSrv = socket(AF_INET, SOCK_DGRAM, 0);
    //IP地址
    Server.sin_addr.s_addr = inet_addr(serverIp.c_str());
    //协议簇
    Server.sin_family = AF_INET;
    //连接端口号
    Server.sin_port = htons(serverPort);
    //socket传输改为非阻塞模式。
    unsigned long ul = 1;
    int ret = ioctlsocket(sockSrv, FIONBIO, (unsigned long*)&ul);
}
```

transmission函数，用于进行发送与接收的控制，在Client与Server端的具体实现略有不同。如下是以Client为例。传输行为涉及先发再收（第一，二次握手，文件传输，挥手）与只发不收（第三次握手）

​		在socket对象初始化时，将其设置未非阻塞模式，从而便于进行超时的控制。对于需要进行超时控制的地方，通过clock()进行及时。针对Client端的超时控制，有**额外的问题即2MSL问题**，具体体现在Client端发出的第三次握手上，但第三次握手的报文发生丢失时，Server端将在超时后重新发送第二次握手。在代码中，针对第三次握手后添加了一定的延迟即2MSL延迟时间，在这一时间内，Client将处于等待状态，如又收到了Server的数据包即代表三次握手丢失，会重新进行发送。保证Client和Server能正确的建立完连接进入到下一阶段。

```C++
void transmission(int ctrl)
{
    int len = sizeof(SOCKADDR);
    int start;
    bool whether;
    switch (ctrl)
    {
        
    case 0:
        //只发不收
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
        //只收不发
        break;
    case 2:
        //先发再收
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
        } while (!checkCheckSum()||(!whether));
       
        break;
    case 3:
        //先收再发
        break;
    default:
        break;
    }
}
```

Client端三次握手建立连接过程。

```C++
cout << "**********begin connect**********" << endl;
    connectEstablishment();
    while (1)
    {
        //第一次握手，校验失败会重传。
        transmission(2);
        if (checkACK()&&checkSYN() && ackValueCheck())
        {
            //收到的包正确且为二次握手
            //准备进行三次握手的回复
            SeqAndAckSet(rand(),1);
            //标志位
            clearFlag();
            setAck();
            calCheckSum();
            transmission(0);
            cout << "**********end   connect**********" << endl;
            break;

        }
    }
```

### 七.程序演示

测试文件路径为：C:\Users\lenovo\Desktop\UDP传输\testfiles

目标路径为：C:\Users\lenovo\Desktop\NetWork，初始为空。

通过路由进行连接。Client端发送报文至路由，路由进行丢包或延迟控制后转发给Server。Server发送的报文也经过路由转发给Client。

**注：本次报文大小一开始设置为32768字节（包括11月18日演示），但路由转发的包大小上限为15000字节。因此在后续将报文大小修改为10028字节，其余保持不变**

当设置丢包率为0%时,可正常完成传输过程

![传输1](C:\Users\lenovo\Desktop\UDP传输\3-1\传输1.png)

![传输结果1.1](C:\Users\lenovo\Desktop\UDP传输\3-1\传输结果1.1.png)

![传输结果1.2](C:\Users\lenovo\Desktop\UDP传输\3-1\传输结果1.2.png)

当设置丢包率为5%时，重新进行运行

![传输结果2](C:\Users\lenovo\Desktop\UDP传输\3-1\传输结果2.png)

程序可正常运行并完整发送文件。可发现在seq=17处发生了一次丢包，经过一定二等超时之后，Client端将此数据包重新进行了发送。

### 八.实验总结

​		通过本次实验，加深了对于UDP传输的理解，同时也对应了理论课中所涉及的差错校验，可靠传输等内容的具体实践。