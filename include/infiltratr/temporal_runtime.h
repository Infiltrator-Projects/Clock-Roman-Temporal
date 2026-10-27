// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file temporal_runtime.h
 * @brief Validated runtime catalogue for native temporal resource modules.
 *
 * Candidate discovery remains owned by temporal_resource.h. This layer adds a
 * deliberately small executable-resource contract: one trusted native module
 * exports stable metadata, while the product that owns the behaviour validates
 * its own implementation ABI separately.
 */
#ifndef INFILTRATR_COMMON_TEMPORAL_RUNTIME_H
#define INFILTRATR_COMMON_TEMPORAL_RUNTIME_H

#include "infiltratr/temporal.h"
#include "infiltratr/temporal_resource.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI 1U
#define INFILTRATR_TEMPORAL_RESOURCE_METADATA_SYMBOL \
    "infiltratr_temporal_resource_metadata_v1"

#define INFILTRATR_TEMPORAL_NATIVE_CALENDAR_DIRECTORY \
    "/System/Resources/Calendars"
#define INFILTRATR_TEMPORAL_NATIVE_CLOCK_DIRECTORY \
    "/System/Resources/Clocks"

/**
 * Product-neutral identity exported by one native temporal resource module.
 *
 * Calendar resources use only id/name/kind. Clock resources additionally use
 * the three clock capability flags. Strings are owned by the loaded module and
 * remain borrowed for the duration of a runtime-scan visitor call.
 */
typedef struct InfiltratrTemporalResourceMetadata {
    size_t struct_size;
    uint32_t abi_version;
    InfiltratrTemporalResourceKind kind;
    const char *id;
    const char *name;
    bool supports_seconds;
    bool requires_latitude;
    bool requires_longitude;
} InfiltratrTemporalResourceMetadata;

typedef const InfiltratrTemporalResourceMetadata *
(*InfiltratrTemporalResourceMetadataEntry)(void);

typedef bool (*InfiltratrTemporalRuntimeVisitor)(
    const InfiltratrTemporalResourceMetadata *metadata,
    const char *module_path,
    void *user_data);

/**
 * Inspect direct file resources in one explicit directory.
 *
 * Each file is opened only long enough to resolve and validate the generic
 * metadata symbol. Non-modules, modules without the symbol, malformed metadata
 * and resources of another kind are ignored independently. Directory bundles
 * remain reserved for a later resource ABI and are not executable in ABI 1.
 * A visitor returning false terminates the scan and makes the call fail.
 */
bool infiltratr_temporal_runtime_scan_directory(
    const char *directory,
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited);

/**
 * Scan the exact case-sensitive InfiltratorOS native resource directory for
 * the requested kind.
 */
bool infiltratr_temporal_runtime_scan_native(
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited);

/**
 * Merged built-in + validated native Calendar metadata catalogue.
 *
 * Built-ins are always first and always win duplicate identifiers. Native
 * resources are appended in deterministic filesystem order. The catalogue is
 * snapshotted on first use for process-lifetime pointer stability.
 */
size_t infiltratr_temporal_installed_calendar_count(void);
const InfiltratrTemporalCalendarInfo *
infiltratr_temporal_installed_calendar_at(size_t index);
const InfiltratrTemporalCalendarInfo *
infiltratr_temporal_installed_calendar_find(const char *id);

/**
 * Merged metadata catalogue for clocks. Behaviour loading remains owned by the
 * clock runtime; consumers must not treat metadata presence alone as proof that
 * a clock implementation can execute.
 */
size_t infiltratr_temporal_installed_clock_mode_count(void);
const InfiltratrTemporalClockModeInfo *
infiltratr_temporal_installed_clock_mode_at(size_t index);
const InfiltratrTemporalClockModeInfo *
infiltratr_temporal_installed_clock_mode_find(const char *id);

#ifdef __cplusplus
}
#endif

#endif
