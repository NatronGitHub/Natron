/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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
NATRON_NAMESPACE_ENTER

DistortionFunction2D::DistortionFunction2D()
: inputNbToDistort(-1)
, transformMatrix()
, func(0)
, customData(0)
, customDataSizeHintInBytes(0)
, customDataFreeFunc(0)
{

}

DistortionFunction2D::~DistortionFunction2D()
{
    if (customDataFreeFunc) {
        customDataFreeFunc(customData);
    }
}


struct Distortion2DStackPrivate
{
    // The effect from which we must retrieve the image
    EffectInstancePtr inputImageEffect;
    std::list<DistortionFunction2DPtr> stack;
};


Distortion2DStack::Distortion2DStack()
: _imp(new Distortion2DStackPrivate)
{

}

Distortion2DStack::~Distortion2DStack()
{
}

EffectInstancePtr
Distortion2DStack::getInputImageEffect() const
{
    return _imp->inputImageEffect;
}

void
Distortion2DStack::setInputImageEffect(const EffectInstancePtr& effect)
{
    _imp->inputImageEffect = effect;
}

void
Distortion2DStack::pushDistortionFunction(const DistortionFunction2DPtr& distortion)
{
    // The distortion is either a function or a transformation matrix.
    assert((!distortion->transformMatrix && distortion->func) ||
           (distortion->transformMatrix && !distortion->func));

    if (!distortion->transformMatrix) {
        _imp->stack.push_back(distortion);
    } else {
        if (_imp->stack.empty()) {
            _imp->stack.push_back(distortion);

        } else {
            // If the last pushed distortion is a matrix and this distortion is also a matrix, concatenate
            DistortionFunction2DPtr lastDistort = _imp->stack.back();
            if (lastDistort->transformMatrix) {
                *lastDistort->transformMatrix = Transform::matMul(*lastDistort->transformMatrix, *distortion->transformMatrix);
            } else {
                // Cannot concatenate, append
                _imp->stack.push_back(distortion);
            }
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
Distortion2DStack::applyDistortionStack(double distortedX, double distortedY, const Distortion2DStack& stack, double* undistortedX, double* undistortedY, bool wantsJacobian, bool* gotJacobianOut, double jacobian[4])
{

    // The jacobian is in the form : dFx/dx, dFx/dy, dFy/dx, dFy/dy
    // We always compute the jacobian if requested
    assert(!wantsJacobian || (gotJacobianOut && jacobian));
    if (gotJacobianOut) {
        *gotJacobianOut = wantsJacobian;
    }

    Transform::Point3D p(distortedX, distortedY, 1.);

    bool jacobianSet = false;
    for (std::list<DistortionFunction2DPtr>::const_reverse_iterator it = stack._imp->stack.rbegin(); it != stack._imp->stack.rend(); ++it) {
        // If there's a matrix, apply, otherwise call the distortion function

        double J[4];
        if ((*it)->transformMatrix) {

            const Transform::Matrix3x3& H = *(*it)->transformMatrix;
            p = Transform::matApply(H, p);

            if (wantsJacobian) {
                // Compute the jacobian of the transformation
                J[0] = (H(0,0) * p.z - p.x * H(2,0)) / (p.z * p.z);
                J[1] = (H(0,1) * p.z - p.x * H(2,1)) / (p.z * p.z);
                J[2] = (H(1,0) * p.z - p.y * H(2,0)) / (p.z * p.z);
                J[3] = (H(1,1) * p.z - p.y * H(2,1)) / (p.z * p.z);
            }
            p.x /= p.z;
            p.y /= p.z;

        } else {
            bool gotJacobian;
            (*it)->func( (*it)->customData, p.x, p.y, wantsJacobian, &p.x, &p.y, &gotJacobian, J);

            if (wantsJacobian && !gotJacobian) {
                // Compute the jacobian with centered finite differences
                // The epsilon used for finite differences here is 0.5 because we want to evaluate the jacobian for a pixel at its center point (0.5,0.5)
                Point pxHigh,pxLow;
                (*it)->func((*it)->customData, p.x + 0.5, p.y, false, &pxHigh.x, &pxHigh.y, 0, 0);
                (*it)->func((*it)->customData, p.x - 0.5, p.y, false, &pxLow.x, &pxLow.y, 0, 0);

                Point pyHigh,pyLow;
                (*it)->func( (*it)->customData, p.x, p.y + 0.5, false, &pyHigh.x, &pyHigh.y, 0, 0);
                (*it)->func( (*it)->customData, p.x, p.y - 0.5, false, &pyLow.x, &pyLow.y, 0, 0);

                // dFx/dx = (f(x + h) - f(x - h)) / 2h   here h = 0.5 so 2h = 1
                J[0] = pxHigh.x - pxLow.x;

                // dFx/dy
                J[1] = pyHigh.x - pyLow.x;

                // dFy/dx
                J[2] = pxHigh.y - pxLow.y;

                // dFy/dy
                J[3] = pyHigh.y - pyLow.y;
            }
        }

        if (wantsJacobian) {
            if (!jacobianSet) {
                jacobianSet = true;
                memcpy(jacobian, J, sizeof(double) * 4);
            } else {
                // Concatenate jacobians
                jacobian[0] = J[0] * jacobian[0] + J[1] * jacobian[2];
                jacobian[1] = J[0] * jacobian[1] + J[1] * jacobian[3];
                jacobian[2] = J[2] * jacobian[0] + J[3] * jacobian[2];
                jacobian[3] = J[2] * jacobian[1] + J[3] * jacobian[3];
            }
        }
    }
    *undistortedX = p.x;
    *undistortedY = p.y;
} // applyDistortionStack

NATRON_NAMESPACE_EXIT
