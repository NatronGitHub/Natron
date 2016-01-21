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

#include "Global/Macros.h"

#include <string>
#include <vector>
#include "Global/Enums.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;


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

namespace Merge {

/**
 * @brief Returns the name of the merge oeprator as described in @openfx-supportext/ofxsMerging.h
 * Keep this in sync with the Merge node's operators otherwise everything will fall apart.
 **/
inline std::string
getOperatorString(MergingFunctionEnum operation)
{
    switch (operation) {
        case eMergeATop:
            
            return "atop";
        case eMergeAverage:
            
            return "average";
        case eMergeColor:
            
            return "color";
        case eMergeColorBurn:
            
            return "color-burn";
        case eMergeColorDodge:
            
            return "color-dodge";
        case eMergeConjointOver:
            
            return "conjoint-over";
        case eMergeCopy:
            
            return "copy";
        case eMergeDifference:
            
            return "difference";
        case eMergeDisjointOver:
            
            return "disjoint-over";
        case eMergeDivide:
            
            return "divide";
        case eMergeExclusion:
            
            return "exclusion";
        case eMergeFreeze:
            
            return "freeze";
        case eMergeFrom:
            
            return "from";
        case eMergeGeometric:
            
            return "geometric";
        case eMergeGrainExtract:
            
            return "grain-extract";
        case eMergeGrainMerge:
            
            return "grain-merge";
        case eMergeHardLight:
            
            return "hard-light";
        case eMergeHue:
            
            return "hue";
        case eMergeHypot:
            
            return "hypot";
        case eMergeIn:
            
            return "in";
        case eMergeLuminosity:
            
            return "luminosity";
        case eMergeMask:
            
            return "mask";
        case eMergeMatte:
            
            return "matte";
        case eMergeMax:
            
            return "max";
        case eMergeMin:
            
            return "min";
        case eMergeMinus:
            
            return "minus";
        case eMergeMultiply:
            
            return "multiply";
        case eMergeOut:
            
            return "out";
        case eMergeOver:
            
            return "over";
        case eMergeOverlay:
            
            return "overlay";
        case eMergePinLight:
            
            return "pinlight";
        case eMergePlus:
            
            return "plus";
        case eMergeReflect:
            
            return "reflect";
        case eMergeSaturation:
            
            return "saturation";
        case eMergeScreen:
            
            return "screen";
        case eMergeSoftLight:
            
            return "soft-light";
        case eMergeStencil:
            
            return "stencil";
        case eMergeUnder:
            
            return "under";
        case eMergeXOR:
            
            return "xor";
    } // switch
    
    return "unknown";
} // getOperationString

inline std::string
getOperatorHelpString(MergingFunctionEnum operation)
{
    switch (operation) {
        case eMergeATop:
            
            return "Ab + B(1 - a)";
        case eMergeAverage:
            
            return "(A + B) / 2";
        case eMergeColor:
            
            return "SetLum(A, Lum(B))";
        case eMergeColorBurn:
            
            return "darken B towards A";
        case eMergeColorDodge:
            
            return "brighten B towards A";
        case eMergeConjointOver:
            
            return "A + B(1-a)/b, A if a > b";
        case eMergeCopy:
            
            return "A";
        case eMergeDifference:
            
            return "abs(A-B)";
        case eMergeDisjointOver:
            
            return "A+B(1-a)/b, A+B if a+b < 1";
        case eMergeDivide:
            
            return "A/B, 0 if A < 0 and B < 0";
        case eMergeExclusion:
            
            return "A+B-2AB";
        case eMergeFreeze:
            
            return "1-sqrt(1-A)/B";
        case eMergeFrom:
            
            return "B-A";
        case eMergeGeometric:
            
            return "2AB/(A+B)";
        case eMergeGrainMerge:
            
            return "B + A - 0.5";
        case eMergeGrainExtract:
            
            return "B - A + 0.5";
        case eMergeHardLight:
            
            return "multiply if A < 0.5, screen if A > 0.5";
        case eMergeHue:
            
            return "SetLum(SetSat(A, Sat(B)), Lum(B))";
        case eMergeHypot:
            
            return "sqrt(A*A+B*B)";
        case eMergeIn:
            
            return "Ab";
        case eMergeLuminosity:
            
            return "SetLum(B, Lum(A))";
        case eMergeMask:
            
            return "Ba";
        case eMergeMatte:
            
            return "Aa + B(1-a) (unpremultiplied over)";
        case eMergeMax:
            
            return "max(A, B)";
        case eMergeMin:
            
            return "min(A, B)";
        case eMergeMinus:
            
            return "A-B";
        case eMergeMultiply:
            
            return "AB, 0 if A < 0 and B < 0";
        case eMergeOut:
            
            return "A(1-b)";
        case eMergeOver:
            
            return "A+B(1-a)";
        case eMergeOverlay:
            
            return "multiply if B<0.5, screen if B>0.5";
        case eMergePinLight:
            
            return "if B >= 0.5 then max(A, 2*B - 1), min(A, B * 2.0 ) else";
        case eMergePlus:
            
            return "A+B";
        case eMergeReflect:
            
            return "A*A / (1 - B)";
        case eMergeSaturation:
            
            return "SetLum(SetSat(B, Sat(A)), Lum(B))";
        case eMergeScreen:
            
            return "A+B-AB";
        case eMergeSoftLight:
            
            return "burn-in if A < 0.5, lighten if A > 0.5";
        case eMergeStencil:
            
            return "B(1-a)";
        case eMergeUnder:
            
            return "A(1-b)+B";
        case eMergeXOR:
            
            return "A(1-b)+B(1-a)";
    } // switch
    
    return "unknown";
} // getOperationHelpString
    
inline
PixmapEnum
getOperatorPixmap(MergingFunctionEnum operation)
{
    switch (operation) {
        case eMergeATop:
            return NATRON_PIXMAP_MERGE_ATOP;
        case eMergeAverage:
            return NATRON_PIXMAP_MERGE_AVERAGE;
        case eMergeColor:
            return NATRON_PIXMAP_MERGE_COLOR;
        case eMergeColorBurn:
            return NATRON_PIXMAP_MERGE_COLOR_BURN;
        case eMergeColorDodge:
            return NATRON_PIXMAP_MERGE_COLOR_DODGE;
        case eMergeConjointOver:
            return NATRON_PIXMAP_MERGE_CONJOINT_OVER;
        case eMergeCopy:
            return NATRON_PIXMAP_MERGE_COPY;
        case eMergeDifference:
            return NATRON_PIXMAP_MERGE_DIFFERENCE;
        case eMergeDisjointOver:
            return NATRON_PIXMAP_MERGE_DISJOINT_OVER;
        case eMergeDivide:
            return NATRON_PIXMAP_MERGE_DIVIDE;
        case eMergeExclusion:
            return NATRON_PIXMAP_MERGE_EXCLUSION;
        case eMergeFreeze:
            return NATRON_PIXMAP_MERGE_FREEZE;
        case eMergeFrom:
            return NATRON_PIXMAP_MERGE_FROM;
        case eMergeGeometric:
            return NATRON_PIXMAP_MERGE_GEOMETRIC;
        case eMergeGrainExtract:
            return NATRON_PIXMAP_MERGE_GRAIN_EXTRACT;
        case eMergeGrainMerge:
            return NATRON_PIXMAP_MERGE_GRAIN_MERGE;
        case eMergeHardLight:
            return NATRON_PIXMAP_MERGE_HARD_LIGHT;
        case eMergeHue:
            return NATRON_PIXMAP_MERGE_HUE;
        case eMergeHypot:
            return NATRON_PIXMAP_MERGE_HYPOT;
        case eMergeIn:
            return NATRON_PIXMAP_MERGE_IN;
        case eMergeLuminosity:
            return NATRON_PIXMAP_MERGE_LUMINOSITY;
        case eMergeMask:
            return NATRON_PIXMAP_MERGE_MASK;
        case eMergeMatte:
            return NATRON_PIXMAP_MERGE_MATTE;
        case eMergeMax:
            return NATRON_PIXMAP_MERGE_MAX;
        case eMergeMin:
            return NATRON_PIXMAP_MERGE_MIN;
        case eMergeMinus:
            return NATRON_PIXMAP_MERGE_MINUS;
        case eMergeMultiply:
            return NATRON_PIXMAP_MERGE_MULTIPLY;
        case eMergeOut:
            return NATRON_PIXMAP_MERGE_OUT;
        case eMergeOver:
            return NATRON_PIXMAP_MERGE_OVER;
        case eMergeOverlay:
            return NATRON_PIXMAP_MERGE_OVERLAY;
        case eMergePinLight:
            return NATRON_PIXMAP_MERGE_PINLIGHT;
        case eMergePlus:
            return NATRON_PIXMAP_MERGE_PLUS;
        case eMergeReflect:
            return NATRON_PIXMAP_MERGE_REFLECT;
        case eMergeSaturation:
            return NATRON_PIXMAP_MERGE_SATURATION;
        case eMergeScreen:
            return NATRON_PIXMAP_MERGE_SCREEN;
        case eMergeSoftLight:
            return NATRON_PIXMAP_MERGE_SOFT_LIGHT;
        case eMergeStencil:
            return NATRON_PIXMAP_MERGE_STENCIL;
        case eMergeUnder:
            return NATRON_PIXMAP_MERGE_UNDER;
        case eMergeXOR:
            return NATRON_PIXMAP_MERGE_XOR;
    } // switch
    return NATRON_PIXMAP_MERGE_OVER;
}

///Keep this in sync with the MergeOperatorEnum !
inline void
getOperatorStrings(std::vector<std::string>* operators,
                              std::vector<std::string>* toolTips)
{
    for (int i = 0; i <= (int)eMergeXOR; ++i) {
        operators->push_back(Merge::getOperatorString((MergingFunctionEnum)i));
        toolTips->push_back(Merge::getOperatorHelpString((MergingFunctionEnum)i));
    }
    
}

} // namespace Merge

NATRON_NAMESPACE_EXIT;
    
#endif // MERGINGENUM_H
