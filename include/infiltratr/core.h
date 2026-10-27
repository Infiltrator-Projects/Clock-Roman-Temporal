// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file core.h
 * @brief Dependency-free C primitives and project identity shared by programs.
 *
 * This interface is C11-compatible and does not depend on GLib, GTK or an
 * operating-system API. Applications may therefore use the same source on
 * Linux and retain a clean boundary for future platform providers.
 *
 * Unless a function documents otherwise, caller-owned output storage remains
 * owned by the caller, no allocation is performed, and functions are
 * re-entrant provided callers do not concurrently modify the same objects.
 *
 * ABI-bearing structures use a size/version prefix. Within one ABI version,
 * implementations require the caller's structure to cover every field defined
 * by that ABI version, but do not require trailing padding or future appended
 * fields. This permits compatible structure extension within the same shared
 * library major version while still rejecting incomplete structures.
 *
 * @author Shannon Smith
 * @copyright Copyright (c) 1993-2026 Shannon Smith
 * @license GPL-3.0-or-later
 */
#ifndef INFILTRATR_COMMON_CORE_H
#define INFILTRATR_COMMON_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFILTRATR_COMMON_VERSION "1.19.78"
#define INFILTRATR_PROJECT_INFO_ABI 1U
#define INFILTRATR_SCALE_OPTIONS_ABI 1U
#define INFILTRATR_ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

/**
 * Stable, toolkit-neutral project identity consumed by application adapters.
 *
 * `struct_size` and `abi_version` form the ABI header. Version 1 validation
 * requires `struct_size` to cover all version-1 fields through
 * `copyright_text`; trailing structure padding and future appended fields are
 * not required. `abi_version` must equal `INFILTRATR_PROJECT_INFO_ABI`.
 * Every string field is required and must point to a non-empty NUL-terminated
 * string for a record to be considered valid.
 */
typedef struct {
    size_t struct_size;
    uint32_t abi_version;
    const char *program_name;
    const char *executable_name;
    const char *application_id;
    const char *version;
    const char *source_id;
    const char *build_profile;
    const char *author;
    const char *website;
    const char *license_id;
    const char *comments;
    const char *icon_name;
    const char *copyright_text;
} InfiltratrProjectInfo;

/** Initialise a project record while keeping added ABI fields zeroed. */
#define INFILTRATR_PROJECT_INFO_INIT \
    { .struct_size = sizeof(InfiltratrProjectInfo), \
      .abi_version = INFILTRATR_PROJECT_INFO_ABI }

/**
 * Scaling policy used by the canonical shared quantity formatter.
 *
 * `minimum_unit` and `maximum_unit` are inclusive indices into the unit table
 * supplied by the caller. `SIZE_MAX` for `maximum_unit` means the last unit.
 * The formatter can therefore auto-scale, force one unit, or constrain a
 * display to any useful range without duplicating the scaling algorithm.
 *
 * `divisor` must be finite and greater than 1. `decimal_places` is limited to
 * 0..9. `integer_threshold` must be finite and non-negative. ABI version 1
 * requires the structure to cover all fields through
 * `zero_below_minimum_unit`; trailing padding and future appended fields are
 * not required.
 */
typedef struct {
    size_t struct_size;
    uint32_t abi_version;
    long double divisor;
    size_t minimum_unit;
    size_t maximum_unit;
    unsigned int decimal_places;
    long double integer_threshold;
    bool integer_at_minimum_unit;
    bool zero_below_minimum_unit;
} InfiltratrScaleOptions;

/** Default binary auto-scaling policy with one fractional digit. */
#define INFILTRATR_SCALE_OPTIONS_INIT \
    { .struct_size = sizeof(InfiltratrScaleOptions), \
      .abi_version = INFILTRATR_SCALE_OPTIONS_ABI, \
      .divisor = 1024.0L, \
      .minimum_unit = 0U, \
      .maximum_unit = SIZE_MAX, \
      .decimal_places = 1U, \
      .integer_threshold = 100.0L, \
      .integer_at_minimum_unit = true, \
      .zero_below_minimum_unit = false }

/** Validate the ABI prefix and every required non-empty project-info string. */
bool infiltratr_project_info_is_valid(const InfiltratrProjectInfo *info);

/**
 * Print stable machine-readable project identity as `name=value` lines.
 * Control bytes in string fields are replaced by spaces. Returns zero on
 * complete output or -1 for invalid input/stream failure.
 */
int infiltratr_project_info_print(FILE *stream,
                                  const InfiltratrProjectInfo *info);

/**
 * Return the canonical human-readable label for a shared build profile.
 *
 * Canonical profiles are `generic`, `native`, `cmake`, `source` and
 * `development`. Unknown, NULL and empty profiles deliberately map to the
 * development label so presentation code never invents a second profile
 * vocabulary.
 */
const char *infiltratr_build_profile_label(const char *profile);

/** Overlap-safe bounded copy that treats NULL source as empty and always NUL-terminates valid storage. */
void infiltratr_copy_string(char *destination, size_t size,
                            const char *source);
/** Trim leading/trailing C-library whitespace from a mutable string in place. */
void infiltratr_trim(char *text);
/** Remove trailing CR/LF bytes only, preserving all other whitespace. */
void infiltratr_trim_line_end(char *text);
/** NULL-safe equality: two NULL pointers compare equal; NULL and text do not. */
bool infiltratr_string_equal(const char *left, const char *right);
/** Return true when `text` begins with `prefix`; NULL input is false. */
bool infiltratr_string_starts_with(const char *text, const char *prefix);
/** Return true when `text` ends with `suffix`; NULL input is false. */
bool infiltratr_string_ends_with(const char *text, const char *suffix);

/** Return true only for the six ASCII whitespace bytes. */
bool infiltratr_ascii_is_space(unsigned char value);
/** Return true only for ASCII A-Z or a-z. */
bool infiltratr_ascii_is_alpha(unsigned char value);
/** Return true only for ASCII 0-9. */
bool infiltratr_ascii_is_digit(unsigned char value);
/** Return true only for ASCII letters or decimal digits. */
bool infiltratr_ascii_is_alnum(unsigned char value);
/** Return true only for ASCII hexadecimal digits 0-9/A-F/a-f. */
bool infiltratr_ascii_is_xdigit(unsigned char value);
/** Convert one ASCII byte to lowercase; all non-A-Z bytes are unchanged. */
unsigned char infiltratr_ascii_to_lower(unsigned char value);
/** Convert one ASCII byte to uppercase; all non-a-z bytes are unchanged. */
unsigned char infiltratr_ascii_to_upper(unsigned char value);
/**
 * Compare complete NUL-terminated strings using deterministic ASCII-only
 * case folding. Two NULL pointers compare equal; NULL and text do not.
 */
bool infiltratr_ascii_equal_ci(const char *left, const char *right);
/** Return true when text begins with prefix using ASCII-only case folding. */
bool infiltratr_ascii_starts_with_ci(const char *text, const char *prefix);
/**
 * Lexically compare complete NUL-terminated strings using deterministic
 * ASCII-only case folding. The result is negative, zero or positive using
 * unsigned-byte ordering after folding A-Z to a-z. Two NULL pointers compare
 * equal; NULL sorts before non-NULL text.
 */
int infiltratr_ascii_compare_ci(const char *left, const char *right);
/** Return true when needle occurs in text using ASCII-only case folding. */
bool infiltratr_ascii_contains_ci(const char *text, const char *needle);

/** Standard 64-bit FNV-1a offset basis for a new stable hash stream. */
#define INFILTRATR_FNV1A64_OFFSET_BASIS UINT64_C(14695981039346656037)

/**
 * Mix one byte into a 64-bit FNV-1a stream.
 *
 * This is a stable, non-cryptographic identity/signature hash. It must not be
 * used for authentication, adversarial collision resistance or secrets.
 */
uint64_t infiltratr_fnv1a64_mix_byte(uint64_t hash, unsigned char value);
/**
 * Mix one NUL-terminated byte string into a 64-bit FNV-1a stream.
 *
 * The terminating NUL is not included. NULL is treated as an empty string.
 */
uint64_t infiltratr_fnv1a64_mix_text(uint64_t hash, const char *text);
/**
 * Mix one uint64 value in explicit little-endian byte order into FNV-1a.
 *
 * Explicit byte order makes the result stable across host architectures.
 */
uint64_t infiltratr_fnv1a64_mix_u64_le(uint64_t hash, uint64_t value);

/**
 * Parse a complete unsigned integer in base 0 or 2..36.
 * Surrounding C-library whitespace is accepted; either sign is rejected.
 * Caller output is unchanged on syntax/range failure.
 */
bool infiltratr_parse_u64(const char *text, unsigned int base,
                          uint64_t *value);
/**
 * Parse a complete signed integer in base 0 or 2..36.
 * Surrounding C-library whitespace and an optional sign are accepted. Caller
 * output is unchanged on syntax/range failure.
 */
bool infiltratr_parse_i64(const char *text, unsigned int base,
                          int64_t *value);
/** Parse an unsigned integer and require the inclusive caller-supplied range. */
bool infiltratr_parse_u64_range(const char *text, unsigned int base,
                                uint64_t minimum, uint64_t maximum,
                                uint64_t *value);
/** Parse a signed integer and require the inclusive caller-supplied range. */
bool infiltratr_parse_i64_range(const char *text, unsigned int base,
                                int64_t minimum, int64_t maximum,
                                int64_t *value);

/**
 * Parse one complete finite ASCII-decimal value independent of process locale.
 *
 * The grammar accepts surrounding ASCII whitespace, optional sign, integer or
 * fractional digits and an optional decimal exponent. NaN/Inf spellings,
 * hexadecimal floats, trailing data, overflow and underflow to zero are
 * rejected. Conversion to IEEE-754 binary64 is performed with exact integer
 * arithmetic and round-to-nearest, ties-to-even semantics; it does not depend
 * on process locale, host `long double` precision or the active floating-point
 * rounding mode. Input length is not capped: digits beyond the representation-
 * derived rounding guard are consumed and tracked with exact sticky semantics.
 * Caller output is unchanged on failure.
 */
bool infiltratr_parse_double(const char *text, double *value);
/** Parse a finite decimal and require the inclusive non-NaN range. */
bool infiltratr_parse_double_range(const char *text, double minimum,
                                   double maximum, double *value);

/** Clamp a finite ordered range; invalid/NaN range input returns `value` unchanged. */
double infiltratr_clamp_double(double value, double lower, double upper);
/** Checked uint64 addition; caller output is unchanged on overflow/failure. */
bool infiltratr_u64_add_checked(uint64_t left, uint64_t right,
                                uint64_t *result);
/** Saturating uint64 addition. */
uint64_t infiltratr_u64_add_saturating(uint64_t left, uint64_t right);
/** Saturating uint64 multiplication. */
uint64_t infiltratr_u64_multiply_saturating(uint64_t left, uint64_t right);
/** Return `part / whole` as a percentage clamped to 0..100; zero whole maps to 0. */
double infiltratr_percent_u64(uint64_t part, uint64_t whole);
/**
 * Calculate the delta between two samples of a monotonic unsigned counter.
 *
 * Counter rollback returns false. Caller output is unchanged on failure so a
 * domain-specific fallback can be established before calling when required.
 */
bool infiltratr_u64_counter_delta(uint64_t current, uint64_t previous,
                                  uint64_t *delta);
/**
 * Convert a monotonic unsigned counter delta into a rate.
 * Counter rollback, invalid scale/time or an unrepresentable result returns
 * false and writes 0.0 to a valid `rate` output.
 */
bool infiltratr_u64_counter_rate(uint64_t current, uint64_t previous,
                                 long double units_per_count,
                                 double elapsed_seconds, double *rate);

/**
 * Apply a validated scaling policy and return the scaled value/unit index.
 * Both outputs are required and remain unchanged on validation failure.
 */
bool infiltratr_scale_quantity(long double value,
                               const InfiltratrScaleOptions *options,
                               size_t unit_count, long double *scaled_value,
                               size_t *unit_index);
/**
 * Scale and render one finite quantity into caller storage.
 * Valid storage is cleared before processing; false reports invalid policy,
 * missing units or truncation.
 */
bool infiltratr_format_scaled_quantity(long double value,
                                        const char *const *units,
                                        size_t unit_count,
                                        const char *suffix,
                                        const InfiltratrScaleOptions *options,
                                        char *buffer, size_t buffer_size);
/** Binary-scale bytes using the compact B/KB/MB/GB/TB project display labels. */
char *infiltratr_format_bytes(uint64_t bytes, char *buffer,
                              size_t buffer_size);
/** Binary-scale a finite non-negative byte rate; invalid/negative input maps to zero. */
char *infiltratr_format_rate(double bytes_per_second, char *buffer,
                             size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
