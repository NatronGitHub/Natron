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

#ifndef MERGINGENUM_H
#define MERGINGENUM_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include <vector>
#include "Global/Enums.h"
#include "Engine/EngineFwd.h"

namespace Natron {


///Keep this in sync with @openfx-supportext/ofxsMerging.h
enum MergingFunctionEnum
{
    eMergeATop = 0,
    eMergeAverage,
    eMergeColor,
    eMergeColorBurn,
    eMergeColorDodge,
    eMergeConjointOver,
    eMergeCopy,
    eMergeDifference,
    eMergeDisjointOver,
    eMergeDivide,
    eMergeExclusion,
    eMergeFreeze,
    eMergeFrom,
    eMergeGeometric,
    eMergeGrainExtract,
    eMergeGrainMerge,
    eMergeHardLight,
    eMergeHue,
    eMergeHypot,
    eMergeIn,
    // eMergeInterpolated,
    eMergeLuminosity,
    eMergeMask,
    eMergeMatte,
    eMergeMax,
    eMergeMin,
    eMergeMinus,
    eMergeMultiply,
    eMergeOut,
    eMergeOver,
    eMergeOverlay,
    eMergePinLight,
    eMergePlus,
    eMergeReflect,
    eMergeSaturation,
    eMergeScreen,
    eMergeSoftLight,
    eMergeStencil,
    eMergeUnder,
    eMergeXOR
};

/**
 * @brief Returns the name of the merge oeprator as described in @openfx-supportext/ofxsMerging.h
 * Keep this in sync with the Merge node's operators otherwise everything will fall apart.
 **/
inline std::string
getNatronOperationString(Natron::MergingFunctionEnum operation)
{
    switch (operation) {
        case Natron::eMergeATop:
            
            return "atop";
        case Natron::eMergeAverage:
            
            return "average";
        case Natron::eMergeColor:
            
            return "color";
        case Natron::eMergeColorBurn:
            
            return "color-burn";
        case Natron::eMergeColorDodge:
            
            return "color-dodge";
        case Natron::eMergeConjointOver:
            
            return "conjoint-over";
        case Natron::eMergeCopy:
            
            return "copy";
        case Natron::eMergeDifference:
            
            return "difference";
        case Natron::eMergeDisjointOver:
            
            return "disjoint-over";
        case Natron::eMergeDivide:
            
            return "divide";
        case Natron::eMergeExclusion:
            
            return "exclusion";
        case Natron::eMergeFreeze:
            
            return "freeze";
        case Natron::eMergeFrom:
            
            return "from";
        case Natron::eMergeGeometric:
            
            return "geometric";
        case Natron::eMergeGrainExtract:
            
            return "grain-extract";
        case Natron::eMergeGrainMerge:
            
            return "grain-merge";
        case Natron::eMergeHardLight:
            
            return "hard-light";
        case Natron::eMergeHue:
            
            return "hue";
        case Natron::eMergeHypot:
            
            return "hypot";
        case Natron::eMergeIn:
            
            return "in";
        case Natron::eMergeLuminosity:
            
            return "luminosity";
        case Natron::eMergeMask:
            
            return "mask";
        case Natron::eMergeMatte:
            
            return "matte";
        case Natron::eMergeMax:
            
            return "max";
        case Natron::eMergeMin:
            
            return "min";
        case Natron::eMergeMinus:
            
            return "minus";
        case Natron::eMergeMultiply:
            
            return "multiply";
        case Natron::eMergeOut:
            
            return "out";
        case Natron::eMergeOver:
            
            return "over";
        case Natron::eMergeOverlay:
            
            return "overlay";
        case Natron::eMergePinLight:
            
            return "pinlight";
        case Natron::eMergePlus:
            
            return "plus";
        case Natron::eMergeReflect:
            
            return "reflect";
        case Natron::eMergeSaturation:
            
            return "saturation";
        case Natron::eMergeScreen:
            
            return "screen";
        case Natron::eMergeSoftLight:
            
            return "soft-light";
        case Natron::eMergeStencil:
            
            return "stencil";
        case Natron::eMergeUnder:
            
            return "under";
        case Natron::eMergeXOR:
            
            return "xor";
    } // switch
    
    return "unknown";
} // getOperationString

inline std::string
getNatronOperationHelpString(Natron::MergingFunctionEnum operation)
{
    switch (operation) {
        case Natron::eMergeATop:
            
            return "Ab + B(1 - a)";
        case Natron::eMergeAverage:
            
            return "(A + B) / 2";
        case Natron::eMergeColor:
            
            return "SetLum(A, Lum(B))";
        case Natron::eMergeColorBurn:
            
            return "darken B towards A";
        case Natron::eMergeColorDodge:
            
            return "brighten B towards A";
        case Natron::eMergeConjointOver:
            
            return "A + B(1-a)/b, A if a > b";
        case Natron::eMergeCopy:
            
            return "A";
        case Natron::eMergeDifference:
            
            return "abs(A-B)";
        case Natron::eMergeDisjointOver:
            
            return "A+B(1-a)/b, A+B if a+b < 1";
        case Natron::eMergeDivide:
            
            return "A/B, 0 if A < 0 and B < 0";
        case Natron::eMergeExclusion:
            
            return "A+B-2AB";
        case Natron::eMergeFreeze:
            
            return "1-sqrt(1-A)/B";
        case Natron::eMergeFrom:
            
            return "B-A";
        case Natron::eMergeGeometric:
            
            return "2AB/(A+B)";
        case Natron::eMergeGrainMerge:
            
            return "B + A - 0.5";
        case Natron::eMergeGrainExtract:
            
            return "B - A + 0.5";
        case Natron::eMergeHardLight:
            
            return "multiply if A < 0.5, screen if A > 0.5";
        case Natron::eMergeHue:
            
            return "SetLum(SetSat(A, Sat(B)), Lum(B))";
        case Natron::eMergeHypot:
            
            return "sqrt(A*A+B*B)";
        case Natron::eMergeIn:
            
            return "Ab";
        case Natron::eMergeLuminosity:
            
            return "SetLum(B, Lum(A))";
        case Natron::eMergeMask:
            
            return "Ba";
        case Natron::eMergeMatte:
            
            return "Aa + B(1-a) (unpremultiplied over)";
        case Natron::eMergeMax:
            
            return "max(A, B)";
        case Natron::eMergeMin:
            
            return "min(A, B)";
        case Natron::eMergeMinus:
            
            return "A-B";
        case Natron::eMergeMultiply:
            
            return "AB, 0 if A < 0 and B < 0";
        case Natron::eMergeOut:
            
            return "A(1-b)";
        case Natron::eMergeOver:
            
            return "A+B(1-a)";
        case Natron::eMergeOverlay:
            
            return "multiply if B<0.5, screen if B>0.5";
        case Natron::eMergePinLight:
            
            return "if B >= 0.5 then max(A, 2*B - 1), min(A, B * 2.0 ) else";
        case Natron::eMergePlus:
            
            return "A+B";
        case Natron::eMergeReflect:
            
            return "A*A / (1 - B)";
        case Natron::eMergeSaturation:
            
            return "SetLum(SetSat(B, Sat(A)), Lum(B))";
        case Natron::eMergeScreen:
            
            return "A+B-AB";
        case Natron::eMergeSoftLight:
            
            return "burn-in if A < 0.5, lighten if A > 0.5";
        case Natron::eMergeStencil:
            
            return "B(1-a)";
        case Natron::eMergeUnder:
            
            return "A(1-b)+B";
        case Natron::eMergeXOR:
            
            return "A(1-b)+B(1-a)";
    } // switch
    
    return "unknown";
} // getOperationHelpString
    
inline
Natron::PixmapEnum
getPixmapForMergingOperator(Natron::MergingFunctionEnum operation)
{
    switch (operation) {
        case Natron::eMergeATop:
            return NATRON_PIXMAP_MERGE_ATOP;
        case Natron::eMergeAverage:
            return NATRON_PIXMAP_MERGE_AVERAGE;
        case Natron::eMergeColor:
            return NATRON_PIXMAP_MERGE_COLOR;
        case Natron::eMergeColorBurn:
            return NATRON_PIXMAP_MERGE_COLOR_BURN;
        case Natron::eMergeColorDodge:
            return NATRON_PIXMAP_MERGE_COLOR_DODGE;
        case Natron::eMergeConjointOver:
            return NATRON_PIXMAP_MERGE_CONJOINT_OVER;
        case Natron::eMergeCopy:
            return NATRON_PIXMAP_MERGE_COPY;
        case Natron::eMergeDifference:
            return NATRON_PIXMAP_MERGE_DIFFERENCE;
        case Natron::eMergeDisjointOver:
            return NATRON_PIXMAP_MERGE_DISJOINT_OVER;
        case Natron::eMergeDivide:
            return NATRON_PIXMAP_MERGE_DIVIDE;
        case Natron::eMergeExclusion:
            return NATRON_PIXMAP_MERGE_EXCLUSION;
        case Natron::eMergeFreeze:
            return NATRON_PIXMAP_MERGE_FREEZE;
        case Natron::eMergeFrom:
            return NATRON_PIXMAP_MERGE_FROM;
        case Natron::eMergeGeometric:
            return NATRON_PIXMAP_MERGE_GEOMETRIC;
        case Natron::eMergeGrainExtract:
            return NATRON_PIXMAP_MERGE_GRAIN_EXTRACT;
        case Natron::eMergeGrainMerge:
            return NATRON_PIXMAP_MERGE_GRAIN_MERGE;
        case Natron::eMergeHardLight:
            return NATRON_PIXMAP_MERGE_HARD_LIGHT;
        case Natron::eMergeHue:
            return NATRON_PIXMAP_MERGE_HUE;
        case Natron::eMergeHypot:
            return NATRON_PIXMAP_MERGE_HYPOT;
        case Natron::eMergeIn:
            return NATRON_PIXMAP_MERGE_IN;
        case Natron::eMergeLuminosity:
            return NATRON_PIXMAP_MERGE_LUMINOSITY;
        case Natron::eMergeMask:
            return NATRON_PIXMAP_MERGE_MASK;
        case Natron::eMergeMatte:
            return NATRON_PIXMAP_MERGE_MATTE;
        case Natron::eMergeMax:
            return NATRON_PIXMAP_MERGE_MAX;
        case Natron::eMergeMin:
            return NATRON_PIXMAP_MERGE_MIN;
        case Natron::eMergeMinus:
            return NATRON_PIXMAP_MERGE_MINUS;
        case Natron::eMergeMultiply:
            return NATRON_PIXMAP_MERGE_MULTIPLY;
        case Natron::eMergeOut:
            return NATRON_PIXMAP_MERGE_OUT;
        case Natron::eMergeOver:
            return NATRON_PIXMAP_MERGE_OVER;
        case Natron::eMergeOverlay:
            return NATRON_PIXMAP_MERGE_OVERLAY;
        case Natron::eMergePinLight:
            return NATRON_PIXMAP_MERGE_PINLIGHT;
        case Natron::eMergePlus:
            return NATRON_PIXMAP_MERGE_PLUS;
        case Natron::eMergeReflect:
            return NATRON_PIXMAP_MERGE_REFLECT;
        case Natron::eMergeSaturation:
            return NATRON_PIXMAP_MERGE_SATURATION;
        case Natron::eMergeScreen:
            return NATRON_PIXMAP_MERGE_SCREEN;
        case Natron::eMergeSoftLight:
            return NATRON_PIXMAP_MERGE_SOFT_LIGHT;
        case Natron::eMergeStencil:
            return NATRON_PIXMAP_MERGE_STENCIL;
        case Natron::eMergeUnder:
            return NATRON_PIXMAP_MERGE_UNDER;
        case Natron::eMergeXOR:
            return NATRON_PIXMAP_MERGE_XOR;
    } // switch
    return NATRON_PIXMAP_MERGE_OVER;
}

///Keep this in sync with the MergeOperatorEnum !
inline void
getNatronCompositingOperators(std::vector<std::string>* operators,
                              std::vector<std::string>* toolTips)
{
    for (int i = 0; i <= (int)Natron::eMergeXOR; ++i) {
        operators->push_back(getNatronOperationString((Natron::MergingFunctionEnum)i));
        toolTips->push_back(getNatronOperationHelpString((Natron::MergingFunctionEnum)i));
    }
    
}

} // namespace Natron
    
#endif // MERGINGENUM_H
