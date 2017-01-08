/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_RENDERVALUESCACHE_H
#define NATRON_ENGINE_RENDERVALUESCACHE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/KnobTypes.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief Small cache held on the thread local storage of a render per node to store values that should not change
 * throughout the whole render of a frame. This includes knob values, rotoshapes control points positions
 * animation curves, node inputs, etc....
 * This class is not thread safe because it is held on thread local storage.
 **/
struct RenderValuesCachePrivate;
class RenderValuesCache
{
public:

    RenderValuesCache();

    ~RenderValuesCache();

    template <typename T>
    bool getCachedKnobValue(const boost::shared_ptr<Knob<T> >& knob, TimeValue time, DimIdx dimension, ViewIdx view, T* value) const;

    template <typename T>
    void setCachedKnobValue(const boost::shared_ptr<Knob<T> >& knob, TimeValue time, DimIdx dimension, ViewIdx view, const T& value);

    CurvePtr getOrCreateCachedParametricKnobCurve(const KnobParametricPtr& knob, const CurvePtr& curve, DimIdx dimension) const;

private:

    boost::scoped_ptr<RenderValuesCachePrivate> _imp;

};


NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_RENDERVALUESCACHE_H
