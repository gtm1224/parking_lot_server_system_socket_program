//
// Created by gtm1224 on 11/29/25.
//

#include "serverP.h"
#define MAXBUFLEN 2048
#define SERVERM_UDPPORT "24514"
#define SERVERP_UDPPORT "23514"

#define MAXDATASIZE 2048
const std::string HOST_IP = "127.0.0.1";
std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> load_parking_info_serverP(std::string file_path);
std::unordered_map<std::string, Member> load_members_info_serverP(std::string file_path);
int get_userId_int(std::string username,std::unordered_map<std::string, Member> member_table);
std::string receive_from_serverP(int sockfd);
void reserve_time_slots_serverP(std::string parking_lot,std::string time_slots,int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table);
std::unordered_map<int, Member_with_price> load_members_info_with_price_serverP(std::string file_path);
double calculate_total_price(std::unordered_map<int, Member_with_price> &members_price,int userId,std::string parking_lot,std::string time_slots);
void get_init_price(std::unordered_map<std::string, std::unordered_map<std::string, std::array<int,12>>> &parking_lot_table,std::unordered_map<int, Member_with_price> &members_price);
int send_request_udp_with_socket_serverP(int sockfd,const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send);
double cancel_time_slots_serverP(std::string parking_lot,std::string time_slots,int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table,std::unordered_map<int, Member_with_price> &members_price);
const int peak_1=5;
const int peak_2=9;
const double multi=1.50;
const double H_PRICE_BASE=15.00;
const double U_PRICE_BASE=10.00;
const double H_PRICE_ADD=10.00;
const double U_PRICE_ADD=7.00;
const double U_REFUND =7.00;
const double H_REFUND =10.00;


//I asked the Chatgpt 5.1 to write a function that turn a double to 2 decimal palces
std::string format_2dec(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}




int main() {
    std::cout << "[Server P] Booting up using UDP on port "<<SERVERP_UDPPORT<<"."<<std::endl;
    //init process
    std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>>parking_lot_table = load_parking_info_serverP("spaces.txt");

    std::unordered_map<std::string, Member> members_table = load_members_info_serverP("members.txt");
    //calculate init prices for each member
    std::unordered_map<int, Member_with_price> members_price =load_members_info_with_price_serverP("members.txt");
    get_init_price(parking_lot_table,members_price);

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //ipv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, SERVERP_UDPPORT, &hints, &servinfo)) != 0) {
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

    while(true){
        std::string request = receive_from_serverP(sockfd);
        std::cout<<"Server P received a pricing request from the main server. \n";
        std::string first_command;//should be all
        std::string username;
        std::string parking_lot;
        std::string time_slots;
        std::stringstream ss(request);
        ss>>first_command;
        if(first_command == "all"){
            ss>>username;
            ss>>parking_lot;
            std::getline(ss >> std::ws, time_slots);
//            std::cout<<"time slotsssssssss:"<<time_slots<<std::endl;

            int userId = get_userId_int(username,members_table);
            double total_price = calculate_total_price(members_price,userId,parking_lot,time_slots);
            reserve_time_slots_serverP(parking_lot,time_slots,userId,parking_lot_table);
            std::cout<<"Calculated total price of  $"<<format_2dec(total_price)<< " for "<< username<<".\n";
            send_request_udp_with_socket_serverP(sockfd,HOST_IP,SERVERM_UDPPORT, format_2dec(total_price));
            std::cout<<"Server P finished sending the price to the main server.\n";
        }else if(first_command == "can"){
            std::cout<<"Server P received a refund request from the main server. \n";
            ss>>username;
            ss>>parking_lot;
            std::getline(ss >> std::ws, time_slots);
            int userId = get_userId_int(username,members_table);
            double refund = cancel_time_slots_serverP(parking_lot,time_slots,userId,parking_lot_table,members_price);
            std::cout<<"Calculated total price of  $"<<format_2dec(refund)<< " for "<< username<<".\n";
//            std::cout<<"refund proceeeeeee::"<<refund<<std::endl;
            send_request_udp_with_socket_serverP(sockfd,HOST_IP,SERVERM_UDPPORT, format_2dec(refund));
            std::cout<<"Server P finished sending the refund amount to the main server. \n";
        }




    }

    return 0;
}

double cancel_time_slots_serverP(std::string parking_lot,std::string time_slots,int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table,std::unordered_map<int, Member_with_price> &members_price){

    Member_with_price &m = members_price[userId];
    std::stringstream ss(time_slots);
    int ts;
    double refund_price =0.0;
    if (parking_lot[0] == 'U') {
        while (ss >> ts) {
            refund_price+=(2.0*U_REFUND);
            m.space_u -= 1;
            parking_lot_table["UPC"][parking_lot][ts-1]=0;
        }
    } else if (parking_lot[0] == 'H') {
        while (ss >> ts) {
            refund_price+=(2.0*H_REFUND);
            m.space_h -= 1;
            parking_lot_table["HSC"][parking_lot][ts-1]=0;
            }

        }


    m.price -= refund_price;

    return refund_price;
}




double calculate_total_price(std::unordered_map<int, Member_with_price> &members_price,
                             int userId,
                             std::string parking_lot,
                             std::string time_slots)
{
    Member_with_price &m = members_price[userId];

    std::stringstream ss(time_slots);
    int ts;
    double temp_price = 0.0;

    if (parking_lot[0] == 'U') {
        while (ss >> ts) {
            if (m.space_u == 0) {
                // first UPC space for this member
                if (ts == peak_1 || ts == peak_2) {
                    temp_price += U_PRICE_BASE * 2.0 * multi;
                } else {
                    temp_price += U_PRICE_BASE * 2.0;
                }
            } else {
                // additional UPC spaces
                if (ts == peak_1 || ts == peak_2) {
                    temp_price += U_PRICE_ADD * 2.0 * multi;
                } else {
                    temp_price += U_PRICE_ADD * 2.0;
                }
            }
            m.space_u += 1;
        }
    } else if (parking_lot[0] == 'H') {
        while (ss >> ts) {
            if (m.space_h == 0) {
                // first HSC space
                if (ts == peak_1 || ts == peak_2) {
                    temp_price += H_PRICE_BASE * 2.0 * multi;
                } else {
                    temp_price += H_PRICE_BASE * 2.0;
                }
            } else {
                // additional HSC spaces
                if (ts == peak_1 || ts == peak_2) {
                    temp_price += H_PRICE_ADD * 2.0 * multi;
                } else {
                    temp_price += H_PRICE_ADD * 2.0;
                }
            }
            m.space_h += 1;
        }
    }

    m.price += temp_price;
    return m.price;
}


std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> load_parking_info_serverP(std::string file_path){
    std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> parking_lot_table;
    std::unordered_map<std::string,std::array<int,12>> U_parking_lots;
    std::unordered_map<std::string,std::array<int,12>> H_parking_lots;
    std::ifstream file;
    file.open(file_path);
    std::string line;
    while (std::getline(file, line)) {


        std::string parking_lot_name;
        std::stringstream read_line(line);
        std::array<int,12> parking_time_slots{};
        read_line >> parking_lot_name;
        for(int i=0;i<12;i++){
            int value;
            read_line>>value;
            parking_time_slots[i] = value;
        }
        if(!parking_lot_name.empty() && parking_lot_name[0]=='H'){
            H_parking_lots[parking_lot_name]=parking_time_slots;
        }else{
            U_parking_lots[parking_lot_name]=parking_time_slots;
        }
//        std::cout<<parking_lot_name<<":"<<std::endl;
//        for(auto a:parking_time_slots){
//            std::cout<<a<<" ";
//        }
//        std::cout<<std::endl;

    }
    parking_lot_table["UPC"] = U_parking_lots;
    parking_lot_table["HSC"] = H_parking_lots;
    return parking_lot_table;
}

std::unordered_map<std::string, Member> load_members_info_serverP(std::string file_path) {
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


std::unordered_map<int, Member_with_price> load_members_info_with_price_serverP(std::string file_path) {
    std::unordered_map<int, Member_with_price> members_table;
    std::ifstream file;
    file.open(file_path);
    std::string line;
    while (std::getline(file, line)) {

        Member_with_price m;
        std::stringstream read_line(line);
        read_line >> m.id >> m.username;
        members_table[m.id] = m;
    }

    return members_table;
}

std::string receive_from_serverP(int sockfd){
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

void reserve_time_slots_serverP(std::string parking_lot,std::string time_slots,int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table){
    std::stringstream ss(time_slots);
    int time_slot;
    std::string parking_lot_structure;
    if(parking_lot[0]=='U'){
        parking_lot_structure="UPC";
    }else{
        parking_lot_structure="HSC";
    }
//    std::cout<<"time_slots"<<time_slots<<std::endl;
//    std::cout<<"parking lot need to modify:"<<parking_lot_structure;
    while(ss>>time_slot){
//        std::cout<<"time_slot"<<time_slot;
        parking_lot_table[parking_lot_structure][parking_lot][time_slot-1]=userId;
    }

}


int get_userId_int(std::string username,std::unordered_map<std::string, Member> member_table){
    return member_table[username].id;
}

void get_init_price(std::unordered_map<std::string, std::unordered_map<std::string, std::array<int,12>>> &parking_lot_table,std::unordered_map<int, Member_with_price> &members_price)
{
    for(auto pair : parking_lot_table["UPC"]){
        std::string parking_lot = pair.first;
        std::array<int,12> time_slots = pair.second;
        for(int i=0;i<12;i++){
            if(time_slots[i]!=0){
                int userId = time_slots[i];
                auto it = members_price.find(userId);
                if(it!=members_price.end()){
                    Member_with_price &m = members_price[userId];

                    if(m.space_u==0){
                        if (i+1 == peak_1 || i+1 == peak_2) {
                            m.price += U_PRICE_BASE * 2.0 * multi;
                        } else {
                            m.price += U_PRICE_BASE * 2.0;
                        }
                    }else{
                        if (i+1 == peak_1 || i+1 == peak_2) {
                            m.price += U_PRICE_ADD * 2.0 * multi;
                        } else {
                            m.price += U_PRICE_ADD * 2.0;
                        }

                    }
                    m.space_u+=1;
                }
                }

        }
    }

    for(auto pair : parking_lot_table["HSC"]){
        std::string parking_lot = pair.first;
        std::array<int,12> time_slots = pair.second;
        for(int i=0;i<12;i++){
            if(time_slots[i]!=0){
                int userId = time_slots[i];
                auto it = members_price.find(userId);
                if(it!=members_price.end()){
                    Member_with_price &m = members_price[userId];

                    if(m.space_h==0){
                        if (i+1 == peak_1 || i+1 == peak_2) {
                            m.price += H_PRICE_BASE * 2.0 * multi;
                        } else {
                            m.price += H_PRICE_BASE * 2.0;
                        }
                    }else{
                        if (i+1 == peak_1 || i+1 == peak_2) {
                            m.price += H_PRICE_ADD * 2.0 * multi;
                        } else {
                            m.price += H_PRICE_ADD * 2.0;
                        }

                    }
                    m.space_h+=1;
                }
            }

        }
    }
}


int send_request_udp_with_socket_serverP(int sockfd,const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send){

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

