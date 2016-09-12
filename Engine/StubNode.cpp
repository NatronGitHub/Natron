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



#include "StubNode.h"

#include <QMutex>

#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

#include "Serialization/NodeSerialization.h"
#include "Serialization/SerializationIO.h"

NATRON_NAMESPACE_ENTER

struct StubNodePrivate
{
    
    mutable QMutex serialObjMutex;
    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialObj;
    
    KnobStringWPtr serializationKnob;

    StubNodePrivate()
    : serialObjMutex()
    , serialObj()
    , serializationKnob()
    {
        
    }
};

StubNode::StubNode(const NodePtr& n)
: NoOpBase(n)
, _imp(new StubNodePrivate())
{
}

StubNode::~StubNode()
{

}

std::string
StubNode::getPluginDescription() const
{
    return tr("This plug-in is used as a temporary replacement for another plug-in when loading a project with a plug-in which cannot be found.").toStdString();
}

std::string
StubNode::getInputLabel(int inputNb) const
{
    if (!_imp->serialObj) {
        return std::string();
    }
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = _imp->serialObj->_inputs.begin(); it != _imp->serialObj->_inputs.end(); ++it, ++i) {
        if (i == inputNb) {
            return it->first;
        }
    }
    return std::string();
}

int
StubNode::getMaxInputCount() const
{
    if (!_imp->serialObj) {
        return 0;
    } else {
        return (int)_imp->serialObj->_inputs.size();
    }
}

std::string
StubNode::getPluginLabel() const
{
    return "Stub";
}

void
StubNode::initializeKnobs()
{
    EffectInstancePtr thisShared = shared_from_this();
    
    KnobPagePtr page = AppManager::createKnob<KnobPage>(thisShared, tr("Controls"));
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(thisShared, tr(kStubNodeParamSerializationLabel));
        param->setName(kStubNodeParamSerialization);
        param->setHintToolTip(tr(kStubNodeParamSerializationHint));
        param->setAsMultiLine();
        page->addKnob(param);
        _imp->serializationKnob = param;
    }
}

void
StubNode::refreshInputsFromSerialization()
{
    std::string str = _imp->serializationKnob.lock()->getValue();
    std::istringstream ss(str);
    
    {
        QMutexLocker k(&_imp->serialObjMutex);
        _imp->serialObj.reset(new SERIALIZATION_NAMESPACE::NodeSerialization);
        SERIALIZATION_NAMESPACE::read(ss, _imp->serialObj.get());
    }
    
    // Re-initialize inputs
    getNode()->initializeInputs();
}

bool
StubNode::knobChanged(const KnobIPtr& k,
                         ValueChangedReasonEnum /*reason*/,
                         ViewSpec /*view*/,
                         double /*time*/,
                         bool /*originatedFromMainThread*/)
{
    if (k == _imp->serializationKnob.lock()) {
        try {
            refreshInputsFromSerialization();
        } catch (const std::exception& e) {
            Dialogs::errorDialog(tr("Content").toStdString(), tr("Error while loading node content: %1").arg(QString::fromUtf8(e.what())).toStdString());
        }
    }
    return true;
}

void
StubNode::onKnobsLoaded()
{
    refreshInputsFromSerialization();
}

SERIALIZATION_NAMESPACE::NodeSerializationPtr
StubNode::getNodeSerialization () const
{
    QMutexLocker k(&_imp->serialObjMutex);
    return _imp->serialObj;
}
NATRON_NAMESPACE_EXIT
