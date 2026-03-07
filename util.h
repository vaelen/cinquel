/*
 * Cinquel String Utilities
 * ANSI C89 compatible
 *
 * Copyright (c) 2026 Andrew C. Young
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef UTIL_H
#define UTIL_H

int is_empty(const char *str);
int char_in_str(char c, const char *str);
void copy_string(char *dest, const char *src, int len);
void to_lower(char *str);
void strip_newline(char *str);

#endif
