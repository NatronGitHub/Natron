/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Distortion2D.h"


#include <list>
#include "Engine/EffectInstance.h"
#include "Engine/Transform.h"
NATRON_NAMESPACE_ENTER;

DistortionFunction2D::DistortionFunction2D()
: inputNbToDistort(-1)
, transformMatrix()
, func(0)
, customData(0)
, customDataSizeHintInBytes(0)
, customDataFreeFunc(0)
, effect()
{

}

DistortionFunction2D::~DistortionFunction2D()
{
    EffectInstancePtr e = effect.lock();
    if (e) {
        customDataFreeFunc(customData);
    }
}


struct Distortion2DStackPrivate
{
    std::list<DistortionFunction2DPtr> stack;
};


Distortion2DStack::Distortion2DStack()
: _imp(new Distortion2DStackPrivate)
{

}

Distortion2DStack::~Distortion2DStack()
{
}

void
Distortion2DStack::pushDistortionFunction(const DistortionFunction2DPtr& distortion)
{
    // The distortion is either a function or a transformation matrix.
    assert(!distortion->transformMatrix && distortion->func);
    _imp->stack.push_back(distortion);
}

void
Distortion2DStack::pushTransformMatrix(const Transform::Matrix3x3& transfo)
{
    if (_imp->stack.empty()) {
        DistortionFunction2DPtr disto(new DistortionFunction2D);
        disto->transformMatrix.reset(new Transform::Matrix3x3(transfo));
    } else {
        // If the last pushed distortion is a matrix and this distortion is also a matrix, concatenate
        DistortionFunction2DPtr lastDistort = _imp->stack.back();
        if (lastDistort->transformMatrix) {
            *lastDistort->transformMatrix = Transform::matMul(*lastDistort->transformMatrix, transfo);
        } else {
            // Cannot concatenate, append
            DistortionFunction2DPtr disto(new DistortionFunction2D);
            disto->transformMatrix.reset(new Transform::Matrix3x3(transfo));
            _imp->stack.push_back(disto);
        }
        
    }
}

void
Distortion2DStack::pushDistortionStack(const Distortion2DStack& stack)
{
    const std::list<DistortionFunction2DPtr>& distos = stack.getStack();
    for (std::list<DistortionFunction2DPtr>::const_iterator it = distos.begin(); it != distos.end(); ++it) {
        pushDistortionFunction(*it);
    }
}

const std::list<DistortionFunction2DPtr>&
Distortion2DStack::getStack() const
{
    return _imp->stack;
}


void
Distortion2DStack::applyDistortionStack(double distortedX, double distortedY, const Distortion2DStack& stack, double* undistortedX, double* undistortedY)
{
    Transform::Point3D p(distortedX, distortedY, 1.);
    for (std::list<DistortionFunction2DPtr>::const_iterator it = stack._imp->stack.begin(); it != stack._imp->stack.end(); ++it) {
        // If there's a matrix, apply, otherwise call the distortion function
        if ((*it)->transformMatrix) {
            p = Transform::matApply(*(*it)->transformMatrix, p);
            p.x /= p.z;
            p.y /= p.z;
        } else {
            (*it)->func(p.x, p.y, (*it)->customData, &p.x, &p.y);
        }
    }
    *undistortedX = p.x;
    *undistortedY = p.y;
}

NATRON_NAMESPACE_EXIT;
