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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
, UserParamHolder(node ? node->getLiveInstance() : 0)
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
Effect::canConnectInput(int inputNumber,const Effect* node) const
{

    if (!node) {
        return false;
    }
    
    if (!node->getInternalNode()) {
        return false;
    }
    Natron::Node::CanConnectInputReturnValue ret = _node->canConnectInput(node->getInternalNode(),inputNumber);
    return ret == Natron::Node::eCanConnectInput_ok;
}

bool
Effect::connectInput(int inputNumber,const Effect* input)
{
    if (canConnectInput(inputNumber, input)) {
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
Effect::getInputLabel(int inputNumber)
{
    try {
        return _node->getInputLabel(inputNumber);
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return std::string();
}

std::string
Effect::getPluginID() const
{
    return _node->getPluginID();
}

Param*
Effect::createParamWrapperForKnob(const boost::shared_ptr<KnobI>& knob)
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
    } else if (isButton) {
        return new ButtonParam(isButton);
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
Effect::getColor(double* r,double *g, double* b) const
{
    _node->getColor(r, g, b);
}

void
Effect::setColor(double r, double g, double b)
{
    _node->setColor(r, g, b);
}

void
Effect::beginChanges()
{
    _node->getLiveInstance()->beginChanges();
}

void
Effect::endChanges()
{
    _node->getLiveInstance()->endChanges();
}

IntParam*
UserParamHolder::createIntParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Int_Knob> knob = _holder->createIntKnob(name, label, 1);
    if (knob) {
        return new IntParam(knob);
    } else {
        return 0;
    }
}

Int2DParam*
UserParamHolder::createInt2DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Int_Knob> knob = _holder->createIntKnob(name, label, 2);
    if (knob) {
        return new Int2DParam(knob);
    } else {
        return 0;
    }

}

Int3DParam*
UserParamHolder::createInt3DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Int_Knob> knob = _holder->createIntKnob(name, label, 3);
    if (knob) {
        return new Int3DParam(knob);
    } else {
        return 0;
    }

}

DoubleParam*
UserParamHolder::createDoubleParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Double_Knob> knob = _holder->createDoubleKnob(name, label, 1);
    if (knob) {
        return new DoubleParam(knob);
    } else {
        return 0;
    }
   
}

Double2DParam*
UserParamHolder::createDouble2DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Double_Knob> knob = _holder->createDoubleKnob(name, label, 2);
    if (knob) {
        return new Double2DParam(knob);
    } else {
        return 0;
    }
}

Double3DParam*
UserParamHolder::createDouble3DParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Double_Knob> knob = _holder->createDoubleKnob(name, label, 3);
    if (knob) {
        return new Double3DParam(knob);
    } else {
        return 0;
    }
}

BooleanParam*
UserParamHolder::createBooleanParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Bool_Knob> knob = _holder->createBoolKnob(name, label);
    if (knob) {
        return new BooleanParam(knob);
    } else {
        return 0;
    }
}

ChoiceParam*
UserParamHolder::createChoiceParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Choice_Knob> knob = _holder->createChoiceKnob(name, label);
    if (knob) {
        return new ChoiceParam(knob);
    } else {
        return 0;
    }
}

ColorParam*
UserParamHolder::createColorParam(const std::string& name, const std::string& label, bool useAlpha)
{
    boost::shared_ptr<Color_Knob> knob = _holder->createColorKnob(name, label, useAlpha ? 4 : 3);
    if (knob) {
        return new ColorParam(knob);
    } else {
        return 0;
    }
}

StringParam*
UserParamHolder::createStringParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<String_Knob> knob = _holder->createStringKnob(name, label);
    if (knob) {
        return new StringParam(knob);
    } else {
        return 0;
    }
}

FileParam*
UserParamHolder::createFileParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<File_Knob> knob = _holder->createFileKnob(name, label);
    if (knob) {
        return new FileParam(knob);
    } else {
        return 0;
    }
}

OutputFileParam*
UserParamHolder::createOutputFileParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<OutputFile_Knob> knob = _holder->createOuptutFileKnob(name, label);
    if (knob) {
        return new OutputFileParam(knob);
    } else {
        return 0;
    }
}

PathParam*
UserParamHolder::createPathParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Path_Knob> knob = _holder->createPathKnob(name, label);
    if (knob) {
        return new PathParam(knob);
    } else {
        return 0;
    }
}

ButtonParam*
UserParamHolder::createButtonParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Button_Knob> knob = _holder->createButtonKnob(name, label);
    if (knob) {
        return new ButtonParam(knob);
    } else {
        return 0;
    }
}

GroupParam*
UserParamHolder::createGroupParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Group_Knob> knob = _holder->createGroupKnob(name, label);
    if (knob) {
        return new GroupParam(knob);
    } else {
        return 0;
    }
}

PageParam*
UserParamHolder::createPageParam(const std::string& name, const std::string& label)
{
    boost::shared_ptr<Page_Knob> knob = _holder->createPageKnob(name, label);
    if (knob) {
        return new PageParam(knob);
    } else {
        return 0;
    }
}

ParametricParam*
UserParamHolder::createParametricParam(const std::string& name, const std::string& label, int nbCurves)
{
    boost::shared_ptr<Parametric_Knob> knob = _holder->createParametricKnob(name, label, nbCurves);
    if (knob) {
        return new ParametricParam(knob);
    } else {
        return 0;
    }
}

bool
UserParamHolder::removeParam(Param* param)
{
    if (!param) {
        return false;
    }
    if (!param->getInternalKnob()) {
        return false;
    }
    if (!param->getInternalKnob()->isUserKnob()) {
        return false;
    }
    
    _holder->removeDynamicKnob(param->getInternalKnob().get());
    
    return true;
}

PageParam*
Effect::getUserPageParam() const
{
    boost::shared_ptr<Page_Knob> page = _node->getLiveInstance()->getOrCreateUserPageKnob();
    assert(page);
    return new PageParam(page);
}

void
UserParamHolder::refreshUserParamsGUI()
{
    _holder->refreshKnobs();
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
                        false,
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

RectD
Effect::getRegionOfDefinition(int time,int view) const
{
    RectD rod;
    if (!_node || !_node->getLiveInstance()) {
        return rod;
    }
    U64 hash = _node->getHashValue();
    RenderScale s;
    s.x = s.y = 1.;
    bool isProject;
    Natron::StatusEnum stat = _node->getLiveInstance()->getRegionOfDefinition_public(hash, time, s, view, &rod, &isProject);
    if (stat != Natron::eStatusOK) {
        return RectD();
    }
    return rod;
}

void
Effect::setSubGraphEditable(bool editable)
{
    if (!_node) {
        return;
    }
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_node->getLiveInstance());
    if (isGroup) {
        isGroup->setSubGraphEditable(editable);
    }
}