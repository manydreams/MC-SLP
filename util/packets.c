#include<string.h>
#include<stdlib.h>

#include"../log/log.h"
#include"types.h"
#include"packets.h"

int handshake_packet_read(network_packet_t packet, handshake_packet_t *handshake){
    char *data = packet.data;
    int len, packet_id;
    is_varint_read_success(varint_read(&data, &len));
    is_varint_read_success(varint_read(&data, &packet_id));

    if(packet_id!= 0x00){
    #ifdef DEBUG
        log_debug(LOG_USE_FILE_LINE, "Is not a handshake packet, packet id: %d", packet_id);
    #endif // DEBUG
        return -1;
    }

    is_varint_read_success(varint_read(&data, &handshake->protocol_version));
    if(string_read(&data, handshake->server_address, sizeof(handshake->server_address)) < 0){
    #ifdef DEBUG
        log_debug(LOG_USE_FILE_LINE, "Failed to read server address");
    #endif // DEBUG
        return -1;
    }

    handshake->server_port = ntohs(*(uint16_t*)data);
    data += 2;

    is_varint_read_success(varint_read(&data, &handshake->next_state));

    return packet_id;
}

int handshake_packet_write(network_packet_t **packet, handshake_packet_t handshake){
    char buffer[273] = {0}; // 273 is the maximum size of a handshake packet
    char packlen[5] = {0};
    char *data = buffer;

    size_t len = 0;
    size_t packlen_len = 0;

    is_varint_write_success(varint_write(&data, 0x00)); // packet id

    is_varint_write_success(varint_write(&data, handshake.protocol_version));
    if(string_write(&data, handshake.server_address, strlen(handshake.server_address)) < 0){
    #ifdef DEBUG
        log_debug(LOG_USE_FILE_LINE, "Failed to write server address");
    #endif // DEBUG
        return -1;
    }
    
    *(uint16_t*)data = htons(handshake.server_port);
    data += 2;

    is_varint_write_success(varint_write(&data, handshake.next_state));

    len = data - buffer;
    data = packlen;
    is_varint_write_success(varint_write(&data, len));

    packlen_len = data - packlen;

    *packet = malloc(sizeof(network_packet_t));
    if(*packet == NULL){
        log_error(LOG_USE_FILE_LINE, "Failed to allocate memory for packet");
        return -1;
    }

    (*packet)->len = len + packlen_len;

    (*packet)->data = malloc((*packet)->len);
    
    if((*packet)->data == NULL){
        log_error(LOG_USE_FILE_LINE, "Failed to allocate memory for packet");
        (*packet)->data = NULL;
        return -1;
    }

    data = (*packet)->data;
    memcpy(data, packlen, packlen_len);
    data += packlen_len;
    memcpy(data, buffer, len);

    return 0;
}

int ping_request_packet_write(network_packet_t **packet, ping_pong_packet_t ping_request){
    *packet = malloc(sizeof(network_packet_t));
    if(*packet == NULL){
        log_error(LOG_USE_FILE_LINE, "Memory allocation failed for ping request packet");
        return -1;
    }
    (*packet)->len = 10;
    (*packet)->data = malloc(10);
    if((*packet)->data == NULL){
        log_error(LOG_USE_FILE_LINE, "Memory allocation failed for ping request packet data");
        free(*packet);
        return -1;
    }
    ((char*)(*packet)->data)[0] = 0x09;
    ((char*)(*packet)->data)[1] = 0x01;
    ((long*)((*packet)->data+2))[0] = ping_request.payload;
    return (*packet)->len;
}

int status_request_packet_check(network_packet_t packet){
    char *data = packet.data;
    int len, packet_id;
    is_varint_read_success(varint_read(&data, &len));
    is_varint_read_success(varint_read(&data, &packet_id));
    if(packet_id!= 0x00){   // packet id should be 0x00 for status request
        log_warn(LOG_USE_FILE_LINE, "Is not a status request, packet id: %d", packet_id);
        return -1;
    }
    return packet_id;
}

int status_request_packet_write(network_packet_t **packet){
    const char data[] = "\x01\x00";
    *packet = NULL;
    *packet = malloc(sizeof(network_packet_t));
    if(*packet == NULL){
        log_error(LOG_USE_FILE_LINE, "malloc failed in status_request_packet_write");
        return -1;
    }
    (*packet)->len = sizeof(char)-1;
    (*packet)->data = malloc((*packet)->len);
    if((*packet)->data == NULL){
    #ifdef DEBUG
        log_error(LOG_USE_FILE_LINE, "malloc failed in status_request_packet_write");
    #endif
        free(*packet);
        return -1;
    }
    memcpy((*packet)->data, data, (*packet)->len);
    return 0;
}

int status_response_packet_read(network_packet_t packet, status_response_packet_t *status_response){
    char *data = packet.data;
    int len, packet_id;
    is_varint_read_success(varint_read(&data, &len));
    is_varint_read_success(varint_read(&data, &packet_id));

    if(packet_id!= 0x00){
    #ifdef DEBUG
        log_debug(LOG_USE_FILE_LINE, "Is not a status request, packet id: %d", packet_id);
    #endif // DEBUG
        return -1;
    }

    if(string_read(&data, status_response->json_response, sizeof(status_response->json_response)) < 0){
    #ifdef DEBUG
        log_debug(LOG_USE_FILE_LINE, "Failed to read json response");
    #endif // DEBUG
        return -1;
    }

    return packet_id;
}

int status_response_packet_write(network_packet_t **packet, status_response_packet_t status_response){
    int len = strlen(status_response.json_response);
    if(len > 32767){
        log_error(LOG_USE_FILE_LINE, "Json response too long, max 32767 characters");
        return -1;
    }
    size_t packet_len;
    char pktlen[5] = {0};
    char *data = malloc(len + 5 + 1);
    if(data == NULL){
        log_error(LOG_USE_FILE_LINE, "Failed to allocate memory for status response packet");
        return -1;
    }
    memset(data, 0, len + 5 + 1);
    char *ptr = data;
    if(varint_write(&ptr, 0x00) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to write packet id");
        free(data);
        return -1;
    }
    if(varint_write(&ptr, len) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to write json response length");
        free(data);
        return -1;
    }
    memcpy(ptr, status_response.json_response, len);
    ptr += len;
    packet_len = ptr - data;

    *packet = malloc(sizeof(network_packet_t));
    if(*packet == NULL){
        log_error(LOG_USE_FILE_LINE, "Failed to allocate memory for network packet");
        free(data);
        return -1;
    }
    (*packet)->len = packet_len;
    ptr = pktlen;
    if(varint_write(&ptr, packet_len) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to write packet length");
        free(data);
        free(*packet);
        return -1;
    }
    packet_len = ptr - pktlen;
    (*packet)->len += packet_len;
    (*packet)->data = malloc((*packet)->len);
    if((*packet)->data == NULL){
        log_error(LOG_USE_FILE_LINE, "Failed to allocate memory for network packet data");
        free(data);
        free(*packet);
        return -1;
    }
    ptr = (*packet)->data;
    memcpy(ptr, pktlen, packet_len);
    ptr += packet_len;
    memcpy(ptr, data, (*packet)->len - packet_len);
    free(data);
    return 0;
}