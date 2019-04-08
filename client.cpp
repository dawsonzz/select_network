#include <stdio.h>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
    #include <WinSock2.h>
    #define WIN32_LEAN_AND_MEAN
#else
    #include <unistd.h>  // uninx std
    #include <arpa/inet.h>
    #include <string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR    (-1)   
#endif


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

int processor(SOCKET _cSock)
{

    char szRecv[1024] = {};
    int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if(nLen <= 0)
    {
        printf("与服务器断开连接， 任务结束。\n");
        return -1;
    }

    switch(header->cmd)
    {
        case CMD_LOGIN_RESULT:
        {
            recv(_cSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            LoginResult* login = (LoginResult*)szRecv;
            printf("收到服务端消息 CMD_LOGIN_RESULT 数据长度：%d  ", login->dataLength);
        }
        break;
        case CMD_LOGOUT_RESULT:
        {   
            recv(_cSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            LogoutResult* login = (LogoutResult*)szRecv;
            printf("收到服务端消息 CMD_LOGOUT_RESULT 数据长度：%d  ", login->dataLength);

        }
        break;
        case CMD_NEW_USER_JOIN:
        {
            recv(_cSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            NewUserJoin* login = (NewUserJoin*)szRecv;
            printf("收到服务端消息 CMD_NEW_USER_JOIN 数据长度：%d  ", login->dataLength);

        }
        break;
        default:
        break;
    }
    return 0;
}

bool g_bRun = true;
void cmdThread(SOCKET _sock)
{
    while(true)
    {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if(0 == strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            printf("退出cmdThread\n");
            break;
        }
        else if(0 == strcmp(cmdBuf, "login"))
        {
            Login login;
            strcpy(login.userName, "lyd");
            strcpy(login.passWord, "lyd");
            send(_sock, (const char*)&login, sizeof(login), 0);
        }
        else if(0 == strcmp(cmdBuf, "logout"))
        {
            Logout login;
            strcpy(login.userName, "lyd");
            // strcpy(logi.passWord, "lyd");
            send(_sock, (const char*)&login, sizeof(login), 0);
        }
        else
        {
            printf("不支持命令");
        }
    }
    
    

}


int main()
{
#ifdef _WIN32
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
#endif
    //1 简历一个socket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
    if(INVALID_SOCKET == _sock)
    {
        printf("错误， 建立套接字失败。。。\n");
    }
    else
    {
        printf("建立套接字成功。。。\n");
    }
    
    //2 链接服务器 connect
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);
#ifdef _WIN32
    _sin.sin_addr.S_un.s_addr = inet_addr("127.0.0.1");
#else
    _sin.sin_addr.s_addr = inet_addr("127.0.0.1"); 
#endif
    int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
    if(SOCKET_ERROR == ret)
    {
        printf("错误， 链接服务器失败。。。\n");
    }
    else
    {
        printf("连接服务器成功。。。\n");
    }

    //启动线程
    std::thread t1(cmdThread, _sock);
    t1.detach();//主线程和子线程分离

    while(g_bRun)
    {   
        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(_sock, &fdReads);
        timeval t = {1, 0};
        int ret = select(_sock+1, &fdReads, 0 ,0, &t);
        if(ret < 0)
        {
            printf("select任务结束");
        }
        if(FD_ISSET(_sock, &fdReads))
        {
            FD_CLR(_sock, &fdReads);
            if( -1 == processor(_sock))
            {
                printf("select 任务结束");
                break;
            }
        }

        // printf("空闲时间\n");
       
    }


    //4 关闭套接字 closesocket
    
#ifdef _WIN32
    closesocket(_sock);
    WSACleanup();
#endif
    close(_sock);
    
    printf("客户端推出，任务结束\n");
    getchar();
    return 0;    
}