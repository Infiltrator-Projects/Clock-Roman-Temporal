// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file i18n.h
 * @brief Allocation-free localisation catalogue lookup and interpolation.
 *
 * Applications compile catalogues into immutable tables and use one portable
 * lookup engine on every platform. Locale parsing is ASCII-deterministic:
 * underscores become hyphens, POSIX encoding/modifier suffixes are ignored,
 * language/script/region casing is normalised, and malformed or overlong tags
 * are rejected rather than silently truncated.
 */
#ifndef INFILTRATR_COMMON_I18N_H
#define INFILTRATR_COMMON_I18N_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFILTRATR_I18N_LOCALE_CAPACITY 32U

/** Immutable key/value entry; both pointers must remain valid with its catalogue. */
typedef struct {
    const char *key;
    const char *value;
} InfiltratrI18nEntry;

/** Immutable locale catalogue backed by caller-owned process-lifetime storage. */
typedef struct {
    const char *locale;
    const InfiltratrI18nEntry *entries;
    size_t entry_count;
} InfiltratrI18nCatalog;

/** Named interpolation argument used by infiltratr_i18n_format(). */
typedef struct {
    const char *name;
    const char *value;
} InfiltratrI18nArgument;

/** Allocation-free selection context; catalogue storage remains caller-owned. */
typedef struct {
    const InfiltratrI18nCatalog *catalogs;
    size_t catalog_count;
    char locale[INFILTRATR_I18N_LOCALE_CAPACITY];
    char fallback_locale[INFILTRATR_I18N_LOCALE_CAPACITY];
} InfiltratrI18n;

/**
 * Initialise a localisation context and canonical fallback locale.
 *
 * The catalogue table is borrowed, not copied. Every catalogue must have a
 * valid locale; a non-zero entry count requires entry storage, and every entry
 * must contain non-NULL key/value pointers. Malformed catalogue/fallback input
 * is rejected without establishing a usable context.
 */
bool infiltratr_i18n_init(InfiltratrI18n *context,
                          const InfiltratrI18nCatalog *catalogs,
                          size_t catalog_count,
                          const char *fallback_locale);

/**
 * Select a locale without mutating the selected locale on malformed input.
 * Returns true when a catalogue matches the requested language. A valid but
 * unavailable locale is retained so normal lookup can fall back canonically.
 */
bool infiltratr_i18n_set_locale(InfiltratrI18n *context, const char *locale);

/** Return the canonical selected locale stored by the context. */
const char *infiltratr_i18n_locale(const InfiltratrI18n *context);

/**
 * Resolve a key through exact locale, language and configured fallback order.
 * Returned strings alias immutable catalogue storage; missing keys fall back
 * to the supplied key rather than allocating a replacement.
 */
const char *infiltratr_i18n_get(const InfiltratrI18n *context,
                                const char *key);

/**
 * Interpolate named arguments into a caller-owned bounded buffer.
 *
 * The operation never allocates. The returned size is the number of bytes
 * required excluding NUL; callers can therefore detect truncation when the
 * result is greater than or equal to `capacity`. If the mathematical result
 * length exceeds SIZE_MAX, the return value saturates at SIZE_MAX rather than
 * wrapping. Format strings and argument values must not overlap writable
 * destination storage.
 */
size_t infiltratr_i18n_format(char *destination, size_t capacity,
                              const char *format,
                              const InfiltratrI18nArgument *arguments,
                              size_t argument_count);

#ifdef __cplusplus
}
#endif

#endif
