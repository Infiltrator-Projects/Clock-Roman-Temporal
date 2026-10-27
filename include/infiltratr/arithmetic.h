// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file arithmetic.h
 * @brief Dependency-free checked arithmetic and allocation-growth primitives.
 *
 * Checked operations leave caller output unchanged on failure. Saturating
 * operations never invoke signed/unsigned overflow and return the nearest
 * representable endpoint when the mathematical result is out of range.
 */
#ifndef INFILTRATR_COMMON_ARITHMETIC_H
#define INFILTRATR_COMMON_ARITHMETIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compute Euclidean floor division for a positive divisor.
 *
 * At least one output must be supplied. On success, `value = quotient *
 * divisor + remainder` with `0 <= remainder < divisor`, including for negative
 * values. A non-positive divisor or two NULL outputs is rejected.
 */
bool infiltratr_i64_floor_divmod(int64_t value, int64_t divisor,
                                 int64_t *quotient, int64_t *remainder);

/** Add signed integers, rejecting overflow or a NULL result. */
bool infiltratr_i64_add_checked(int64_t left, int64_t right, int64_t *result);
/** Multiply signed integers, rejecting overflow or a NULL result. */
bool infiltratr_i64_multiply_checked(int64_t left, int64_t right,
                                     int64_t *result);
/** Add signed integers and clamp overflow to INT64_MIN/INT64_MAX. */
int64_t infiltratr_i64_add_saturating(int64_t left, int64_t right);
/** Subtract signed integers and clamp overflow to INT64_MIN/INT64_MAX. */
int64_t infiltratr_i64_subtract_saturating(int64_t left, int64_t right);
/** Multiply signed integers and clamp overflow to INT64_MIN/INT64_MAX. */
int64_t infiltratr_i64_multiply_saturating(int64_t left, int64_t right);

/** Subtract unsigned integers, rejecting underflow or a NULL result. */
bool infiltratr_u64_subtract_checked(uint64_t left, uint64_t right,
                                     uint64_t *result);
/** Multiply unsigned integers, rejecting overflow or a NULL result. */
bool infiltratr_u64_multiply_checked(uint64_t left, uint64_t right,
                                     uint64_t *result);

/** Add allocation sizes, rejecting SIZE_MAX overflow or a NULL result. */
bool infiltratr_size_add_checked(size_t left, size_t right, size_t *result);
/** Multiply allocation sizes, rejecting SIZE_MAX overflow or a NULL result. */
bool infiltratr_size_multiply_checked(size_t left, size_t right,
                                      size_t *result);

/**
 * Ensure a contiguous allocation can hold at least `required` elements.
 *
 * Capacity grows geometrically from the existing capacity or
 * `initial_capacity` (falling back to one). `realloc` is used only after the
 * byte count is proven representable. On failure both the caller pointer and
 * capacity remain unchanged. A non-zero capacity requires a non-NULL array.
 */
bool infiltratr_array_reserve(void **array, size_t *capacity,
                              size_t element_size, size_t required,
                              size_t initial_capacity);

#ifdef __cplusplus
}
#endif
#endif
