#include "EasyTcpServer.hpp"


int main()
{
    EasyTcpServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);

    while(server.isRun())
    {
        server.OnRun();
    }
    server.Close();
    printf("以退出。\n");
    getchar();
    return 0;
}