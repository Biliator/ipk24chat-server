#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <errno.h>
#include "client.h"

#define DEFAULT_CONF_TIMEOUT 250
#define DEFAULT_MAX_RETRANSMISSIONS 3
#define DEFAULT_SERVER_PORT 4567
#define DEFAULT_SERVER_IP "0.0.0.0"
#define BUFFER_SIZE 3000
#define MAX_EVENTS 10

volatile sig_atomic_t received_signal = 0;

void print_help();
void client_end(Client **clients, int comm_socket, int epoll_fd);
int send_msg_to_clients(Client *clients, int sender_socket, char *channel, const char *message, enum message_type msg_type);
int print_addr_port(int comm_socket, char *recv_send, const char *message_type);
void connect_sockets(char *ip_address, int port);