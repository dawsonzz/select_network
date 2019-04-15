#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_
#include "MessageHeader.hpp"

#ifdef _WIN32
    #define FD_SETSIZE 64
    #include <windows.h>
    #include <WinSock2.h>
    #pragma comment(lib, "ws2_32.lib");
    #define WIN32_LEAN_AND_MEAN
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
#else
    #include <unistd.h>  // uninx std
    #include <arpa/inet.h>
    #include <string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR    (-1)   
#endif


#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE  10240
#endif

#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "CELLTimestamp.hpp"
#include <map>

class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
        _lastPos = 0;
    }

    SOCKET sockfd()
    {
        return _sockfd;
    }

    char* msgBuf()
    {
        return _szMsgBuf;
    }

    int getLastPos()
    {
        return _lastPos;
    }

    void setLastPos(int pos)
    {
        _lastPos = pos;
    }
    //发送数据
    int SendData(DataHeader* header)
    {
        if(header)
        {
            return send(_sockfd, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }

private:
    SOCKET _sockfd; //socket fd_set ,file desc set文件描述符集合
    //消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE * 5] = {}; 
    //消息缓冲区数据尾部
    int _lastPos = 0;

};

//网络事件接口
class INetEvent
{
public:
    //客户端加入  
    virtual void OnNetJoin(ClientSocket* pClient) = 0;//纯虚函数
    //客户端离开
    virtual void OnNetLeave(ClientSocket* pClient) = 0;//纯虚函数
    //客户端消息
    virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) = 0;//纯虚函数
     //recv消息
    virtual void OnNetRecv(ClientSocket* pClient) = 0;//纯虚函
};

class CellServer
{
public:
    CellServer(SOCKET sock = INVALID_SOCKET)
    {
        _sock = sock;
        _pNetEvent = nullptr;

    }
    ~CellServer()
    {
        Close();
        _sock = INVALID_SOCKET;

    }


    void setEventObj(INetEvent* pNetEvent)
    {
        _pNetEvent = pNetEvent;
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
            for(auto iter : _clients)
            {
                closesocket(iter.second->sockfd());
                delete iter.second;
            }
            closesocket(_sock);
            
#else
            for(auto iter : _clients)
                {
                    close(iter.second->sockfd());
                    delete iter.second;
                }
            close(_sock);
#endif      
            _clients.clear();
        }
        _sock = INVALID_SOCKET;
    }

    //是否工作中
    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }

    //处理网络消息
    // 备份socket fd_set
    fd_set _fdRead_bak;
    //客户列表是否有变化
    bool _clients_change;
    SOCKET _maxSock;
    bool OnRun()
    {
        while(isRun())
        {
            if(_clientsBuff.size() > 0)
            {
                //从缓冲队列中去除客户数据
                std::lock_guard<std::mutex> lock(_mutex);
                for(auto pClient : _clientsBuff)
                {
                    _clients[pClient->sockfd()]=pClient;
                }
                _clientsBuff.clear();
                _clients_change = true;
            }
            if(_clients.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                continue;
            }
            // 伯克利 socket
            fd_set fdRead;

            FD_ZERO(&fdRead); //清空
            if(_clients_change)
            {
                _clients_change = false;
                _maxSock = _clients.begin()->second->sockfd();

                for(auto iter : _clients)
                {
                    FD_SET(iter.second->sockfd(), &fdRead);
                    if(_maxSock < iter.second->sockfd())
                    {
                        _maxSock = iter.second->sockfd();
                    }
                }
                memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));;
            }
            else
            {
                memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
            }
            
            
            //nfds 是一个整数，描述socket的范围而不是数量
            timeval t = {1, 0};
            int ret = select(_maxSock+1, &fdRead, nullptr, nullptr, nullptr);
            if(ret < 0)
            {
                printf("select 任务结束\n");
                Close();
                return false;
            }
            else if (ret ==0)
            {
                continue;
            }


            std::vector<ClientSocket*> temp;
            for(auto iter : _clients)
            {
                if(FD_ISSET(iter.second->sockfd(), &fdRead))
                {
                    if(-1 == RecvData(iter.second))
                    {
                        if(_pNetEvent)
                            _pNetEvent->OnNetLeave(iter.second);
                        _clients_change = true;
                        temp.push_back(iter.second);
                    }
                }
            }
            for(auto pClient : temp)
            {
                _clients.erase(pClient->sockfd());
                delete pClient;
            }   
        }
    }

    //接受数据 处理粘包 拆分宝
    char _szRecv[RECV_BUFF_SIZE] = {};
    int RecvData(ClientSocket* pClient)
    {
        int nLen = recv(pClient->sockfd(), _szRecv, sizeof(_szRecv), 0);
        _pNetEvent->OnNetRecv(pClient);

        if(nLen <= 0)
        {
            printf("客户端<Socket = %d>已推出， 任务结束。\n", pClient->sockfd());
            return -1;
        }
        memcpy(pClient->msgBuf()+ pClient->getLastPos(), _szRecv, nLen);
        //消息缓冲区的数据尾部位置后移
        pClient->setLastPos(pClient->getLastPos() + nLen);
        //判断消息缓冲区的数据长度是否大于消息头
        while(pClient->getLastPos() >= sizeof(DataHeader))
        {
            //这时就可以知道当前消息体的长度
            DataHeader* header = (DataHeader*)pClient->msgBuf();
            if(pClient->getLastPos() > header->dataLength)
            {
                //剩余未处理消息缓冲区数据的长度
                int nSize = pClient->getLastPos() - header->dataLength;
                //处理网络消息
                OnNetMsg(pClient, header);
                //将剩余未处理消息缓冲区数据前移
                memcpy(pClient->msgBuf(), pClient->msgBuf()+header->dataLength, nSize);
                pClient->setLastPos(nSize);;
            }
            else
            {   
                //剩余数据不够一条完整消息
                break;
            }
            
        }
        return 0;
    }

    //响应网络数据
    virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
    {
        _pNetEvent->OnNetMsg(pClient, header);
        // switch(header->cmd)
        // {
        //     case CMD_LOGIN:
        //     {
        //         Login* login = (Login*)header;
        //         // printf("收到<Socket = %d>命令：CMD_LOGIN 数据长度：%d  userName=%s PassWord=%s\n",
        //         //         cSock, login->dataLength, login->userName, login->passWord);

        //         // 判断用户密码是否正确
        //         LoginResult ret;
        //         pClient->SendData(&ret);
        //     }
        //     break;
        //     case CMD_LOGOUT:
        //     {   
        //         Logout* logout = (Logout*) header;
        //         // printf("收到<Socket = %d>命令：CMD_LOGOUT 数据长度：%d\n",
        //                 // cSock, logout->dataLength);
        //         //判断用户密码是否正确
        //         // LogoutResult ret;
        //         // SendData(cSock, &ret);
        //     }
        //     break;
        //     default:
        //     {
        //         // DataHeader header;
        //         // SendData(cSock, &header);
        //         printf("收到<Socket = %d>未定义消息 数据长度：%d\n",
        //                 pClient->sockfd(), header->dataLength);
                
        //     }
        //     break;
        //     }
    }
    //发送指定socket数据
    int SendData(SOCKET cSock, DataHeader* header)
    {
        if(isRun() && header)
        {
            return send(cSock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }


    void addClient(ClientSocket* pClient)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // _mutex.lock();
        _clientsBuff.push_back(pClient);
        // _mutex.unlock();
    }

    void Start()
    {
        _thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
    }

    size_t getClientCount()
    {
        return _clients.size() + _clientsBuff.size();
    }

private:
    SOCKET _sock;
    //正式客户队列
    std::map<SOCKET, ClientSocket*> _clients;
    //客户缓冲区
    std::vector<ClientSocket*> _clientsBuff;
    //缓冲队列锁
    std::mutex _mutex;
    std::thread _thread;
    INetEvent* _pNetEvent;
};

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
            addClientToCellServer(new ClientSocket(cSock));
            //获取IP地址 inet_ntoa(clientAddr.sin_addr)
        }

        return cSock;
    }

    void addClientToCellServer(ClientSocket* pClient)
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

    virtual void OnNetJoin(ClientSocket* pClient)
    {
        _clientCount++;
    }


    virtual void OnNetLeave(ClientSocket* pClient)
    {
        _clientCount--;
    }

    virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
    {
        _recvCount++;
    }



};

#endif