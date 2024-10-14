#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include<signal.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#include"log/log.h"
#include"util/thread_pool.h"
#include"util/packets.h"
#include"util/queue.h"
#include"util/base64.h"
#include"util/config.h"

#ifdef DEBUG
 #define pmsg(msg) log_debug(LOG_USE_FILE_LINE, msg)
#else
 #define pmsg(msg)
#endif

bool enable_all_protocols = true;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
queue_t *cnnts;
thrdpool_t *tpool;
int sockfd;
const char *def_motd = "a C language SLP server";
const char *def_name = "MC SLP Server";
config_t defcfg;

void set_lock(bool lock, void *udata){
    switch(lock){
        case true:
            pthread_mutex_lock(udata);
            break;
        case false:
            pthread_mutex_unlock(udata);
            break;
    }
}

void get_stop(){
    char buf[128];
    while(1){
        if(fgets(buf, 128, stdin) == NULL){
            log_fatal(LOG_USE_FILE_LINE, "Failed to read from stdin");
            continue;
        }
        if(strcmp(buf, "stop\n") == 0){            
            exit(0);;
        }
    }
}

void handle_client(){
    network_packet_t *recv_packet, *send_packet;
    handshake_packet_t hs_packet;
    status_response_packet_t status_packet;
    struct timeval timeout = {.tv_sec = 5,.tv_usec = 0};
    config_t cfg = defcfg;
    char *json;

    int client_sockfd = queue_dequeue(cnnts);
    if(client_sockfd < 3) return;

    if(setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout, sizeof(struct timeval)) != 0){
        log_error(LOG_USE_FILE_LINE, "Failed to set socket timeout");
        network_close(client_sockfd);
        return;
    }

    /* Handshake */
    pmsg("Handling client connection");
    if(network_recv(client_sockfd, &recv_packet) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to receive packet");
        network_close(client_sockfd);
        return;
    }

    if(handshake_packet_read(*recv_packet, &hs_packet) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to read handshake packet");
        network_close(client_sockfd);
        return;
    }

    network_free_packet(recv_packet);

    /* Status Check */
    pmsg("Checking status");
    if(hs_packet.next_state != 0x01){
        log_error(LOG_USE_FILE_LINE, "Invalid next state");
        network_close(client_sockfd);
        return;
    }

    /* Check request */
    pmsg("Checking request");
    if(network_recv(client_sockfd, &recv_packet) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to receive packet");
        network_close(client_sockfd);
        return;
    }

    if(status_request_packet_check(*recv_packet)){
        log_fatal(LOG_USE_FILE_LINE, "Invalid status request packet");
        network_close(client_sockfd);
        return;
    }
    network_free_packet(recv_packet);

    /* Send status response */
    pmsg("Sending status response");
    if(enable_all_protocols) cfg.protocol = hs_packet.protocol_version;

    if(config2json(cfg, &json) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to convert config to json");
        network_close(client_sockfd);
        return;
    }

    if(strlen(json) > sizeof(status_packet.json_response)){
        log_error(LOG_USE_FILE_LINE, "JSON string too long");
        network_close(client_sockfd);
        free(json);
        return;
    }

    strcpy(status_packet.json_response, json);

    if(status_response_packet_write(&send_packet, status_packet) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to write status response packet");
        network_close(client_sockfd);
        free(json);
        return;
    }

    if(network_send(client_sockfd, *send_packet) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to send packet");
        network_close(client_sockfd);
        network_free_packet(send_packet);
        free(json);
        return;
    }
    free(json);
    network_free_packet(send_packet);

    /* Ping-Pong */
    pmsg("Ping-Pong");
    if(network_recv(client_sockfd, &recv_packet) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to receive packet");
        network_close(client_sockfd);
        return;
    }

    if(network_send(client_sockfd, *recv_packet) < 0){
        log_error(LOG_USE_FILE_LINE, "Failed to send packet");
        network_close(client_sockfd);
        network_free_packet(recv_packet);
        return;
    }
    network_free_packet(recv_packet);
    
    pmsg("Closing client connection");
    network_close(client_sockfd);
}

int main(int argc, char const **argv){
    int ch;
    memset(&defcfg, 0, sizeof(config_t));

    uint32_t address = INADDR_ANY;
    uint16_t port = 25565;
    int max_players = 200;
    int online_players = 40;
    defcfg.motd = (char*) def_motd;
    defcfg.name = (char*) def_name;
    char *favicon = NULL;
    struct timeval timeout = {.tv_sec = 5,.tv_usec = 0};

    while((ch = getopt(argc, (char *const *)argv, "a:p:M:O:m:n:i:h:")) != -1){
        switch(ch){
            case 'a':
                pmsg("Setting address");
                address = inet_addr(optarg);
                break;
            case 'O':
                pmsg("Setting online players");
                online_players = atoi(optarg);
                break;
            case 'M':
                pmsg("Setting max players");
                max_players = atoi(optarg);
                break;
            case 'm':
                pmsg("Setting motd");
                defcfg.motd = optarg;
                break;
            case 'p':
                pmsg("Setting port");
                port = (uint16_t)atoi(optarg);
                break;
            case 'i':
                pmsg("Setting favicon");
                FILE *f = fopen(optarg, "rb");
                if(f == NULL){
                    log_fatal(LOG_USE_FILE_LINE, "Failed to open favicon file: %s", optarg);
                    return 1;
                }
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);
                char *tmp_read = malloc(size);
                if(tmp_read == NULL){
                    log_fatal(LOG_USE_FILE_LINE, "Failed to allocate memory for favicon");
                    return 1;
                }
                if(fread(tmp_read, 1, size, f) != size){
                    log_fatal(LOG_USE_FILE_LINE, "Failed to read favicon file");
                    return 1;
                }
                fclose(f);
                int len;
                if(base64_encode(tmp_read, size, &favicon, &len) < 0){
                    log_fatal(LOG_USE_FILE_LINE, "Failed to encode favicon");
                    return 1;
                }
                free(tmp_read);
                break;
            case 'n':
                pmsg("Setting name");
                defcfg.name = optarg;
                break;
            case 'h':
                printf("MC SLP Server\n");
                printf("Usage: [-a address] [-p port] [-M max_players] [-O online_players] [-m motd] [-n name] [-i favicon_file] [-h help]\n");
                return 0;
            case '?':
            default:
                log_fatal(LOG_USE_FILE_LINE, "Invalid option");
                break;
        }
    }

    defcfg.favicon = favicon;
    defcfg.max_players = max_players;
    defcfg.online_players = online_players;

    log_info(LOG_USE_FILE_LINE, "Starting server...");

    cnnts = queue_create();
    if(cnnts == NULL){
        log_fatal(LOG_USE_FILE_LINE, "Failed to create queue");
        return 1;
    }

    if(thrdpool_create(200, &tpool) < 0){
        log_fatal(LOG_USE_FILE_LINE, "Failed to create thread pool");
        return 1;
    }

    if(network_init(port, address, &sockfd, 100) < 0){
        log_fatal(LOG_USE_FILE_LINE, "Failed to initialize network");
        return 1;
    }

    if(thrdpool_add_work((thrdpool_work_t){.arg = NULL, .func = get_stop}, tpool) < 0){
        log_fatal(LOG_USE_FILE_LINE, "Failed to add work to thread pool");
        return 1;
    }

    while(1){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len;
        int client_sockfd;
        
        client_sockfd = network_accept(sockfd, &client_addr, &client_addr_len);
        if(client_sockfd < 0){
            log_fatal(LOG_USE_FILE_LINE, "Failed to accept client connection, client_sockfd=%d", client_sockfd);
            continue;
        }
        
        log_info(LOG_USE_FILE_LINE, "Accepted client connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        queue_enqueue(cnnts, client_sockfd);

        if(thrdpool_add_work((thrdpool_work_t){.arg = NULL, .func = handle_client}, tpool)){
            log_fatal(LOG_USE_FILE_LINE, "Failed to add work to thread pool");
            continue;
        }
    }
}
