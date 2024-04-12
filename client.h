#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcp.h"

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
    char *channel;
    char *username;
    char *display_name;
    char *secret;
    enum State state;
} Data;

typedef struct Client
{
    Data data;
    struct Client *next;
} Client;

int add_client(Client **clients, int socket, int protocol);
Client *search_client(Client **clients, int socket);
void remove_client(Client **clients, int socket);
void free_clients(Client **clients);
int next_state(Client **clients, int socket, char *buff, char **response);