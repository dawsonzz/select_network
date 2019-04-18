#ifndef _CellClient_hpp_ 
#define _CellClient_hpp_

#include "Cell.hpp"

//客户端心跳检测死亡计时时间
#define CLIENT_HEART_DEAD_TIME 5000

class CellClient
{
public:
    CellClient(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
        _lastPos = 0;
        memset(_szSendBuf, 0, sizeof(_szSendBuf));
        _lastSendPos = 0;
        resetDtHeart();
    }

    SOCKET sockfd()
    {
        return _sockfd;
    }

    char* msgBuf()
    {
        return _szMsgBuf;
    }

    int getLastPos()
    {
        return _lastPos;
    }

    void setLastPos(int pos)
    {
        _lastPos = pos;
    }
    //发送数据
    int SendData(netmsg_DataHeader* header)
    {
        int ret = SOCKET_ERROR;
        int nSendLen = header->dataLength;
        const char* pSendData = (const char*)header;
        while(true)
        {
             if(_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
            {
                //计算可拷贝数据长度
                int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
                //拷贝数据
                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
                //计算剩余数据位置
                pSendData += nCopyLen;
                //计算剩余数据长度
                nSendLen  -= nCopyLen;
                ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
                _lastSendPos = 0;

                //运行错误，一般是断开连接
                if(SOCKET_ERROR == ret)
                {
                    return ret;
                }
            }
            else
            {
                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
                _lastSendPos += nSendLen;
                break;
            }

        }
       
        

        return ret;
    }

    void resetDtHeart()
    {   
        _dtHeart = 0;
    }

    bool checkHeart(time_t dt)
    {
        _dtHeart += dt;
        if(_dtHeart >= CLIENT_HEART_DEAD_TIME)   
        {
            printf("checkHeart dead:%d, time=%d\n", _sockfd, _dtHeart);
            return true;
        }
        return false;
    }
private:
    SOCKET _sockfd; //socket fd_set ,file desc set文件描述符集合
    //消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE]; 
    //消息缓冲区数据尾部
    int _lastPos;

    //发送缓冲区
    char _szSendBuf[SEND_BUFF_SIZE];
    int _lastSendPos;

    //心跳死亡计时
    time_t _dtHeart;

};


#endif 
