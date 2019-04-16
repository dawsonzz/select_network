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


class MyServer : public EasyTcpServer
{
public:
    virtual void OnNetJoin(ClientSocket* pClient)
    {
        EasyTcpServer::OnNetJoin(pClient);
    }


    virtual void OnNetLeave(ClientSocket* pClient)
    {
        EasyTcpServer::OnNetLeave(pClient);
    }

    virtual void OnNetMsg(CellServer* pCellServer, ClientSocket* pClient, DataHeader* header)
    {
        EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
        switch(header->cmd)
        {
            case CMD_LOGIN:
            {
                Login* login = (Login*)header;
                // printf("收到<Socket = %d>命令：CMD_LOGIN 数据长度：%d  userName=%s PassWord=%s\n",
                //         cSock, login->dataLength, login->userName, login->passWord);

                // 判断用户密码是否正确
                LoginResult* ret = new LoginResult();
                // pClient->SendData(&ret);
                pCellServer->addSendTask(pClient, ret);
            }
            break;
            case CMD_LOGOUT:
            {   
                Logout* logout = (Logout*) header;
                // printf("收到<Socket = %d>命令：CMD_LOGOUT 数据长度：%d\n",
                        // cSock, logout->dataLength);
                //判断用户密码是否正确
                // LogoutResult ret;
                // SendData(cSock, &ret);
            }
            break;
            default:
            {
                // DataHeader header;
                // SendData(cSock, &header);
                printf("收到<Socket = %d>未定义消息 数据长度：%d\n",
                        pClient->sockfd(), header->dataLength);
                
            }
            break;
            }
    }


};

int main()
{
    MyServer server;
    // EasyTcpServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);
    server.Start(4);

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