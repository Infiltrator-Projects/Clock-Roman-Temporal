// SPDX-License-Identifier: GPL-3.0-or-later
#include "infiltratr/arithmetic.h"

#include <limits.h>
#include <stdlib.h>

bool infiltratr_i64_floor_divmod(int64_t value, int64_t divisor,
                                 int64_t *quotient, int64_t *remainder)
{
    if (divisor <= 0 || (!quotient && !remainder)) return false;
    int64_t q = value / divisor;
    int64_t r = value % divisor;
    if (r < 0) {
        q--;
        r += divisor;
    }
    if (quotient) *quotient = q;
    if (remainder) *remainder = r;
    return true;
}


