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
#include "common_mutex.h"
#include "common_thread.h"
#include <limits>

#define INT_SIZE 4
#define END_SIG "End\n"
#define PORTNAME_POS 1
#define SIZE_QUEUE 100
#define DAYS_OF_MARCH 31
// Unreal low temperature. Should be -infinity, but this works as well since
// it's impossible to reach this temperature
#define LOWEST_TEMP -5000

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

    std::vector< std::pair<std::string, int> >& get(int day){
        return this->temp_data[day - 1];
    }

    void print(){
        for (int i = 0; i < 31; ++i){
            std::cout << "Dia: " << i + 1 << ". Info: ";
            for (std::vector< std::pair<std::string, int> >::iterator it = this->temp_data[i].begin();
                it != this->temp_data[i].end(); ++it){
                std::cout << it->first << " " << it->second << " - ";
            }
            std::cout << std::endl;
        }
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
        char *city_c_str = (char*)malloc(sizeof(char) * city_len + 1);
        memset(city_c_str, 0, city_len + 1);
        socket_receive(skt, city_c_str, city_len);
        std::string aux(city_c_str);
        this->city = aux;
        free(city_c_str);
    }

    void receive_temps(){
        while(true){
            int info_len;
            socket_receive(skt, &info_len, INT_SIZE);
            char *info_c_str = (char*)malloc(sizeof(char) * info_len + 1);
            memset(info_c_str, 0, info_len + 1);
            socket_receive(skt, info_c_str, info_len);
            std::string aux(info_c_str);
            free(info_c_str);
            std::cout << aux << std::endl;
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

    ~Receptor(){
        socket_close(this->skt);
        free(this->skt);
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
            bool status = ServerPeerConnection::accept_client(skt, peer_skt);
            if (!status) {
                free(peer_skt);
                break;
            }
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
        for (size_t i = 0; i < receptors.size(); ++i){
            delete receptors[i];
        }
    }
};

class Reducer: public Thread {
private:
    TemperatureTable& temp_data;
    std::vector<int>& max_temps;
    std::vector< std::vector<std::string> >& cities;
    int day;
public:
    Reducer(TemperatureTable& temp_data,
            std::vector<int>& max_temps,
            std::vector< std::vector<std::string> >& cities,
            int& day): temp_data(temp_data), max_temps(max_temps), 
                      cities(cities){
        this->day = day;
    }

    void execute(){
        for (std::vector< std::pair<std::string, int> >::iterator it = 
                this->temp_data.get(day).begin();
            it != this->temp_data.get(day).end(); ++it){
            if (it->second > this->max_temps[this->day - 1]){
                this->max_temps[this->day - 1] = it->second;
                cities[this->day - 1].clear();
                cities[this->day - 1].push_back(it->first);
            }
            else if (it->second == max_temps[this->day - 1]){
                cities[this->day - 1].push_back(it->first);
            }
        }
    }
};

int main(int argc, char *argv[]){
    socket_t server;
    ServerPeerConnection::setup(&server, argv[PORTNAME_POS]);
    std::string line;
    bool keep_accepting_mappers = true;
    TemperatureTable temp_data;
    ClientReceiver client_receiver(temp_data, keep_accepting_mappers, &server);
    client_receiver.start();
    while (line != "q"){
        std::cout << "Esperando la q: ";
        std::getline(std::cin, line);
        std::cout << std::endl;
    }
    keep_accepting_mappers = false;
    socket_shutdown(&server, SHUT_RDWR);
    client_receiver.join();

    std::vector< std::vector<std::string> > cities(DAYS_OF_MARCH, 
                                                   std::vector<std::string>(0));
    std::vector<int> max_temps(31, LOWEST_TEMP);
    std::vector<Reducer*> reducers;
    for (int i = 1; i <= DAYS_OF_MARCH; ++i){
        Reducer *r = new Reducer(temp_data, max_temps, cities, i);
        r->start();
        reducers.push_back(r);
    }

    for (std::vector<Reducer*>::iterator it = reducers.begin(); 
         it != reducers.end(); ++it){
        (*it)->join();
    }

    // Print the results
    for (int i = 0; i < 31; ++i){
        std::cout << i + 1 << ": ";
        bool aux = false;
        for (std::vector<std::string>::iterator it = cities[i].begin();
            it != cities[i].end(); ++it){
            if (aux) std::cout << "/";
            std::cout << *it;
        }
        std::cout << " (" << max_temps[i] << ")" << std::endl;
    }
    for (size_t i = 0; i < reducers.size(); ++i){
        delete reducers[i];
    }
    return 0;
}
