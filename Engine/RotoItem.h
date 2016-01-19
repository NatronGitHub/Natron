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

#ifndef Engine_RotoItem_h
#define Engine_RotoItem_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <set>
#include <string>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QObject>
#include <QMutex>
#include <QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/EngineFwd.h"


#define kRotoLayerBaseName "Layer"
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

NATRON_NAMESPACE_ENTER;

struct RotoItemPrivate;
class RotoItem
: public QObject, public boost::enable_shared_from_this<RotoItem>
{
public:
    
    enum SelectionReasonEnum
    {
        eSelectionReasonOverlayInteract = 0, ///when the user presses an interact
        eSelectionReasonSettingsPanel, ///when the user interacts with the settings panel
        eSelectionReasonOther ///when the project loader restores the selection
    };


    RotoItem(const boost::shared_ptr<RotoContext>& context,
             const std::string & name,
             boost::shared_ptr<RotoLayer> parent = boost::shared_ptr<RotoLayer>());

    virtual ~RotoItem();

    virtual void clone(const RotoItem*  other);

    ///only callable on the main-thread
    bool setScriptName(const std::string & name);

    std::string getScriptName() const;
    
    std::string getFullyQualifiedName() const;
    
    std::string getLabel() const;
    
    void setLabel(const std::string& label);

    ///only callable on the main-thread
    void setParentLayer(boost::shared_ptr<RotoLayer> layer);

    ///MT-safe
    boost::shared_ptr<RotoLayer> getParentLayer() const;

    ///only callable from the main-thread
    void setGloballyActivated(bool a, bool setChildren);

    ///MT-safe
    bool isGloballyActivated() const;

    bool isDeactivatedRecursive() const;

    void setLocked(bool l,bool lockChildren,RotoItem::SelectionReasonEnum reason);
    bool getLocked() const;

    bool isLockedRecursive() const;

    /**
     * @brief Returns at which hierarchy level the item is.
     * The base layer is 0.
     * All items into that base layer are on level 1.
     * etc...
     **/
    int getHierarchyLevel() const;

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj);

    /**
     * @brief Returns the name of the node holding this item
     **/
    std::string getRotoNodeName() const;

    boost::shared_ptr<RotoContext> getContext() const;
    
    boost::shared_ptr<RotoItem> getPreviousItemInLayer() const;

protected:


    ///This mutex protects every-member this class and the derived class might have.
    ///That is for the RotoItem class:
    ///  - name
    ///  - globallyActivated
    ///  - locked
    ///  - parentLayer
    ///
    ///For the RotoDrawableItem:
    ///  - overlayColor
    ///
    ///For the RotoLayer class:
    ///  - items
    ///
    ///For the Bezier class:
    ///  - points
    ///  - featherPoints
    ///  - finished
    ///  - pointsAtDistance
    ///  - featherPointsAtDistance
    ///  - featherPointsAtDistanceVal
    mutable QMutex itemMutex;

private:

    void setGloballyActivated_recursive(bool a);
    void setLocked_recursive(bool locked,RotoItem::SelectionReasonEnum reason);

    boost::scoped_ptr<RotoItemPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

Q_DECLARE_METATYPE(Natron::RotoItem*);

#endif // Engine_RotoItem_h
