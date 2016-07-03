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

#include "KnobSerialization.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>

#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"

NATRON_NAMESPACE_ENTER;

ValueSerialization::ValueSerialization()
: _serialization(0)
, _knob()
, _dimension(0)
, _master()
, _expression()
, _exprHasRetVar(false)
{

}

ValueSerialization::ValueSerialization(KnobSerializationBase* serialization,
                                       const KnobIPtr & knob,
                                       int dimension)
    : _serialization(serialization)
    , _knob(knob)
    , _dimension(dimension)
    , _master()
    , _expression()
    , _exprHasRetVar(false)
{
}

void
ValueSerialization::initForLoad(KnobSerializationBase* serialization,
                 const KnobIPtr & knob,
                 int dimension)
{
    _serialization = serialization;
    _knob = knob;
    _dimension = dimension;
}

ValueSerialization::ValueSerialization(const KnobIPtr & knob,
                                       int dimension,
                                       bool exprHasRetVar,
                                       const std::string& expr)
    : _serialization(0)
    , _knob()
    , _dimension(0)
    , _master()
    , _expression()
    , _exprHasRetVar(false)
{
    initForSave(knob, dimension, exprHasRetVar, expr);
}

void
ValueSerialization::initForSave(const KnobIPtr & knob,
                 int dimension,
                 bool exprHasRetVar,
                 const std::string& expr)
{
    _knob = knob;
    _dimension = dimension;
    _expression = expr;
    _exprHasRetVar = exprHasRetVar;

    std::pair< int, KnobIPtr > m = knob->getMaster(dimension);

    if ( m.second && !knob->isMastersPersistenceIgnored() ) {
        _master.masterDimension = m.first;
        NamedKnobHolderPtr holder = boost::dynamic_pointer_cast<NamedKnobHolder>( m.second->getHolder() );
        assert(holder);

        TrackMarkerPtr isMarker = boost::dynamic_pointer_cast<TrackMarker>(holder);
        if (isMarker) {
            _master.masterTrackName = isMarker->getScriptName_mt_safe();
            _master.masterNodeName = isMarker->getContext()->getNode()->getScriptName_mt_safe();
        } else {
            // coverity[dead_error_line]
            _master.masterNodeName = holder ? holder->getScriptName_mt_safe() : "";
        }
        _master.masterKnobName = m.second->getName();
    } else {
        _master.masterDimension = -1;
    }

}

void
ValueSerialization::setChoiceExtraLabel(const std::string& label)
{
    assert(_serialization);
    _serialization->setChoiceExtraString(label);
}

KnobIPtr
KnobSerialization::createKnob(const std::string & typeName,
                              int dimension)
{
    KnobIPtr ret;

    if ( typeName == KnobInt::typeNameStatic() ) {
        ret = KnobInt::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobBool::typeNameStatic() ) {
        ret = KnobBool::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobDouble::typeNameStatic() ) {
        ret = KnobDouble::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobChoice::typeNameStatic() ) {
        ret = KnobChoice::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobString::typeNameStatic() ) {
        ret = KnobString::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobParametric::typeNameStatic() ) {
        ret = KnobParametric::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobColor::typeNameStatic() ) {
        ret = KnobColor::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobPath::typeNameStatic() ) {
        ret = KnobPath::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobLayers::typeNameStatic() ) {
        ret = KnobLayers::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobFile::typeNameStatic() ) {
        ret = KnobFile::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobOutputFile::typeNameStatic() ) {
        ret = KnobOutputFile::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobButton::typeNameStatic() ) {
        ret = KnobButton::create(KnobHolderPtr(), std::string(), dimension, false);
    } else if ( typeName == KnobSeparator::typeNameStatic() ) {
        ret = KnobSeparator::create(KnobHolderPtr(), std::string(), dimension, false);
    }

    if (ret) {
        ret->populate();
    }

    return ret;
}

static KnobIPtr
findMaster(const KnobIPtr & knob,
           const NodesList & allNodes,
           const std::string& masterKnobName,
           const std::string& masterNodeName,
           const std::string& masterTrackName,
           const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    ///we need to cycle through all the nodes of the project to find the real master
    NodePtr masterNode;
    std::string masterNodeNameToFind = masterNodeName;

    /*
       When copy pasting, the new node copied has a script-name different from what is inside the serialization because 2
       nodes cannot co-exist with the same script-name. We keep in the map the script-names mapping
     */
    std::map<std::string, std::string>::const_iterator foundMapping = oldNewScriptNamesMapping.find(masterNodeName);

    if ( foundMapping != oldNewScriptNamesMapping.end() ) {
        masterNodeNameToFind = foundMapping->second;
    }

    for (NodesList::const_iterator it2 = allNodes.begin(); it2 != allNodes.end(); ++it2) {
        if ( (*it2)->getScriptName() == masterNodeNameToFind ) {
            masterNode = *it2;
            break;
        }
    }
    if (!masterNode) {
        qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeNameToFind.c_str();

        return KnobIPtr();
    }

    if ( !masterTrackName.empty() ) {
        TrackerContextPtr context = masterNode->getTrackerContext();
        if (context) {
            TrackMarkerPtr marker = context->getMarkerByName(masterTrackName);
            if (marker) {
                return marker->getKnobByName(masterKnobName);
            }
        }
    } else {
        ///now that we have the master node, find the corresponding knob
        const std::vector< KnobIPtr > & otherKnobs = masterNode->getKnobs();
        for (std::size_t j = 0; j < otherKnobs.size(); ++j) {
            if ( (otherKnobs[j]->getName() == masterKnobName) && otherKnobs[j]->getIsPersistant() ) {
                return otherKnobs[j];
                break;
            }
        }
    }

    qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeNameToFind.c_str();

    return KnobIPtr();
}

void
KnobSerialization::restoreKnobLinks(const KnobIPtr & knob,
                                    const NodesList & allNodes,
                                    const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    int i = 0;

    if (_masterIsAlias) {
        /*
         * _masters can be empty for example if we expand a group: the slaved knobs are no longer slaves
         */
        if ( !_masters.empty() ) {
            const std::string& aliasKnobName = _masters.front().masterKnobName;
            const std::string& aliasNodeName = _masters.front().masterNodeName;
            const std::string& masterTrackName  = _masters.front().masterTrackName;
            KnobIPtr alias = findMaster(knob, allNodes, aliasKnobName, aliasNodeName, masterTrackName, oldNewScriptNamesMapping);
            if (alias) {
                knob->setKnobAsAliasOfThis(alias, true);
            }
        }
    } else {
        for (std::list<MasterSerialization>::iterator it = _masters.begin(); it != _masters.end(); ++it) {
            if (it->masterDimension != -1) {
                KnobIPtr master = findMaster(knob, allNodes, it->masterKnobName, it->masterNodeName, it->masterTrackName, oldNewScriptNamesMapping);
                if (master) {
                    knob->slaveTo(i, master, it->masterDimension);
                }
            }
            ++i;
        }
    }
}

void
KnobSerialization::restoreExpressions(const KnobIPtr & knob,
                                      const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    int dims = std::min( knob->getDimension(), _knob->getDimension() );

    try {
        for (int i = 0; i < dims; ++i) {
            if ( !_expressions[i].first.empty() ) {
                QString expr( QString::fromUtf8( _expressions[i].first.c_str() ) );

                //Replace all occurrences of script-names that we know have changed
                for (std::map<std::string, std::string>::const_iterator it = oldNewScriptNamesMapping.begin();
                     it != oldNewScriptNamesMapping.end(); ++it) {
                    expr.replace( QString::fromUtf8( it->first.c_str() ), QString::fromUtf8( it->second.c_str() ) );
                }
                knob->restoreExpression(i, expr.toStdString(), _expressions[i].second);
            }
        }
    } catch (const std::exception& e) {
        QString err = QString::fromUtf8("Failed to restore expression: %1").arg( QString::fromUtf8( e.what() ) );
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( knob->getName().c_str() ),err);
    }
}

void
KnobSerialization::setChoiceExtraString(const std::string& label)
{
    assert(_extraData);
    ChoiceExtraData* cData = dynamic_cast<ChoiceExtraData*>(_extraData);
    assert(cData);
    if (cData) {
        cData->_choiceString = label;
    }
}

NATRON_NAMESPACE_EXIT;
