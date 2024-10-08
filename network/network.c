#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>

#include"network.h"
#include"../util/types.h"
#include"../log/log.h" 

int network_init(uint16_t port, uint32_t addr, int *sd, size_t max_connect){
    
    *sd = socket(AF_INET, SOCK_STREAM, 0);

    if(*sd == -1){
        log_error(LOG_USE_FILE_LINE, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(*sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        log_error(LOG_USE_FILE_LINE, "Failed to bind socket: %s", strerror(errno));
        return -1;
    }

    if(listen(*sd, max_connect) == -1){
        log_error(LOG_USE_FILE_LINE, "Failed to listen on socket: %s", strerror(errno));
        return -1;
    }

    return 0;

}

int network_accept(int sd, struct sockaddr_in *client_addr, socklen_t *addrlen){
    int client_sd;
    
    if((client_sd = accept(sd, (struct sockaddr*)client_addr, addrlen)) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to accept connection: %s", strerror(errno));
        return -1;
    }
    return client_sd;
}

int network_send(int sd, network_packet_t pkt){
    if(pkt.len == 0){
        return 0;
    }

    if(send(sd, pkt.data, pkt.len, 0) != pkt.len){
        log_error(LOG_USE_FILE_LINE, "Failed to send data: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int network_recv(int sd, network_packet_t **pkt){
    
    *pkt = malloc(sizeof(network_packet_t));    // allocate memory for packet, it's freed in netwrok_free_packet()
    if(*pkt == NULL){
        log_error(LOG_USE_FILE_LINE, "Failed to allocate memory for packet");
        return -1;
    }

    char data[5] = {0};       // 5 is the maximum length of a varint
    char *ptr = data;   // pointer to the current position in the data buffer
    int ret = recv(sd, data, sizeof(data), MSG_PEEK);

    if(ret < 0){    // peek at the first 5 bytes to get the length of the packet
        log_error(LOG_USE_FILE_LINE, "Failed to receive data: %s", strerror(errno));
        log_error(LOG_USE_FILE_LINE, "ret = %d", ret);
        goto clean_packet;
    }

    if(varint_read(&ptr, &(*pkt)->len) < 0){    // read the length of the packet
        log_error(LOG_USE_FILE_LINE, "Failed to read packet length");
        goto clean_packet;
    }

    if(ptr <= data){
        goto clean_packet;
    }
    (*pkt)->len += ptr - data;
    (*pkt)->data = malloc((*pkt)->len);   // allocate memory for the packet data

    if((*pkt)->data == NULL){   // if allocation fails, free the packet and return
        log_error(LOG_USE_FILE_LINE, "Failed to allocate memory for packet data");
        goto clean_packet;
    }

    ret = recv(sd, (*pkt)->data, (*pkt)->len, 0);    // read the packet data

    if(ret == (*pkt)->len){    // if all the data was read successfully, return
        return 0;
    }

    if(ret < 0){    // if there was an error or the connection was closed, free the packet and returns
        log_error(LOG_USE_FILE_LINE, "Failed to receive data: %s", strerror(errno));
        log_error(LOG_USE_FILE_LINE, "ret = %d", ret);
    }
    free((*pkt)->data);   // free the packet data if there was an error or the connection was closed
clean_packet:
    free(*pkt); 
    *pkt = NULL; 
    return -1; 
}

void network_free_packet(network_packet_t *pkt){
    if(pkt == NULL){    
        return;
    }
    if(pkt->data != NULL){
        free(pkt->data);
    }
    free(pkt);
    return;
}

void network_close(int sd){
    close(sd);
    return;
}