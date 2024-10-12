//
//  getport.cpp
//  Test
//
//  Created by Андрей Буренков on 23.06.22.
//

#include "getport.hpp"


int getFreeUDPPort1() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // Let the OS choose the port
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(sockfd);
        return -1;
    }
    
    socklen_t addr_len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr*)&addr, &addr_len) < 0) {
        std::cerr << "Error getting socket name" << std::endl;
        close(sockfd);
        return -1;
    }
    
    int freePort = ntohs(addr.sin_port);
    close(sockfd);
    return freePort;
}
