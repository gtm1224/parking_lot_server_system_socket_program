//
// Created by gtm1224 on 11/29/25.
//

#ifndef EE450_SERVERR_H
#define EE450_SERVERR_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <array>
#include <vector>
#include <algorithm>
struct  Member
{
    int id;
    std::string username;
    std::string password;
};

class serverR {

};


#endif //EE450_SERVERR_H
