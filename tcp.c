#include "tcp.h"

/**
 * @brief check if message ends with '\r\n'
 * 
 * @param buff received message
 * @param length message length
 * @return int returns -1 if message doesn't end with '\r\n', 0 if message ends with just '\r'
 * and 1 if message ends with '\r\n'
 */
int find_crlf(const char *buff, ssize_t length)
{
    for (int i = 0; i < length - 1; i++)
    {
        if (buff[i] == '\r')
        {
            int result = 0;
            if (i + 1 < length && buff[i + 1] == '\n')
                result = 1;
            return result;
        }
    }
    return -1;
}

/**
 * @brief frees params
 * 
 * @param buff_copy 
 * @param param1 
 * @param param2 
 * @param param3 
 */
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

/**
 * @brief check what kind of message was received from client and returns enum corresponding to
 * message type
 * 
 * @param buff received message
 * @param param1 first parameter of message (username, displayName, etc.)
 * @param param2 secend parameter of message (displayName, message)
 * @param param3 third parameter of message (secret)
 * @return enum message_type enum corresponding to message type
 */
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

/**
 * @brief Checks if the AUTH message from the client is in the correct format
 *
 * @param buff message from the server
 * @param username client's userName
 * @param display_name client's displayName
 * @param secret client's password
 * @return int 1 if the shape is wrong, 0 otherwise
 */
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

/**
 * @brief Checks if the JOIN message from the client is in the correct format
 *
 * @param buff message from the server
 * @param channel_id channel client wants to join
 * @param display_name client displayName
 * @return int 1 if the shape is wrong, 0 otherwise
 */
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

/**
 * @brief Checks if the MSG message from the client is in the correct format
 *
 * @param reply message from the client
 * @param display_name message author
 * @param message the message
 * @return int 1 if the shape is wrong, 0 otherwise
 */
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

/**
 * @brief Checks if the ERR message from the client is in the correct form
 *
 * @param reply message from the client
 * @param display_name message author
 * @param message the error message
 * @return int 1 if the shape is wrong, 0 otherwise
 */
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

/**
 * @brief Checks whether the BYE message from the client is in the correct form
 *
 * @param reply message from the client
 * @return int 1 if the shape is wrong, 0 otherwise
 */
int tcp_check_bye(char *reply)
{
    char *token;

    token = strtok(reply, "\r\n");
    if (token == NULL || strcasecmp(token, "BYE") != 0) return 1;

    token = strtok(NULL, "\r\n");
    if (token != NULL && strlen(token) > 0) return 1;

    return 0;
}

/**
 * @brief Builds an REPLY message and stores it in the buff variable
 *
 * @param buff final message
 * @param reply_status OK | NOK
 * @param message response message
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
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

/**
 * @brief Builds the ERR message and stores it in the buff variable
 *
 * @param buff final message
 * @param display_name the name to be represented by
 * @param message the error message
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
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

/**
 * @brief Builds an MSG message that informs everyone about someone's connection or if someone joined
 * channel and stores it in the buff variable
 *
 * @param buff final message
 * @param display_name client that left the channel
 * @param channel client channel
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
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

/**
 * @brief Builds an MSG message that informs everyone about someone's disconnection or if someone left
 * channel and stores it in the buff variable
 *
 * @param buff final message
 * @param display_name client that left the channel
 * @param channel client channel
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
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

/**
 * @brief Sestaví BYE zprávy a uloží jí do proměnné buff
 * 
 * @param buff finální zpráva
 * @return int 1 pokud nastala chyba při alokaci, 0 jinak
 */
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