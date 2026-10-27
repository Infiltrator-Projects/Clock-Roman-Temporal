// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file compiler.h
 * @brief Portable compiler annotations shared by The Infiltratr C projects.
 *
 * All annotations degrade to behaviour-preserving no-ops on compilers that do
 * not expose the corresponding GNU-style facility. They may therefore be used
 * in public headers without making optimisation hints part of program logic.
 *
 * @author Shannon Smith
 * @copyright Copyright (c) 1993-2026 Shannon Smith
 * @license GPL-3.0-or-later
 */
#ifndef INFILTRATR_COMMON_COMPILER_H
#define INFILTRATR_COMMON_COMPILER_H

#if defined(__GNUC__) || defined(__clang__)
/** Branch-probability hint for conditions expected to be true. */
#define INFILTRATR_LIKELY(expression) __builtin_expect(!!(expression), 1)
/** Branch-probability hint for conditions expected to be false. */
#define INFILTRATR_UNLIKELY(expression) __builtin_expect(!!(expression), 0)
/** Mark an infrequently executed function for cold-path optimisation. */
#define INFILTRATR_COLD __attribute__((cold))
/** Request compile-time printf-style format checking for a function. */
#define INFILTRATR_PRINTF_FORMAT(format_index, first_argument) \
    __attribute__((format(printf, format_index, first_argument)))
#else
#define INFILTRATR_LIKELY(expression) (!!(expression))
#define INFILTRATR_UNLIKELY(expression) (!!(expression))
#define INFILTRATR_COLD
#define INFILTRATR_PRINTF_FORMAT(format_index, first_argument)
#endif

#endif
