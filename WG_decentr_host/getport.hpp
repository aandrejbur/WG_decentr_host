//
//  getport.hpp
//  Test
//
//  Created by Андрей Буренков on 23.06.22.
//

#ifndef getport_hpp
#define getport_hpp

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int getFreeUDPPort1();
#endif /* getport_hpp */
