#include "tcp.h"

int find_crlf(const char *buff, ssize_t length)
{
    for (int i = 0; i < length - 1; i++)
    {
        if (buff[i] == '\r')
        {
            int result = 1;
            if (i + 1 < length && buff[i + 1] == '\n')
                result = 0;
            return result;
        }
    }
    return -1;
}

void free_params(char **buff_copy, char **param1, char **param2, char **param3)
{
    if (*buff_copy != NULL) free(*buff_copy);
    if (*param1 != NULL) free(*param1);
    if (*param2 != NULL) free(*param2);
    if (*param3 != NULL) free(*param3);
     
    *buff_copy = NULL;
    *param1 = NULL;
    *param2 = NULL;
    *param3 = NULL;
}

enum message_type check_message_type(char *buff, char **param1, char **param2, char **param3)
{
    enum message_type msg_type = UKNOWN;
    char *buff_copy = strdup(buff);

    if (!tcp_check_auth(buff_copy, param1, param2, param3))
    {
        free(buff_copy);
        return AUTH;
    } 
    free_params(&buff_copy, param1, param2, param3);
    buff_copy = strdup(buff);
    if (!tcp_check_msg(buff_copy, param1, param2))
    {
        free(buff_copy);
        return MSG;
    } 
    free_params(&buff_copy, param1, param2, param3);
    buff_copy = strdup(buff);
    if (!tcp_check_join(buff_copy, param1, param2))
    {
        free(buff_copy);
        return JOIN;
    } 
    free_params(&buff_copy, param1, param2, param3);
    buff_copy = strdup(buff);
    if (!tcp_check_err(buff_copy, param1, param2))
    {
        free(buff_copy);
        return ERR;
    } 
    free_params(&buff_copy, param1, param2, param3);
    buff_copy = strdup(buff);
    if (!tcp_check_bye(buff_copy))
    {
        free(buff_copy);
        return BYE;
    } 
    free_params(&buff_copy, param1, param2, param3);
    return msg_type;
}

int tcp_check_auth(char *buff, char **username, char **display_name, char **secret)
{
    char *token;

    token = strtok(buff, " ");
    if (token == NULL || strcasecmp(token, "AUTH") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL) return 1;

    *username = strdup(token);

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "AS") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL) return 1;

    *display_name = strdup(token);

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "USING") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token == NULL) return 1;
    
    *secret = strdup(token);

    return 0;
}

int tcp_check_join(char *buff, char **channel_id, char **display_name)
{
    char *token;

    token = strtok(buff, " ");
    if (token == NULL || strcasecmp(token, "JOIN") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL) return 1;

    *channel_id = strdup(token);

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "AS") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token == NULL) return 1;
    
    *display_name = strdup(token);

    return 0;
}

int tcp_check_msg(char *buff, char **display_name, char **message)
{
    char *token;

    token = strtok(buff, " ");
    if (token == NULL || strcasecmp(token, "MSG") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "FROM") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL) return 1;

    *display_name = strdup(token);

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "IS") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token == NULL) return 1;

    *message = strdup(token);
    
    return 0;
}

int tcp_check_err(char *reply, char **display_name, char **message)
{
    char *token;

    token = strtok(reply, " ");
    if (token == NULL || strcasecmp(token, "ERR") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "FROM") != 0) return 1;

    token = strtok(NULL, " ");
    if (token == NULL) return 1;

    *display_name = token;

    token = strtok(NULL, " ");
    if (token == NULL || strcasecmp(token, "IS") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token == NULL) return 1;

    *message = token;

    return 0;
}

int tcp_check_bye(char *reply)
{
    char *token;

    token = strtok(reply, "\r\n");
    if (token == NULL || strcasecmp(token, "BYE") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token != NULL && strlen(token) > 0) return 1;

    return 0;
}

int content_response(char **buff, char *reply_status, char *message)
{
    size_t length = strlen(reply_status) + strlen(message) + 13;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "REPLY %s IS %s\r\n", reply_status, message);
    return 0;
}

int content_err(char **buff, char *display_name, char *message)
{
    size_t length = strlen(display_name) + strlen(message) + 16;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "ERR FROM %s IS %s\r\n", display_name, message);
    return 0;
}

int content_joined_msg(char **buff, char *display_name, char *channel)
{
    size_t length = strlen(display_name) + strlen(channel) + 38;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERROR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "MSG FROM Server IS %s has joined %s\r\n", display_name, channel);
    return 0;
}

int content_left_msg(char **buff, char *display_name, char *channel)
{
    size_t length = strlen(display_name) + strlen(channel) + 32;
    *buff = (char *) malloc(length);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERROR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, length, "MSG FROM Server IS %s has left %s\r\n", display_name, channel);
    return 0;
}

int content_bye(char **buff)
{
    *buff = (char *) malloc(6);

    if (*buff == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*buff, 6, "BYE\r\n");
    return 0;
}