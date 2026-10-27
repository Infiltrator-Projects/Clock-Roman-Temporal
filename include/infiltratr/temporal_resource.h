// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file temporal_resource.h
 * @brief Filesystem discovery for installable calendar and clock resources.
 *
 * This API deliberately discovers resource objects without defining their
 * eventual payload format. A discovered object may later prove to be data,
 * native code or a bundle; discovery-by-presence is the stable concept.
 */
#ifndef INFILTRATR_COMMON_TEMPORAL_RESOURCE_H
#define INFILTRATR_COMMON_TEMPORAL_RESOURCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFILTRATR_TEMPORAL_RESOURCE_INFO_ABI 1U

typedef enum InfiltratrTemporalResourceKind {
    INFILTRATR_TEMPORAL_RESOURCE_CALENDAR = 1,
    INFILTRATR_TEMPORAL_RESOURCE_CLOCK = 2
} InfiltratrTemporalResourceKind;

typedef enum InfiltratrTemporalResourceForm {
    INFILTRATR_TEMPORAL_RESOURCE_FILE = 1,
    INFILTRATR_TEMPORAL_RESOURCE_DIRECTORY = 2
} InfiltratrTemporalResourceForm;

/**
 * One discovered direct child of a temporal resource directory.
 *
 * `name` and `path` are borrowed and valid only for the duration of the
 * visitor call. Discovery does not interpret the resource payload.
 */
typedef struct InfiltratrTemporalResourceInfo {
    size_t struct_size;
    uint32_t abi_version;
    InfiltratrTemporalResourceKind kind;
    InfiltratrTemporalResourceForm form;
    const char *name;
    const char *path;
} InfiltratrTemporalResourceInfo;

typedef bool (*InfiltratrTemporalResourceVisitor)(
    const InfiltratrTemporalResourceInfo *resource,
    void *user_data);

/**
 * Enumerate direct non-hidden file/directory children in deterministic order.
 *
 * Missing directories are an empty successful catalogue. Symlinks/reparse
 * points and non-file/non-directory objects are ignored. `visitor` may be NULL
 * for a count-only scan. `visited` is optional and is updated only on success.
 */
bool infiltratr_temporal_resource_scan_directory(
    const char *directory,
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalResourceVisitor visitor,
    void *user_data,
    size_t *visited);

/**
 * Enumerate `<root>/calendars` followed by `<root>/clocks`.
 *
 * This establishes the directory-as-catalogue concept without choosing the
 * final resource file, bundle or executable format.
 */
bool infiltratr_temporal_resource_scan_root(
    const char *root,
    InfiltratrTemporalResourceVisitor visitor,
    void *user_data,
    size_t *visited);

#ifdef __cplusplus
}
#endif

#endif
