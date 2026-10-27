// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file temporal_clock_resource.h
 * @brief Executable system clock-resource ABI and validated loader.
 *
 * Canonical time remains Unix microseconds; modules only control human-facing
 * notation and the next visible presentation boundary. No toolkit or Calendar
 * application ABI crosses this versioned Common interface.
 */
#ifndef INFILTRATR_COMMON_TEMPORAL_CLOCK_RESOURCE_H
#define INFILTRATR_COMMON_TEMPORAL_CLOCK_RESOURCE_H

#include "infiltratr/temporal_runtime.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFILTRATR_TEMPORAL_CLOCK_PROVIDER_ABI 1U
#define INFILTRATR_TEMPORAL_CLOCK_PROVIDER_SYMBOL \
    "infiltratr_temporal_clock_provider_v1"
#define INFILTRATR_TEMPORAL_CLOCK_TEXT_CAPACITY 256U

typedef struct InfiltratrTemporalClockProvider InfiltratrTemporalClockProvider;

typedef bool (*InfiltratrTemporalClockFormatFunc)(
    const InfiltratrTemporalClockProvider *provider,
    const char *locale, int64_t unix_microseconds,
    int32_t utc_offset_seconds, bool show_seconds, bool vertical,
    double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length);

typedef bool (*InfiltratrTemporalClockDelayFunc)(
    const InfiltratrTemporalClockProvider *provider,
    int64_t unix_microseconds, int32_t utc_offset_seconds,
    bool show_seconds, double latitude, double longitude,
    uint32_t *milliseconds);

/** Both entry points must export precisely the same stable clock ID. */
struct InfiltratrTemporalClockProvider {
    size_t struct_size;
    uint32_t abi_version;
    const char *clock_id;
    InfiltratrTemporalClockFormatFunc format;
    InfiltratrTemporalClockDelayFunc next_tick;
};

typedef const InfiltratrTemporalClockProvider *
(*InfiltratrTemporalClockProviderEntry)(void);

typedef struct InfiltratrTemporalClockModule InfiltratrTemporalClockModule;

/** Open one explicit, trusted clock module (also used by isolated tests). */
InfiltratrTemporalClockModule *
infiltratr_temporal_clock_module_open(const char *module_path);

/** Load from the canonical /System/Resources/Clocks directory only. */
InfiltratrTemporalClockModule *
infiltratr_temporal_clock_module_open_id(const char *clock_id);

void infiltratr_temporal_clock_module_free(InfiltratrTemporalClockModule *module);

const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_clock_module_metadata(const InfiltratrTemporalClockModule *module);

/** Both wrappers preserve caller output on failure. */
bool infiltratr_temporal_clock_format(
    const InfiltratrTemporalClockModule *module,
    const char *locale, int64_t unix_microseconds,
    int32_t utc_offset_seconds, bool show_seconds, bool vertical,
    double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length);

bool infiltratr_temporal_clock_next_tick(
    const InfiltratrTemporalClockModule *module,
    int64_t unix_microseconds, int32_t utc_offset_seconds,
    bool show_seconds, double latitude, double longitude,
    uint32_t *milliseconds);

/**
 * Long-lived system presentation handle. Every explicit clock requires its
 * validated installed resource. Compiled metadata never supplies arithmetic.
 * Missing or invalid modules fail without changing the caller's output.
 * Callers may retain this handle across repeated UI refreshes.
 */
typedef struct InfiltratrTemporalClockPresentation InfiltratrTemporalClockPresentation;

InfiltratrTemporalClockPresentation *
infiltratr_temporal_clock_presentation_new(const char *clock_id);
void infiltratr_temporal_clock_presentation_free(
    InfiltratrTemporalClockPresentation *presentation);

/** Output is unchanged on failure, including an absent required resource. */
bool infiltratr_temporal_clock_presentation_format(
    const InfiltratrTemporalClockPresentation *presentation,
    const char *locale, int64_t unix_microseconds,
    int32_t utc_offset_seconds, bool show_seconds, bool vertical,
    bool location_configured, double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length);

/**
 * Exact resource tick boundary; returns false when the resource fails.
 * Does not change milliseconds on failure.
 */
bool infiltratr_temporal_clock_presentation_next_tick(
    const InfiltratrTemporalClockPresentation *presentation,
    int64_t unix_microseconds, int32_t utc_offset_seconds,
    bool show_seconds, double latitude, double longitude,
    uint32_t *milliseconds);

#ifdef __cplusplus
}
#endif
#endif
