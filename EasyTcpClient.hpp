#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
    #include <windows.h>
    #include <WinSock2.h>
    #define WIN32_LEAN_AND_MEAN
#else
    #include <unistd.h>  // unix std
    #include <arpa/inet.h>
    #include <string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR    (-1)   
#endif

#include <stdio.h>
#include "MessageHeader.hpp"


class EasyTcpClient
{
    SOCKET _sock;
public:
    EasyTcpClient()
    {
        _sock = INVALID_SOCKET;
    }

    virtual ~EasyTcpClient()
    {
        Close();
    }

    int initSocket()
    {
        //启动Win sock2.x环境
#ifdef _WIN32
        WORD ver = MAKEWORD(2, 2);
        WSADATA dat;
        WSAStartup(ver, &dat);
#endif
        //1 建立一个socket
        if(INVALID_SOCKET != _sock)
        {
            printf("<socket=%d>关闭旧连接。。。\n", _sock);
            Close();
        }

        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        
        if(INVALID_SOCKET == _sock)
        {
            printf("错误， 建立套接字失败。。。\n");
        }
        else
        {
            printf("建立套接字成功。。。\n");
        }

        return 0;
    }

    int Connect(char* ip, short port)
    {   
        if(INVALID_SOCKET == _sock)
        {
            initSocket();
        }

        //2 链接服务器 connect
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port);
#ifdef _WIN32
        _sin.sin_addr.S_un.s_addr = inet_addr(ip);
#else
        _sin.sin_addr.s_addr = inet_addr(ip); 
#endif
        int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
        if(SOCKET_ERROR == ret)
        {
            printf("<socket=%d>错误， 链接服务器<%s:%d>失败。。。\n", _sock, ip, port);
        }
        else
        {
            printf("<socket=%d>连接服务器<%s:%d>成功。。。\n", _sock, ip, port);
        }
        return ret;
    }

    void Close()
    {
        //关闭Win sock2.x环境
        //4 关闭套接字 closesocket
        if(_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            closesocket(_sock);
            WSACleanup();
#endif
            close(_sock);
            // printf("客户端推出，任务结束\n");
            _sock = INVALID_SOCKET;
        }
    }

    int _ncount=0;
    bool OnRun()
    {   
        if(isRun())
        {
            fd_set fdReads;
            FD_ZERO(&fdReads);
            FD_SET(_sock, &fdReads);
            timeval t = {1, 0};
            int ret = select(_sock+1, &fdReads, 0 ,0, &t);
            // printf("select ret = %d count=%d\n", ret, _ncount++);
            if(ret < 0)
            {
                printf("<socket=%d>select任务结束1\n", _sock);
                Close();
                return false;
            }
            if(FD_ISSET(_sock, &fdReads))
            {
                FD_CLR(_sock, &fdReads);
                if( -1 == RecvData(_sock))
                {
                    printf("<socket=%d>select任务结束2\n", _sock);
                    Close();
                    return false;
                }
            }
            return true;
        }
        return false;
        
    }

    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }

    //接受缓冲区
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE  10240
#endif
    char _szRecv[RECV_BUFF_SIZE]={};
    //消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE * 10] = {}; 
    int _lastPos = 0;

    //接受数据 处理粘包 拆分
    int RecvData(SOCKET cSock)
    {
        int nLen = (int)recv(cSock, _szRecv, sizeof(_szRecv), 0);
        // printf("nLen=%d\n", nLen);
        
        if(nLen <= 0)
        {
            printf("<socket=%d>与服务器断开连接， 任务结束。\n", cSock);
            return -1;
        }
        //将收取到的数据拷贝到消息缓冲区
        memcpy(_szMsgBuf+_lastPos, _szRecv, nLen);
        //消息缓冲区的数据尾部位置后移
        _lastPos += nLen;
        //判断消息缓冲区的数据长度是否大于消息头
        while(_lastPos >= sizeof(DataHeader))
        {
            //这时就可以知道当前消息体的长度
            DataHeader* header = (DataHeader*)_szMsgBuf;
            if(_lastPos > header->dataLength)
            {
                //剩余未处理消息缓冲区数据的长度
                int nSize = _lastPos - header->dataLength;
                //处理网络消息
                OnNetMsg(header);
                //将剩余未处理消息缓冲区数据前移
                memcpy(_szMsgBuf, _szMsgBuf+header->dataLength, nSize);
                _lastPos = nSize;
            }
            else
            {   
                //剩余数据不够一条完整消息
                break;
            }
            
        }

        // recv(cSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
        // OnNetMsg(header);

        
        return 0;
    }

    //响应
    virtual void OnNetMsg(DataHeader* header)
    {
        switch(header->cmd)
        {
            case CMD_LOGIN_RESULT:
            {
                LoginResult* login = (LoginResult*)header;
                printf("<socket=%d>收到服务端消息 CMD_LOGIN_RESULT 数据长度：%d  \n",
                _sock, login->dataLength);
            }
            break;
            case CMD_LOGOUT_RESULT:
            {   
                LogoutResult* logout = (LogoutResult*)header;
                // printf("<socket=%d>收到服务端消息 CMD_LOGOUT_RESULT 数据长度：%d  \n",
                // _sock, logout->dataLength);

            }
            break;
            case CMD_NEW_USER_JOIN:
            {
                NewUserJoin* userjoin = (NewUserJoin*)header;
                // printf("<socket=%d>收到服务端消息 CMD_NEW_USER_JOIN 数据长度：%d  \n", 
                // _sock, userjoin->dataLength);

            }
            break;
            case CMD_ERROR:
            {
                printf("<socket=%d>收到服务端消息 CMD_ERROR 数据长度：%d  \n", 
                _sock, header->dataLength);
            }
            break;
            default:
            {
                printf("<socket=%d>收到未定义消息 数据长度：%d  \n", 
                _sock, header->dataLength);
            }
            break;
        }
    }

    //发送数据
    int SendData(DataHeader* header)
    {
        if(isRun() && header)
        {
            send(_sock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
        
    }
};

#endif