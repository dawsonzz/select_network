#ifdef _WIN32
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

#include <stdio.h>
#include <thread>
#include <vector>

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};
struct DataHeader
{
    short dataLength;
    short cmd;
};
struct Login:public DataHeader
{   
    Login()
    {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char passWord[32];
};
struct LoginResult:public DataHeader
{
    LoginResult()
    {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
    }
    int result;

};

struct Logout:public DataHeader
{
    Logout()
    {
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
    }
    char userName[32];
};

struct LogoutResult:public DataHeader
{
    LogoutResult()
    {
        dataLength = sizeof(LogoutResult);
        cmd = CMD_LOGOUT_RESULT;
        result = 0;
    }
    int result;
};

struct NewUserJoin:public DataHeader
{
    NewUserJoin()
    {
        dataLength = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        sock = 0;
    }
    int sock;
};

std::vector<SOCKET> g_clients;

int processor(SOCKET _cSock)
{

    char szRecv[1024] = {};
    int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if(nLen <= 0)
    {
        printf("客户端<Socket = %d>已推出， 任务结束。\n", _cSock);
        return -1;
    }

    switch(header->cmd)
    {
        case CMD_LOGIN:
        {
            recv(_cSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            Login* login = (Login*)szRecv;
            printf("收到<Socket = %d>命令：CMD_LOGIN 数据长度：%d  userName=%s PassWord=%s\n",
                    _cSock, login->dataLength, login->userName, login->passWord);

            //判断用户密码是否正确
            LoginResult ret;
            send(_cSock, (char*)&ret, sizeof(ret), 0);
        }
        break;
        case CMD_LOGOUT:
        {   
            // Logout logout = {};
            recv(_cSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            Logout* logout = (Logout*) szRecv;
            printf("收到<Socket = %d>命令：CMD_LOGOUT 数据长度：%d\n",
                    _cSock, logout->dataLength);
            //判断用户密码是否正确
            LogoutResult ret;
            send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
        }
        break;
        default:
        {
            DataHeader header = {0, CMD_ERROR};
            // header.cmd = CMD_ERROR;
            // header.dataLength = 0;
            send(_cSock, (char*)&header, sizeof(header), 0);
        }
        break;
    }
    return 0;
}

int main()
{
#ifdef _WIN32
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
#endif
    //1.定义一个socket套接字
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //2.bind 绑定用于接受客户端链接的网络端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);//host to net unsigned short
#ifdef _WIN32
    _sin.sin_addr.S_un.s_addr = inet_addr("127.0.0.1");
    _sin.sin_addr.S_un.s_addr = INADDR_ANY;
#else
    _sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    _sin.sin_addr.s_addr = INADDR_ANY;
#endif

    if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) )
    {
        printf("ERROR, 绑定网络端口失败。。。\n");
    }
    else
    {
        printf("绑定网络端口成功...\n");
    }

    //3 listen 监听网络端口
    if(SOCKET_ERROR == listen(_sock, 5))
    {
        printf("错误， 绑定网络端口失败。。。\n");
    }
    else
    {
        printf("绑定网络端口成功。。。\n");
    }


    

    while(true)
    {   
        // 伯克利 socket
        fd_set fdRead;
        fd_set fdWrite;
        fd_set fdExp;

        FD_ZERO(&fdRead); //清空
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdExp);

        FD_SET(_sock, &fdRead);
        FD_SET(_sock, &fdWrite);
        FD_SET(_sock, &fdExp);
        SOCKET maxSock = _sock;

        for(int64_t n=0; n<g_clients.size(); n++)
        {
           FD_SET(g_clients[n], &fdRead);
           if(maxSock < g_clients[n])
           {
               maxSock = g_clients[n];
           }
        }
        //nfds 是一个整数，描述socket的范围而不是数量
        timeval t = {0, 0};
        int ret = select(maxSock+1, &fdRead, &fdWrite, &fdExp, &t);
        if(ret < 0)
        {
            printf("select 任务结束\n");
            break;
        }

        if(FD_ISSET(_sock, &fdRead))
        {
            FD_CLR(_sock, &fdRead);
                        //4 accept 等待接受的客户端链接
            sockaddr_in clientAddr = {};
            int nAddrLen = sizeof(sockaddr_in);
            SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
            _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
            _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
            if(INVALID_SOCKET == _cSock)
            {
                printf("错误，接收到无效客户端SOCKET。。。\n");
            }
            else
            {
                for(int64_t n=0; n<g_clients.size(); n++)
                {
                    NewUserJoin userJoin;
                    send(g_clients[n], (const char*)&userJoin, sizeof(userJoin), 0);
                }
                printf("新客户端加入：socket = %d, IP = %s\n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
                g_clients.push_back(_cSock);
            }
        }
        for(int n = (int)g_clients.size() - 1; n>=0; n--)
        {
            if(FD_ISSET(g_clients[n], &fdRead))
            {
                if(-1 == processor(g_clients[n]))
                {
                    auto iter = g_clients.begin() + n;
                    if(iter != g_clients.end())
                    {
                        g_clients.erase(iter);
                    }
                }
            }
        }

        
    }    

#ifdef _WIN32
    for(int n=0; n<g_clients.size(); n++)
    {
        closesocket(g_clients[n]);
    }
    //6 关闭套接字closesocket
    WSACleanup();
#else
    for(int n=0; n<g_clients.size(); n++)
        {
            close(g_clients[n]);
        }
#endif
    printf("服务器推出， 任务结束\n");
    getchar();
    return 0;    
}