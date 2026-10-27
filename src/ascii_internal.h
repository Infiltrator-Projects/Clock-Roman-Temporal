// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef INFILTRATR_COMMON_ASCII_INTERNAL_H
#define INFILTRATR_COMMON_ASCII_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

static inline bool infiltratr_ascii_space(unsigned char value)
{
    return value == ' ' || value == '\t' || value == '\r' ||
           value == '\n' || value == '\f' || value == '\v';
}

static inline bool infiltratr_ascii_alpha(unsigned char value)
{
    return (value >= 'A' && value <= 'Z') ||
           (value >= 'a' && value <= 'z');
}

static inline bool infiltratr_ascii_digit(unsigned char value)
{
    return value >= '0' && value <= '9';
}

static inline bool infiltratr_ascii_alnum(unsigned char value)
{
    return infiltratr_ascii_alpha(value) || infiltratr_ascii_digit(value);
}

static inline bool infiltratr_ascii_xdigit(unsigned char value)
{
    return infiltratr_ascii_digit(value) ||
           (value >= 'A' && value <= 'F') ||
           (value >= 'a' && value <= 'f');
}

static inline unsigned char infiltratr_ascii_lower(unsigned char value)
{
    return value >= 'A' && value <= 'Z'
        ? (unsigned char)(value + ('a' - 'A')) : value;
}

static inline unsigned char infiltratr_ascii_upper(unsigned char value)
{
    return value >= 'a' && value <= 'z'
        ? (unsigned char)(value - ('a' - 'A')) : value;
}

static inline bool infiltratr_ascii_equal_ci_span(const char *text,
                                                  size_t length,
                                                  const char *candidate)
{
    if (!text || !candidate) return false;
    size_t index = 0U;
    for (; index < length && candidate[index] != '\0'; ++index) {
        const unsigned char left =
            infiltratr_ascii_lower((unsigned char)text[index]);
        const unsigned char right =
            infiltratr_ascii_lower((unsigned char)candidate[index]);
        if (left != right) return false;
    }
    return index == length && candidate[index] == '\0';
}

#endif
