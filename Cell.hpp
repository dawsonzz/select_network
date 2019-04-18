#ifndef _CELL_HPP_
#define _CELL_HPP_

//SOCKET
#ifdef _WIN32
    #define FD_SETSIZE 64
    #include <windows.h>
    #include <WinSock2.h>
    #pragma comment(lib, "ws2_32.lib");
    #define WIN32_LEAN_AND_MEAN
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
#else
    #include <unistd.h>  // uninx std
    #include <arpa/inet.h>
    #include <string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR    (-1)   
#endif


#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"

#include <stdio.h>

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE  10240*5
#define SEND_BUFF_SIZE  RECV_BUFF_SIZE
#endif


#endif