#ifndef NETWORK_H
#define NETWORK_H
#include<sys/socket.h>
#include<netdb.h>

typedef struct network_packet {
    int len;
    void *data;
} network_packet_t;

/*
init a socketfd and bind it to a port with a address,
return -1 if fail, 0 if success

you can use inet_pton to set the address, like this:
    uint32_t addr;
    if (inet_pton(AF_INET, "127.0.0.1", &addr) != 1) {
        perror("inet_pton");
        return -1;
    }
*/
int network_init(uint16_t port, uint32_t addr, int *sd, size_t max_connect);

/*
accept a connection from a client,
return -1 if fail, 0 if success

if you want to get the client's address, you can set the client_addr and addrlen
else, you will set them to NULL
*/
int network_accept(int sd, struct sockaddr_in *client_addr, socklen_t *addrlen);

/*
send a packet to a socket,
return -1 if fail, 0 if success
*/
int network_send(int sd, network_packet_t pkt);

/*
recieve a packet from a socket,
return -1 if fail, 0 if success

the function will malloc memory for the packet,
you should call netwrok_free_packet to free it after use
*/
int network_recv(int sd, network_packet_t **pkt);

/*
free a packet, no return value
you should call this function after use network_recv
*/
void network_free_packet(network_packet_t *pkt);

/*
close a socket, no return value
*/
void network_close(int sd);

#endif /* NETWORK_H */