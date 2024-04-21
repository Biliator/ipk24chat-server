#include "udp.h"

void message_id_increase(uint8_t *lsb, uint8_t *msb)
{
    if (*msb == 0xFF && *lsb == 0xFF)
        *msb = *lsb = 0x00;
    else if (*lsb == 0xFF)
    {
        *lsb = 0x00;
        (*msb)++;
    }
    else (*lsb)++;
}

int udp_message_next(char input[], char **output, int start, size_t input_size)
{
    int i = start;

    while (i < (int) input_size && input[i] != 0x00)
        i++;

    *output = (char *) malloc((i - start + 1) * sizeof(char));
    if (*output == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    i = start;
    while (i < (int) input_size && input[i] != 0x00)
    {
        (*output)[i - start] = input[i];
        i++;
    }

    (*output)[i - start] = '\0';
    return 0;
}

int check_auth_udp(char *buff, char **param1, char **param2, char **param3)
{
    if (buff[0] != 0x02) return 0;
    if (udp_message_next(buff, param1, 3, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        return -1;
    }
    if (udp_message_next(buff, param2, strlen(*param1) + 4, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        if (*param2 != NULL) free(*param2);
        return -1;
    }
    if (udp_message_next(buff, param3, 3, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        if (*param2 != NULL) free(*param2);
        if (*param3 != NULL) free(*param3);
        return -1;
    }
    if (*param1 == NULL || *param2 == NULL || *param3 == NULL) return 0;
    if (strlen(*param1) <= 0 || strlen(*param2) <= 0 || strlen(*param3) <= 0) return 0;

    return 1;
}

int check_msg_udp(char *buff, char **param1, char **param2)
{
    if (buff[0] != 0x04) return 0;
    if (udp_message_next(buff, param1, 3, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        return -1;
    }
    if (udp_message_next(buff, param2, strlen(*param1) + 4, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        if (*param2 != NULL) free(*param2);
        return -1;
    }

    if (*param1 == NULL || *param2 == NULL) return 0;
    if (strlen(*param1) <= 0 || strlen(*param2) <= 0) return 0;
    return 1;
}

int check_join_udp(char *buff, char **param1, char **param2)
{
    if (buff[0] != 0x03) return 0;
    if (udp_message_next(buff, param1, 3, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        return -1;
    }
    if (udp_message_next(buff, param2, strlen(*param1) + 4, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        if (*param2 != NULL) free(*param2);
        return -1;
    }

    if (*param1 == NULL || *param2 == NULL) return 0;
    if (strlen(*param1) <= 0 || strlen(*param2) <= 0) return 0;
    return 1;
}

int check_err_udp(char *buff, char **param1, char **param2)
{
    if (buff[0] != -2) return 0;
    if (udp_message_next(buff, param1, 3, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        return -1;
    }
    if (udp_message_next(buff, param2, strlen(*param1) + 4, sizeof(buff)))
    {
        if (*param1 != NULL) free(*param1);
        if (*param2 != NULL) free(*param2);
        return -1;
    }

    if (*param1 == NULL || *param2 == NULL) return 0;
    if (strlen(*param1) <= 0 || strlen(*param2) <= 0) return 0;
    return 1;
}

int check_bye_udp(char *buff)
{
    if (buff[0] != -1) return 0;
    return 1;
}

int check_confirm_udp(char *buff)
{
    if (buff[0] != 0) return 0;
    return 1;
}


enum message_type check_message_type_udp(char *buff, char **param1, char **param2, char **param3)
{
    int result = check_auth_udp(buff, param1, param2, param3);
    if (result == -1)
        return INT_ERR;
    else if (result == 1)
        return AUTH;

    if (*param1 != NULL) free(*param1);
    if (*param2 != NULL) free(*param2);
    if (*param3 != NULL) free(*param3);
    *param1 = NULL;
    *param2 = NULL;
    *param3 = NULL;

    result = check_msg_udp(buff, param1, param2);
    if (result == -1)
        return INT_ERR;
    else if (result == 1)
        return MSG;
    
    if (*param1 != NULL) free(*param1);
    if (*param2 != NULL) free(*param2);
    *param1 = NULL;
    *param2 = NULL;
    
    result = check_join_udp(buff, param1, param2);
    if (result == -1)
        return INT_ERR;
    else if (result == 1)
        return JOIN;

    if (*param1 != NULL) free(*param1);
    if (*param2 != NULL) free(*param2);
    *param1 = NULL;
    *param2 = NULL;
    
    result = check_err_udp(buff, param1, param2);
    if (result == -1)
        return INT_ERR;
    else if (result == 1)
        return ERR;

    if (*param1 != NULL) free(*param1);
    if (*param2 != NULL) free(*param2);
    *param1 = NULL;
    *param2 = NULL;

    result = check_bye_udp(buff);
    if (result == 1)
        return BYE;

    result = check_confirm_udp(buff);
    if (result == 1)
        return CONFIRM;
    return UKNOWN;
}

/**
  * @brief Constructs a CONFIRM message and executes it in the buff variable
  *
  * @param content message
  * @param length pointer to the total length of the string, which is initially 0
  * @param lsb least significant bit
  * @param msb the most significant bit
  * @return int 1 if an allocation error occurred, 0 otherwise
  */
int confirm(char **content, size_t *length, uint8_t lsb, uint8_t msb)
{
    *length = 3;
    *content = (char *) malloc(*length + 1);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length + 1, "%c%c%c", 0x00, msb, lsb);
    return 0;
}

int reply(char **content, size_t *length, uint8_t lsb, uint8_t msb, uint8_t result, uint8_t ref_lsb, uint8_t ref_msb, char *message_contents)
{
    *length = strlen(message_contents) + 7;
    *content = (char *) malloc(*length);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length, "%c%c%c%c%c%c%s%c", 0x01, msb, lsb, result, ref_lsb, ref_msb, message_contents, 0x00);
    return 0;
}

/**
 * @brief Builds a MSG message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @param display_name the name to be represented by
 * @param message_contents the message specified by the user
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int msg(char **content, size_t *length, uint8_t lsb, uint8_t msb, char *display_name, char *message_contents)
{
    *length = strlen(display_name) + strlen(message_contents) + 5;
    *content = (char *) malloc(*length);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length, "%c%c%c%s%c%s%c", 0x04, msb, lsb, display_name, 0x00, message_contents, 0x00);
    return 0;
}

/**
 * @brief Builds an MSG message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @param display_name the name to be represented by
 * @param message_contents the error message
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int err(char **content, size_t *length, uint8_t lsb, uint8_t msb, char *display_name, char *message_contents)
{
    *length = strlen(display_name) + strlen(message_contents) + 5;
    *content = (char *) malloc(*length);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length, "%c%c%c%s%c%s%c", 0xFE, msb, lsb, display_name, 0x00, message_contents, 0x00);
    return 0;
}

/**
 * @brief Builds a BYE message and stores it in the buff variable
 *
 * @param content the message
 * @param length pointer to the total length of the string, which is initially 0
 * @param lsb least significant bit
 * @param msb most significant bit
 * @return int 1 if an allocation error occurred, 0 otherwise
 */
int bye(char **content, size_t *length, uint8_t lsb, uint8_t msb)
{
    *length = 3;
    *content = (char *) malloc(*length + 1);

    if (*content == NULL)
    {
        fprintf(stderr, "ERR: Memory allocation failed!\n");
        return 1;
    }

    snprintf(*content, *length + 1, "%c%c%c", 0xFF, msb, lsb);
    return 0;
}
