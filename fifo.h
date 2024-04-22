#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct msg_list
{
    char *msg;
    int socket;
    struct msg_list *next;
} msg_list;

msg_list* create_node(char *msg, int socket);
void insert_at_end(msg_list **head, char *msg);
msg_list* remove_from_front(msg_list **head);
void free_fifo(msg_list *head);