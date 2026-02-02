/*
*  took beej's simple stream server as a reference, modified by myself.
*  I modularize some part of original Beej's code, so I can use them in other servers .cpp file
*/

#include "serverM.h"
#define  TCPPORT "25514"// the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define SERVERA_UDPPORT "21514"
#define SERVERM_UDPPORT "24514"
#define SERVERP_UDPPORT "23514"
#define SERVERR_UDPPORT "22514"
#define MAXDATASIZE 2048
const std::string HOST_IP = "127.0.0.1";
const std::string GUEST = "G";
const std::string MEMBER = "M";
const std::string FAIL_AUTH = "F";
// reap dead child processes
void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    int saved_errno = errno;      // save errno

    // Reap all dead children without blocking
    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;          // restore errno
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {   // IPv4
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    // else IPv6
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int send_request_udp(const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send);
int bind_socket(const std::string &UDPPORT);
std::string receive_from(int sockfd);
int send_request_udp_with_socket(int sockfd,const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send);
std::string get_username_from_buf(const std::string &buf);
void parse_command_serverM(const std::string &user_input,std::string username,int udp_socket, int tcp_socket);
void parse_reserve_feedback(const std::string feedback,int udp_socket, int tcp_socket);
std::string receive_from_TCP(int sockfd);
int main(void)
{
    int sockfd, new_fd, numbytes;                 // listen socket, new connection socket
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;   // connector's address info
    socklen_t sin_size;
    struct sigaction sa;
    char buf[MAXDATASIZE];
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;




    // 1. Fill in hints for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;          // IPv4 only (AF_UNSPEC = v4 or v6)
    hints.ai_socktype = SOCK_STREAM;      // TCP
    hints.ai_flags    = AI_PASSIVE;       // use my IP (for bind)

    // 2. Get a list of suitable local addresses to bind to
    if ((rv = getaddrinfo(NULL, TCPPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // 3. Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        // create socket with this candidate's family/type/protocol
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // allow reuse of the port quickly after restart
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // try to bind this socket to the address
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        // bind succeeded
        break;
    }

    freeaddrinfo(servinfo); // done with this list

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    // 4. Start listening
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // 5. Set up SIGCHLD handler to reap children
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }


    //start a socket and bind to ServerM UDP Port:
    int serverM_UDP_socket = bind_socket(SERVERM_UDPPORT);
    std::cout << "[Server M] Booting up using UDP on port " << SERVERM_UDPPORT << ". \n";

    // 6. Main accept() loop
    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        // Print where the connection came from, for test only, need comment out later
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
//        printf("server: got connection from %s\n", s);

        // 7. Fork a child to handle this client
        if (!fork()) {  // child process
            close(sockfd);  // child doesn't need the listening socket

            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("server: recv");
                close(new_fd);
                exit(1);
            }
            buf[numbytes] = '\0';
            std::string username = get_username_from_buf(buf);
            printf("Server M received username %s and password ****** \n", username.c_str());  // this should be "username encrypted_password"

            // try to send the authentication to serverA:
            send_request_udp(HOST_IP,SERVERA_UDPPORT,buf);
            std::cout<< "Server M sent the authentication request to Server A. \n";

            // check the received auth from the serverA
            std::string received_auth = receive_from(serverM_UDP_socket);
//            std::cout<<received_auth<<std::endl;
            std::cout<<"Server M received the response from Server A using UDP over port "<<SERVERM_UDPPORT<<".\n";

            const char *auth_to_client = received_auth.c_str();
            if (send(new_fd, auth_to_client, (int)strlen(auth_to_client), 0) == -1) {
                perror("server: send");
            }
            std::cout<<"Server M sent the response to the client using TCP over port "<<TCPPORT<<".\n";
            if(received_auth==FAIL_AUTH){
                close(new_fd);
                exit(0);
            }

            while(true){
                int numbytes_received = recv(new_fd, buf, MAXDATASIZE-1, 0);
                if (numbytes_received== -1) {
                    perror("server: recv");
                    break;
                }
                if(numbytes_received == 0){
                    std::cout<<username<<" quit.\n";
                    break;
                }

                buf[numbytes_received] = '\0';
//                printf("Server M received %s \n", buf);
//                std::cout<<buf<<std::endl;
                parse_command_serverM(buf,username,serverM_UDP_socket,new_fd);




            }


            close(new_fd);
            exit(0);
        }


        // parent process
        close(new_fd);  // parent doesn't need this connected socket
    }

    return 0;
}




/*
 * I asked chatgpt 5.1 to debug my code, I used the chatgpt 5.1 generated fixed version as a reference here is my prompt:
 * int send_request_udp(std::string host, std::string SERVERPORT,std::string info_to_send ){ int sockfd; struct addrinfo hints, *servinfo, *p; int rv; int numbytes; memset(&hints, 0, sizeof hints); hints.ai_family = AF_INET; // set to AF_INET to use IPv4 hints.ai_socktype = SOCK_DGRAM; rv = getaddrinfo(host, SERVERPORT, &hints, &servinfo); if (rv != 0) { fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); return 1; } // loop through all the results and make a socket for(p = servinfo; p != NULL; p = p->ai_next) { if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==-1) { perror("talker: socket"); continue; } break; } if (p == NULL) { fprintf(stderr, "talker: failed to create socket\n"); return 2; } do{ numbytes = sendto(sockfd, info_to_send, (int)info_to_send.size(), 0,p->ai_addr, p->ai_addrlen); }while(numbytes==-1); freeaddrinfo(servinfo); printf("talker: sent %d bytes to %s\n", numbytes, host); close(sockfd); return 0; } I made UDP in a function, help me debug and fix, I am trying to send the information through this
 *
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
//    printf("talker: sent %d bytes to %s\n", numbytes, host.c_str());
    close(sockfd);
    return 0;

}

/*
 * I created this function using the code from Beej's simple UDP server, this function returns a socket descriptor that is bind
 */

int bind_socket(const std::string &UDPPORT){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //ipv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, UDPPORT.c_str(), &hints, &servinfo)) != 0) {
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
    return sockfd;
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

std::string receive_from(int sockfd){
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXDATASIZE];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                             (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        return "";
    }

//    printf("listener:gotpacketfrom %s\n",
//           inet_ntop(their_addr.ss_family,
//                     get_in_addr((struct sockaddr *) &their_addr),
//                     s, sizeof s));
//    printf("listener:packetis %d byteslong\n", numbytes);
    buf[numbytes] = '\0';
//    printf("listener:packetcontains \"%s\"\n", buf);
    return std::string(buf);
}

std::string get_username_from_buf(const std::string &buf){
    std::stringstream read_buf(buf);
    std::string username;
    read_buf>>username;

    return username;

}

void parse_command_serverM(const std::string &user_input,std::string username,int udp_socket, int tcp_socket){
    std::string first_word;
    std::stringstream ss(user_input);
    std::string print_username;
    ss>>first_word;
    if (username=="guest"){
        print_username = "Guest";
    }else{
        print_username = username;
    }

    if(first_word=="search"){
        std::string parking_lot_name;
        ss>>parking_lot_name;
        if(parking_lot_name.size()==0){
            parking_lot_name = "UPC and HSC";
        }
        std::cout<<"Server M received an availability request from "<< print_username <<" for "<<parking_lot_name
        <<" using TCP over port"<<TCPPORT<< ".\n";
        std::string string_to_send =username +" " +user_input;
        send_request_udp(HOST_IP,SERVERR_UDPPORT,string_to_send);
        std::cout<<"Server M sent the availability request to Server R. \n";
        std::string available_parking_slots = receive_from(udp_socket);
        std::cout<<"Server M received the response from Server R using UDP over port "<<SERVERM_UDPPORT<<".\n";
        //send the parking lots and time slots using tcp:
        if (send(tcp_socket, available_parking_slots.c_str(),(int)strlen(available_parking_slots.c_str()), 0) == -1) {
            perror("server: send");
        }
        std::cout<<"Server M sent the availability information to the client.\n";
    }else if(first_word=="reserve"){
        std::cout<<"Server M received a reservation request from <"<<username<<"> using TCP over port "<<TCPPORT<<"."<<std::endl;
        std::string string_to_send = username+" "+user_input;
        std::cout<<string_to_send<<std::endl;
        std::cout<<"Server M sent the reservation request to Server R.\n";
        send_request_udp(HOST_IP,SERVERR_UDPPORT,string_to_send);
        std::string reserve_feedback = receive_from(udp_socket);
//        std::cout<<"reserve_feedback_firstloop: "<<reserve_feedback<<std::endl;
        parse_reserve_feedback(reserve_feedback,udp_socket,tcp_socket);

    }else if(first_word=="lookup"){
        std::cout<<"Server M received a lookup request from "<<username<<" using TCP over port "<<TCPPORT<<".\n";

        std::string string_to_send = username+" "+user_input;
        send_request_udp_with_socket(udp_socket,HOST_IP,SERVERR_UDPPORT,string_to_send);
        std::cout<<"Server M sent the lookup request to Server R.\n";
        std::string res = receive_from(udp_socket);
        std::cout<<"Server M received the response from Server R using UDP over port "<<SERVERM_UDPPORT<<".\n";
        if(res.size()==0){
            res = "\n";
        }
        if (send(tcp_socket, res.c_str(),(int)strlen(res.c_str()), 0) == -1) {
            perror("server: send");
        }
        std::cout<<"Server M sent the lookup result to the client. \n";

    }else if(first_word=="cancel"){
        std::cout<<"Server M received a cancellation request from "<<username<<" using TCP over port "<<TCPPORT<<".\n";
        std::cout<<"Server M sent the cancellation request to Server R.\n";
        std::string string_to_send = username+" "+user_input;
        send_request_udp_with_socket(udp_socket,HOST_IP,SERVERR_UDPPORT,string_to_send);
        std::string feedback = receive_from(udp_socket);
        std::cout<<"Server M received the response from Server R using UDP over port "<<SERVERM_UDPPORT<<".\n";
        std::stringstream ss(feedback);
        std::string first_word;
        ss>>first_word;
        if(first_word=="can"){
            send_request_udp_with_socket(udp_socket,HOST_IP,SERVERP_UDPPORT,feedback);
            std::cout<<"Server M sent the refund request to Server P .\n";
            std::string cost = receive_from(udp_socket);
            std::cout<< "Server M received the refund information from Server P using UDP over port " <<SERVERM_UDPPORT<<".\n";
            std::string feedback_to_client = feedback+" cost "+cost;
            if (send(tcp_socket, feedback_to_client.c_str(),(int)strlen(feedback_to_client.c_str()), 0) == -1) {
                perror("server: send");
            }

        }else if(first_word=="fail"){
            if (send(tcp_socket, feedback.c_str(),(int)strlen(feedback.c_str()), 0) == -1) {
                perror("server: send");
            }
        }

    }

}

void parse_reserve_feedback(const std::string feedback,int udp_socket, int tcp_socket){
    std::string first_feedback;
    std::string username;
    std::string parking_lot;
    std::stringstream ss(feedback);
    ss>>first_feedback;
    ss>>username;
    ss>>parking_lot;
    if(first_feedback=="all"){
        //send feedback TCP:
        std::string time_slots;
        std::getline(ss >> std::ws, time_slots);

        send_request_udp_with_socket(udp_socket,HOST_IP,SERVERP_UDPPORT,feedback);
        std::cout<<"Server M sent the pricing request to Server P.\n";
        std::string cost = receive_from(udp_socket);
        std::cout<<"Server M received the pricing information from Server P using UDP over port "<<SERVERM_UDPPORT<<".\n";
        std::string feedback_to_client = feedback+" cost "+cost;

        if (send(tcp_socket, feedback_to_client.c_str(),(int)strlen(feedback_to_client.c_str()), 0) == -1) {
            perror("server: send");
        }
        std::cout<<"Server M sent the reservation result to the client.\n";

    }else if(first_feedback=="fail"){
        if (send(tcp_socket, feedback.c_str(),(int)strlen(feedback.c_str()), 0) == -1) {
            perror("server: send");
        }


    }else if(first_feedback=="partial"){
        std::cout<<"Server M received the response from Server R using UDP over port "<<SERVERM_UDPPORT<<std::endl;
        std::cout<<"Server M sent the partial reservation confirmation request to the client.\n";
        if (send(tcp_socket, feedback.c_str(),(int)strlen(feedback.c_str()), 0) == -1) {
            perror("server: send");
        }
        //must use tcp
        std::string user_yes_or_no_response = receive_from_TCP(tcp_socket);
        std::cout<<"Server M sent the confirmation response to Server R.\n";
        send_request_udp_with_socket(udp_socket,HOST_IP,SERVERR_UDPPORT,user_yes_or_no_response);
        if(user_yes_or_no_response[0]=='Y' ||user_yes_or_no_response[0]=='y'){

        }
        std::string final_confirm = receive_from(udp_socket);
//        std::cout<<"Server M sent the reservation result to the client.\n";
        parse_reserve_feedback(final_confirm,udp_socket,tcp_socket);

    }

}

std::string receive_from_TCP(int sockfd){
    char buf[MAXDATASIZE];
    int numbytes_received = recv(sockfd, buf, MAXDATASIZE-1, 0);
    buf[numbytes_received] = '\0';
    return buf;
}