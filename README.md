a. Full name: Tianming Guo 
<br>
b. Your Student ID:  9863196514
<br>
c. What you have done in the assignment: I write all the code by myself, only asked AI to check some syntax, some design and some errors, and made adjustment based on that.
I wrote where I made change subject to AI in the comments.
<br>
d. What your code files are and what each one of them does. (Please do not repeat the project description;
just name your code files and briefly mention what they do). 
### **client.cpp**
Implements the client program.  
Handles:
- User authentication over TCP (via Server M)
- Sending commands: `search`, `reserve`, `lookup`, `cancel`
- Receiving responses from Server M
- Sending Y/N confirmation for partial reservations

### **serverA.cpp**
Authentication server (UDP).  
Handles:
- Validating username/password
- Returning authentication results: `"M"` (member), `"G"` (guest), `"F"` (fail)

### **serverM.cpp**
Main coordination server.  
Handles:
- Client TCP connection
- Authentication forwarding to Server A
- Forwarding commands to Server R and Server P via UDP
- Sending results back to client  
  Commands handled: `search`, `reserve`, `lookup`, `cancel`

### **serverR.cpp**
Reservation management server (UDP).  
Handles:
- Checking available time slots
- Processing reservations (including partial availability)
- Validating cancellation requests
- Returning lookup results

### **serverP.cpp**
Pricing and refund server (UDP).  
Handles:
- Computing reservation price
- Computing refunds
- Maintaining usage count per member

### **spaces.txt**
Stores all parking lot time-slot information for UPC and HSC structures.

### **members.txt**
Stores all member records including ID, username, password.
Summarized by Chatgpt 5.1. Note, I uploaded my code to chatgpt 5.1 and asked it to summarize only!

e. The format of all the messages exchanged, e.g., username and password are concatenated and delimited
by a comma, etc.
Authentication:
username password
M / G / F 
M means member, G means guest, F means fail

Search:
username search structure
<availability list>

Reservation:
username reserve lot slots
all username lot slots
partial username lot slots
fail username lot
Y / N

Cancellation:
username cancel lot slots
can username lot slots
fail can
<refund>

Lookup:
username lookup
<list of reservations>





f. Any idiosyncrasy of your project. It should specify under what conditions the project fails, if any. 
I carefully designed the edge cases. so for example, if you type reserve sjjwj 1 22123, it will not make the reservation.
If you type reserve H888 100 9 8 1, the 100 will be dropped. The program is pretty robust.

g. Reused Code: Did you use code from anywhere for your project? If not, say so. If so, state what
functions and where they're from. (Also identify this with a comment in the source code).   
I mostly used the Beej's simple TCP server, UDP server, UDP client ,and TCP client. I made modifications.
some functions that is from AI:
get_my_tcp_port(int TCP_Connect_Sock);
std::string format_2dec(double value);
others I only fix some error based on AI suggestions.
The AI I was used:chatgpt 5.1

h. Which version of Ubuntu (only the Ubuntu versions that we provided to you) are you using?
24.04.1 LTS WSL2


usage:
<br>
in one terminal: make clean<br>
then do make all<br>
feel free to replace members.txt and spaces.txt 
I did not modify any members.txt or spaces.txt they all coming from
original folder.
<br>
in separate terminals, run :
<br>
./serverM
<br>
./serverA
<br>
./serverR
<br>
./serverP
<br>
./client username password
 examples:
<br>
./client guest 123456
<br>
./client James SODids392