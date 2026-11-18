/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_GLOBAL_MATHUTILS_H
#define NATRON_GLOBAL_MATHUTILS_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>

#include "../Global/Macros.h"


NATRON_NAMESPACE_ENTER

namespace MathUtils {

template<class T, class Compare>
constexpr const T&
clamp(const T& v, const T& lo, const T& hi, Compare comp)
{
    return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template<class T>
constexpr const T&
clamp(const T& v, const T& lo, const T& hi)
{
    return MathUtils::clamp(v, lo, hi, std::less<T>{});
}

// The raw bits of a floating-point value.
// Testing the bits is not enough with -ffast-math: clang marks the arguments of
// a fast-math function as never NaN or infinite (nofpclass(nan inf)), and then
// deduces the result of a bit test made on a memcpy of that value. The empty asm
// statement is an optimization barrier: the compiler can't know anything about
// the bits after it, so the test is really performed (same as openfx #151).
inline std::uint64_t
floatBits(double x)
{
    std::uint64_t bits;
    std::memcpy(&bits, &x, sizeof(bits));
#if defined(__GNUC__) // gcc and clang
    __asm__("" : "+r"(bits));
#endif

    return bits;
}

inline std::uint32_t
floatBits(float x)
{
    std::uint32_t bits;
    std::memcpy(&bits, &x, sizeof(bits));
#if defined(__GNUC__) // gcc and clang
    __asm__("" : "+r"(bits));
#endif

    return bits;
}

// isFinite(x) is true if x is neither NaN nor infinite.
// It tests the exponent bits instead of calling std::isfinite or comparing
// floating-point values, because -ffast-math (-ffinite-math-only) lets the
// compiler fold std::isnan, std::isinf, std::isfinite and x != x to constants.
// Use it only to validate values that come from outside (plug-ins, Python,
// files); computations must make sure they never create a NaN or an infinity.
inline bool
isFinite(double x)
{
    return (floatBits(x) & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL;
}

inline bool
isFinite(float x)
{
    return (floatBits(x) & 0x7f800000U) != 0x7f800000U;
}

// isNaN(x) is true if x is a NaN. See isFinite() for why it doesn't use std::isnan.
inline bool
isNaN(float x)
{
    return (floatBits(x) & 0x7fffffffU) > 0x7f800000U;
}

} // namespace MathUtils

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_GLOBAL_MATHUTILS_H
