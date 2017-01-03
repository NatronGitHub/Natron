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

#include <sstream> // stringstream
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

PluginPtr
StubNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create((void*)StubNode::create, PLUGINID_NATRON_STUB, "Stub", 1, 0, grouping);

    QString desc = tr("This plug-in is used as a temporary replacement for another plug-in when loading a project with a plug-in which cannot be found.");
    ret->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafeFrame);
    return ret;
}


StubNode::StubNode(const NodePtr& n)
: NoOpBase(n)
, _imp(new StubNodePrivate())
{
}

StubNode::~StubNode()
{

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
        SERIALIZATION_NAMESPACE::read(std::string(), ss, _imp->serialObj.get());
    }
    
    // Re-initialize inputs
    getNode()->initializeInputs();
    getNode()->setLabel(_imp->serialObj->_nodeLabel);
    if (_imp->serialObj->_nodeLabel != _imp->serialObj->_nodeScriptName) {
        getNode()->setScriptName(_imp->serialObj->_nodeScriptName);
    }
    if (_imp->serialObj->_nodeColor[0] != -1 || _imp->serialObj->_nodeColor[1] != -1 || _imp->serialObj->_nodeColor[2] != -1) {
        getNode()->setColor(_imp->serialObj->_nodeColor[0], _imp->serialObj->_nodeColor[1], _imp->serialObj->_nodeColor[2]);
    }
    if (_imp->serialObj->_nodePositionCoords[0] != INT_MIN || _imp->serialObj->_nodePositionCoords[1] != INT_MIN) {
        getNode()->setPosition(_imp->serialObj->_nodePositionCoords[0], _imp->serialObj->_nodePositionCoords[1]);
    }

    // Check for label knob
    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = _imp->serialObj->_knobsValues.begin(); it != _imp->serialObj->_knobsValues.end(); ++it) {
        if ((*it)->_scriptName == kUserLabelKnobName) {
            KnobStringPtr labelKnob = getNode()->getExtraLabelKnob();
            if (labelKnob) {
                labelKnob->fromSerialization(**it);
            }
            break;
        }
    }
}

bool
StubNode::knobChanged(const KnobIPtr& k,
                         ValueChangedReasonEnum /*reason*/,
                         ViewSetSpec /*view*/,
                         TimeValue /*time*/)
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
StubNode::onEffectCreated(const CreateNodeArgs& /*defaultParamValues*/)
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
NATRON_NAMESPACE_USING
#include "moc_StubNode.cpp"
