//
// Created by gtm1224 on 11/29/25.
//

#ifndef EE450_SERVERP_H
#define EE450_SERVERP_H
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
#include <iomanip>
struct  Member
{
    int id;
    std::string username;
    std::string password;

};
struct Member_with_price
{
    int id;
    std::string username;
    double price=0.0;
    int space_u=0;
    int space_h=0;
};

class serverP {

};


#endif //EE450_SERVERP_H
