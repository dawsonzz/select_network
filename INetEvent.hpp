#ifndef _INetEvent_HPP_
#define _INetEvent_HPP_

#include "Cell.hpp"
#include "CellClient.hpp"

class CellServer;

//网络事件接口
class INetEvent
{
public:
    //客户端加入  
    virtual void OnNetJoin(CellClient* pClient) = 0;//纯虚函数
    //客户端离开
    virtual void OnNetLeave(CellClient* pClient) = 0;//纯虚函数
    //客户端消息
    virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header) = 0;//纯虚函数
    //recv消息
    virtual void OnNetRecv(CellClient* pClient) = 0;//纯虚函
};

#endif