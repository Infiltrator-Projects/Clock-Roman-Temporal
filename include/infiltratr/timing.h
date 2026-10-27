// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file timing.h
 * @brief Portable elapsed, periodic and fixed-step timing policy primitives.
 *
 * @author Shannon Smith
 * @copyright Copyright (c) 1993-2026 Shannon Smith
 * @license GPL-3.0-or-later
 */
#ifndef INFILTRATR_COMMON_TIMING_H
#define INFILTRATR_COMMON_TIMING_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decide whether a monotonic interval should run now.
 *
 * A missing/invalid baseline, invalid interval, non-finite clock value or
 * clock rollback is treated as due so callers can safely re-establish state.
 *
 * @param [in] now Current monotonic time in caller-selected units.
 * @param [in] last Previous successful time in the same units.
 * @param [in] interval Required positive interval in the same units.
 * @return true when work is due or the baseline should be re-established.
 */
bool infiltratr_interval_due(double now, double last, double interval);

/**
 * Exact rational fixed-step scheduler state.
 *
 * The scheduler consumes an integer monotonic tick counter and converts elapsed
 * ticks into simulation steps without a floating-point accumulator. For a
 * nanosecond clock running a 60 Hz simulation, for example, configure
 * ticks_per_second=1000000000 and steps_per_second=60.
 *
 * The fractional phase is retained exactly as a numerator over
 * ticks_per_second, so rates such as 60 Hz do not accumulate rounding drift.
 * Consumers should treat the fields as read-only and mutate the scheduler only
 * through the functions below.
 */
typedef struct InfiltratrFixedStepScheduler {
    uint64_t ticks_per_second;
    uint64_t steps_per_second;
    uint64_t max_elapsed_ticks;
    uint64_t max_steps_per_update;
    uint64_t previous_tick;
    uint64_t phase_numerator;
    bool initialized;
} InfiltratrFixedStepScheduler;

/** Result from one fixed-step scheduler advance. */
typedef struct InfiltratrFixedStepResult {
    uint64_t steps_to_run;
    uint64_t dropped_steps;
    uint64_t clamped_ticks;
    uint64_t phase_numerator;
    bool clock_reset;
} InfiltratrFixedStepResult;

/**
 * Configure an exact fixed-step scheduler.
 *
 * `ticks_per_second` describes the caller's monotonic clock. `steps_per_second`
 * is the desired simulation cadence. A simulation rate higher than the clock
 * resolution is rejected because such boundaries cannot be represented by the
 * supplied clock. `max_elapsed_ticks` clamps a single delayed frame and
 * `max_steps_per_update` bounds catch-up work to prevent a spiral of death.
 *
 * Configuration clears the previous baseline and fractional phase.
 *
 * @return false for NULL state, zero values, or steps_per_second greater than
 *         ticks_per_second. Caller state is unchanged on failure.
 */
bool infiltratr_fixed_step_configure(InfiltratrFixedStepScheduler *scheduler,
                                     uint64_t ticks_per_second,
                                     uint64_t steps_per_second,
                                     uint64_t max_elapsed_ticks,
                                     uint64_t max_steps_per_update);

/**
 * Reset a configured scheduler to a known monotonic baseline.
 *
 * Fractional phase is discarded. This is useful after pause/resume or an
 * intentional timeline discontinuity.
 */
bool infiltratr_fixed_step_reset(InfiltratrFixedStepScheduler *scheduler,
                                 uint64_t now_tick);

/**
 * Advance a fixed-step scheduler to `now_tick`.
 *
 * The first sample establishes a baseline and returns zero steps. Normal
 * samples convert elapsed ticks to whole simulation steps using exact integer
 * rational arithmetic. Long frames are first clamped by `max_elapsed_ticks`,
 * then catch-up is limited by `max_steps_per_update`; skipped whole steps are
 * reported in `dropped_steps`. Fractional phase is preserved.
 *
 * If the monotonic source moves backwards, the scheduler automatically resets
 * its baseline and phase, returns zero steps, and sets `clock_reset=true`.
 *
 * @return false for invalid/unconfigured state, NULL result, or an
 *         unrepresentable arithmetic result. Result is unchanged on failure.
 */
bool infiltratr_fixed_step_advance(InfiltratrFixedStepScheduler *scheduler,
                                   uint64_t now_tick,
                                   InfiltratrFixedStepResult *result);

/**
 * Return the interpolation alpha in [0,1) for the current fractional phase.
 *
 * Alpha is derived only for rendering/interpolation convenience; scheduling
 * itself remains exact integer arithmetic.
 */
bool infiltratr_fixed_step_alpha(const InfiltratrFixedStepScheduler *scheduler,
                                 long double *alpha);

/**
 * Calculate the positive distance to the next repeating boundary.
 *
 * `position` and `period` use caller-selected but identical units. Exact
 * boundaries return one full period rather than zero, which is the useful
 * result for one-shot scheduling after a boundary has just been processed.
 * Negative positions are handled with Euclidean phase semantics.
 *
 * This floating-point form is intended for genuinely continuous quantities.
 * When position and period are integral, use
 * infiltratr_i64_period_remaining() so boundary classification is exact.
 *
 * @return false for NULL output, non-finite input or a non-positive period;
 *         caller output is unchanged on failure.
 */
bool infiltratr_period_remaining(long double position, long double period,
                                 long double *remaining);

/**
 * Calculate the exact positive integer distance to the next repeating boundary.
 *
 * `period` must be strictly positive. The position may be any signed 64-bit
 * value; Euclidean remainder semantics make negative positions behave as the
 * mathematical repeating phase. An exact boundary returns one full period.
 *
 * @return false for NULL output or a non-positive period. Caller output is
 *         unchanged on failure.
 */
bool infiltratr_i64_period_remaining(int64_t position, int64_t period,
                                     uint64_t *remaining);

/**
 * Partition an integer cycle into an arbitrary number of equal rational parts.
 *
 * `position` is reduced modulo `cycle_length`. `partition_index` receives the
 * exact value
 *
 *   floor((position mod cycle_length) * partition_count / cycle_length)
 *
 * without overflowing a 64-bit intermediate, even when the mathematical
 * product exceeds UINT64_MAX. `units_until_next`, when requested, receives the
 * smallest positive whole number of input units that is not earlier than the
 * next exact rational partition boundary. At an exact boundary this therefore
 * describes the complete next partition rather than zero.
 *
 * Either output may be NULL, but at least one output must be supplied. This
 * operation performs no floating-point arithmetic and is exact across the
 * complete uint64_t input domain.
 *
 * @return false for zero cycle/partition counts or when both outputs are NULL;
 *         supplied outputs are unchanged on failure.
 */
bool infiltratr_cycle_partition_u64(uint64_t position,
                                    uint64_t cycle_length,
                                    uint64_t partition_count,
                                    uint64_t *partition_index,
                                    uint64_t *units_until_next);

/**
 * Convert a positive whole-microsecond duration to an upward-rounded
 * millisecond delay using exact integer arithmetic.
 *
 * Positive sub-millisecond values become one millisecond. The complete
 * uint64_t microsecond domain is representable after division, so this
 * operation cannot overflow its result.
 *
 * @return false for NULL output or zero duration; caller output is unchanged
 *         on failure.
 */
bool infiltratr_microseconds_to_milliseconds_ceil(uint64_t microseconds,
                                                   uint64_t *milliseconds);

/**
 * Convert a positive finite duration in seconds to an upward-rounded
 * millisecond delay.
 *
 * Positive sub-millisecond durations become one millisecond. Values above the
 * uint64 range saturate to UINT64_MAX. Invalid input leaves caller output
 * unchanged and returns false.
 */
bool infiltratr_seconds_to_milliseconds_ceil(long double seconds,
                                              uint64_t *milliseconds);

/**
 * Advance a periodic deadline to the first boundary strictly after `now`.
 *
 * If `deadline` is already in the future it advances by exactly one interval,
 * matching the semantics of marking that scheduled occurrence as dispatched.
 * If one or more occurrences were missed, they are skipped without cadence
 * drift. Arithmetic saturates rather than wrapping.
 *
 * @return false for a zero interval or NULL output; caller output is unchanged
 *         on failure.
 */
bool infiltratr_periodic_deadline_advance(uint64_t deadline,
                                          uint64_t now,
                                          uint64_t interval,
                                          uint64_t *next_deadline);

#ifdef __cplusplus
}
#endif

#endif
