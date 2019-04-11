#include "EasyTcpClient.hpp"
#include <thread>

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
    //windows中fd_set决定最大数量为63个客户端+1个服务器
    //mac 和 windows 修改宏定义
    //linux中最大数量为1024，且写在内核中无法修改
    const int cCount = 10;
    //TODO: mac中数量超过252连接失败，客户端二次连接程序报错

    EasyTcpClient* client[cCount];
    for(int n=0; n<cCount; n++)
    {
        if(!g_bRun)
        {
            return 0;
        }
        client[n] = new EasyTcpClient();
        // client[n]->Connect("127.0.0.1", 4567);
    }

    for(int n=0; n<cCount; n++)
    {
        if(!g_bRun)
        {
            return 0;
        }
        client[n]->Connect("127.0.0.1", 4567);
    }
    
    
    std::thread t1(cmdThread);
    t1.detach();

    Login login;
    strcpy(login.userName, "lyd");
    strcpy(login.passWord, "lyd");

    while(g_bRun)
    {
        for(int n=0; n<cCount; n++)
        {
            client[n]->SendData(&login);
            // client[n]->OnRun();
        }
    }

    for(int n=0; n<cCount; n++)
    {
        client[n]->Close();
    }

    printf("已退出。\n");
    // getchar();
    return 0;
}