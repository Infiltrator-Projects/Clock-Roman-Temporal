// SPDX-License-Identifier: GPL-3.0-or-later
#include "infiltratr/i18n.h"
#include "infiltratr/core.h"
#include "ascii_internal.h"

#include <string.h>

static size_t normalise_locale(char *destination, size_t capacity,
                               const char *source)
{
    if (!destination || capacity == 0U) return 0U;
    destination[0] = '\0';
    if (!source || !source[0]) return 0U;

    size_t out = 0U;
    size_t segment_length = 0U;
    for (size_t i = 0U; source[i] != '\0'; ++i) {
        const unsigned char c = (unsigned char)source[i];
        if (c == '.' || c == '@') break;
        if (c == '-' || c == '_') {
            if (segment_length == 0U || out + 1U >= capacity) {
                destination[0] = '\0';
                return 0U;
            }
            destination[out++] = '-';
            segment_length = 0U;
            continue;
        }
        if (!infiltratr_ascii_alnum(c) || out + 1U >= capacity) {
            destination[0] = '\0';
            return 0U;
        }
        destination[out++] = infiltratr_ascii_lower(c);
        segment_length++;
    }
    if (segment_length == 0U) {
        destination[0] = '\0';
        return 0U;
    }
    destination[out] = '\0';

    size_t start = 0U;
    while (start < out) {
        size_t end = start;
        while (end < out && destination[end] != '-') end++;
        const size_t length = end - start;
        if (start != 0U) {
            bool all_alpha = true;
            for (size_t i = start; i < end; ++i)
                if (!infiltratr_ascii_alpha((unsigned char)destination[i])) all_alpha = false;
            if (all_alpha && length == 2U) {
                for (size_t i = start; i < end; ++i)
                    destination[i] = infiltratr_ascii_upper((unsigned char)destination[i]);
            } else if (all_alpha && length == 4U) {
                destination[start] = infiltratr_ascii_upper((unsigned char)destination[start]);
            }
        }
        start = end + 1U;
    }
    return out;
}

static size_t language_length(const char *locale)
{
    size_t length = 0U;
    if (!locale) return 0U;
    while (locale[length] != '\0' && locale[length] != '-') ++length;
    return length;
}

static int same_language(const char *left, const char *right)
{
    const size_t left_length = language_length(left);
    const size_t right_length = language_length(right);
    if (left_length == 0U || left_length != right_length) return 0;
    return strncmp(left, right, left_length) == 0;
}

static const InfiltratrI18nCatalog *find_catalog_exact(
    const InfiltratrI18n *context, const char *locale)
{
    char normalised[INFILTRATR_I18N_LOCALE_CAPACITY];
    if (!context || !locale) return NULL;
    for (size_t i = 0U; i < context->catalog_count; ++i) {
        if (normalise_locale(normalised, sizeof(normalised),
                             context->catalogs[i].locale) == 0U)
            continue;
        if (strcmp(normalised, locale) == 0) return &context->catalogs[i];
    }
    return NULL;
}

static const InfiltratrI18nCatalog *find_catalog_language(
    const InfiltratrI18n *context, const char *locale)
{
    char normalised[INFILTRATR_I18N_LOCALE_CAPACITY];
    const InfiltratrI18nCatalog *first_match = NULL;
    if (!context || !locale) return NULL;
    for (size_t i = 0U; i < context->catalog_count; ++i) {
        if (normalise_locale(normalised, sizeof(normalised),
                             context->catalogs[i].locale) == 0U)
            continue;
        if (!same_language(normalised, locale)) continue;
        if (!strchr(normalised, '-')) return &context->catalogs[i];
        if (!first_match) first_match = &context->catalogs[i];
    }
    return first_match;
}

static const char *catalog_lookup(const InfiltratrI18nCatalog *catalog,
                                  const char *key)
{
    if (!catalog || !key || !catalog->entries) return NULL;
    for (size_t i = 0U; i < catalog->entry_count; ++i)
        if (catalog->entries[i].key && strcmp(catalog->entries[i].key, key) == 0)
            return catalog->entries[i].value;
    return NULL;
}

static const char *lookup_locale(const InfiltratrI18n *context,
                                 const char *locale, const char *key)
{
    const InfiltratrI18nCatalog *catalog = find_catalog_exact(context, locale);
    const char *value = catalog_lookup(catalog, key);
    if (value) return value;
    catalog = find_catalog_language(context, locale);
    return catalog_lookup(catalog, key);
}

static bool catalog_table_valid(const InfiltratrI18nCatalog *catalogs,
                                size_t catalog_count)
{
    char normalised[INFILTRATR_I18N_LOCALE_CAPACITY];

    if (!catalogs || catalog_count == 0U) return false;
    for (size_t i = 0U; i < catalog_count; ++i) {
        const InfiltratrI18nCatalog *catalog = &catalogs[i];
        if (!catalog->locale ||
            normalise_locale(normalised, sizeof(normalised),
                             catalog->locale) == 0U)
            return false;
        if (catalog->entry_count > 0U && !catalog->entries)
            return false;
        for (size_t j = 0U; j < catalog->entry_count; ++j)
            if (!catalog->entries[j].key || !catalog->entries[j].value)
                return false;
    }
    return true;
}

bool infiltratr_i18n_init(InfiltratrI18n *context,
                          const InfiltratrI18nCatalog *catalogs,
                          size_t catalog_count,
                          const char *fallback_locale)
{
    InfiltratrI18n configured = {0};

    if (!context) return false;
    memset(context, 0, sizeof(*context));
    if (!fallback_locale || !catalog_table_valid(catalogs, catalog_count))
        return false;

    configured.catalogs = catalogs;
    configured.catalog_count = catalog_count;
    if (normalise_locale(configured.fallback_locale,
                         sizeof(configured.fallback_locale),
                         fallback_locale) == 0U)
        return false;
    infiltratr_copy_string(configured.locale, sizeof(configured.locale),
                           configured.fallback_locale);
    if (!find_catalog_language(&configured, configured.fallback_locale))
        return false;

    *context = configured;
    return true;
}

bool infiltratr_i18n_set_locale(InfiltratrI18n *context, const char *locale)
{
    char normalised[INFILTRATR_I18N_LOCALE_CAPACITY];
    if (!context || !locale) return false;
    if (normalise_locale(normalised, sizeof(normalised), locale) == 0U) return false;
    infiltratr_copy_string(context->locale, sizeof(context->locale), normalised);
    return find_catalog_language(context, context->locale) != NULL;
}


const char *infiltratr_i18n_get(const InfiltratrI18n *context, const char *key)
{
    if (!key) return "";
    if (!context) return key;
    const char *value = lookup_locale(context, context->locale, key);
    if (value) return value;
    if (strcmp(context->locale, context->fallback_locale) != 0) {
        value = lookup_locale(context, context->fallback_locale, key);
        if (value) return value;
    }
    return key;
}


