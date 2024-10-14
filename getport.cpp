#include "getport.hpp"

// link with Ws2_32.lib
#pragma comment (lib,"Ws2_32.lib")
#pragma comment (lib,"Mswsock.lib")
#pragma comment (lib,"AdvApi32.lib")

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>



int GetFreeUDPPort()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }

    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // Let the OS choose the port

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return -1;
    }

    int addr_len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr*)&addr, &addr_len) == SOCKET_ERROR) {
        std::cerr << "Error getting socket name" << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return -1;
    }

    int freePort = ntohs(addr.sin_port);
    closesocket(sockfd);
    WSACleanup();
    return freePort;
}