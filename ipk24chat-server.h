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
#include <errno.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#define DEFAULT_CONF_TIMEOUT 250
#define DEFAULT_MAX_RETRANSMISSIONS 3
#define DEFAULT_SERVER_PORT "4567"
#define BUFFER_SIZE 1500
#define MAX_EVENTS 10

volatile sig_atomic_t received_signal = 0;

void print_help();