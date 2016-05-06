#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "common_skt.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

bool socket_create(socket_t *skt, struct addrinfo *info){
    struct addrinfo *iter;
    int fd;
    for (iter = info; iter != NULL; iter = iter->ai_next){
        fd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
        if (fd != -1){ 
            skt->skt_fd = fd;
            return true;
        }
    }
    return false;
}

void socket_destroy(socket_t *skt){
    shutdown(skt->skt_fd, SHUT_RDWR);
    close(skt->skt_fd);
}

bool socket_bind(socket_t *skt, struct addrinfo *info){
    return !(bind(skt->skt_fd, info->ai_addr, info->ai_addrlen) == -1);
}

bool socket_listen(socket_t *skt, int tam_cola){
    return !(listen(skt->skt_fd, tam_cola));
}

bool socket_accept(socket_t *skt, socket_t *new_skt){
    int new_fd = accept(skt->skt_fd, NULL, NULL);
    if (new_fd != -1){ 
        new_skt->skt_fd = new_fd;
        return true;
    }
    return false;
}

bool socket_connect(socket_t *skt, struct addrinfo *info){
    return !(connect(skt->skt_fd, info->ai_addr, info->ai_addrlen) == -1);
}

bool socket_shutdown(socket_t *skt, int sig){
    return !(shutdown(skt->skt_fd, sig) == -1);
}

bool socket_send(socket_t *skt, const void *msg, int len){
    int sent = 0;
    while (sent < len){
        int s = send(skt->skt_fd, (&msg)[sent], len - sent, MSG_NOSIGNAL);
        if (s == -1) return false;
        sent += s;
    }
    return sent == len;
}

bool socket_receive(socket_t *skt, void *buffer, int len){
    int received = 0;
    while (received < len){
        int r = recv(skt->skt_fd, (&buffer)[received], len - received, 
                    MSG_NOSIGNAL);
        if (r <= 0) break;
        received += r;
    }
    return received == len;
}

bool socket_close(socket_t *skt){
    return !(close(skt->skt_fd) == -1);
}
