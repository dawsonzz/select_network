#include "EasyTcpServer.hpp"

bool g_bRun = true;
void cmdThread()
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
        else
        {
            printf("不支持命令");
        }
    }
}

int main()
{
    EasyTcpServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);
    server.Start();

    std::thread t1(cmdThread);
    t1.detach();

    while(g_bRun)
    {
        server.OnRun();
    }
    server.Close();
    printf("已退出。\n");
    getchar();
    return 0;
}