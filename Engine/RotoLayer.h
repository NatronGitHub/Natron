/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_RotoLayer_h
#define Engine_RotoLayer_h

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
#include "Engine/RotoItem.h"
#include "Engine/EngineFwd.h"


/**
 * @class A base class for all items made by the roto context
 **/

/**
 * @class A RotoLayer is a group of RotoItem. This allows the context to sort
 * and build hierarchies of layers.
 * Children items are rendered in reverse order of their ordering in the children list
 * i.e: the last item will be rendered first, etc...
 * Visually, in the GUI the top-most item of a layer corresponds to the first item in the children list
 **/
struct RotoLayerPrivate;
class RotoLayer
    : public RotoItem
{
public:

    RotoLayer(const boost::shared_ptr<RotoContext>& context,
              const std::string & name,
              const boost::shared_ptr<RotoLayer>& parent);

    explicit RotoLayer(const RotoLayer & other);

    virtual ~RotoLayer();

    virtual void clone(const RotoItem* other) OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj) OVERRIDE;

    ///only callable on the main-thread
    ///No check is done to figure out if the item already exists in this layer
    ///this is up to the caller responsability
    void addItem(const boost::shared_ptr<RotoItem>& item,bool declareToPython = true);

    ///Inserts the item into the layer before the indicated index.
    ///The same restrictions as addItem are applied.
    void insertItem(const boost::shared_ptr<RotoItem>& item,int index);

    ///only callable on the main-thread
    void removeItem(const boost::shared_ptr<RotoItem>& item);

    ///Returns the index of the given item in the layer, or -1 if not found
    int getChildIndex(const boost::shared_ptr<RotoItem>& item) const;

    ///only callable on the main-thread
    const std::list< boost::shared_ptr<RotoItem> >& getItems() const;

    ///MT-safe
    std::list< boost::shared_ptr<RotoItem> > getItems_mt_safe() const;

    
private:

    boost::scoped_ptr<RotoLayerPrivate> _imp;
};


#endif // Engine_RotoLayer_h
