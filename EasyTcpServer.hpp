#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#include "Cell.hpp"
#include "CellClient.hpp"
#include "CellServer.hpp"
#include "INetEvent.hpp"

#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>






class EasyTcpServer : public INetEvent
{
private:
    SOCKET _sock;
    //消息处理对象，内部会创建线程
    std::vector<CellServer*> _cellServers;
    //每秒消息计时
    CELLTimestamp _tTime;

protected:
    std::atomic_int _recvCount;
    std::atomic_int _msgCount;
    //客户端计数
    std::atomic_int _clientCount;
    

public:
    EasyTcpServer()
    {
        _sock = INVALID_SOCKET;
        _recvCount = 0;
        _clientCount = 0;
        _msgCount = 0;
    }

    virtual ~EasyTcpServer()
    {
        Close();
    }

    //初始化socket
    SOCKET InitSocket()
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
            printf("建立套接字Socket=<%d>成功。。。\n", _sock);
        }

    }
    //绑定IP and 端口号
    int Bind(const char* ip, unsigned short port)
    {
        if(INVALID_SOCKET == _sock)
        {
            InitSocket();
        }
        //2.bind 绑定用于接受客户端链接的网络端口
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port);//host to net unsigned short
#ifdef _WIN32
        if(ip)
            _sin.sin_addr.S_un.s_addr = inet_addr(ip);
        else
            _sin.sin_addr.S_un.s_addr = INADDR_ANY;
#else
        if(ip)
            _sin.sin_addr.s_addr = inet_addr(ip);
        else
            _sin.sin_addr.s_addr = INADDR_ANY;
#endif
        int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
        if (SOCKET_ERROR == ret)
        {
            printf("ERROR, 绑定网络端口<%d>失败。。。\n",port);
        }
        else
        {
            printf("绑定网络端口<%d>成功...\n", port);
        }

        return ret;
    }
    //监听端口
    int Listen(int n)
    {
        //3 listen 监听网络端口
        int ret = listen(_sock, n);
        if(SOCKET_ERROR == ret)
        {
            printf("Socket=<%d>错误， 绑定网络端口失败。。。\n", _sock);
        }
        else
        {
            printf("Socket=<%d>绑定网络端口成功。。。\n", _sock);
        }
        return ret;
    }
    //客户端连接
    SOCKET Accept()
    {
                    //4 accept 等待接受的客户端链接
        sockaddr_in clientAddr = {};
        int nAddrLen = sizeof(sockaddr_in);
        SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
        cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
        cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
        if(INVALID_SOCKET == cSock)
        {
            printf("Socket=<%d>错误，接收到无效客户端SOCKET。。。\n", _sock);
        }
        else
        {
            addClientToCellServer(new CellClient(cSock));
            //获取IP地址 inet_ntoa(clientAddr.sin_addr)
        }

        return cSock;
    }

    void addClientToCellServer(CellClient* pClient)
    {
        //查找客户数量最少的cellserver
        auto pMinServer = _cellServers[0];
        for(auto pCellserver : _cellServers)
        {
            if(pMinServer->getClientCount() > pCellserver->getClientCount())
            {
                pMinServer = pCellserver;
            }
        }
        pMinServer->addClient(pClient);
        OnNetJoin(pClient);

    }

    void Start(int nCellServer)
    {
        for(int n=0; n< nCellServer; n++)
        {
            auto ser = new CellServer(_sock);
            _cellServers.push_back(ser);
            //注册网络事件接收对象
            ser->setEventObj(this);//使用代理进行优化
            //启动消息处理线程
            ser->Start();
        }
    }
    //关闭socket连接
    //关闭网络
    void Close()
    {
        //关闭Win sock2.x环境
        //4 关闭套接字 closesocket
        if(_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            //6 关闭套接字closesocket
            closesocket(_sock);
            // 清除windows socket环境
            WSACleanup();
#else
            close(_sock);
#endif      
        }
        _sock = INVALID_SOCKET;
    }

    //处理网络消息
    bool OnRun()
    {
        if(isRun())
        {
            time4msg();
            // 伯克利 socket
            fd_set fdRead;
            // fd_set fdWrite;
            // fd_set fdExp;

            FD_ZERO(&fdRead); //清空
            // FD_ZERO(&fdWrite);
            // FD_ZERO(&fdExp);

            FD_SET(_sock, &fdRead);
            // FD_SET(_sock, &fdWrite);
            // FD_SET(_sock, &fdExp);
            //nfds 是一个整数，描述socket的范围而不是数量
            timeval t = {1, 0};
            // int ret = select(_sock+1, &fdRead, &fdWrite, &fdExp, &t);
            int ret = select(_sock+1, &fdRead, nullptr, nullptr, &t);

            if(ret < 0)
            {
                printf("select 任务结束\n");
                Close();
                return false;
            }
            if(FD_ISSET(_sock, &fdRead))
            {
                FD_CLR(_sock,  &fdRead);
                Accept();
                return true;
            }
            return true;
        }
        return false;
    }

    //是否工作中
    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }


    //计算并输出每秒收到对网络消息
    void time4msg()
    {
        auto t1 =_tTime.getElapsedSecond();
        if(t1 >=1.0)
        {
            printf("thread<%lu>, time<%lf>, socket<%d>, clients<%d>, recv<%d>, msg<%d>\n",
            _cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount/t1), int(_msgCount/t1));
            _tTime.update();
            _recvCount = 0;
            _msgCount = 0;
        }
    }

    virtual void OnNetJoin(CellClient* pClient)
    {
        _clientCount++;
        // printf("client<%d> join\n", pClient->sockfd());
    }


    virtual void OnNetLeave(CellClient* pClient)
    {
        _clientCount--;
        // printf("client<%d> exit\n", pClient->sockfd());
    }

    virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header)
    {
        _msgCount++;
    }

    virtual void OnNetRecv(CellClient* pClient)
    {
        _recvCount++;
    }


};

#endif