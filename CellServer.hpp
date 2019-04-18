#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include "Cell.hpp"
#include "INetEvent.hpp"
#include "CellClient.hpp"

#include <vector>
#include <map>

class CellsendMsg2ClientTask:public CellTask
{
    CellClient* _pClient;
    netmsg_DataHeader* _pHeader;

public:
    CellsendMsg2ClientTask(CellClient* pClient,netmsg_DataHeader* header)
    {
        _pClient = pClient;
        _pHeader = header;
    }

    void doTask()
    {
        _pClient->SendData(_pHeader);
        delete _pHeader;
    }
};
//网络数据接受服务类
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
    void  OnRun()
    {
        while(isRun())
        {
            if(!_clientsBuff.empty())
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
            timeval t = {0, 10};
            int ret = select(_maxSock+1, &fdRead, nullptr, nullptr, nullptr);
            if(ret < 0)
            {
                printf("select 任务结束\n");
                Close();
                return;
            }
            else if (ret ==0)
            {
                continue;
            }


            std::vector<CellClient*> temp;
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
    int RecvData(CellClient* pClient)
    {
        char* szRecv = pClient->msgBuf() + pClient->getLastPos();
        int nLen = recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE - pClient->getLastPos(), 0);
        _pNetEvent->OnNetRecv(pClient);

        if(nLen <= 0)
        {
            printf("客户端<Socket = %d>已推出， 任务结束。\n", pClient->sockfd());
            return -1;
        }
        // memcpy(pClient->msgBuf()+ pClient->getLastPos(), _szRecv, nLen);
        //消息缓冲区的数据尾部位置后移
        pClient->setLastPos(pClient->getLastPos() + nLen);
        //判断消息缓冲区的数据长度是否大于消息头
        while(pClient->getLastPos() >= sizeof(netmsg_DataHeader))
        {
            //这时就可以知道当前消息体的长度
            netmsg_DataHeader* header = (netmsg_DataHeader*)pClient->msgBuf();
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
    virtual void OnNetMsg(CellClient* pClient, netmsg_DataHeader* header)
    {
        _pNetEvent->OnNetMsg(this, pClient, header);
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
        //         // netmsg_DataHeader header;
        //         // SendData(cSock, &header);
        //         printf("收到<Socket = %d>未定义消息 数据长度：%d\n",
        //                 pClient->sockfd(), header->dataLength);
                
        //     }
        //     break;
        //     }
    }
    //发送指定socket数据
    int SendData(SOCKET cSock, netmsg_DataHeader* header)
    {
        if(isRun() && header)
        {
            return send(cSock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }


    void addClient(CellClient* pClient)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // _mutex.lock();
        _clientsBuff.push_back(pClient);
        // _mutex.unlock();
    }

    void Start()
    {
        _thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
        _taskServer.Start();
    }

    size_t getClientCount()
    {
        return _clients.size() + _clientsBuff.size();
    }

    void addSendTask(CellClient* pClient, netmsg_DataHeader* header)
    {
        CellsendMsg2ClientTask* task = new CellsendMsg2ClientTask(pClient, header);
        _taskServer.addTask(task);
    }

private:
    SOCKET _sock;
    //正式客户队列
    std::map<SOCKET, CellClient*> _clients;
    //客户缓冲区
    std::vector<CellClient*> _clientsBuff;
    //缓冲队列锁
    std::mutex _mutex;
    std::thread _thread;
    INetEvent* _pNetEvent;
    CellTaskServer _taskServer;
};

#endif