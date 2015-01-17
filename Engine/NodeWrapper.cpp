//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NodeWrapper.h"

#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoWrapper.h"

Effect::Effect(const boost::shared_ptr<Natron::Node>& node)
: Group()
, _node(node)
{
    if (node) {
        boost::shared_ptr<NodeGroup> grp;
        if (node->getLiveInstance()) {
            grp = boost::dynamic_pointer_cast<NodeGroup>(node->getLiveInstance()->shared_from_this());
            init(boost::dynamic_pointer_cast<NodeCollection>(grp));
        }
        
    }
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
Effect::getScriptName() const
{
    return _node->getScriptName_mt_safe();
}

bool
Effect::setScriptName(const std::string& scriptName)
{
    return _node->setScriptName(scriptName);
}

std::string
Effect::getLabel() const
{
    return _node->getLabel_mt_safe();
}


void
Effect::setLabel(const std::string& name)
{
    return _node->setLabel(name);
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
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(knob);
    
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
    } else if (isParametric) {
        return new ParametricParam(isParametric);
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

void
Effect::setPosition(double x,double y)
{
    _node->setPosition(x, y);
}

void
Effect::getPosition(double* x, double* y) const
{
    _node->getPosition(x, y);
}

void
Effect::setSize(double w,double h)
{
    _node->setSize(w, h);
}

void
Effect::getSize(double* w, double* h) const
{
    _node->getSize(w, h);
}

void
Effect::blockEvaluation()
{
    _node->getLiveInstance()->blockEvaluation();
}

void
Effect::allowEvaluation()
{
    _node->getLiveInstance()->unblockEvaluation();
}

IntParam*
Effect::createIntParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Int_Knob> knob = _node->getLiveInstance()->createIntKnob(name, label, 1);
    if (knob) {
        return new IntParam(knob);
    } else {
        return 0;
    }
}

Int2DParam*
Effect::createInt2DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Int_Knob> knob = _node->getLiveInstance()->createIntKnob(name, label, 2);
    if (knob) {
        return new Int2DParam(knob);
    } else {
        return 0;
    }

}

Int3DParam*
Effect::createInt3DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Int_Knob> knob = _node->getLiveInstance()->createIntKnob(name, label, 3);
    if (knob) {
        return new Int3DParam(knob);
    } else {
        return 0;
    }

}

DoubleParam*
Effect::createDoubleParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Double_Knob> knob = _node->getLiveInstance()->createDoubleKnob(name, label, 1);
    if (knob) {
        return new DoubleParam(knob);
    } else {
        return 0;
    }
   
}

Double2DParam*
Effect::createDouble2DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Double_Knob> knob = _node->getLiveInstance()->createDoubleKnob(name, label, 2);
    if (knob) {
        return new Double2DParam(knob);
    } else {
        return 0;
    }
}

Double3DParam*
Effect::createDouble3DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Double_Knob> knob = _node->getLiveInstance()->createDoubleKnob(name, label, 3);
    if (knob) {
        return new Double3DParam(knob);
    } else {
        return 0;
    }
}

BooleanParam*
Effect::createBooleanParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Bool_Knob> knob = _node->getLiveInstance()->createBoolKnob(name, label);
    if (knob) {
        return new BooleanParam(knob);
    } else {
        return 0;
    }
}

ChoiceParam*
Effect::createChoiceParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Choice_Knob> knob = _node->getLiveInstance()->createChoiceKnob(name, label);
    if (knob) {
        return new ChoiceParam(knob);
    } else {
        return 0;
    }
}

ColorParam*
Effect::createColorParam(const std::string& name, const std::string& label, bool useAlpha)
{
    boost::shared_ptr<Color_Knob> knob = _node->getLiveInstance()->createColorKnob(name, label, useAlpha ? 4 : 3);
    if (knob) {
        return new ColorParam(knob);
    } else {
        return 0;
    }
}

StringParam*
Effect::createStringParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<String_Knob> knob = _node->getLiveInstance()->createStringKnob(name, label);
    if (knob) {
        return new StringParam(knob);
    } else {
        return 0;
    }
}

FileParam*
Effect::createFileParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<File_Knob> knob = _node->getLiveInstance()->createFileKnob(name, label);
    if (knob) {
        return new FileParam(knob);
    } else {
        return 0;
    }
}

OutputFileParam*
Effect::createOutputFileParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<OutputFile_Knob> knob = _node->getLiveInstance()->createOuptutFileKnob(name, label);
    if (knob) {
        return new OutputFileParam(knob);
    } else {
        return 0;
    }
}

PathParam*
Effect::createPathParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Path_Knob> knob = _node->getLiveInstance()->createPathKnob(name, label);
    if (knob) {
        return new PathParam(knob);
    } else {
        return 0;
    }
}

ButtonParam*
Effect::createButtonParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Button_Knob> knob = _node->getLiveInstance()->createButtonKnob(name, label);
    if (knob) {
        return new ButtonParam(knob);
    } else {
        return 0;
    }
}

GroupParam*
Effect::createGroupParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Group_Knob> knob = _node->getLiveInstance()->createGroupKnob(name, label);
    if (knob) {
        return new GroupParam(knob);
    } else {
        return 0;
    }
}

PageParam*
Effect::createPageParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Page_Knob> knob = _node->getLiveInstance()->createPageKnob(name, label);
    if (knob) {
        return new PageParam(knob);
    } else {
        return 0;
    }
}

ParametricParam*
Effect::createParametricParam(const std::string& name, const std::string& label, int nbCurves)
{
    boost::shared_ptr<Parametric_Knob> knob = _node->getLiveInstance()->createParametricKnob(name, label, nbCurves);
    if (knob) {
        return new ParametricParam(knob);
    } else {
        return 0;
    }
}

PageParam*
Effect::getUserPageParam() const
{
    boost::shared_ptr<Page_Knob> page = _node->getLiveInstance()->getOrCreateUserPageKnob();
    assert(page);
    return new PageParam(page);
}

void
Effect::refreshUserParamsGUI()
{
    _node->getLiveInstance()->refreshKnobs();
}

Effect*
Effect::createChild()
{
    if (!_node->isMultiInstance()) {
        return 0;
    }
    CreateNodeArgs args( _node->getPluginID().c_str(),
                        _node->getScriptName(),
                        -1,-1,
                        true,
                        INT_MIN,INT_MIN,
                        false,  //< never use the undo-stack of the nodegraph since we use the one of the dockablepanel
                        true,
                        QString(),
                        CreateNodeArgs::DefaultValuesList(),
                        _node->getGroup());
    NodePtr child = _node->getApp()->createNode(args);
    if (child) {
        return new Effect(child);
    }
    return 0;
}

Roto*
Effect::getRotoContext() const
{
    boost::shared_ptr<RotoContext> roto = _node->getRotoContext();
    if (roto) {
        return new Roto(roto);
    }
    return 0;
}