// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file token.h
 * @brief Allocation-free numeric token parsing for larger record grammars.
 *
 * Unlike the whole-string parsers in core.h, token parsers stop at the first
 * byte that is not part of the number and return that position through the
 * caller's cursor. Leading C-library whitespace is skipped and never becomes
 * part of the token; parsing otherwise allocates nothing.
 */
#ifndef INFILTRATR_COMMON_TOKEN_H
#define INFILTRATR_COMMON_TOKEN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse one unsigned token in base 0 or 2..36.
 *
 * Leading C-library whitespace is skipped. '+'/'-' signs are rejected. On
 * success `*cursor` advances to the
 * first unconsumed byte and `*value` receives the number. Cursor and value are
 * unchanged on invalid input or overflow.
 */
bool infiltratr_parse_u64_token(const char **cursor, unsigned int base,
                                uint64_t *value);

/**
 * Parse one signed token in base 0 or 2..36.
 *
 * Leading C-library whitespace is skipped and an optional sign is accepted.
 * On success `*cursor` advances to the
 * first unconsumed byte and `*value` receives the number. Cursor and value are
 * unchanged on invalid input or overflow.
 */
bool infiltratr_parse_i64_token(const char **cursor, unsigned int base,
                                int64_t *value);

/**
 * Parse one finite ASCII-decimal binary64 token.
 *
 * Leading ASCII whitespace is skipped. When @p allow_sign is true an optional
 * leading '+' or '-' is accepted; otherwise a sign is not part of the token.
 * Integer/fractional digits and an optional decimal exponent are recognised
 * with the same exact, locale-independent conversion and range semantics as
 * infiltratr_parse_double(). Parsing stops at the first byte not belonging to
 * the numeric token. NaN/Inf spellings, hexadecimal floats, malformed
 * exponents, overflow and underflow rounded to zero are rejected.
 *
 * Cursor and value are unchanged on failure.
 */
bool infiltratr_parse_double_token(const char **cursor, bool allow_sign,
                                   double *value);

#ifdef __cplusplus
}
#endif
#endif
