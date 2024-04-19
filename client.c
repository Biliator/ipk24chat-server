#include "client.h"

int add_client(Client **clients, int socket, int protocol)
{
    Client *new_client = (Client *) malloc(sizeof(Client));
    if (new_client == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed\n");
        return 1;
    }

    new_client->data.socket = socket;
    new_client->data.protocol = protocol;
    new_client->data.channel = strdup("default");
    new_client->data.state = START;
    new_client->data.username = NULL;
    new_client->data.display_name = NULL;
    new_client->data.secret = NULL;
    new_client->data.msg_buff = NULL;
    new_client->next = NULL;

    if (*clients == NULL)
    {
        *clients = new_client;
        return 0;
    }

    Client *current = *clients;
    while (current->next != NULL)
        current = current->next;

    current->next = new_client;
    return 0;
}

Client *search_client(Client **clients, int socket)
{
    Client *current = *clients;
    while (current != NULL)
    {
        if (current->data.socket == socket) 
            return current;
        current = current->next;
    }
    return NULL;
}

int search_client_name(Client *clients, char *username)
{
    Client *current = clients;
    while (current != NULL)
    {
        if (current->data.username != NULL)
            if (!strcmp(current->data.username, username)) 
                return 1;
        current = current->next;
    }
    return 0;
}

void remove_client(Client **clients, int socket)
{
    Client *current = *clients;
    Client *prev = NULL;

    while (current != NULL)
    {
        if (current->data.socket == socket) 
        {
            if (prev == NULL) 
                *clients = current->next;
            else 
                prev->next = current->next;
            if (current->data.channel != NULL) free(current->data.channel);
            if (current->data.username != NULL) free(current->data.username);
            if (current->data.display_name != NULL) free(current->data.display_name);
            if (current->data.secret != NULL) free(current->data.secret);
            if (current->data.msg_buff != NULL) free(current->data.msg_buff);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void free_clients(Client **clients)
{
    Client *current = *clients;
    while (current != NULL)
    {
        Client *temp = current;
        current = current->next;
        if (temp->data.channel != NULL) free(temp->data.channel);
        if (temp->data.username != NULL) free(temp->data.username);
        if (temp->data.display_name != NULL) free(temp->data.display_name);
        if (temp->data.secret != NULL) free(temp->data.secret);
        if (temp->data.msg_buff != NULL) free(temp->data.msg_buff);
        free(temp);
    }
    *clients = NULL;
}

int next_state(Client *clients, Client *client, char *buff, char **response, enum message_type *msg_type)
{
    char *param1 = NULL;
    char *param2 = NULL;
    char *param3 = NULL;
    *msg_type = check_message_type(buff, &param1, &param2, &param3);
    int result = 0;
    
    if (*msg_type == UKNOWN)
        client->data.state = ERROR;

    switch (client->data.state)
    {
    case START:
        if (*msg_type == AUTH)
        {
            if (!search_client_name(clients, param1))
            {
                client->data.state = OPEN;
                client->data.username = strdup(param1);
                client->data.display_name = strdup(param2);
                client->data.secret = strdup(param3);
                result = content_response(response, "OK", "Auth success.");
                if (param1 != NULL) free(param1);
                if (param2 != NULL) free(param2);
                if (param3 != NULL) free(param3);
            }
            else
            {
                if (param1 != NULL) free(param1);
                if (param2 != NULL) free(param2);
                if (param3 != NULL) free(param3);
                result = content_response(response, "NOK", "Auth fail.");
            }
        }
        else if (*msg_type == BYE)
        {
            result = -2;
        }
        else
        {
            result = content_err(response, "Server", "Forbiden action!");
        }
        break;
    case OPEN:
        if (*msg_type == MSG)
        {
            if (param1 != NULL) free(param1);
            if (param2 != NULL) free(param2);
            result = -1;
        }
        else if (*msg_type == JOIN)
        {
            if (client->data.channel != NULL) free(client->data.channel);
            client->data.channel = strdup(param1);
            result = content_response(response, "OK", "Join success.");
            if (param1 != NULL) free(param1);
            if (param2 != NULL) free(param2);
        }
        else if (*msg_type == ERR)
        {
            result = content_bye(response);
        }
        else if (*msg_type == BYE)
        {
            result = -2;
        }
        else
        {
            result = content_err(response, "Server", "Forbiden action!");
        }
        break;
    case ERROR:
        result = content_err(response, "Server", "Uknown message!");
        break;
    default:
        break;
    }
    return result;
}
