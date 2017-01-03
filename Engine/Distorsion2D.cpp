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

#include "Distorsion2D.h"


#include <list>
#include "Engine/Transform.h"
NATRON_NAMESPACE_ENTER;

struct Distorsion2DStackPrivate
{
    std::list<DistorsionFunction2DPtr> stack;
};


Distorsion2DStack::Distorsion2DStack()
: _imp(new Distorsion2DStackPrivate)
{

}

Distorsion2DStack::~Distorsion2DStack()
{

}

void
Distorsion2DStack::pushDistorsion(const DistorsionFunction2DPtr& distorsion)
{
    // The distorsion is either a function or a transformation matrix.
    assert((distorsion->transformMatrix && !distorsion->func) || (!distorsion->transformMatrix && distorsion->func));

    if (_imp->stack.empty()) {
        _imp->stack.push_back(distorsion);
    } else {
        // If the last pushed distorsion is a matrix and this distorsion is also a matrix, concatenate
        DistorsionFunction2DPtr lastDistort = _imp->stack.back();
        if (lastDistort->transformMatrix && distorsion->transformMatrix) {
            *lastDistort->transformMatrix = Transform::matMul(*lastDistort->transformMatrix, *distorsion->transformMatrix);
        } else {
            // Cannot concatenate, append
            _imp->stack.push_back(distorsion);
        }

    }
}

const std::list<DistorsionFunction2DPtr>&
Distorsion2DStack::getStack() const
{
    return _imp->stack;
}


void
Distorsion2DStack::applyDistorsionStack(double distortedX, double distortedY, const Distorsion2DStack& stack, double* undistortedX, double* undistortedY)
{
    Transform::Point3D p(distortedX, distortedY, 1.);
    for (std::list<DistorsionFunction2DPtr>::const_iterator it = stack._imp->stack.begin(); it != stack._imp->stack.end(); ++it) {
        // If there's a matrix, apply, otherwise call the distorsion function
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