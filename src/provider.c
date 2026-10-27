// SPDX-License-Identifier: GPL-3.0-or-later
/* Seasonal clock resources. Sole owner of clock-face arithmetic and scheduling. */

#define CLOCK_ID "roman-temporal"
#define CLOCK_SECONDS true
#define CLOCK_LOCATION true
#include <infiltratr/temporal_clock_resource.h>
#include "temporal_solar_compact.h"

#include "infiltratr/arithmetic.h"
#include "infiltratr/core.h"
#include "infiltratr/i18n.h"
#include "infiltratr/timing.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define MICROSECONDS_PER_SECOND INT64_C(1000000)
#define SECONDS_PER_MINUTE INT64_C(60)
#define SECONDS_PER_HOUR INT64_C(3600)
#define SECONDS_PER_DAY INT64_C(86400)
#define MICROSECONDS_PER_DAY (SECONDS_PER_DAY * MICROSECONDS_PER_SECOND)
#define INFILTRATR_PI 3.141592653589793238462643383279502884L

static bool write_text(char *buffer, size_t capacity, size_t *length,
                       const char *text)
{
    size_t needed;

    if (buffer == NULL || capacity == 0U || text == NULL) {
        return false;
    }
    needed = strlen(text);
    if (needed >= capacity) {
        return false;
    }
    memcpy(buffer, text, needed + 1U);
    if (length != NULL) {
        *length = needed;
    }
    return true;
}

static bool finish_output(char *buffer,size_t capacity,size_t *length,int written)
{
    if (written < 0 || (size_t)written >= capacity) {
        if (buffer != NULL && capacity != 0U) buffer[0]='\0';
        return false;
    }
    if (length != NULL) *length=(size_t)written;
    return true;
}

static int64_t positive_modulo(int64_t value, int64_t modulus)
{
    int64_t remainder = 0;

    if (!infiltratr_i64_floor_divmod(value, modulus, NULL, &remainder)) {
        return 0;
    }
    return remainder;
}

static long double mean_solar_seconds(int64_t unix_microseconds,
                                      double longitude)
{
    const int64_t utc_microseconds =
        positive_modulo(unix_microseconds, MICROSECONDS_PER_DAY);
    long double seconds =
        (long double)utc_microseconds / MICROSECONDS_PER_SECOND +
        (long double)temporal_solar_clamp_longitude(longitude) * 240.0L;

    seconds = fmodl(seconds, (long double)SECONDS_PER_DAY);
    if (seconds < 0.0L) {
        seconds += (long double)SECONDS_PER_DAY;
    }
    return seconds;
}

static long double apparent_solar_seconds(int64_t unix_microseconds,
                                          double longitude)
{
    double equation_minutes = 0.0;
    long double seconds;

    if (!temporal_solar_compact_terms(unix_microseconds, &equation_minutes, NULL)) {
        return mean_solar_seconds(unix_microseconds, longitude);
    }
    seconds = mean_solar_seconds(unix_microseconds, longitude) +
              (long double)equation_minutes * 60.0L;
    seconds = fmodl(seconds, (long double)SECONDS_PER_DAY);
    if (seconds < 0.0L) {
        seconds += (long double)SECONDS_PER_DAY;
    }
    return seconds;
}

static bool solar_day_boundaries(int64_t unix_microseconds,
                                 double latitude,
                                 double solar_depression_degrees,
                                 long double *dawn_seconds,
                                 long double *dusk_seconds)
{
    double declination = 0.0;
    long double hour_angle;
    long double half_day_seconds;

    if (dawn_seconds == NULL || dusk_seconds == NULL ||
        !isfinite(solar_depression_degrees) ||
        solar_depression_degrees < 0.0 ||
        solar_depression_degrees >= 90.0 ||
        !temporal_solar_compact_terms(unix_microseconds, NULL, &declination)) {
        return false;
    }

    if (!temporal_solar_hour_angle(latitude, declination,
                                   solar_depression_degrees, &hour_angle)) {
        return false;
    }
    half_day_seconds =
        hour_angle * (long double)SECONDS_PER_DAY / (2.0L * INFILTRATR_PI);
    *dawn_seconds = (long double)SECONDS_PER_DAY / 2.0L - half_day_seconds;
    *dusk_seconds = (long double)SECONDS_PER_DAY / 2.0L + half_day_seconds;
    return true;
}

typedef struct SeasonalPeriod {
    bool daylight;
    unsigned index;
    long double fraction;
} SeasonalPeriod;

static bool seasonal_period_at(int64_t unix_microseconds,
                               double latitude,
                               double longitude,
                               double solar_depression_degrees,
                               unsigned daylight_parts,
                               unsigned night_parts,
                               SeasonalPeriod *period)
{
    long double dawn;
    long double dusk;
    const long double solar =
        apparent_solar_seconds(unix_microseconds, longitude);
    long double span;
    long double position;
    long double unit;
    unsigned parts;

    if (period == NULL || daylight_parts == 0U || night_parts == 0U ||
        !solar_day_boundaries(unix_microseconds, latitude,
                              solar_depression_degrees, &dawn, &dusk)) {
        return false;
    }

    if (solar >= dawn && solar < dusk) {
        period->daylight = true;
        span = dusk - dawn;
        position = solar - dawn;
        parts = daylight_parts;
    } else {
        period->daylight = false;
        span = (long double)SECONDS_PER_DAY - (dusk - dawn);
        position = solar >= dusk
            ? solar - dusk
            : (long double)SECONDS_PER_DAY - dusk + solar;
        parts = night_parts;
    }

    unit = span / parts;
    {
        long double scaled = position / unit;

        if (scaled < 0.0L) {
            scaled = 0.0L;
        }
        if (scaled >= (long double)parts) {
            scaled = nextafterl((long double)parts, 0.0L);
        }
        period->index = (unsigned)floorl(scaled);
        period->fraction = scaled - (long double)period->index;
    }
    return true;
}


static bool mode_location_valid(const InfiltratrTemporalClockModeInfo *info,
                                bool location_configured,
                                double latitude,
                                double longitude)
{
    if (info == NULL) {
        return false;
    }
    if ((info->requires_latitude || info->requires_longitude) &&
        !location_configured) {
        return false;
    }
    return (!info->requires_latitude ||
            (isfinite(latitude) && latitude >= -90.0 && latitude <= 90.0)) &&
           (!info->requires_longitude ||
            (isfinite(longitude) && longitude >= -180.0 &&
             longitude <= 180.0));
}


/* Names are transliterated proper names, not animal/help labels. Wording
 * belongs to the selected language; the shared phase calculation stays below. */
#define CLOCK_REMAINING_WORDS(hour, watch, twelfth, twelfths) \
    { "hour", hour }, { "watch", watch }, \
    { "twelfth", twelfth }, { "twelfths", twelfths }
#define JAPANESE_BRANCHES \
    { "jp-ne", "Ne" }, { "jp-ushi", "Ushi" }, { "jp-tora", "Tora" }, \
    { "jp-u", "U" }, { "jp-tatsu", "Tatsu" }, { "jp-mi", "Mi" }, \
    { "jp-uma", "Uma" }, { "jp-hitsuji", "Hitsuji" }, { "jp-saru", "Saru" }, \
    { "jp-tori", "Tori" }, { "jp-inu", "Inu" }, { "jp-i", "I" }
#define JAPANESE_NATIVE_BRANCHES \
    { "jp-ne", "子" }, { "jp-ushi", "丑" }, { "jp-tora", "寅" }, \
    { "jp-u", "卯" }, { "jp-tatsu", "辰" }, { "jp-mi", "巳" }, \
    { "jp-uma", "午" }, { "jp-hitsuji", "未" }, { "jp-saru", "申" }, \
    { "jp-tori", "酉" }, { "jp-inu", "戌" }, { "jp-i", "亥" }
#define EDO_WORDS(horizontal, vertical) \
    { "edo", horizontal }, { "edo-vertical", vertical }
static const InfiltratrI18nEntry clock_en[] = {
    CLOCK_REMAINING_WORDS("Hour", "Watch", "twelfth", "twelfths"),


};
static const InfiltratrI18nEntry clock_de[] = {
    CLOCK_REMAINING_WORDS("Stunde", "Wache", "Zwölftel", "Zwölftel"),

};
static const InfiltratrI18nEntry clock_es[] = {
    CLOCK_REMAINING_WORDS("Hora", "Vigilia", "doceavo", "doceavos"),

};
static const InfiltratrI18nEntry clock_fr[] = {
    CLOCK_REMAINING_WORDS("Heure", "Veille", "douzième", "douzièmes"),

};
static const InfiltratrI18nEntry clock_it[] = {
    CLOCK_REMAINING_WORDS("Ora", "Vigilia", "dodicesimo", "dodicesimi"),

};
static const InfiltratrI18nEntry clock_pt[] = {
    CLOCK_REMAINING_WORDS("Hora", "Vigília", "duodécimo", "duodécimos"),

};
static const InfiltratrI18nEntry clock_ru[] = {
    CLOCK_REMAINING_WORDS("Час", "Стража", "двенадцатая", "двенадцатых"),

};
static const InfiltratrI18nEntry clock_ja[] = {
    CLOCK_REMAINING_WORDS("時", "夜番", "十二分の一", "十二分の一"),


};
static const InfiltratrI18nEntry clock_zh[] = {
    CLOCK_REMAINING_WORDS("时", "更", "十二分之一时", "十二分之一时"),


};
static const InfiltratrI18nEntry clock_zh_hant[] = {
    CLOCK_REMAINING_WORDS("時", "更", "十二分之一時", "十二分之一時"),


};

#undef CLOCK_REMAINING_WORDS
#undef JAPANESE_BRANCHES
#undef JAPANESE_NATIVE_BRANCHES
#undef EDO_WORDS

#define CLOCK_CATALOG(locale, entries) \
    { locale, entries, sizeof(entries) / sizeof((entries)[0]) }
static const InfiltratrI18nCatalog clock_catalogs[] = {
    CLOCK_CATALOG("en", clock_en), CLOCK_CATALOG("de", clock_de),
    CLOCK_CATALOG("es", clock_es), CLOCK_CATALOG("fr", clock_fr),
    CLOCK_CATALOG("it", clock_it), CLOCK_CATALOG("pt", clock_pt),
    CLOCK_CATALOG("ru", clock_ru), CLOCK_CATALOG("ja", clock_ja),
    
    CLOCK_CATALOG("zh", clock_zh), CLOCK_CATALOG("zh-TW", clock_zh_hant),
    CLOCK_CATALOG("zh-HK", clock_zh_hant), CLOCK_CATALOG("zh-MO", clock_zh_hant),
    CLOCK_CATALOG("zh-Hant", clock_zh_hant),
    CLOCK_CATALOG("zh-Hant-TW", clock_zh_hant),
    CLOCK_CATALOG("zh-Hant-HK", clock_zh_hant)
};
#undef CLOCK_CATALOG


static bool format_clock_mode(const InfiltratrI18n *language,
                                           const char *mode,
                                           int64_t unix_microseconds,
                                           int32_t utc_offset_seconds,
                                           bool show_seconds,
                                           bool vertical,
                                           bool location_configured,
                                           double latitude,
                                           double longitude,
                                           char *buffer,
                                           size_t capacity,
                                           size_t *length)
{
    (void)mode;
    const InfiltratrTemporalClockModeInfo *info;

    // The extension modes use absolute instants and, when applicable, the
    // selected solar location. Civil UTC offsets remain part of the stable
    // caller contract, but do not affect this custom-mode renderer.
    (void)utc_offset_seconds;
    if (buffer == NULL || capacity == 0U || mode == NULL) {
        return false;
    }
    buffer[0] = '\0';

    const InfiltratrTemporalClockModeInfo selected = {
        CLOCK_ID, INFILTRATR_TEMPORAL_DISPLAY_NAME, CLOCK_SECONDS, CLOCK_LOCATION, CLOCK_LOCATION
    };
    info = strcmp(mode, CLOCK_ID) == 0 ? &selected : NULL;
    if (info == NULL ||
        !mode_location_valid(
            info, location_configured, latitude, longitude)) {
        return false;
    }

    {
        static const char *const roman[] = {
            "I", "II", "III", "IV", "V", "VI",
            "VII", "VIII", "IX", "X", "XI", "XII"
        };
        SeasonalPeriod period;

        if (!seasonal_period_at(
                unix_microseconds, latitude, longitude,
                0.833, 12U, 4U, &period)) {
            return write_text(
                buffer, capacity, length,
                vertical ? "N/A\nROM" : "N/A ROM");
        }

        /*
         * Roman daylight horae were seasonal, so finer presentation must
         * subdivide the current unequal hora rather than append modern
         * minutes. An uncia is one twelfth of an hour. Night remains in the
         * selected four-watch civil convention because a fractional military
         * watch would manufacture a precision convention this profile does
         * not claim historically.
         */
        if (show_seconds && period.daylight) {
            unsigned uncia =
                (unsigned)floorl(period.fraction * 12.0L);

            if (uncia > 11U) {
                uncia = 11U;
            }
            if (uncia != 0U) {
                return finish_output(buffer, capacity, length,
            snprintf(buffer, capacity, vertical ? "%s\n%s\n%s %s"
                             : "%s %s · %s %s", language != NULL ? infiltratr_i18n_get(language, "hour") : "Hora", roman[period.index], roman[uncia - 1U], language != NULL ? infiltratr_i18n_get(language,
                        uncia == 1U ? "twelfth" : "twelfths")
                        : (uncia == 1U ? "uncia" : "unciae")));
            }
        }

        return finish_output(buffer, capacity, length,
            snprintf(buffer, capacity, vertical ? "%s\n%s" : "%s %s", language != NULL ? infiltratr_i18n_get(language,
                period.daylight ? "hour" : "watch")
                : (period.daylight ? "Hora" : "Vigilia"), roman[period.index]));
    }

    ;

    ;

    ;

    return false;
}

static bool seasonal_format_clock_mode(const char *mode,
    int64_t unix_microseconds, int32_t utc_offset_seconds,
    bool show_seconds, bool vertical, bool location_configured,
    double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length)
{
    (void)mode;
    return format_clock_mode(NULL, mode, unix_microseconds, utc_offset_seconds,
        show_seconds, vertical, location_configured, latitude, longitude,
        buffer, capacity, length);
}

static bool seasonal_format_clock_mode_localized(
    const char *locale, const char *mode,
    int64_t unix_microseconds, int32_t utc_offset_seconds,
    bool show_seconds, bool vertical, bool location_configured,
    double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length)
{
    (void)mode;
    InfiltratrI18n language;
    if (!infiltratr_i18n_init(&language, clock_catalogs,
            sizeof(clock_catalogs) / sizeof(clock_catalogs[0]), "en")) {
        return false;
    }
    if (locale != NULL) (void)infiltratr_i18n_set_locale(&language, locale);
    return format_clock_mode(&language, mode, unix_microseconds, utc_offset_seconds,
        show_seconds, vertical, location_configured, latitude, longitude,
        buffer, capacity, length);
}

static bool render(const InfiltratrTemporalClockProvider *self,
    const char *locale, int64_t instant, int32_t offset, bool seconds,
    bool vertical, double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length)
{
    (void)self;
    if (locale == NULL)
        return seasonal_format_clock_mode(CLOCK_ID, instant, offset, seconds,
            vertical, true, latitude, longitude, buffer, capacity, length);
    return seasonal_format_clock_mode_localized(locale, CLOCK_ID, instant,
        offset, seconds, vertical, true, latitude, longitude,
        buffer, capacity, length);
}

/* Bracket and refine the first visible boundary against this very renderer.
 * Rendering and scheduling cannot disagree over moving seasonal boundaries,
 * Wendetage or floating-point quantisation. This is a bounded one-shot search.
 */
static bool next_tick(const InfiltratrTemporalClockProvider *self,
    int64_t instant, int32_t offset, bool seconds,
    double latitude, double longitude, uint32_t *milliseconds)
{
    char initial[256], future[256];
    uint32_t low = 0U, high = 1U, step = 1U;
    const uint32_t maximum = 129600000U;
    (void)self;
    if (milliseconds == NULL || !render(NULL, NULL, instant, offset, seconds,
            false, latitude, longitude, initial, sizeof(initial), NULL))
        return false;
    /* Solar events are undefined in polar N/A periods. Retry without
     * searching 36 hours of identical unavailable clock faces. */
    if (strncmp(initial, "N/A", 3U) == 0) {
        *milliseconds = 60000U;
        return true;
    }
    for (;;) {
        const int64_t delta = (int64_t)high * 1000;
        if (instant > INT64_MAX - delta ||
            !render(NULL, NULL, instant + delta, offset, seconds, false,
                latitude, longitude, future, sizeof(future), NULL))
            return false;
        if (strcmp(initial, future) != 0) break;
        if (high == maximum)
            return false; /* A valid clock must actually change. */
        low = high;
        if (step < 60000U) step = step > 30000U ? 60000U : step * 2U;
        high = maximum - high < step ? maximum : high + step;
    }
    while (high - low > 1U) {
        const uint32_t middle = low + (high - low) / 2U;
        if (!render(NULL, NULL, instant + (int64_t)middle * 1000, offset,
                seconds, false, latitude, longitude, future, sizeof(future), NULL))
            return false;
        if (strcmp(initial, future) == 0) low = middle;
        else high = middle;
    }
    *milliseconds = high;
    return true;
}

static const InfiltratrTemporalResourceMetadata metadata = {
    sizeof(InfiltratrTemporalResourceMetadata),
    INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    INFILTRATR_TEMPORAL_RESOURCE_CLOCK, CLOCK_ID, INFILTRATR_TEMPORAL_DISPLAY_NAME,
    CLOCK_SECONDS, CLOCK_LOCATION, CLOCK_LOCATION
};
static const InfiltratrTemporalClockProvider provider = {
    sizeof(InfiltratrTemporalClockProvider),
    INFILTRATR_TEMPORAL_CLOCK_PROVIDER_ABI, CLOCK_ID, render, next_tick
};
#if defined(_WIN32)
#define RESOURCE_EXPORT __declspec(dllexport)
#else
#define RESOURCE_EXPORT __attribute__((visibility("default")))
#endif
RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void);
RESOURCE_EXPORT const InfiltratrTemporalClockProvider *
infiltratr_temporal_clock_provider_v1(void);
RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void) { return &metadata; }
RESOURCE_EXPORT const InfiltratrTemporalClockProvider *
infiltratr_temporal_clock_provider_v1(void) { return &provider; }
