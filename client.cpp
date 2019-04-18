#include "EasyTcpClient.hpp"
#include "CELLTimestamp.hpp"
#include <thread>
#include <chrono>
#include <atomic>

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

//windows中fd_set决定最大数量为63个客户端+1个服务器
//mac 和 windows 修改宏定义
//linux中最大数量为1024，且写在内核中无法修改
const int cCount = 8;
//TODO: mac中数量超过252连接失败，客户端二次连接程序报错

//线程数量
const int tCount = 4;

EasyTcpClient* client[cCount];
std::atomic_int sendCount;
std::atomic_int readyCount;
void sendThread(int id) //四个线程 ID1～4
{
    printf("thread<%d>, start\n", id);
    int c = (cCount / tCount);
    int begin = (id - 1) * c;
    int end = id * c;
    for(int n=begin; n<end; n++)
    {
        client[n] = new EasyTcpClient();
        // client[n]->Connect("127.0.0.1", 4567);
    }

    for(int n=begin; n<end; n++)
    {
        client[n]->Connect("127.0.0.1", 4567);
    }

    printf("thread<%d>, Connect<begin=%d, end=%d>\n", id, begin, end);
    
    readyCount++;
    while( readyCount < tCount)
    {
        //等待其他线程准备好发送数据
        std::chrono::milliseconds t(10);
        std::this_thread::sleep_for(t);
    }

    
    netmsg_Login login;
    strcpy(login.userName, "lyd");
    strcpy(login.passWord, "lyd");

    while(g_bRun)
    {
        for(int n=begin; n<end; n++)
        {
            if(SOCKET_ERROR != client[n]->SendData(&login, sizeof(login)))
            {
                sendCount++;
            }
            client[n]->OnRun();
        }
    }

    for(int n=begin; n<end; n++)
    {
        client[n]->Close();
        delete client[n];
    }

    printf("thread<%d>, exit<begin=%d, end=%d>\n", id, begin, end);

   
}

int main()
{


    std::thread t(cmdThread);
    t.detach(); 


    //启动发送线程
    for(int n=0; n<tCount; n++)
    {
        std::thread t1(sendThread, n+1);
        t1.detach();
    }

    CELLTimestamp tTime;

    while(g_bRun)
    {
        auto t = tTime.getElapsedSecond();
        if(t>=1.0)
        {
            printf("thread<%d>, clients<%d>, time<%lf>, send<%d>\n",
                tCount, cCount, t, (int)(sendCount/t));
            sendCount = 0;
            tTime.update();
        }
        sleep(1);
    }

    printf("已退出。\n");
    return 0;
}