//
// Created by gtm1224 on 11/25/25.
//

#ifndef EE450_SERVERA_H
#define EE450_SERVERA_H
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
struct  Member
{
    int id;
    std::string username;
    std::string password;
};
class serverA
{

};


#endif //EE450_SERVERA_H