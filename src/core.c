// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file core.c
 * @brief Dependency-free implementation of shared C project primitives.
 *
 * @author Shannon Smith
 * @copyright Copyright (c) 1993-2026 Shannon Smith
 * @license GPL-3.0-or-later
 */
#include "infiltratr/core.h"
#include "infiltratr/compiler.h"
#include "infiltratr/token.h"
#include "ascii_internal.h"

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>

#define INFILTRATR_PROJECT_INFO_ABI1_SIZE \
    (offsetof(InfiltratrProjectInfo, copyright_text) + \
     sizeof(((InfiltratrProjectInfo *)0)->copyright_text))
#define INFILTRATR_SCALE_OPTIONS_ABI1_SIZE \
    (offsetof(InfiltratrScaleOptions, zero_below_minimum_unit) + \
     sizeof(((InfiltratrScaleOptions *)0)->zero_below_minimum_unit))


void infiltratr_copy_string(char *destination, size_t size,
                            const char *source)
{
    if (INFILTRATR_UNLIKELY(!destination || size == 0U)) return;
    if (!source) source = "";

    size_t length = 0U;
    while (length + 1U < size && source[length] != '\0') length++;
    memmove(destination, source, length);
    destination[length] = '\0';
}


/*
 * Correctly rounded binary64 conversion
 * -------------------------------------
 *
 * A binary64 rounding midpoint is an odd integer times 2^-k with k <= 1075.
 * Written exactly in decimal, such a midpoint has fewer than 770 significant
 * digits: the odd integer contributes at most 17 decimal digits and 5^1075
 * contributes fewer than 752. Keeping 800 significant input digits plus a
 * sticky "discarded non-zero" bit is therefore sufficient to decide every
 * binary64 rounding boundary exactly, while still accepting input strings of
 * arbitrary length.
 *
 * The fixed big-integer workspace is also representation-derived rather than
 * an input limit. 800 decimal digits need fewer than 3200 bits; the largest
 * relevant power of five and binary scaling keep all intermediates below 4443
 * bits. 160 32-bit limbs provide 5120 bits of checked workspace.
 */
#define INFILTRATR_DOUBLE_DECIMAL_DIGITS 800U
#define INFILTRATR_DOUBLE_BIG_LIMBS 160U

_Static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 &&
               DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021 &&
               sizeof(double) == sizeof(uint64_t),
               "infiltratr_parse_double requires IEEE-754 binary64 double");

typedef struct {
    uint32_t limbs[INFILTRATR_DOUBLE_BIG_LIMBS];
    size_t length;
} InfiltratrBigUInt;

typedef struct {
    bool negative;
    size_t magnitude;
} InfiltratrDecimalExponent;


