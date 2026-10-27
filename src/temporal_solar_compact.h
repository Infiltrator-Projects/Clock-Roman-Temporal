// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef INFILTRATR_TEMPORAL_SOLAR_COMPACT_H
#define INFILTRATR_TEMPORAL_SOLAR_COMPACT_H
/*
 * Private NOAA/GML compact solar model. Independently linked clock resources
 * compile these same equations without linking to another provider.
 *
 * Input: signed Unix microseconds. Outputs: equation of time in minutes,
 * solar declination in radians. This model is not an observational ephemeris.
 * Callers own historical interpretation and sunrise/sunset conventions.
 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

static inline int64_t temporal_solar_floor_div(int64_t n, int64_t d)
{
    const int64_t quotient = n / d;
    return n % d < 0 ? quotient - 1 : quotient;
}

static inline int64_t temporal_solar_mod(int64_t n, int64_t d)
{
    const int64_t remainder = n % d;
    return remainder < 0 ? remainder + d : remainder;
}

/* Gregorian 400-year arithmetic without time_t, valid for all int64_t
 * microsecond instants. A negative date retains a positive UTC-day phase. */
static inline bool temporal_solar_day_of_year_with_leap(int64_t instant,
                                                         int *ordinal,
                                                         bool *leap_year)
{
    static const int cumulative_days[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    const int64_t days =
        temporal_solar_floor_div(instant, INT64_C(86400000000));
    const int64_t z = days + INT64_C(719468);
    const int64_t era = temporal_solar_floor_div(z, INT64_C(146097));
    const int64_t doe = z - era * INT64_C(146097);
    const int64_t yoe = (doe - doe / 1460 + doe / 36524 -
                         doe / 146096) / 365;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int64_t mp = (5 * doy + 2) / 153;
    const int day = (int)(doy - (153 * mp + 2) / 5 + 1);
    const int month = (int)(mp + (mp < 10 ? 3 : -9));
    const int64_t year = yoe + era * 400 + (month <= 2);
    const bool leap =
        (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (ordinal == NULL || month < 1 || month > 12 ||
        day < 1 || day > 31) return false;
    *ordinal = cumulative_days[month - 1] + day +
               (leap && month > 2 ? 1 : 0);
    if (leap_year != NULL) *leap_year = leap;
    return true;
}

static inline bool temporal_solar_day_of_year(int64_t instant, int *ordinal)
{
    return temporal_solar_day_of_year_with_leap(instant, ordinal, NULL);
}

static inline bool temporal_solar_compact_terms(int64_t instant,
                                                double *equation_minutes,
                                                double *declination_radians)
{
    const long double pi = 3.141592653589793238462643383279502884L;
    const long double utc_hour =
        (long double)temporal_solar_mod(instant, INT64_C(86400000000)) /
        3600000000.0L;
    int ordinal;
    long double gamma;

    if (!temporal_solar_day_of_year(instant, &ordinal)) return false;
    gamma = 2.0L * pi / 365.0L *
            ((long double)ordinal - 1.0L +
             (utc_hour - 12.0L) / 24.0L);
    if (equation_minutes != NULL)
        *equation_minutes = 229.18 *
            (0.000075 + 0.001868 * cos((double)gamma) -
             0.032077 * sin((double)gamma) -
             0.014615 * cos((double)(2.0L * gamma)) -
             0.040849 * sin((double)(2.0L * gamma)));
    if (declination_radians != NULL)
        *declination_radians =
            0.006918 - 0.399912 * cos((double)gamma) +
            0.070257 * sin((double)gamma) -
            0.006758 * cos((double)(2.0L * gamma)) +
            0.000907 * sin((double)(2.0L * gamma)) -
            0.002697 * cos((double)(3.0L * gamma)) +
            0.001480 * sin((double)(3.0L * gamma));
    return true;
}

/* Geographic bounds shared by clock providers, without relaxing their ABI. */
static inline bool temporal_solar_valid_longitude(double longitude)
{
    return isfinite(longitude) && longitude >= -180.0 && longitude <= 180.0;
}

static inline double temporal_solar_clamp_coordinate(double value, double limit)
{
    if (!isfinite(value)) return 0.0;
    if (value < -limit) return -limit;
    if (value > limit) return limit;
    return value;
}

static inline double temporal_solar_clamp_longitude(double longitude)
{
    return temporal_solar_clamp_coordinate(longitude, 180.0);
}

static inline double temporal_solar_clamp_latitude(double latitude)
{
    return temporal_solar_clamp_coordinate(latitude, 90.0);
}

/* One solar hour-angle equation for absolute clock resources and elapsed
 * seasonal durations. Each caller still owns its event-time and era rules.
 * Failed/polar geometry does not modify the caller's result. */
static inline bool temporal_solar_hour_angle(
    double latitude, double declination_radians,
    double solar_depression_degrees, long double *angle_radians)
{
    const long double pi = 3.141592653589793238462643383279502884L;
    const double latitude_radians =
        temporal_solar_clamp_latitude(latitude) * (double)pi / 180.0;
    double zenith_radians;
    double cosine_lat_decl;
    double cosine_hour_angle;

    if (angle_radians == NULL ||
        !isfinite(declination_radians) ||
        !isfinite(solar_depression_degrees) ||
        solar_depression_degrees < 0.0 ||
        solar_depression_degrees >= 90.0) {
        return false;
    }
    zenith_radians =
        (90.0 + solar_depression_degrees) * (double)pi / 180.0;
    cosine_lat_decl = cos(latitude_radians) * cos(declination_radians);
    if (fabs(cosine_lat_decl) < 1.0e-12) return false;

    cosine_hour_angle = cos(zenith_radians) / cosine_lat_decl -
        tan(latitude_radians) * tan(declination_radians);
    if (cosine_hour_angle < -1.0 || cosine_hour_angle > 1.0) return false;

    *angle_radians = acosl((long double)cosine_hour_angle);
    return true;
}
#endif
