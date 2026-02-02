/*
 * took Beej's simple datagram server as a reference, modified by myself.
 * I modularize some part of original Beej's code, so I can use them in other servers .cpp file
 *
 *
 */


#include "serverA.h"

#define SERVERA_UDPPORT "21514"
#define SERVERM_UDPPORT "24514"
#define MAXBUFLEN 2048
const std::string HOST_IP = "127.0.0.1";
const std::string GUEST = "G";
const std::string MEMBER = "M";
const std::string FAIL_AUTH = "F";
std::unordered_map<std::string, Member> load_members_info(std::string file_path);

void *get_in_addr(struct sockaddr *sa);
std::pair<std::string, std::string> decode_get_username_password(const char buf[]);
bool check_auth_member(std::string username,std::string password, std::unordered_map<std::string, Member> members);
int send_request_udp(const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send);
bool check_auth_guest(std::string username,std::string password);
std::string check_auth_all_kind(std::string username,std::string password, std::unordered_map<std::string, Member> members);
int send_request_udp_with_socket(int sockfd,const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send);
std::string get_userId(std::string username,std::unordered_map<std::string, Member> member_table);

int main() {
    std::cout << "[Server A] Booting up using UDP on port "<<SERVERA_UDPPORT<<".\n";
    std::unordered_map<std::string, Member> members_table = load_members_info("members.txt");
//    std::cout << "--- Member Information ---" << std::endl;
//    // Iterate through the map using a range-based for loop
//    for (const auto &pair: members_table) {
//        // 'pair' is a std::pair<const std::string, Member>
//        // pair.first is the key (username)
//        // pair.second is the value (Member struct)
//        std::cout << "Username (Key): " << pair.first << std::endl;
//        std::cout << "  ID: " << pair.second.id << std::endl;
//        std::cout << "  Password: " << pair.second.password << std::endl;
//        std::cout << "Username: " << pair.second.username << std::endl;
//        std::cout << "--------------------------" << std::endl;
//    }
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //ipv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, SERVERA_UDPPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }


    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) ==-1) {
            perror("listener:socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener:bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener:failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

//    printf("listener:waitingtorecvfrom...\n");
    bool flag = true;
    while (flag){
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

////        printf("listener:gotpacketfrom %s\n",
//               inet_ntop(their_addr.ss_family,
//                         get_in_addr((struct sockaddr *) &their_addr),
//                         s, sizeof s));
//        printf("listener:packetis %d byteslong\n", numbytes);
        buf[numbytes] = '\0';
//        printf("listener:packetcontains \"%s\"\n", buf);
        std::pair<std::string, std::string> username_password_pair = decode_get_username_password(buf);
        printf("[Server A] Received username %s and password ******.\n", username_password_pair.first.c_str());


        std::string info_to_send;
        info_to_send = check_auth_all_kind(username_password_pair.first,username_password_pair.second,members_table);
//        if(info_to_send==MEMBER){
//            std::string userId = get_userId(username_password_pair.first,members_table);
//            info_to_send+=" "+userId;
//        }

        send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,info_to_send);

    }
    close(sockfd);
    return 0;

}


std::unordered_map<std::string, Member> load_members_info(std::string file_path) {
    std::unordered_map<std::string, Member> members_table;
    std::ifstream file;
    file.open(file_path);
    std::string line;
    while (std::getline(file, line)) {

        Member m;
        std::stringstream read_line(line);
        read_line >> m.id >> m.username >> m.password;
        members_table[m.username] = m;
    }

    return members_table;
}


void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

std::pair<std::string, std::string> decode_get_username_password(const char buf[]){
    std::string username;
    std::string password;
    std::stringstream read_buf(buf);
    read_buf>>username;
    read_buf>>password;
    return std::make_pair(username,password);

}

bool check_auth_member(std::string username,std::string password, std::unordered_map<std::string, Member> members){
    if(members.count(username)){
        Member candidate = members[username];
        std::string correct_password = candidate.password;
//        std::cout<<correct_password;
//        std::cout<<password;
        if(correct_password==password){
            return true;
        }else{
            return false;
        }

    }else{
        return false;
    }
}


bool check_auth_guest(std::string username,std::string password){

    if(username=="guest" && password=="123456"){
        return true;
    }
    return false;
}


std::string check_auth_all_kind(std::string username,std::string password, std::unordered_map<std::string, Member> members){
    std::string final_result;
    if(check_auth_guest(username,password)){
        printf("[Server A] Guest has been authenticated.\n");
        final_result = GUEST;

    }
    else if(check_auth_member(username,password,members)){
        printf("[Server A] Member %s has been authenticated.\n",username.c_str());
        final_result = MEMBER;

    }else{
        printf("[Server A] The username %s or password ****** is incorrect.\n",username.c_str());
        final_result = FAIL_AUTH;
    }
    return  final_result;
}

/*
 * I asked chatgpt 5.1 to debug my code, I used the chatgpt 5.1 generated fixed version as a reference here is my prompt:
 * int send_request_udp(std::string host, std::string SERVERPORT,std::string info_to_send ){ int sockfd; struct addrinfo hints, *servinfo, *p; int rv; int numbytes; memset(&hints, 0, sizeof hints); hints.ai_family = AF_INET; // set to AF_INET to use IPv4 hints.ai_socktype = SOCK_DGRAM; rv = getaddrinfo(host, SERVERPORT, &hints, &servinfo); if (rv != 0) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); return 1; } // loop through all the results and make a socket for(p = servinfo; p != NULL; p = p->ai_next) { if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==-1) { perror("talker: socket"); continue; } break; } if (p == NULL) { fprintf(stderr, "talker: failed to create socket\n"); return 2; } do{ numbytes = sendto(sockfd, info_to_send, (int)info_to_send.size(), 0,p->ai_addr, p->ai_addrlen); }while(numbytes==-1); freeaddrinfo(servinfo); printf("talker: sent %d bytes to %s\n", numbytes, host); close(sockfd); return 0; } I made UDP in a function, help me debug and fix, I am trying to send the information through this
 * this function is been used in other server*.cpp (only for test)
 *
 *
 *
 */


int send_request_udp(const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    rv = getaddrinfo(host.c_str(), SERVERPORT.c_str(), &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) ==-1) {
            perror("talker: socket");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }


    do{
        numbytes = sendto(sockfd, info_to_send.c_str(),(int)info_to_send.size(), 0,p->ai_addr, p->ai_addrlen);
    }while(numbytes==-1);

    freeaddrinfo(servinfo);
    printf("talker: sent %d bytes to %s\n", numbytes, host.c_str());
    close(sockfd);
    return 0;

}

/*
 * modified send_request_udp to use the sockfd, so instead of create a new socket for sending information then close
 * I use the preassigned socket to send information, the socket in bind to defined serverA UDP port.
 */

int send_request_udp_with_socket(int sockfd,const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send){

    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    rv = getaddrinfo(host.c_str(), SERVERPORT.c_str(), &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    p = servinfo;
    if (p == NULL) {
        return -1;
    }


    do{
        numbytes = sendto(sockfd, info_to_send.c_str(),(int)info_to_send.size(), 0,p->ai_addr, p->ai_addrlen);
    }while(numbytes==-1);

    freeaddrinfo(servinfo);
//    printf("talker: sent %d bytes to %s\n", numbytes, host.c_str());

    return 1;

}

std::string get_userId(std::string username,std::unordered_map<std::string, Member> member_table){
    int userId = member_table[username].id;
    return std::to_string(userId);
}
