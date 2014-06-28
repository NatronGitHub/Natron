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
#include <QTableWidgetItem>
#include <QPixmap>
#include <QDebug>

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/SpinBox.h"

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
    
    Button* addButton;
    Button* removeButton;
    Button* selectAll;
    
    MultiInstancePanelPrivate(MultiInstancePanel* publicI,const boost::shared_ptr<Node>& node)
    : publicInterface(publicI)
    , instances()
    , table(0)
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , removeButton(0)
    , selectAll(0)
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
    
    void addTableRow(Node* node);
    
    void getInstanceSpecificKnobs(const Node* node,std::list<boost::shared_ptr<KnobI> >* knobs) const
    {
        const std::vector<boost::shared_ptr<KnobI> >& instanceKnobs = node->getKnobs();
        for (U32 i = 0; i < instanceKnobs.size(); ++i) {
            Int_Knob* isInt = dynamic_cast<Int_Knob*>(instanceKnobs[i].get());
            Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(instanceKnobs[i].get());
            Double_Knob* isDouble = dynamic_cast<Double_Knob*>(instanceKnobs[i].get());
            Color_Knob* isColor = dynamic_cast<Color_Knob*>(instanceKnobs[i].get());
            String_Knob* isString = dynamic_cast<String_Knob*>(instanceKnobs[i].get());
            
            if (instanceKnobs[i]->isInstanceSpecific()) {
                if (!isInt && !isBool && !isDouble && !isColor && !isString) {
                    qDebug() << "Multi-instance panel doesn't support the following type of knob: " << instanceKnobs[i]->typeName().c_str();
                    continue;
                }
                
                knobs->push_back(instanceKnobs[i]);
            }
        }

    }
};

MultiInstancePanel::MultiInstancePanel(const boost::shared_ptr<Node>& node)
: QObject()
, KnobHolder(node->getApp())
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
    _imp->getInstanceSpecificKnobs(_imp->getMainInstance(), &instanceSpecificKnobs);
    
    _imp->table = new QTableWidget(layout->parentWidget());
    QStringList dimensionNames;
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = instanceSpecificKnobs.begin();it!=instanceSpecificKnobs.end();++it) {
        QString knobDesc((*it)->getDescription().c_str());
        for (int i = 0; i < (*it)->getDimension(); ++it) {
            dimensionNames.push_back(knobDesc + "_" + QString((*it)->getDimensionName(i).c_str()));
        }
    }
    dimensionNames.prepend("Enabled");
    
    _imp->table->setColumnCount(dimensionNames.size());
    _imp->table->setHorizontalHeaderLabels(dimensionNames);
    _imp->table->setAttribute(Qt::WA_MacShowFocusRect,0);
    QObject::connect(_imp->table,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(onItemDataChanged(QTableWidgetItem*)));
    
    layout->addWidget(_imp->table);
    
    _imp->buttonsContainer = new QWidget(layout->parentWidget());
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    
    _imp->addButton = new Button(QIcon(),"+",_imp->buttonsContainer);
    _imp->buttonsLayout->addWidget(_imp->addButton);
    _imp->addButton->setFixedSize(18,18);
    QObject::connect(_imp->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddButtonClicked()));
    
    _imp->removeButton = new Button(QIcon(),"-",_imp->buttonsContainer);
    _imp->buttonsLayout->addWidget(_imp->removeButton);
    _imp->removeButton->setFixedSize(18,18);
    QObject::connect(_imp->removeButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveButtonClicked()));
    
    QPixmap selectAll;
    appPTR->getIcon(NATRON_PIXMAP_SELECT_ALL, &selectAll);
    
    _imp->selectAll = new Button(QIcon(),"",_imp->buttonsContainer);
    _imp->buttonsLayout->addWidget(_imp->selectAll);
    _imp->selectAll->setFixedSize(18,18);
    QObject::connect(_imp->selectAll, SIGNAL(clicked(bool)), this, SLOT(onSelectAllButtonClicked()));
    
    appendExtraGui(layout);
    
    ///finally insert the main instance in the table
    _imp->addTableRow(_imp->getMainInstance());
}

void MultiInstancePanel::onAddButtonClicked()
{
    Node* mainInstance = _imp->getMainInstance();
    
    boost::shared_ptr<Node> newInstance = _imp->getMainInstance()->getApp()->createNode(mainInstance->pluginID().c_str(),
                                                                                        false); //< don't create its gui
    _imp->instances.push_back(newInstance);
    _imp->addTableRow(newInstance.get());
}

namespace {
    
    static QWidget* createCheckBoxForTable(bool checked)
    {
        QWidget* enabledContainer = new QWidget();
        QHBoxLayout* enabledLayout = new QHBoxLayout(enabledContainer);
        enabledLayout->setContentsMargins(0, 0, 0, 0);
        AnimatedCheckBox* enabledBox = new AnimatedCheckBox(enabledContainer);
        enabledBox->setChecked(checked);
        enabledLayout->addWidget(enabledBox);
        return enabledContainer;
    }
}

void MultiInstancePanelPrivate::addTableRow(Node* node)
{
    int newRowIndex = table->rowCount();
    table->insertRow(newRowIndex);
    
    std::list<boost::shared_ptr<KnobI> > instanceSpecificKnobs;
    getInstanceSpecificKnobs(node, &instanceSpecificKnobs);
    
    ///first add the enabled column
    {
        QWidget* enabledContainer = createCheckBoxForTable(true);
        table->setCellWidget(newRowIndex, 0, enabledContainer);
    }
    
    int columnIndex = 1;
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = instanceSpecificKnobs.begin();it!=instanceSpecificKnobs.end();++it) {
        Int_Knob* isInt = dynamic_cast<Int_Knob*>(it->get());
        Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(it->get());
        Double_Knob* isDouble = dynamic_cast<Double_Knob*>(it->get());
        Color_Knob* isColor = dynamic_cast<Color_Knob*>(it->get());
        String_Knob* isString = dynamic_cast<String_Knob*>(it->get());
        
        if (!isInt || !isBool || !isDouble || !isColor || !isString) {
            continue;
        }
        
        bool createCheckBox = false;
        bool createSpinBox = false;
        if (isBool) {
            createCheckBox = true;
        } else if (isInt || isDouble || isColor) {
            createSpinBox = true;
        }
        
        for (int i = 0; i < (*it)->getDimension(); ++it) {
            if (createCheckBox) {
                assert(isBool);
                bool checked = isBool->getValue();
                QWidget* enabledContainer = createCheckBoxForTable(checked);
                table->setCellWidget(newRowIndex, columnIndex, enabledContainer);
            } else if (createSpinBox) {
                double mini = INT_MIN,maxi = INT_MAX;
                SpinBox::SPINBOX_TYPE type = SpinBox::DOUBLE_SPINBOX;
                if (isInt) {
                    mini = isInt->getMinimums()[i];
                    maxi = isInt->getMaximums()[i];
                    type = SpinBox::INT_SPINBOX;
                } else if (isDouble) {
                    mini = isDouble->getMinimums()[i];
                    maxi = isDouble->getMaximums()[i];
                }
                SpinBox* sb = new SpinBox(NULL,type);
                sb->setMinimum(mini);
                sb->setMaximum(maxi);
                table->setCellWidget(newRowIndex, columnIndex, sb);
            } else {
                assert(isString);
                std::string value = isString->getValue();
                LineEdit* le = new LineEdit(NULL);
                le->setText(value.c_str());
                table->setCellWidget(newRowIndex, columnIndex, le);
            }
            ++columnIndex;
        }
        
    }
}

void MultiInstancePanel::onRemoveButtonClicked()
{

}

void MultiInstancePanel::onSelectAllButtonClicked()
{
    
}

///An item in the table changed, forward it to the good knob
void MultiInstancePanel::onItemDataChanged(QTableWidgetItem* item)
{
    
}


/////////////// Tracker panel

TrackerPanel::TrackerPanel(const boost::shared_ptr<Natron::Node>& node)
: MultiInstancePanel(node)
{
    
}

TrackerPanel::~TrackerPanel()
{
    
}

void TrackerPanel::appendExtraGui(QVBoxLayout* layout)
{
    
}

void TrackerPanel::appendButton(QHBoxLayout* buttonLayout)
{
    
}
