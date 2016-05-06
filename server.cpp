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
#include <vector>
#include "common_skt.h"
#include <sstream>
#include "mapper.h"
#include "mutex.h"
#include "thread.h"

#define INT_SIZE 4
#define END_SIG "End\n"
#define HOSTNAME_POS 2
#define PORTNAME_POS 3
#define SIZE_QUEUE 100

class ServerPeerConnection{
public:
    static bool setup(socket_t *skt, char *port){
        int s;
        bool status = false;
        struct addrinfo hints;
        struct addrinfo *ptr;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;       
        hints.ai_socktype = SOCK_STREAM;
        s = getaddrinfo(NULL, port, &hints, &ptr);
        if (s == -1) return status;
        if (socket_create(skt, ptr) && 
            socket_bind(skt, ptr) &&
            socket_listen(skt, SIZE_QUEUE)) status = true;
        freeaddrinfo(ptr);
        return status;
    }

    static bool accept_client(socket_t *skt, socket_t *peer_skt){
        return socket_accept(skt, peer_skt);
    }
};

class TemperatureTable{
private:
    Mutex m;
    std::vector< std::vector< std::pair<std::string, int> > > temp_data;

public:
    TemperatureTable(): temp_data(31) {}

    void add_info(int day, std::pair<std::string, int> info){
        m.lock();
        this->temp_data[day - 1].push_back(info);
        m.unlock();
    }
};

class Receptor: public Thread{
private:
    socket_t *skt;
    std::string city;
    TemperatureTable& temp_data;

    void receive_city(){
        int city_len;
        socket_receive(skt, &city_len, INT_SIZE);
        char *city_c_str = (char*)malloc(sizeof(char) * city_len);
        socket_receive(skt, city_c_str, city_len);
        std::string aux(city_c_str);
        this->city = aux;
    }

    void receive_temps(){
        while(true){
            int info_len;
            socket_receive(skt, &info_len, INT_SIZE);
            char *info_c_str = (char*)malloc(sizeof(char) * info_len);
            socket_receive(skt, info_c_str, info_len);
            std::string aux(info_c_str);

            if (aux == END_SIG) return;
            
            std::stringstream ss(aux);
            
            std::string s_day;
            int day;
            std::string s_temp;
            int temp;
            
            ss >> s_day;
            std::istringstream is_day(s_day);
            is_day >> day;
            
            ss >> s_temp;
            std::istringstream is_temp(s_temp);
            is_temp >> temp;

            std::pair<std::string, int> pair(this->city, temp);
            this->temp_data.add_info(day, pair);
        }
    }
public:
    Receptor(socket_t *skt, TemperatureTable& temp_data): temp_data(temp_data){
        this->skt = skt;
    }

    void execute(){
        receive_city();
        receive_temps();
    }
};

class ClientReceiver: public Thread {
private:
    std::vector<Receptor*> receptors;
    TemperatureTable& temp_data;
    bool& keep_accepting;
    socket_t *skt;

public:
    ClientReceiver(TemperatureTable& temp_data,
                   bool& keep_accepting,
                   socket_t *skt): temp_data(temp_data),
                                   keep_accepting(keep_accepting) {
        this->skt = skt;
    }

    void execute(){
        // keep_accepting is modified by other thread
        while (keep_accepting){
            socket_t *peer_skt = (socket_t *)malloc(sizeof(socket_t));
            ServerPeerConnection::accept_client(skt, peer_skt);
            Receptor *r = new Receptor(peer_skt, temp_data);
            r->start();
            receptors.push_back(r);
            // Should delete the ones that have already finished
            // from the receptor vector
        }
        for (std::vector<Receptor*>::iterator it = receptors.begin(); 
             it != receptors.end(); ++it){
            (*it)->join();
        } 
    }
};

int main(int argc, char *argv[]){
    socket_t server;
    ServerPeerConnection::setup(&server, argv[PORTNAME_POS]);
    std::string line;
    std::getline(std::cin, line);
    bool keep_accepting_mappers = true;
    TemperatureTable temp_data;
    ClientReceiver client_receiver(temp_data, keep_accepting_mappers, &server);
    client_receiver.start();
    while (line != "q"){
        std::getline(std::cin, line);
    }
    keep_accepting_mappers = false;
    client_receiver.join();
    // For each day, throw a reducer in a thread.
    // Print the results
}
