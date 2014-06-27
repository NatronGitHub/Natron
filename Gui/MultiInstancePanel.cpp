//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include "MultiInstancePanel.h"

#include <QVBoxLayout>
#include <QTableWidget>

#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

using namespace Natron;

struct MultiInstancePanelPrivate
{
    
    MultiInstancePanel* publicInterface;
    std::list < boost::shared_ptr<Node> > instances;
    
    QTableWidget* table;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    
    MultiInstancePanelPrivate(MultiInstancePanel* publicI,const boost::shared_ptr<Node>& node)
    : publicInterface(publicI)
    , instances()
    , table(0)
    , buttonsContainer(0)
    , buttonsLayout(0)
    {
        instances.push_back(node);
    }
    
    Node* getMainInstance() const { assert(!instances.empty()); return instances.front().get(); }
    
    void createKnob(const boost::shared_ptr<KnobI>& ref)
    {
        if (ref->isInstanceSpecific()) {
            return;
        }
        
        boost::shared_ptr<KnobI> ret;
        if (dynamic_cast<Int_Knob*>(ref.get())) {
            ret = Natron::createKnob<Int_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<Bool_Knob*>(ref.get())) {
            ret = Natron::createKnob<Bool_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<Double_Knob*>(ref.get())) {
            ret = Natron::createKnob<Double_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<Choice_Knob*>(ref.get())) {
            ret = Natron::createKnob<Choice_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<String_Knob*>(ref.get())) {
            ret = Natron::createKnob<String_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<Parametric_Knob*>(ref.get())) {
            ret = Natron::createKnob<Parametric_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<Color_Knob*>(ref.get())) {
            ret = Natron::createKnob<Color_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<Path_Knob*>(ref.get())) {
            ret = Natron::createKnob<Path_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<File_Knob*>(ref.get())) {
            ret = Natron::createKnob<File_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<OutputFile_Knob*>(ref.get())) {
            ret = Natron::createKnob<OutputFile_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        assert(ret);
        ret->setName(ref->getName());
        ret->setAnimationEnabled(ref->isAnimationEnabled());
        ret->setHintToolTip(ref->getHintToolTip());
        ret->setEvaluateOnChange(ref->getEvaluateOnChange());
        ret->setIsPersistant(false);
        
    }
};

MultiInstancePanel::MultiInstancePanel(const boost::shared_ptr<Node>& node)
: KnobHolder(node->getApp())
, _imp(new MultiInstancePanelPrivate(this,node))
{
}

MultiInstancePanel::~MultiInstancePanel()
{
    
}

void MultiInstancePanel::initializeKnobs()
{
    const std::vector<boost::shared_ptr<KnobI> >& mainInstanceKnobs = _imp->getMainInstance()->getKnobs();
    for (U32 i = 0; i < mainInstanceKnobs.size(); ++i) {
        _imp->createKnob(mainInstanceKnobs[i]);
    }
}


void MultiInstancePanel::createMultiInstanceGui(QVBoxLayout* layout)
{
    std::list<boost::shared_ptr<KnobI> > instanceSpecificKnobs;
    const std::vector<boost::shared_ptr<KnobI> >& mainInstanceKnobs = _imp->getMainInstance()->getKnobs();
    for (U32 i = 0; i < mainInstanceKnobs.size(); ++i) {
        if (mainInstanceKnobs[i]->isInstanceSpecific()) {
            instanceSpecificKnobs.push_back(mainInstanceKnobs[i]);
        }
    }
    _imp->table = new QTableWidget(layout->parentWidget());
    QStringList dimensionNames;
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = instanceSpecificKnobs.begin();it!=instanceSpecificKnobs.end();++it) {
        QString knobDesc((*it)->getDescription().c_str());
        for (int i = 0; i < (*it)->getDimension(); ++it) {
            dimensionNames.push_back(knobDesc + "_" + QString((*it)->getDimensionName(i).c_str()));
        }
    }
    _imp->table->setColumnCount(dimensionNames.size());
    _imp->table->setHorizontalHeaderLabels(dimensionNames);
    
    appendExtraGui(layout);
}