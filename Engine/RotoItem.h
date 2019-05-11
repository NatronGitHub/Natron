/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_RotoItem_h
#define Engine_RotoItem_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/KnobItemsTable.h"
#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


#define kRotoLayerBaseName "Group"
#define kPlanarTrackLayerBaseName "PlanarTrackGroup"
#define kRotoBezierBaseName "Bezier"
#define kRotoOpenBezierBaseName "Pencil"
#define kRotoEllipseBaseName "Ellipse"
#define kRotoRectangleBaseName "Rectangle"
#define kRotoPaintBrushBaseName "Brush"
#define kRotoPaintEraserBaseName "Eraser"
#define kRotoPaintBlurBaseName "Blur"
#define kRotoPaintSmearBaseName "Smear"
#define kRotoPaintSharpenBaseName "Sharpen"
#define kRotoPaintCloneBaseName "Clone"
#define kRotoPaintRevealBaseName "Reveal"
#define kRotoPaintDodgeBaseName "Dodge"
#define kRotoPaintBurnBaseName "Burn"
#define kRotoCompItemBaseName "Layer"

#define kParamRotoItemEnabled "enabledButton"
#define kParamRotoItemEnabledLabel "Enabled"
#define kParamRotoItemEnabledHint "When unchecked, the parameter will not be rendered.\n" \
"This is a global switch that is saved with the project and supersedes the life time parameter."

#define kParamRotoItemLocked "lockedButton"
#define kParamRotoItemLockedLabel "Locked"
#define kParamRotoItemLockedHint "When checked, the parameter is no longer editable in the viewer and its parameters are greyed out."

#define kParamRotoItemSolo "soloButton"
#define kParamRotoItemSoloLabel "Solo"
#define kParamRotoItemSoloHint "Includes the current item in renders, ignoring others without this switch set"


struct RotoItemPrivate;
class RotoItem
    : public KnobTableItem
{
public:

    // This class is virtual pure so no need to privatize the constructor

    RotoItem(const KnobItemsTablePtr& model);

    RotoItem(const RotoItemPtr& other, const FrameViewRenderKey& key);

    virtual ~RotoItem();

    virtual bool isItemContainer() const OVERRIDE { return false; }
    
    ///MT-safe
    bool isGloballyActivated() const;

    bool isGloballyActivatedRecursive() const;

    bool isLockedRecursive() const;

    KnobButtonPtr getLockedKnob() const;

    KnobButtonPtr getActivatedKnob() const;

    KnobButtonPtr getSoloKnob() const;

    virtual std::string getBaseItemName() const OVERRIDE = 0;

    void getTransformAtTime(TimeValue time, ViewIdx view, Transform::Matrix3x3* matrix) const;

protected:

    virtual bool getTransformAtTimeInternal(TimeValue time, ViewIdx view, Transform::Matrix3x3* matrix) const;

    virtual void fetchRenderCloneKnobs() OVERRIDE;

    virtual void initializeKnobs() OVERRIDE;


    virtual bool onKnobValueChanged(const KnobIPtr& k,
                                    ValueChangedReasonEnum reason,
                                    TimeValue time,
                                    ViewSetSpec view) OVERRIDE;

private:

    boost::scoped_ptr<RotoItemPrivate> _imp;
};


inline RotoItemPtr
toRotoItem(const KnobHolderPtr& holder)
{
    return boost::dynamic_pointer_cast<RotoItem>(holder);
}

inline RotoItemConstPtr
toRotoItem(const KnobHolderConstPtr& holder)
{
    return boost::dynamic_pointer_cast<const RotoItem>(holder);
}


NATRON_NAMESPACE_EXIT


Q_DECLARE_METATYPE(NATRON_NAMESPACE::RotoItem*);

#endif // Engine_RotoItem_h
