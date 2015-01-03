//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#include "NodeWrapper.h"
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/EffectInstance.h"
#include "Engine/NodeGroup.h"
Effect::Effect(const boost::shared_ptr<Natron::Node>& node)
: Group(boost::dynamic_pointer_cast<NodeCollection>(
                                                               boost::dynamic_pointer_cast<NodeGroup>(node->getLiveInstance()->shared_from_this())))
, _node(node)
{

}

Effect::~Effect()
{
    
}

boost::shared_ptr<Natron::Node>
Effect::getInternalNode() const
{
    return _node;
}

void
Effect::destroy(bool autoReconnect)
{
    _node->destroyNode(autoReconnect);
}

int
Effect::getMaxInputCount() const
{
    return _node->getMaxInputCount();
}

bool
Effect::canSetInput(int inputNumber,const Effect* node) const
{

    ///Input already connected
    if (_node->getInput(inputNumber)) {
        return false;
    }
    
    ///No-one is allowed to connect to the other node
    if (!node->_node->canOthersConnectToThisNode()) {
        return false;
    }
    
    ///Applying this connection would create cycles in the graph
    if (!_node->checkIfConnectingInputIsOk(node->_node.get())) {
        return false;
    }
    
    ///Ok, following connection would be clean.
    return true;
}

bool
Effect::connectInput(int inputNumber,const Effect* input)
{
    if (canSetInput(inputNumber, input)) {
        return _node->connectInput(input->_node, inputNumber);
    } else {
        return false;
    }
}

void
Effect::disconnectInput(int inputNumber)
{
    _node->disconnectInput(inputNumber);
}

Effect*
Effect::getInput(int inputNumber) const
{
    boost::shared_ptr<Natron::Node> node = _node->getInput(inputNumber);
    if (node) {
        return new Effect(node);
    }
    return NULL;
}

std::string
Effect::getName() const
{
    return _node->getName_mt_safe();
}


std::string
Effect::getPluginID() const
{
    return _node->getPluginID();
}

Param* createParamWrapperForKnob(const boost::shared_ptr<KnobI>& knob)
{
    int dims = knob->getDimension();
    boost::shared_ptr<Int_Knob> isInt = boost::dynamic_pointer_cast<Int_Knob>(knob);
    boost::shared_ptr<Double_Knob> isDouble = boost::dynamic_pointer_cast<Double_Knob>(knob);
    boost::shared_ptr<Bool_Knob> isBool = boost::dynamic_pointer_cast<Bool_Knob>(knob);
    boost::shared_ptr<Choice_Knob> isChoice = boost::dynamic_pointer_cast<Choice_Knob>(knob);
    boost::shared_ptr<Color_Knob> isColor = boost::dynamic_pointer_cast<Color_Knob>(knob);
    boost::shared_ptr<String_Knob> isString = boost::dynamic_pointer_cast<String_Knob>(knob);
    boost::shared_ptr<File_Knob> isFile = boost::dynamic_pointer_cast<File_Knob>(knob);
    boost::shared_ptr<OutputFile_Knob> isOutputFile = boost::dynamic_pointer_cast<OutputFile_Knob>(knob);
    boost::shared_ptr<Path_Knob> isPath = boost::dynamic_pointer_cast<Path_Knob>(knob);
    boost::shared_ptr<Button_Knob> isButton = boost::dynamic_pointer_cast<Button_Knob>(knob);
    boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knob);
    boost::shared_ptr<Page_Knob> isPage = boost::dynamic_pointer_cast<Page_Knob>(knob);
    
    if (isInt) {
        switch (dims) {
            case 1:
                return new IntParam(isInt);
            case 2:
                return new Int2DParam(isInt);
            case 3:
                return new Int3DParam(isInt);
            default:
                break;
        }
    } else if (isDouble) {
        switch (dims) {
            case 1:
                return new DoubleParam(isDouble);
            case 2:
                return new Double2DParam(isDouble);
            case 3:
                return new Double3DParam(isDouble);
            default:
                break;
        }
    } else if (isBool) {
        return new BooleanParam(isBool);
    } else if (isChoice) {
        return new ChoiceParam(isChoice);
    } else if (isColor) {
        return new ColorParam(isColor);
    } else if (isString) {
        return new StringParam(isString);
    } else if (isFile) {
        return new FileParam(isFile);
    } else if (isOutputFile) {
        return new OutputFileParam(isOutputFile);
    } else if (isPath) {
        return new PathParam(isPath);
    } else if (isGroup) {
        return new GroupParam(isGroup);
    } else if (isPage) {
        return new PageParam(isPage);
    }
    return NULL;
}

std::list<Param*>
Effect::getParams() const
{
    std::list<Param*> ret;
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _node->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        Param* p = createParamWrapperForKnob(*it);
        if (p) {
            ret.push_back(p);
        }
    }
    return ret;
}

Param*
Effect::getParam(const std::string& name) const
{
    boost::shared_ptr<KnobI> knob = _node->getKnobByName(name);
    if (knob) {
        return createParamWrapperForKnob(knob);
    } else {
        return NULL;
    }
}

int
Effect::getCurrentTime() const
{
    return _node->getLiveInstance()->getCurrentTime();
}
