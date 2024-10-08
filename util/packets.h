#ifndef UTIL_PACKETS_H
#define UTIL_PACKETS_H

#include<stdbool.h>
#include"types.h"
#include"../network/network.h"

//*  Handshake   *//

/*
Packet: Handshake(client -> server)
    
    Packet ID
        | Protocol    => 0x00
        | Resource    => intentions
    State       => Handshaking
    Recv in     => Server
-------------------------
Packet data:
    protocol_version => varint , you can see https://wiki.vg/Protocol_version_numbers
    server_address   => string (255)
    server_port      => unsigned short
    next_state       => varint (1 byte), 0x01 for Status, 0x02 for Login, 0x03 for Transfer
*/

typedef struct handshake_packet{
    int protocol_version;
    char server_address[255];
    unsigned short server_port;
    int next_state;
}handshake_packet_t;

int handshake_packet_read(network_packet_t packet, handshake_packet_t *handshake);
int handshake_packet_write(network_packet_t **packet, handshake_packet_t handshake);


//*  Status   *//

/*
Packet: Status Request(client -> server)
    
    Packet ID
        | Protocol    => 0x00
        | Resource    => status_request
    State       => Status
    Recv in     => Server
-------------------------
Packet data:
    nothing
*/

int status_request_packet_check(network_packet_t packet);
int status_request_packet_write(network_packet_t **packet);

/*
Packet: Status Response(server -> client)
    
    Packet ID
        | Protocol    => 0x00
        | Resource    => status_response
    State       => Status
    Recv in     => Client
-------------------------
Packet data:
    JSON Response   => String (32767)
*/

typedef struct status_response_packet{
    char json_response[32767];
}status_response_packet_t;

int status_response_packet_read(network_packet_t packet, status_response_packet_t *status_response);
int status_response_packet_write(network_packet_t **packet, status_response_packet_t status_response);

/*
Packet: Ping Request/Pong Response(client <-> server)
    
    Packet ID
        | Protocol    => 0x01
        | Resource    => ping_request/pong_response
    State       => Status
    Recv in     => Both
-------------------------
Packet data:
    payload   => long long

*Note: the pong_response_* functions will not be implemented here
*Note: you can use the same packet for both Ping Request and Pong Response,
    in server: you can send a Pong Response packet to client with the same Ping Request packet, it's ok
      e.g:  network_packet_t *packet;
            network_recv(sock, &packet);
            network_send(sock, *packet);
            network_free_packet(packet);

    in client: you will receive a Pong Response packet and it's same as your Ping Request packet
               you can call ping_pong_check() to check the pong response is same as ping request
*/

typedef struct ping_pong_packet{
    long payload;
}ping_pong_packet_t;

int ping_request_packet_write(network_packet_t **packet, ping_pong_packet_t ping_request);

#endif // UTIL_PACKETS_H