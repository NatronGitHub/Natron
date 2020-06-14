/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/InputDescription.h"
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
    PluginPtr ret = Plugin::create(StubNode::create, StubNode::createRenderClone, PLUGINID_NATRON_STUB, "Stub", 1, 0, grouping);

    QString desc = tr("This plug-in is used as a temporary replacement for another plug-in when loading a project with a plug-in which cannot be found.");
    ret->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
    {
        InputDescriptionPtr input = InputDescription::create("Source", "Source", "", true, false, std::bitset<4>(std::string("1111")));
        ret->addInputDescription(input);
    }
    return ret;
}


StubNode::StubNode(const NodePtr& n)
: NoOpBase(n)
, _imp(new StubNodePrivate())
{
}

StubNode::StubNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: NoOpBase(mainInstance, key)
, _imp(new StubNodePrivate())
{

}

StubNode::~StubNode()
{

}


void
StubNode::initializeKnobs()
{
    EffectInstancePtr thisShared = shared_from_this();
    
    KnobPagePtr page = createKnob<KnobPage>("controlsPage");
    page->setLabel(tr("Controls"));
    {
        KnobStringPtr param = createKnob<KnobString>(kStubNodeParamSerialization);
        param->setLabel(tr(kStubNodeParamSerializationLabel));
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
    {
        getNode()->beginInputEdition();

        std::vector<InputDescriptionPtr> inputs;
        int i = 0;
        for (std::map<std::string, std::string>::const_iterator it = _imp->serialObj->_inputs.begin(); it != _imp->serialObj->_inputs.end(); ++it, ++i) {
            InputDescriptionPtr desc(new InputDescription);
            getNode()->getInputDescription(i, desc.get());
            desc->setProperty<std::string>(kInputDescPropLabel, it->first);
            inputs.push_back(desc);
            getNode()->changeInputDescription(i, desc);
        }
        getNode()->endInputEdition(true);

    }
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
            KnobStringPtr labelKnob = getExtraLabelKnob();
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
