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

#include "RotoItem.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cassert>
#include <stdexcept>

#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Global/MemoryInfo.h"
#include "Engine/RotoContextPrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/RotoItemSerialization.h"



#define kParamGloballyEnabled "enabled"
#define kParamGloballyEnabledLabel "Enabled"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER;


////////////////////////////////////RotoItem////////////////////////////////////


NATRON_NAMESPACE_ANONYMOUS_ENTER

class RotoMetaTypesRegistration
{
public:
    inline RotoMetaTypesRegistration()
    {
        qRegisterMetaType<RotoItem*>("RotoItem");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


static RotoMetaTypesRegistration registration;

RotoItem::RotoItem(const KnobItemsTablePtr& model,
                   const std::string & name,
                   RotoLayerPtr parent)
    : KnobTableItem(context->getNode()->getApp())
    , SERIALIZATION_NAMESPACE::SerializableObjectBase()
    , itemMutex()
    , _imp( new RotoItemPrivate(context, name, parent) )
{
}

RotoItem::~RotoItem()
{
}

void
RotoItem::clone(const RotoItem*  other)
{
    QMutexLocker l(&itemMutex);

    _imp->parentLayer = other->_imp->parentLayer;
    _imp->scriptName = other->_imp->scriptName;
    _imp->label = other->_imp->label;
    _imp->globallyActivated = other->_imp->globallyActivated;
    _imp->locked = other->_imp->locked;
}

void
RotoItem::setParentLayer(RotoLayerPtr layer)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );

    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    if (isStroke) {
        if (!layer) {
            isStroke->deactivateNodes();
        } else {
            isStroke->activateNodes();
        }
    }

    QMutexLocker l(&itemMutex);
    _imp->parentLayer = layer;
}

RotoLayerPtr
RotoItem::getParentLayer() const
{
    QMutexLocker l(&itemMutex);

    return _imp->parentLayer.lock();
}

void
RotoItem::setGloballyActivated_recursive(bool a)
{
    {
        _imp->globallyActivated.lock()->setValue(a);
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            const RotoItems & children = layer->getItems();
            for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
                (*it)->setGloballyActivated_recursive(a);
            }
        }
    }
    invalidateCacheHashAndEvaluate(true, false);
}

void
RotoItem::initializeKnobs()
{
    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(shared_from_this(), tr(kParamGloballyEnabledLabel));
        param->setName(kParamGloballyEnabled);
        param->setDefaultValue(true);
        param->setSecret(true);
        _imp->globallyActivated = param;
    }
}

void
RotoItem::setGloballyActivated(bool a,
                               bool setChildren)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    if (setChildren) {
        setGloballyActivated_recursive(a);
    } else {
        _imp->globallyActivated.lock()->setValue(a);
    }
    invalidateCacheHashAndEvaluate(true, false);
}

bool
RotoItem::isGloballyActivated() const
{
    KnobBoolPtr knob = _imp->globallyActivated.lock();
    return knob ? knob->getValue() : true;
}

static bool
isDeactivated_imp(const RotoLayerPtr& item)
{
    if ( !item->isGloballyActivated() ) {
        return true;
    } else {
        RotoLayerPtr parent = item->getParentLayer();
        if (parent) {
            return isDeactivated_imp(parent);
        }
    }

    return false;
}

bool
RotoItem::isDeactivatedRecursive() const
{
    RotoLayerPtr parent;
    {
        QMutexLocker l(&itemMutex);
        if (!_imp->globallyActivated.lock()->getValue()) {
            return true;
        }
        parent = _imp->parentLayer.lock();
    }

    if (parent) {
        return isDeactivated_imp(parent);
    }

    return false;
}

void
RotoItem::setLocked_recursive(bool locked,
                              RotoItem::SelectionReasonEnum reason)
{
    {
        {
            QMutexLocker m(&itemMutex);
            _imp->locked = locked;
        }
        getContext()->onItemLockedChanged(toRotoItem(shared_from_this()), reason);
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            const RotoItems & children = layer->getItems();
            for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
                (*it)->setLocked_recursive(locked, reason);
            }
        }
    }
}

void
RotoItem::setLocked(bool l,
                    bool lockChildren,
                    RotoItem::SelectionReasonEnum reason)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    if (!lockChildren) {
        {
            QMutexLocker m(&itemMutex);
            _imp->locked = l;
        }
        getContext()->onItemLockedChanged(toRotoItem(shared_from_this()), reason);
    } else {
        setLocked_recursive(l, reason);
    }
}

bool
RotoItem::getLocked() const
{
    QMutexLocker l(&itemMutex);

    return _imp->locked;
}

static
bool
isLocked_imp(const RotoLayerPtr& item)
{
    if ( item->getLocked() ) {
        return true;
    } else {
        RotoLayerPtr parent = item->getParentLayer();
        if (parent) {
            return isLocked_imp(parent);
        }
    }

    return false;
}

bool
RotoItem::isLockedRecursive() const
{
    RotoLayerPtr parent;
    {
        QMutexLocker l(&itemMutex);
        if (_imp->locked) {
            return true;
        }
        parent = _imp->parentLayer.lock();
    }

    if (parent) {
        return isLocked_imp(parent);
    } else {
        return false;
    }
}

int
RotoItem::getHierarchyLevel() const
{
    int ret = 0;
    RotoLayerPtr parent;

    {
        QMutexLocker l(&itemMutex);
        parent = _imp->parentLayer.lock();
    }

    while (parent) {
        parent = parent->getParentLayer();
        ++ret;
    }

    return ret;
}

RotoContextPtr
RotoItem::getContext() const
{
    return _imp->context.lock();
}

bool
RotoItem::setScriptName(const std::string & name)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );

    if ( name.empty() ) {
        return false;
    }


    std::string cpy = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(name);

    if ( cpy.empty() ) {
        return false;
    }

    RotoItemPtr existingItem = getContext()->getItemByName(name);
    if ( existingItem && (existingItem.get() != this) ) {
        return false;
    }

    std::string oldFullName = getFullyQualifiedName();
    bool oldNameEmpty;
    {
        QMutexLocker l(&itemMutex);
        oldNameEmpty = _imp->scriptName.empty();
        _imp->scriptName = cpy;
    }
    std::string newFullName = getFullyQualifiedName();
    RotoContextPtr c = _imp->context.lock();
    if (c) {
        if (!oldNameEmpty) {
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
            ///Strokes are unsupported in Python currently
            if (!isStroke) {
                c->changeItemScriptName(oldFullName, newFullName);
            }
        }
        c->onItemScriptNameChanged( toRotoItem(shared_from_this()) );
    }

    return true;
}

static void
getScriptNameRecursive(RotoLayer* item,
                       std::string* scriptName)
{
    scriptName->insert(0, ".");
    scriptName->insert( 0, item->getScriptName() );
    RotoLayerPtr parent = item->getParentLayer();
    if (parent) {
        getScriptNameRecursive(parent.get(), scriptName);
    }
}

std::string
RotoItem::getFullyQualifiedName() const
{
    std::string name = getScriptName();
    RotoLayerPtr parent = getParentLayer();

    if (parent) {
        getScriptNameRecursive(parent.get(), &name);
    }

    return name;
}

std::string
RotoItem::getScriptName() const
{
    QMutexLocker l(&itemMutex);

    return _imp->scriptName;
}

std::string
RotoItem::getLabel() const
{
    QMutexLocker l(&itemMutex);

    return _imp->label;
}

void
RotoItem::setLabel(const std::string& label)
{
    {
        QMutexLocker l(&itemMutex);
        _imp->label = label;
    }
    RotoContextPtr c = _imp->context.lock();

    if (c) {
        c->onItemLabelChanged( toRotoItem(shared_from_this()) );
    }
}

void
RotoItem::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj) 
{

    SERIALIZATION_NAMESPACE::RotoItemSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::RotoItemSerialization*>(obj);
    if (!serialization) {
        return;
    }
    RotoLayerPtr parent;
    {
        QMutexLocker l(&itemMutex);
        serialization->name = _imp->scriptName;
        serialization->label = _imp->label;
        serialization->locked = _imp->locked;
        parent = _imp->parentLayer.lock();
    }

    if (parent) {
        serialization->parentLayerName = parent->getScriptName();
    }
}

void
RotoItem::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase &obj)
{
    const SERIALIZATION_NAMESPACE::RotoItemSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::RotoItemSerialization*>(&obj);
    if (!serialization) {
        return;
    }
    {
        QMutexLocker l(&itemMutex);
        _imp->locked = serialization->locked;
        _imp->scriptName = serialization->name;
        if ( !serialization->label.empty() ) {
            _imp->label = serialization->label;
        } else {
            _imp->label = _imp->scriptName;
        }
        std::locale loc;
        std::string cpy;
        for (std::size_t i = 0; i < _imp->scriptName.size(); ++i) {
            ///Ignore starting digits
            if ( cpy.empty() && std::isdigit(_imp->scriptName[i], loc) ) {
                continue;
            }

            ///Spaces becomes underscores
            if ( std::isspace(_imp->scriptName[i], loc) ) {
                cpy.push_back('_');
            }
            ///Non alpha-numeric characters are not allowed in python
            else if ( (_imp->scriptName[i] == '_') || std::isalnum(_imp->scriptName[i], loc) ) {
                cpy.push_back(_imp->scriptName[i]);
            }
        }
        if ( !cpy.empty() ) {
            _imp->scriptName = cpy;
        } else {
            l.unlock();
            std::string name = getContext()->generateUniqueName(kRotoBezierBaseName);
            l.relock();
            _imp->scriptName = name;
        }
    }
    RotoLayerPtr parent = getContext()->getLayerByName(serialization->parentLayerName);

    {
        QMutexLocker l(&itemMutex);
        _imp->parentLayer = parent;
    }
}

std::string
RotoItem::getRotoNodeName() const
{
    return getContext()->getRotoNodeName();
}

static RotoItemPtr
getPreviousInLayer(const RotoLayerPtr& layer,
                   const boost::shared_ptr<const RotoItem>& item)
{
    RotoItems layerItems = layer->getItems_mt_safe();

    if ( layerItems.empty() ) {
        return RotoItemPtr();
    }
    RotoItems::iterator found = layerItems.end();
    if (item) {
        for (RotoItems::iterator it = layerItems.begin(); it != layerItems.end(); ++it) {
            if (*it == item) {
                found = it;
                break;
            }
        }
        assert( found != layerItems.end() );
    } else {
        found = layerItems.end();
    }

    if ( found != layerItems.end() ) {
        ++found;
        if ( found != layerItems.end() ) {
            return *found;
        }
    }

    //Item was still not found, find in great parent layer
    RotoLayerPtr parentLayer = layer->getParentLayer();
    if (!parentLayer) {
        return RotoItemPtr();
    }
    RotoItems greatParentItems = parentLayer->getItems_mt_safe();

    found = greatParentItems.end();
    for (RotoItems::iterator it = greatParentItems.begin(); it != greatParentItems.end(); ++it) {
        if (*it == layer) {
            found = it;
            break;
        }
    }
    assert( found != greatParentItems.end() );
    RotoItemPtr ret = getPreviousInLayer(parentLayer, layer);
    assert(ret != item);

    return ret;
}

RotoItemPtr
RotoItem::getPreviousItemInLayer() const
{
    RotoLayerPtr layer = getParentLayer();

    if (!layer) {
        return RotoItemPtr();
    }
    RotoItemConstPtr thisShared = toRotoItem(shared_from_this());
    return getPreviousInLayer( layer, thisShared);
}


NATRON_NAMESPACE_EXIT;
