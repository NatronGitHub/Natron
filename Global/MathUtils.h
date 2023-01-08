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
    return clamp(v, lo, hi, std::less<T>{});
}

} // namespace MathUtils

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_GLOBAL_MATHUTILS_H
