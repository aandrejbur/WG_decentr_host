//
//  getport.hpp
//  Test
//
//  Created by Андрей Буренков on 23.06.22.
//

#ifndef getport_hpp
#define getport_hpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

uint16_t GetFreeUDPPort();

#endif /* getport_hpp */
