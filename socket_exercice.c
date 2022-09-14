//
// Created by HADDAD on 14/09/2022.
//

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "socket_tcp_conf.h"

void closeSocket(int * socket){

    if (shutdown(*socket, SHUT_WR)) {
        fprintf(stderr, "Client fails to shutdown the connected socket: %d , %s .\n",
                errno, strerror(errno));
        close(*socket);
        exit(EXIT_FAILURE);    }

    if (close(*socket)) {
        fprintf(stderr, "Client error on closing connected socket : %d , %s .\n",
                errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

}

void envoyer(int * socket, uint32_t * send_buffer) {

    ssize_t msg_size;

    *send_buffer = htonl(*send_buffer);
    msg_size = (ssize_t)send(*socket, send_buffer, sizeof(*send_buffer), 0);

    if (msg_size == -1) {
        fprintf(stderr, "Client sending error : %d , %s .\n",
                errno, strerror(errno));
        close(*socket);
        exit(EXIT_FAILURE);
    }

    while(msg_size < sizeof(*send_buffer)) {

        fprintf(stderr, "Client only sends %d bytes instead of %ul, trying to send again....\n",
                (int)msg_size, sizeof(*send_buffer));

        msg_size += send(*socket, send_buffer + msg_size, sizeof(*send_buffer) - msg_size, 0);

        fprintf(stderr, "Client sends %d bytes of %ul.\n",
                (int)msg_size, sizeof(*send_buffer));
    }

    *send_buffer = ntohl(*send_buffer);

}

void recevoir(int * socket, uint32_t * rcv_buffer){

    ssize_t msg_size;

    msg_size = recv(*socket, rcv_buffer, sizeof(*rcv_buffer), 0);

    if (msg_size == -1) {
        fprintf(stderr, "Client error when receiving server draw: %d , %s .\n",
                errno, strerror(errno));
        close(*socket);
        exit(EXIT_FAILURE);
    }

    *rcv_buffer = ntohl(*rcv_buffer);

}

int main(int argc, char **argv){

    struct sockaddr_in server_address;
    int connected_socket;

    unsigned int seed;
    uint32_t secret_token;
    uint32_t secret_key;

    /* get my random number */
    seed = time(NULL);
    secret_token = (uint32_t)rand_r(&seed);

    connected_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connected_socket == -1) {
        fprintf(stderr, "Client error on creating the socket : %d, %s.\n",
                errno, strerror(errno));
        goto clean;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(GIVER_TCP_PORT);
    if (!inet_aton(LOCALHOST_IPV4_ADDR, &server_address.sin_addr)) {
        fprintf(stderr, "Client socket adress is not valid : %s \n",
                LOCALHOST_IPV4_ADDR);
        goto clean;
    }

    if (connect(connected_socket, (struct sockaddr *)&server_address,
                sizeof(server_address))) {
        fprintf(stderr, "Client error on connecting socket : %d , %s .\n",
                errno, strerror(errno));
        goto clean;
    }

    envoyer(&connected_socket,&secret_token);
    recevoir(&connected_socket,&secret_key);

    closeSocket(&connected_socket);

    connected_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_port = htons(CHECKER_TCP_PORT);

    if (connect(connected_socket, (struct sockaddr *)&server_address,
                sizeof(server_address))) {
        fprintf(stderr, "Client error on connecting socket : %d , %s .\n",
                errno, strerror(errno));
        goto clean;
    }

    envoyer(&connected_socket,&secret_token);
    envoyer(&connected_socket,&secret_key);

    closeSocket(&connected_socket);

    clean:
    close(connected_socket);
    exit(EXIT_FAILURE);

};