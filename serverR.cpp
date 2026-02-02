//
// Created by gtm1224 on 11/29/25.
//

#include "serverR.h"
#define MAXBUFLEN 2048
#define SERVERM_UDPPORT "24514"
#define SERVERP_UDPPORT "23514"
#define SERVERR_UDPPORT "22514"
#define MAXDATASIZE 2048
const std::string HOST_IP = "127.0.0.1";

std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> load_parking_info(std::string file_path);
std::string available_parking_lots_of_structure(std::unordered_map<std::string,std::array<int,12>> parking_lots);
std::string receive_from(int sockfd);
void parse_command(const std::string commands,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lots_table,std::unordered_map<std::string, Member> &members_table,int sockfd);
int send_request_udp_with_socket(int sockfd,const std::string &host, const std::string &SERVERPORT,const std::string &info_to_send);
std::pair<std::string,std::string> check_reservation(const std::string commands,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lots_table);
std::unordered_map<std::string, Member> load_members_info_serverR(std::string file_path);
int get_userId(std::string username,std::unordered_map<std::string, Member> member_table);
void reserve_time_slots(std::string parking_lot,std::string time_slots,int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table);
std::string lookup_for_userId(int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table);
bool cancellation_request(int userId,std::string parking_lot,std::string time_slots,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lots_table);



int main() {
    std::cout << "[Server R] Booting up using UDP on port "<<SERVERR_UDPPORT<<"."<<std::endl;
    std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>>parking_lot_table = load_parking_info("spaces.txt");

    std::unordered_map<std::string, Member> members_table = load_members_info_serverR("members.txt");
//    std::string res = available_parking_lots_of_structure(parking_lot_table["UPC"]);
//    std::string res2 = available_parking_lots_of_structure(parking_lot_table["HSC"]);
//    std::cout<<"mwwqwmwmq"<<std::endl;
//    std::cout<<res<<std::endl;
//    std::cout<<"mwwqwmwmq"<<std::endl;
//    std::cout<<res2<<std::endl;

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
//    int numbytes;
//    struct sockaddr_storage their_addr;
//    char buf[MAXBUFLEN];
//    socklen_t addr_len;
//    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //ipv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, SERVERR_UDPPORT, &hints, &servinfo)) != 0) {
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
        std::string request = receive_from(sockfd);
//        std::cout<<"request::::::"<<request<<std::endl;
        parse_command(request,parking_lot_table,members_table,sockfd);


    }

    return 0;
}

std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> load_parking_info(std::string file_path){
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

std::string available_parking_lots(std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> parking_lots_table){
    std::string available_UPC = available_parking_lots_of_structure(parking_lots_table["UPC"]);
    std::string available_HSC = available_parking_lots_of_structure(parking_lots_table["HSC"]);

    return available_UPC +"\n" + available_HSC;

}

std::string available_parking_lots_of_structure(std::unordered_map<std::string,std::array<int,12>> parking_lots){
    std::string res = "";
    for (auto pair : parking_lots){

        std::string parking_lot_name = pair.first;
        std::array<int,12> parking_lot_time_slots = pair.second;
        std::string available_time_slots = "";
        for(int i =0;i<12;i++){
            if(parking_lot_time_slots[i]==0){
                available_time_slots += std::to_string(i+1)+ " ";
            }
        }
        if (available_time_slots.size()==0){
            continue;
        }else{
            res+=parking_lot_name+": "+available_time_slots+"\n";
        }
    }
    return res;
    
}


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
//    char s[INET6_ADDRSTRLEN];
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

void parse_command(const std::string commands,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lots_table,std::unordered_map<std::string, Member> &members_table,int sockfd){
    std::string username;
    std::string first_word;
    std::stringstream ss(commands);
    ss>>username;
    ss>>first_word;
//    std::cout<<"username: "<<username<<std::endl;
//    std::cout<<"first words: "<<first_word<<std::endl;

    if(first_word=="search"){
        std::cout<<"Server R received an availability request from the main server. \n";
        std::string parking_lot_name;
        ss>>parking_lot_name;
        std::string available_lots;
        if(parking_lot_name.size()==0){
           available_lots =available_parking_lots(parking_lots_table);
            if(available_lots.size()==0){
                available_lots= "No available spaces in UPC and HSC. \n";
            }
        }else{
           if(parking_lot_name == "UPC"){
               available_lots=available_parking_lots_of_structure(parking_lots_table["UPC"]);
               if(available_lots.size()==0){
                   available_lots= "No available spaces in UPC. \n";
               }
           }else if(parking_lot_name == "HSC"){
               available_lots = available_parking_lots_of_structure(parking_lots_table["HSC"]);
               if(available_lots.size()==0){
                   available_lots= "No available spaces in HSC. \n";
               }
           }else{
               available_lots = "Invalid parking lot name.\n";
           }
        }
//        std::cout<<"available parking lots: "<<available_lots<<std::endl;

        int success = -1;
        while(success==-1){
            success = send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,available_lots);
        }
        std::cout<<"Server R finished sending the response to the main server. \n";



    }else if(first_word=="reserve"){
        std::cout<<"Server R received a reservation request from the main server."<<std::endl;
        std::pair<std::string,std::string> check_r_res = check_reservation(commands,parking_lots_table);
        std::string a_slots = check_r_res.first;
        std::string n_a_slots = check_r_res.second;
        std::string parking_lot_re;
        ss>>parking_lot_re;
        int userId = get_userId(username,members_table);

        if(a_slots.size()==0 ||(n_a_slots.size()==0 && a_slots.size()==0)){
            std::cout<<"Time slot(s) < "<<n_a_slots<<">"<<"not available for <"<<parking_lot_re<<">."<<std::endl;
            std::cout<<"Reservation cancelled."<<std::endl;
            std::string string_send ="fail "+ username+" "+parking_lot_re+" ";
            int success = -1;
            while(success==-1){
                success = send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,string_send);
            }
        }
        //all time slots available
        else if (n_a_slots.size()==0){
            std::cout<<"All requested time slots are available. \n";
            reserve_time_slots(parking_lot_re,a_slots,userId,parking_lots_table);
            std::cout<<"Successfully reserved "<<parking_lot_re <<" at time slot(s) "<<a_slots<<"for "<<username<<".\n";
            std::string string_send = "all " + username+" "+parking_lot_re+" "+ a_slots;
            int success = -1;
            while(success==-1) {
                success = send_request_udp_with_socket(sockfd, HOST_IP, SERVERM_UDPPORT, string_send);
            }
            //this is the partial case
        }else{
            std::cout<<"Time slot(s) "<<n_a_slots<<"not available for "<<parking_lot_re<<" ."<<std::endl;
            std::cout<<"Requesting to reserve rest available slots (Y/N). \n";
            std::string string_send ="partial "+ username+" "+parking_lot_re+" "+n_a_slots;
            int success = -1;
            while(success==-1){
                success = send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,string_send);
            }
            std::string user_yes_or_no = receive_from(sockfd);
            if(user_yes_or_no[0]=='Y' || user_yes_or_no[0]=='y'){
                std::cout<<"User confirmed partial reservation. \n";
                reserve_time_slots(parking_lot_re,a_slots,userId,parking_lots_table);
                std::cout<<"Successfully reserved "<<parking_lot_re <<" at time slot(s) "<<a_slots<<"for "<<username<<".\n";

                std::string string_send_final ="all "+ username+" "+parking_lot_re+" "+a_slots;
                int success = -1;
                while(success==-1){
                    success = send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,string_send_final);
                }
            }else{

                std::cout<<"Reservation cancelled."<<std::endl;
                std::string string_send ="fail "+ username+" "+parking_lot_re+" ";
                int success = -1;
                while(success==-1){
                    success = send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,string_send);
                }
            }

        }

    }else if(first_word=="lookup"){
        std::cout<<"Server R received a lookup request from the main server.\n";
        int userId = get_userId(username,members_table);
        std::string res = lookup_for_userId(userId,parking_lots_table);
        send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,res);
        std::cout<<"Server R finished sending the reservation information to the main server. \n";
    }else if(first_word == "cancel"){
        std::cout<<"Server R received a cancellation request from the main server. "<<std::endl;
        std::string parking_lot;
        std::string time_slots;
        ss>>parking_lot;
        std::string send_string;
        std::getline(ss >> std::ws, time_slots);
        int userId = get_userId(username,members_table);
        if(cancellation_request(userId,parking_lot,time_slots,parking_lots_table)){
            std::cout<<"Successfully cancelled reservation for "<<parking_lot<<" at time slot(s) "<<time_slots<<" for "<<username<<std::endl;
            send_string = "can "+ username+" "+parking_lot+" "+time_slots;
        }else{
            std::cout<<"No reservation found for "<<username<<" at "<<parking_lot<<" time slot(s) "<<time_slots<<". \n";
            send_string = "fail can";
        }
        send_request_udp_with_socket(sockfd,HOST_IP,SERVERM_UDPPORT,send_string);

    }
}
bool cancellation_request(int userId,std::string parking_lot,std::string time_slots,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lots_table){
    std::vector<int> need_modify;
    std::stringstream ss(time_slots);
    int ts;

    while(ss>>ts){
        need_modify.push_back(ts);
    }

    if(parking_lot[0]=='H'){
        std::unordered_map<std::string,std::array<int,12>> &p = parking_lots_table["HSC"];
        if(p.find(parking_lot)!=p.end()){
            for(auto ts:need_modify){
                if(ts>12||ts<0){
                    return false;
                }else{
                    if(p[parking_lot][ts-1]!=userId){
                        return false;
                    }
                }
            }
            for(auto ts:need_modify){
                p[parking_lot][ts-1] = 0;
            }
        }else{
            return false;
        }

    }else if(parking_lot[0]=='U'){
        std::unordered_map<std::string,std::array<int,12>> &p = parking_lots_table["UPC"];
        if(p.find(parking_lot)!=p.end()){
            for(auto ts:need_modify){
                if(ts>12||ts<=0){
                    return false;
                }else{
                    if(p[parking_lot][ts-1]!=userId){
                        return false;
                    }
                }

            }
            for(auto ts:need_modify){
                p[parking_lot][ts-1] = 0;
            }

        }else{
            return false;
        }
    }else{
        return false;
    }
    return true;


}



std::pair<std::string,std::string> check_reservation(const std::string commands,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lots_table){
    std::string username;
    std::string first_word;
    std::string parking_lot;
    std::vector<int> extracted_time_slots;
    std::string available_slots="";
    std::string not_available_slots="";
    std::stringstream ss(commands);
    ss>>username;
    ss>>first_word;
    ss>>parking_lot;
    int time_slot;
    while(ss>>time_slot){
        extracted_time_slots.push_back(time_slot);
    }
    std::sort(extracted_time_slots.begin(), extracted_time_slots.end());
    auto last_unique_it = std::unique(extracted_time_slots.begin(), extracted_time_slots.end());
    extracted_time_slots.erase(last_unique_it, extracted_time_slots.end());
    std::unordered_map<std::string,std::array<int,12>> parking_lots_time_slots;
    if(parking_lot[0]=='H'){
        parking_lots_time_slots = parking_lots_table["HSC"];
    }else{
        parking_lots_time_slots = parking_lots_table["UPC"];
    }
    if (parking_lots_time_slots.find(parking_lot) == parking_lots_time_slots.end()) {
        // parking lot does NOT exist → invalid request
        return {"", ""};
    }

    for(int time_s:extracted_time_slots){
        if(time_s<=12 && time_s>0){
            if(parking_lots_time_slots[parking_lot][time_s-1]==0){
                available_slots+= std::to_string(time_s)+ " ";
            }else{
                not_available_slots+=std::to_string(time_s)+ " ";
            }
        }

    }
    return std::make_pair(available_slots,not_available_slots);
}

void reserve_time_slots(std::string parking_lot,std::string time_slots,int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table){
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

std::unordered_map<std::string, Member> load_members_info_serverR(std::string file_path) {
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
int get_userId(std::string username,std::unordered_map<std::string, Member> member_table){
    int userId = member_table[username].id;
    return userId;
}

std::string lookup_for_userId(int userId,std::unordered_map<std::string,std::unordered_map<std::string,std::array<int,12>>> &parking_lot_table){
    std::string res = "";
    for(auto pair :parking_lot_table["UPC"]){
        std::string parking_lot =pair.first;

        std::array<int,12> time_slots = pair.second;
        std::string temp = "";
        for(int i =0;i<12;i++){
            if(time_slots[i]==userId){
                temp+=std::to_string(i+1)+" ";
            }

        }
        if(temp.size()!=0){
            std::string add_info = parking_lot+":"+" Time slot(s) "+temp+"\n";
            res+=add_info;
        }
    }

    for(auto pair :parking_lot_table["HSC"]){
        std::string parking_lot =pair.first;

        std::array<int,12> time_slots = pair.second;
        std::string temp = "";
        for(int i =0;i<12;i++){
            if(time_slots[i]==userId){
                temp+=std::to_string(i+1)+" ";
            }
        }
        if(temp.size()!=0){
            std::string add_info = parking_lot+":"+" Time slot(s) "+temp+"\n";
            res+=add_info;
        }
    }


    return res;



}