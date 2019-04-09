#include "EasyTcpClient.hpp"
#include <thread>

void cmdThread(EasyTcpClient* client)
{
    while(true)
    {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if(0 == strcmp(cmdBuf, "exit"))
        {
            client->Close();
            printf("退出cmdThread\n");
            break;
        }
        else if(0 == strcmp(cmdBuf, "login"))
        {
            Login login;
            strcpy(login.userName, "lyd");
            strcpy(login.passWord, "lyd");
            client->SendData(&login);
        }
        else if(0 == strcmp(cmdBuf, "logout"))
        {
            Logout login;
            strcpy(login.userName, "lyd");
            client->SendData(&login);
        }
        else
        {
            printf("不支持命令");
        }
    }
    
    

}


int main()
{
    EasyTcpClient client;
    // client.initSocket();
    client.Connect("127.0.0.1", 4567);
    
    std::thread t1(cmdThread, &client);
    t1.detach();

    while(client.isRun())
    {
        client.OnRun();
    }
    client.Close();

    printf("已退出。\n");
    getchar();
    return 0;
}