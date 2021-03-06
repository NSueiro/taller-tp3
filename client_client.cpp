#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <string>
#include "common_skt.h"
#include <sstream>
#include "client_mapper.h"

#define INT_SIZE 4
#define END_SIG "End\n"
#define HOSTNAME_POS 1
#define PORTNAME_POS 2

class ClientConnection{
public:
    static bool setup(socket_t *skt, const char *hostname, const char *service){
        int s;
        bool status = false;
        struct addrinfo hints;
        struct addrinfo *ptr;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;       
        hints.ai_socktype = SOCK_STREAM;
        s = getaddrinfo(hostname, service, &hints, &ptr);
        if (s == -1) return status;
        if (socket_create(skt, ptr) && socket_connect(skt, ptr)) status = true;
        freeaddrinfo(ptr);
        return status;
    }
};

int main(int argc, char *argv[]){
    socket_t client;
    ClientConnection::setup(&client, argv[HOSTNAME_POS], argv[PORTNAME_POS]);
    std::string line;
    bool flag = false;
    while (std::getline(std::cin, line)){
        Mapper m(&client);
        m.map(line);
        if (!flag){
            m.send_city();
            flag = true;
        }
        m.send_temp_info();
    }
    // Horrible magic number. MUST CHANGE  
    int i = 4;
    socket_send(&client, &i, INT_SIZE);  
    socket_send(&client, END_SIG, 4);
    return 0;
}
