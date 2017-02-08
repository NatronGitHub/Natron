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

#ifndef Engine_Distortion2D_h
#define Engine_Distortion2D_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include <ofxNatron.h>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct DistortionFunction2D
{
    DistortionFunction2D();
    
    DistortionFunction2D(const DistortionFunction2D& other);

    // Index of the input to distort
    int inputNbToDistort;

    // This is set if the transformation can be represented as a 3x3 matrix (in canonical coordinates)
    boost::shared_ptr<Transform::Matrix3x3> transformMatrix;

    // These below are set if the distortion cannot be represented as a 3x3 matrix

    // A pointer to the distortion function of the plug-in
    OfxDistortionFunctionV1 func;

    // A pointer to plug-in data that should be passed back to the distortion function
    void* customData;

    // A size in bytes estimated of the data held in customData to hint the cache about the size.
    int customDataSizeHintInBytes;

    // A pointer to a function to free the customData
    OfxDistortionFreeDataFunctionV1 customDataFreeFunc;
};

/**
 * @brief Represents a chain of distortion to apply to distorted positions to retrieve the undistorted position.
 **/
struct Distortion2DStackPrivate;
class Distortion2DStack
{
public:

    Distortion2DStack();

    ~Distortion2DStack();

    /**
     * @brief Appends a new distortion function to apply.
     **/
    void pushDistortion(const DistortionFunction2DPtr& distortion);

    /**
     * @brief Appends a stack of distortion functions to apply.
     **/
    void pushDistortionStack(const Distortion2DStack& stack);

    const std::list<DistortionFunction2DPtr>& getStack() const;

    /**
     * @brief Applies a distortion stack onto a 2D position in canonical coordinates.
     **/
    static void applyDistortionStack(double distortedX, double distortedY, const Distortion2DStack& stack, double* undistortedX, double* undistortedY);

private:

    boost::scoped_ptr<Distortion2DStackPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;


#endif // Engine_Distortion2D_h
