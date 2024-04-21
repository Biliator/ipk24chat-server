#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "tcp.h"
#include "udp.h"

enum State
{
    START,
    OPEN,
    ERROR,
    END
};

typedef struct
{
    int socket;
    int protocol;
    uint8_t lsb;
    uint8_t msb;
    char *channel;
    char *username;
    char *display_name;
    char *secret;
    char *msg_buff;
    enum State state;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
} Data;

typedef struct Client
{
    Data data;
    struct Client *next;
} Client;

int add_client(Client **clients, int socket, int protocol);
Client *search_client(Client **clients, int socket);
int search_client_name(Client *clients, char *display_name);
void remove_client(Client **clients, int socket);
void free_clients(Client **clients);
int modify_client_buff(Client **client, const char *buff, size_t buff_len);
int next_state(Client *clients, Client *client, char *buff, char **response, enum message_type *msg_type);

int next_state_udp(Client *clients, Client *client, char *buff, char **response, size_t *response_length, enum message_type *msg_type);