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
#include <QPixmap>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QHeaderView>

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/SpinBox.h"
#include "Gui/TableModelView.h"

#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

using namespace Natron;

struct MultiInstancePanelPrivate
{
    
    MultiInstancePanel* publicInterface;
    std::list < boost::shared_ptr<Node> > instances;
    
    TableView* view;
    TableModel* model;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    
    Button* addButton;
    Button* removeButton;
    Button* selectAll;
    
    MultiInstancePanelPrivate(MultiInstancePanel* publicI,const boost::shared_ptr<Node>& node)
    : publicInterface(publicI)
    , instances()
    , view(0)
    , model(0)
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
        ret->setAllDimensionsEnabled(false);
        
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
    delete _imp->model;
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
    
    _imp->view = new TableView(layout->parentWidget());
    TableItemDelegate* delegate = new TableItemDelegate(_imp->view);
    _imp->view->setItemDelegate(delegate);
    
    _imp->model = new TableModel(0,0,_imp->view);
    _imp->view->setTableModel(_imp->model);
    QStringList dimensionNames;
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = instanceSpecificKnobs.begin();it!=instanceSpecificKnobs.end();++it) {
        QString knobDesc((*it)->getDescription().c_str());
        for (int i = 0; i < (*it)->getDimension(); ++i) {
            dimensionNames.push_back(knobDesc + "_" + QString((*it)->getDimensionName(i).c_str()));
        }
    }
    dimensionNames.prepend("Enabled");
    
    _imp->view->setColumnCount(dimensionNames.size());
    _imp->view->setHorizontalHeaderLabels(dimensionNames);
    _imp->view->setAttribute(Qt::WA_MacShowFocusRect,0);
    _imp->view->header()->setResizeMode(QHeaderView::ResizeToContents);
    
    layout->addWidget(_imp->view);
    
    _imp->buttonsContainer = new QWidget(layout->parentWidget());
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->setContentsMargins(0, 0, 0, 0);
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
    
    _imp->selectAll = new Button(QIcon(selectAll),"",_imp->buttonsContainer);
    _imp->selectAll->setToolTip("Select all");
    _imp->buttonsLayout->addWidget(_imp->selectAll);
    _imp->selectAll->setFixedSize(18,18);
    QObject::connect(_imp->selectAll, SIGNAL(clicked(bool)), this, SLOT(onSelectAllButtonClicked()));
    
    layout->addWidget(_imp->buttonsContainer);
    appendExtraGui(layout);
    appendButtons(_imp->buttonsLayout);
    _imp->buttonsLayout->addStretch();
    ///finally insert the main instance in the table
    _imp->addTableRow(_imp->getMainInstance());
}

void MultiInstancePanel::onAddButtonClicked()
{
    Node* mainInstance = _imp->getMainInstance();
    
    boost::shared_ptr<Node> newInstance = _imp->getMainInstance()->getApp()->createNode(mainInstance->pluginID().c_str(),
                                                                                        mainInstance->getName()); //< don't create its gui
    _imp->instances.push_back(newInstance);
    _imp->addTableRow(newInstance.get());
}

const std::list<boost::shared_ptr<Natron::Node> >& MultiInstancePanel::getInstances() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->instances;
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

void MultiInstancePanel::addRow(Natron::Node* node)
{
    _imp->addTableRow(node);
}

void MultiInstancePanelPrivate::addTableRow(Node* node)
{
    int newRowIndex = view->rowCount();
    model->insertRow(newRowIndex);

    std::list<boost::shared_ptr<KnobI> > instanceSpecificKnobs;
    getInstanceSpecificKnobs(node, &instanceSpecificKnobs);
    
    ///first add the enabled column
    {
        QWidget* enabledBox = createCheckBoxForTable(true);
        view->setCellWidget(newRowIndex, 0, enabledBox);
        view->resizeColumnToContents(0);
    }
    
    
    int columnIndex = 1;
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = instanceSpecificKnobs.begin();it!=instanceSpecificKnobs.end();++it) {
        Int_Knob* isInt = dynamic_cast<Int_Knob*>(it->get());
        Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(it->get());
        Double_Knob* isDouble = dynamic_cast<Double_Knob*>(it->get());
        Color_Knob* isColor = dynamic_cast<Color_Knob*>(it->get());
        String_Knob* isString = dynamic_cast<String_Knob*>(it->get());
        
        if (!isInt && !isBool && !isDouble && !isColor && !isString) {
            continue;
        }
        
        bool createCheckBox = false;
        bool createSpinBox = false;
        if (isBool) {
            createCheckBox = true;
        } else if (isInt || isDouble || isColor) {
            createSpinBox = true;
        }
        
        for (int i = 0; i < (*it)->getDimension(); ++i) {
            if (createCheckBox) {
                assert(isBool);
                bool checked = isBool->getValue();
                QWidget* enabledContainer = createCheckBoxForTable(checked);
                view->setCellWidget(newRowIndex, columnIndex, enabledContainer);
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
                view->setCellWidget(newRowIndex, columnIndex, sb);
            } else {
                assert(isString);
                std::string value = isString->getValue();
                LineEdit* le = new LineEdit(NULL);
                le->setText(value.c_str());
                view->setCellWidget(newRowIndex, columnIndex, le);
            }
            view->resizeColumnToContents(columnIndex);
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

void TrackerPanel::appendButtons(QHBoxLayout* buttonLayout)
{
    
}
