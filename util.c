/*
 * Cinquel String Utilities
 * ANSI C89 compatible
 *
 * Copyright (c) 2026 Andrew C. Young
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include <string.h>
#include <ctype.h>

#include "util.h"

/* Check if a string is empty */
int is_empty(const char *str)
{
    return str[0] == '\0';
}

/* Check if a character is in a string */
int char_in_str(char c, const char *str)
{
    while (*str) {
        if (*str == c) {
            return 1;
        }
        str++;
    }
    return 0;
}

/* Copy string with guaranteed null termination */
void copy_string(char *dest, const char *src, int len)
{
    strncpy(dest, src, len);
    dest[len - 1] = '\0';
}

/* Convert string to lowercase in place */
void to_lower(char *str)
{
    while (*str) {
        *str = tolower((unsigned char)*str);
        str++;
    }
}

/* Strip newline from end of string */
void strip_newline(char *str)
{
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[--len] = '\0';
    }
}
