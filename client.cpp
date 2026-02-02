//
// took beej's simple stream client as a reference, modified by myself.
//

#include "client.h"
#define PORT "25514" // the port client will be connecting to
#define MAXDATASIZE 2048
const std::string GUEST = "G";
const std::string MEMBER = "M";
const std::string FAIL_AUTH = "F";
const std::string HOST_IP = "127.0.0.1";
void *get_in_addr(struct sockaddr *sa);
std::string encryption_password(std::string password);
int tcp_connect(const std::string &host,const std::string &port);
int identify_credentials(std::string received_auth,std::string username);
void display_help_guest();
void display_help_member();
void display_help(int credentials);
void send_command(int tcp_scoket,std::string send_string);
void parse_command_client(const std::string &user_input,int credential,int tcp_socket,std::string username);
std::string receive_tcp(int tcp_socket);
void parse_reserve_feedback_client(std::string feedback,int tcp_socket,std::string username);
int get_my_tcp_port(int TCP_Connect_Sock);
int main(int argc, char *argv[])
{
    //boot up message:
    std::cout<<"Client is up and running. "<<std::endl;


    int numbytes;
    char buf[MAXDATASIZE];
    int credentials;


    if (argc != 3) {
        fprintf(stderr,"Authentication failed: username or password is incorrect. \n");
        exit(1);

    }
    std::string send_string;
    std::string username = argv[1];
    std::string password = argv[2];
    std::string encrypted_password = encryption_password(password);
    if (username == "guest"){
        send_string= username + " " + password;
    }
    else{
        send_string = username + " " + encrypted_password;
    }

//    std::cout<<send_string<<std::endl;
//
//    std::cout<<username<<std::endl;
//    std::cout<<password<<std::endl;

    int tcp_scoket = tcp_connect(HOST_IP,PORT);
    int success = -1;
    while(success==-1){
            success = send(tcp_scoket, send_string.c_str(),(int)send_string.size(), 0);
        }

    std::cout<<username<<" sent an authentication request to the main server. \n";
    if ((numbytes = recv(tcp_scoket, buf, MAXDATASIZE-1, 0)) ==-1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';
   // printf("client: received '%s'\n",buf);
//    close(tcp_scoket);
    std::string received_auth = buf;

    credentials = identify_credentials(received_auth,username);

    std::string commands;
    bool flag = true;
    while(flag){
        std::getline(std::cin, commands);
        if(commands=="quit"){
            flag =false;
        }
        else if(commands.empty()){
            continue;
        }else{
            parse_command_client(commands,credentials,tcp_scoket,username);
        }


    }
    close(tcp_scoket);
    return 0;
}

// get sockaddr, IPv4 or IPv6, took from Beej
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//I asked chatgpt 5.1 to check  for this function, I made some modification, mostly my work for this function.
std::string encryption_password(std::string password){

    std::string encrypted = "";

    for(char c : password){

        // Uppercase A–Z
        if(c >= 'A' && c <= 'Z'){
            char new_c = c + 3;
            if(new_c > 'Z'){
                new_c = 'A' + (new_c - 'Z' - 1);
            }
            encrypted += new_c;
        }

            // Lowercase a–z
        else if(c >= 'a' && c <= 'z'){
            char new_c = c + 3;
            if(new_c > 'z'){
                new_c = 'a' + (new_c - 'z' - 1);
            }
            encrypted += new_c;
        }

        else if(c >= '0' && c <= '9'){
            char new_c = c + 3;
            if(new_c > '9'){
                new_c = '0' + (new_c - '9' - 1);
            }
            encrypted += new_c;
        }
        else{
            encrypted += c;
        }
    }

    return encrypted;
    }

//modularize the TCP socket bind, listen, connect

int tcp_connect(const std::string &host,const std::string &port){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) ==-1) {
            perror("client: socket");
            continue;
        }
        inet_ntop(p->ai_family,
                  get_in_addr((struct sockaddr *)p->ai_addr),
                  s, sizeof s);
//        printf("client: attempting connection to %s\n", s);
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) ==-1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }
    inet_ntop(p->ai_family,
              get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    //printf("client: connected to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

    return sockfd;
}

int identify_credentials(std::string received_auth,std::string username){
    if(received_auth==GUEST){
        std::cout<<"You have been granted guest access."<<std::endl;
        return 0;
    }else if(received_auth==MEMBER){
        std::cout<<username<<" received the authentication result."<<std::endl;
        std::cout<<"Authentication successful."<<std::endl;
        return 1;

    }else{
        std::cout<<"Authentication failed: username or password is incorrect. "<<std::endl;
        exit(-1);
    }
}

void display_help(int credentials){
    if(credentials==1){
        display_help_member();
    }else if(credentials==0){
        display_help_guest();
    }
}


void display_help_guest(){
    std::cout<<"Please enter the command: <search <parking lot>>, <quit> "<<std::endl;
}

void display_help_member(){
    std::cout<<"Please enter the command: <search <parking lot>>, <reserve <space> <timeslots>>, <lookup>, <cancel <space> <timeslots>>, <quit> "<<std::endl;
}

void parse_command_client(const std::string &user_input,int credential,int tcp_socket,std::string username){
    std::string first_word;
    std::stringstream ss(user_input);
    std::string print_username;
    int dynamic_tcp_port =  get_my_tcp_port(tcp_socket);
    ss>>first_word;
    if(first_word=="help"){
        display_help(credential);
    }else if(first_word=="search"){
        if(username=="guest"){
            std::cout<<"Guest sent an availability request to the main server. "<<std::endl;
        }else{
            printf("%s sent an availability request to the main server. \n",username.c_str());
        }
        send_command(tcp_socket,user_input);
        std::string parking_info = receive_tcp(tcp_socket);
        std::cout<<"The client received the response from the main server using TCP over port "<<dynamic_tcp_port<< "."<<std::endl;
        std::cout<<parking_info<<std::endl;
        std::cout<<"---Start a new request--- "<<std::endl;
    }else if(first_word == "reserve"){
        if(username=="guest"){
            std::cout<<"Guests can only check availability. Please log in as a member for full access. \n"
                     <<"---Start a new request--- \n"
                     <<std::endl;
        }else{
            std::string parking_lot;
            std::string time_slots;
            ss>>parking_lot;
            bool flag = true;
            if(parking_lot.size()==0 || (parking_lot[0]!='U' && parking_lot[0]!='H') ){
                std::cout<<"Error: Space code and timeslot(s) are required. Please specify a space code and at least one timeslot. \n";
                flag = false;
            }else{
                ss>>time_slots;
                if(time_slots.size()==0){
                    std::cout<<"Error: Space code and timeslot(s) are required. Please specify a space code and at least one timeslot. \n";
                    flag = false;
                }
            }
            if(flag){

                printf("%s sent a reservation request to the main server. \n",username.c_str());
                send_command(tcp_socket,user_input);
                std::string feedback = receive_tcp(tcp_socket);
//                std::cout<<feedback<<std::endl;
                parse_reserve_feedback_client(feedback,tcp_socket,username);
            }
        }

    }else if(first_word=="lookup"){
        if(username=="guest"){
            std::cout<<"Guests can only check availability. Please log in as a member for full access. \n"
                     <<"---Start a new request--- \n"
                     <<std::endl;

        }else{
            std::string send_string = "lookup";
            std::cout<<username<<" sent a lookup request to the main server.\n";
            send_command(tcp_socket,send_string);
            std::string lookup_feedback=receive_tcp(tcp_socket);
            std::cout<<" The client received the response from the main server using TCP over port number "<<dynamic_tcp_port<<std::endl;
            if(lookup_feedback.size()<=1){
                std::cout<<"You have no current reservations.\n";
                std::cout<<"---Start a new request--- \n";
            }else{
                std::cout<<"Your reservations: \n";
                std::cout<<lookup_feedback<<std::endl;
                std::cout<<"---Start a new request--- \n";
            }


        }

    }else if(first_word=="cancel"){
        if(username=="guest"){
            std::cout<<"Guests can only check availability. Please log in as a member for full access. \n"
                     <<"---Start a new request--- \n"
                     <<std::endl;
        }else{
            std::string parking_lot;
            std::string time_slots;
            ss>>parking_lot;
            bool flag = true;
            if(parking_lot.size()==0 || (parking_lot[0]!='U' && parking_lot[0]!='H') ){
                std::cout<<"Error: Space code and timeslot(s) are required. Please specify what to cancel. \n";
                flag = false;
            }else{
                ss>>time_slots;
                if(time_slots.size()==0){
                    std::cout<<"Error: Space code and timeslot(s) are required. Please specify what to cancel. \n";;
                    flag = false;
                }
            }
            if(flag){
                printf("%s sent a cancellation request to the main server. \n",username.c_str());
                send_command(tcp_socket,user_input);
                std::string feedback = receive_tcp(tcp_socket);
                std::stringstream ss(feedback);
                std::string first_word;
                ss>>first_word;
                std::cout<<"The client received the response from the main server using TCP over port "<<dynamic_tcp_port<<" .\n";
                if(first_word=="can"){
                    std::string feedback_username;
                    std::string feedback_parking_lot;

                    ss>>feedback_username>>feedback_parking_lot;
                    std::string feedback_time_slots = "";
                    std::string time_slot;
                    while(ss>>time_slot){
                        if(time_slot=="cost"){
                            break;
                        }else{
                            feedback_time_slots+=time_slot+" ";
                        }
                    }
                    std::string cost;
                    ss>>cost;
                    std::cout<<"Cancellation successful for "<<parking_lot<<" at time slot(s) "<<feedback_time_slots<<"."<<std::endl;
                    std::cout<<"Refund amount: $"<<cost<<std::endl;
                    std::cout<<"---Start a new request--- \n";

                }else if(first_word == "fail"){
                    std::cout<<"Cancellation failed: You do not have reservations for the specified slots.\n";
                    std::cout<<"---Start a new request--- \n";
                }
            }
        }
    }

}

void send_command(int tcp_scoket,std::string send_string){
    int success = -1;
    while(success==-1){
        success = send(tcp_scoket, send_string.c_str(),(int)send_string.size(), 0);
    }
}
std::string receive_tcp(int tcp_socket){
    int numbytes;
    char buf[MAXDATASIZE];
    if ((numbytes = recv(tcp_socket,buf, MAXDATASIZE-1, 0)) ==-1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    std::string res = buf;
    return res;
}
void parse_reserve_feedback_client(std::string feedback,int tcp_socket,std::string username){
    std::string first_feedback;
    std::string username_from_feedback;
    std::string parking_lot;
    std::stringstream ss(feedback);
    ss>>first_feedback;
    ss>>username_from_feedback;
    ss>>parking_lot;
    int dynamic_tcp_socket = get_my_tcp_port(tcp_socket);
    if(first_feedback=="all"){
        //send feedback TCP:
        std::string time_slots = "";
        std::string time_slot;
        while(ss>>time_slot){
            if(time_slot=="cost"){
                break;
            }else{
                time_slots+=time_slot+" ";
            }
        }
        std::string cost;
        ss>>cost;

        std::string feedback_to_client = feedback+"cost "+cost;
        std::cout<<"The client received the response from the main server using TCP over port "<<dynamic_tcp_socket<<".\n";
        std::cout<<"Reservation successful for "+ parking_lot +" at time slot(s) "+time_slots+".\n";
        std::cout<<"Total cost: $"<<cost<<std::endl;
        std::cout<<"---Start a new request--- \n";

    }else if(first_feedback=="fail"){
        std::cout<<"The client received the response from the main server using TCP over port "<<dynamic_tcp_socket<<".\n";
        std::cout<<"Reservation failed. No slots were reserved.\n";
        std::cout<<"---Start a new request--- \n";

    }else if(first_feedback=="partial"){
        std::string time_slots;
        std::getline(ss >> std::ws, time_slots);
        std::cout<<"Time slot(s) " <<time_slots<< "not available. Do you want to reserve there maining slots? (Y/N):\n";
        std::string yes_or_no;
        std::cin>>yes_or_no;


        if (send(tcp_socket, yes_or_no.c_str(),(int)strlen(yes_or_no.c_str()), 0) == -1) {
            perror("server: send");
        }
        std::string final_feedback_loop = receive_tcp(tcp_socket);
        parse_reserve_feedback_client(final_feedback_loop,tcp_socket,username);

    }
}
//I asked chatgpt 5.1 "write the function to check my TCP port for me based on the code i provided 1. /* Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure */
//2. getsock_check = getsockname(TCP_Connect_Sock, (struct sockaddr*)&my_addr, (socklen_t
//*)&addrlen);
//3. // Error checking
//4. if (getsock_check == -1) {
//5.
//perror("getsockname");
//6.
//}
//exit(1); "
int get_my_tcp_port(int TCP_Connect_Sock)
{
    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);

    int getsock_check = getsockname(TCP_Connect_Sock,
                                    (struct sockaddr*)&my_addr,
                                    (socklen_t*)&addrlen);

    if (getsock_check == -1) {
        perror("getsockname");
        exit(1);
    }

    /* Convert port from network byte order to host byte order */
    int my_port = ntohs(my_addr.sin_port);

    return my_port;
}