// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file temporal.h
 * @brief Portable system-wide temporal presentation policy and clock formatting.
 *
 * Canonical timestamps remain owned by applications and operating systems.
 * This API owns only the user's human-facing clock presentation policy.
 *
 * @author Shannon Smith
 * @copyright Copyright (c) 1993-2026 Shannon Smith
 * @license GPL-3.0-or-later
 */
#ifndef INFILTRATR_COMMON_TEMPORAL_H
#define INFILTRATR_COMMON_TEMPORAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFILTRATR_TEMPORAL_POLICY_VERSION 1U
#define INFILTRATR_TEMPORAL_POLICY_V3_VERSION 3U
#define INFILTRATR_TEMPORAL_ID_CAPACITY 64U

/*
 * Optional date-provider ABI. Calendar owns chronology and implements this
 * renderer; Common owns binding, bounded UTF-8 output and consumer policy.
 * The provider uses caller-owned storage so no toolkit types, allocated
 * strings or type-registration lifetime cross the library boundary.
 * Supported parts are "short" and "full". Failure leaves output unchanged.
 */
#define INFILTRATR_TEMPORAL_DATE_RENDERER_SYMBOL "calendar_format_date_v1"
#define INFILTRATR_TEMPORAL_DATE_CAPACITY 512U
typedef bool (*InfiltratrTemporalDateRenderer)(
    const char *calendar_id,
    int32_t gregorian_year,
    int32_t gregorian_month,
    int32_t gregorian_day,
    const char *part,
    char *buffer,
    size_t capacity,
    size_t *length);

typedef enum InfiltratrClockProfile {
    INFILTRATR_CLOCK_PROFILE_SYSTEM = 0,
    INFILTRATR_CLOCK_PROFILE_CONVENTIONAL_12,
    INFILTRATR_CLOCK_PROFILE_CONVENTIONAL_24,
    INFILTRATR_CLOCK_PROFILE_DECIMAL_10
} InfiltratrClockProfile;

typedef struct InfiltratrTemporalPolicy {
    uint32_t struct_size;
    uint32_t version;
    InfiltratrClockProfile clock_profile;
    bool show_seconds;
} InfiltratrTemporalPolicy;

typedef struct InfiltratrTemporalClockModeInfo {
    const char *id;
    const char *name;
    bool supports_seconds;
    bool requires_latitude;
    bool requires_longitude;
} InfiltratrTemporalClockModeInfo;

typedef struct InfiltratrTemporalCalendarInfo {
    const char *id;
    const char *name;
} InfiltratrTemporalCalendarInfo;

/*
 * Version 3 is the current system-wide authority. It deliberately has one
 * calendar only. The calendar member is a stable capability ID, not proof that
 * the capability is installed on this machine; runtime consumers resolve that
 * separately. The retired secondary-calendar concept is not part of the API.
 */
typedef struct InfiltratrTemporalPolicyV3 {
    uint32_t struct_size;
    uint32_t version;
    char clock_mode[INFILTRATR_TEMPORAL_ID_CAPACITY];
    char calendar[INFILTRATR_TEMPORAL_ID_CAPACITY];
    bool show_seconds;
    bool location_configured;
    double latitude;
    double longitude;
} InfiltratrTemporalPolicyV3;

/** Initialise a policy to conservative system-following defaults. */
bool infiltratr_temporal_policy_default(InfiltratrTemporalPolicy *policy);

/** Parse a stable clock-profile identifier. Caller output is unchanged on failure. */
bool infiltratr_clock_profile_from_id(const char *id,
                                      InfiltratrClockProfile *profile);

/** Return the stable persistence identifier for a profile, or NULL. */
const char *infiltratr_clock_profile_id(InfiltratrClockProfile profile);

/** Return the stable English presentation name for a profile, or NULL. */
const char *infiltratr_clock_profile_name(InfiltratrClockProfile profile);

/** Number of profiles in the portable catalogue. */
size_t infiltratr_clock_profile_count(void);

/** Return a profile by catalogue index; false when out of range. */
bool infiltratr_clock_profile_at(size_t index, InfiltratrClockProfile *profile);

/**
 * Parse a versioned key=value temporal-policy document.
 *
 * Required key: version=1. Optional recognised keys are clock-profile and
 * show-seconds. Unknown keys are ignored for forward-compatible readers.
 * Caller output is unchanged on malformed or unsupported input.
 */
bool infiltratr_temporal_policy_parse(const char *text,
                                      InfiltratrTemporalPolicy *policy);

/**
 * Serialise a temporal policy into deterministic key=value text.
 *
 * On insufficient capacity the destination remains unchanged. @length may be
 * NULL; when provided it receives the byte count excluding the trailing NUL.
 */
bool infiltratr_temporal_policy_serialize(const InfiltratrTemporalPolicy *policy,
                                          char *buffer,
                                          size_t capacity,
                                          size_t *length);

/**
 * Return the local civil microsecond phase within one 24-hour day.
 *
 * @unix_microseconds is a canonical Unix-microsecond instant.
 * @utc_offset_seconds is the caller-supplied civil UTC offset including DST.
 * The result is always in the inclusive range 0 through 86,399,999,999,
 * including for negative instants and extreme signed inputs.
 */
int64_t infiltratr_temporal_local_microseconds_of_day(
    int64_t unix_microseconds,
    int32_t utc_offset_seconds);

/**
 * Format a local civil clock from a canonical Unix-microsecond instant.
 *
 * Historical ABI symbol for clock-profile formatting. All concrete profiles
 * have been migrated to independent native clock resources, so this symbol
 * now deliberately returns false; use the Common resource presentation ABI.
 *
 * The legacy compatibility symbol deliberately returns false for every
 * profile. Install the system clock resource and render through the Common
 * temporal_clock_resource presentation ABI instead; no duplicate arithmetic
 * or silent fallback is permitted in this portable policy layer.
 */
bool infiltratr_temporal_format_clock(InfiltratrClockProfile profile,
                                      int64_t unix_microseconds,
                                      int32_t utc_offset_seconds,
                                      bool show_seconds,
                                      char *buffer,
                                      size_t capacity,
                                      size_t *length);

/**
 * Format any explicit clock mode in the system-wide temporal catalogue.
 *
 * "standard" deliberately returns false because OS-locale rendering belongs
 * to the platform adapter. Extracted modes (including explicit 12/24-hour
 * and Chinese hundred-kè/mean-solar) also return false here: use the native
 * temporal_clock_resource presentation ABI instead. Unmigrated modes retain
 * their portable formatter temporarily. Location-aware modes require
 * @location_configured plus finite coordinates when their catalogue metadata
 * declares latitude and/or longitude.
 *
 * @show_seconds requests the clock mode's closest meaningful finer
 * presentation. Historical modes may therefore use native subdivisions rather
 * than literal modern SI seconds.
 * @vertical selects newline-separated clock fields for narrow presentations.
 * Applications that render ordinary rows or timestamps should pass false.
 */
bool infiltratr_temporal_format_clock_mode(const char *mode,
                                           int64_t unix_microseconds,
                                           int32_t utc_offset_seconds,
                                           bool show_seconds,
                                           bool vertical,
                                           bool location_configured,
                                           double latitude,
                                           double longitude,
                                           char *buffer,
                                           size_t capacity,
                                           size_t *length);

/**
 * Legacy localized absolute-clock formatter, retained for ABI compatibility.
 *
 * All explicit clocks are independently installed resources. This entry point
 * returns false and preserves caller output for every mode. Use the executable
 * resource presentation API in temporal_runtime.h for rendering and scheduling.
 */
bool infiltratr_temporal_format_clock_mode_localized(
    const char *locale, const char *mode,
    int64_t unix_microseconds, int32_t utc_offset_seconds,
    bool show_seconds, bool vertical, bool location_configured,
    double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length);

/**
 * Format an elapsed interval using the selected clock mode's meaningful units.
 *
 * The measured interval remains canonical SI microseconds. Fixed-unit systems
 * are projected into their own elapsed representation: decimal time uses
 * 10/100/100 units, Internet Time uses beats, hexadecimal uses 65,536 ticks
 * per day, binary uses binary H:M:S fields, Julian modes use fractional days,
 * sidereal time uses the shared sidereal rate, Chinese hundred-kè durations
 * use native 日/刻 hierarchy rather than modern day prefixes or denominator
 * fractions, Chinese double-hours retain their twelve-part day notation, and
 * Indian ghaṭī uses 60 ghaṭī/day and 60 vighaṭī/ghaṭī.
 *
 * @end_unix_microseconds anchors modes whose rate depends on civil date.
 * Apparent-solar intervals include equation-of-time drift. Roman and Edo
 * seasonal intervals integrate their real astronomical day/night unit lengths:
 * Roman presentation first collapses complete 12-horae/4-vigiliae cycles into
 * dies, then reports residual daylight horae and night vigiliae (with unciae
 * when fractional precision is requested), while Edo presentation reports
 * seasonal koku/toki with the historically attested half-period marker.
 * INT64_MIN denotes an unanchored accumulated quantity; seasonal modes then
 * remain explicit SI H:M:S because no honest civil-seasonal conversion exists.
 * Equal-hour origin systems keep conventional hour/minute/second units because
 * sunrise/sunset changes only their zero point. The Renaissance European mode
 * historically called "Babylonian hours" is distinct from the genuinely
 * ancient Babylonian seasonal scheme, which divides daylight and night into
 * twelve simānu each. Nuremberg presentation uses equal hours with a discrete
 * Wendetag-style integer allocation between day and night rather than resetting
 * the count continuously at each day's astronomical sunrise and sunset.
 *
 * Location requirements follow the corresponding clock mode. @vertical only
 * changes separators; it never changes the represented interval.
 */
bool infiltratr_temporal_format_duration_mode(
    const char *mode,
    uint64_t elapsed_microseconds,
    int64_t end_unix_microseconds,
    bool show_seconds,
    bool vertical,
    bool location_configured,
    double latitude,
    double longitude,
    char *buffer,
    size_t capacity,
    size_t *length);

/** Complete system-wide clock catalogue shared by Settings and Calendar. */
size_t infiltratr_temporal_clock_mode_count(void);
const InfiltratrTemporalClockModeInfo *
infiltratr_temporal_clock_mode_at(size_t index);
const InfiltratrTemporalClockModeInfo *
infiltratr_temporal_clock_mode_find(const char *id);

/**
 * Empty legacy calendar catalogue, retained for ABI compatibility.
 *
 * Calendar implementations are independently installed resources. These
 * symbols return zero/NULL; use temporal_runtime.h's installed executable
 * catalogue, which validates each resource before advertising it.
 */
size_t infiltratr_temporal_calendar_count(void);
const InfiltratrTemporalCalendarInfo *
infiltratr_temporal_calendar_at(size_t index);
const InfiltratrTemporalCalendarInfo *
infiltratr_temporal_calendar_find(const char *id);

/**
 * Initialise the current system-wide temporal authority.
 *
 * The default is Standard time (OS locale), Gregorian calendar, seconds off
 * and no configured geographic location. System Settings owns this policy;
 * consumers read it rather than maintaining competing temporal preferences.
 */
bool infiltratr_temporal_policy_v3_default(InfiltratrTemporalPolicyV3 *policy);

/**
 * Parse current temporal policy. Older on-disk documents may be migrated
 * internally, but no older temporal-policy ABI is exposed publicly. Calendar
 * IDs are validated structurally here; installation is a runtime concern.
 */
bool infiltratr_temporal_policy_v3_parse(const char *text,
                                         InfiltratrTemporalPolicyV3 *policy);

/** Serialize the current authority as deterministic version-3 text. */
bool infiltratr_temporal_policy_v3_serialize(
    const InfiltratrTemporalPolicyV3 *policy,
    char *buffer,
    size_t capacity,
    size_t *length);

/** Neutral Gregorian/JDN coordinates; custom chronology lives only in modules. */
bool infiltratr_temporal_gregorian_is_leap(int64_t year);
int32_t infiltratr_temporal_gregorian_month_length(int64_t year, int32_t month);
bool infiltratr_temporal_gregorian_date_valid(int32_t year, int32_t month, int32_t day);
int64_t infiltratr_temporal_gregorian_to_jdn(int64_t year, int32_t month, int32_t day);
void infiltratr_temporal_jdn_to_gregorian(int64_t jdn, int32_t *year, int32_t *month, int32_t *day);
int32_t infiltratr_temporal_gregorian_day_of_year(int32_t year, int32_t month, int32_t day);
int32_t infiltratr_temporal_iso_weekday(int64_t jdn);

#ifdef __cplusplus
}
#endif

#endif
