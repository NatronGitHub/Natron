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
    bool guiCreated;
    //pair <pointer,selected?>
    std::list < std::pair<boost::shared_ptr<Node>,bool> > instances;
    
    TableView* view;
    TableModel* model;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    
    Button* addButton;
    Button* removeButton;
    Button* selectAll;
    
    MultiInstancePanelPrivate(MultiInstancePanel* publicI,const boost::shared_ptr<Node>& node)
    : publicInterface(publicI)
    , guiCreated(false)
    , instances()
    , view(0)
    , model(0)
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , removeButton(0)
    , selectAll(0)
    {
        instances.push_back(std::make_pair(node,false));
    }
    
    boost::shared_ptr<Natron::Node> getMainInstance() const { assert(!instances.empty()); return instances.front().first; }
    
    void createKnob(const boost::shared_ptr<KnobI>& ref)
    {
        if (ref->isInstanceSpecific() || !ref->isDeclaredByPlugin()) {
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
        else if (dynamic_cast<Button_Knob*>(ref.get())) {
            ret = Natron::createKnob<Button_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        else if (dynamic_cast<Page_Knob*>(ref.get())) {
            ret = Natron::createKnob<Page_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),false);
        }
        assert(ret);
        ret->clone(ref);
        ret->setName(ref->getName());
        ret->setAnimationEnabled(ref->isAnimationEnabled());
        ret->setHintToolTip(ref->getHintToolTip());
        ret->setEvaluateOnChange(ref->getEvaluateOnChange());
        ret->setIsPersistant(false);
        ret->setAllDimensionsEnabled(false);
        if (ret->isNewLineTurnedOff()) {
            ret->turnOffNewLine();
        }
        
    }
    
    void addTableRow(const boost::shared_ptr<Natron::Node>& node);
    
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
    
    void getNodesFromSelection(const QModelIndexList& indexes,std::list<std::pair<Node*,bool> >* nodes) ;
};

MultiInstancePanel::MultiInstancePanel(const boost::shared_ptr<Node>& node)
: QObject()
, NamedKnobHolder(node->getApp())
, _imp(new MultiInstancePanelPrivate(this,node))
{
}

MultiInstancePanel::~MultiInstancePanel()
{
    delete _imp->model;
}

std::string MultiInstancePanel::getName_mt_safe() const
{
    return _imp->getMainInstance()->getName_mt_safe();
}

void MultiInstancePanel::initializeKnobs()
{
    const std::vector<boost::shared_ptr<KnobI> >& mainInstanceKnobs = _imp->getMainInstance()->getKnobs();
    for (U32 i = 0; i < mainInstanceKnobs.size(); ++i) {
        if (!mainInstanceKnobs[i]->isDeclaredByPlugin()) {
            addKnob(mainInstanceKnobs[i]);
        } else {
            _imp->createKnob(mainInstanceKnobs[i]);
        }
    }
}

bool MultiInstancePanel::isGuiCreated() const
{
    return _imp->guiCreated;
}

void MultiInstancePanel::createMultiInstanceGui(QVBoxLayout* layout)
{
    std::list<boost::shared_ptr<KnobI> > instanceSpecificKnobs;
    _imp->getInstanceSpecificKnobs(_imp->getMainInstance().get(), &instanceSpecificKnobs);
    
    _imp->view = new TableView(layout->parentWidget());
    //TableItemDelegate* delegate = new TableItemDelegate(_imp->view);
    //  _imp->view->setItemDelegate(delegate);
    
    _imp->model = new TableModel(0,0,_imp->view);
    QObject::connect(_imp->model,SIGNAL(s_itemChanged(TableItem*)),this,SLOT(onItemDataChanged(TableItem*)));
    _imp->view->setTableModel(_imp->model);
    
    QItemSelectionModel *selectionModel = _imp->view->selectionModel();
    QObject::connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this,
                     SLOT(onSelectionChanged(QItemSelection,QItemSelection)));
    QStringList dimensionNames;
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = instanceSpecificKnobs.begin();it!=instanceSpecificKnobs.end();++it) {
        QString knobDesc((*it)->getDescription().c_str());
        int dims = (*it)->getDimension();
        for (int i = 0; i < dims; ++i) {
            QString dimName(knobDesc);
            if (dims > 1) {
                dimName += ' ';
                dimName += (*it)->getDimensionName(i).c_str();
            }
            dimensionNames.push_back(dimName);
        }
    }
    dimensionNames.prepend("Enabled");
    
    _imp->view->setColumnCount(dimensionNames.size());
    _imp->view->setHorizontalHeaderLabels(dimensionNames);
    
    _imp->view->setAttribute(Qt::WA_MacShowFocusRect,0);
    _imp->view->header()->setResizeMode(QHeaderView::ResizeToContents);
    _imp->view->header()->setStretchLastSection(true);
    
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
    
    _imp->guiCreated = true;
}

void MultiInstancePanel::onAddButtonClicked()
{
    boost::shared_ptr<Natron::Node> mainInstance = _imp->getMainInstance();
    
    boost::shared_ptr<Node> newInstance = _imp->getMainInstance()->getApp()->createNode(mainInstance->pluginID().c_str(),
                                                                                        mainInstance->getName()); //< don't create its gui
    _imp->addTableRow(newInstance);
}

const std::list< std::pair<boost::shared_ptr<Natron::Node>,bool> >& MultiInstancePanel::getInstances() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->instances;
}


void MultiInstancePanel::addRow(const boost::shared_ptr<Natron::Node>& node)
{
    _imp->addTableRow(node);
}

void MultiInstancePanelPrivate::addTableRow(const boost::shared_ptr<Natron::Node>& node)
{
    instances.push_back(std::make_pair(node,false));
    int newRowIndex = view->rowCount();
    model->insertRow(newRowIndex);

    std::list<boost::shared_ptr<KnobI> > instanceSpecificKnobs;
    getInstanceSpecificKnobs(node.get(), &instanceSpecificKnobs);
    
    ///first add the enabled column
    {
        AnimatedCheckBox* checkbox = new AnimatedCheckBox();
        checkbox->setChecked(true);
        view->setCellWidget(newRowIndex, 0, checkbox);
        TableItem* newItem = new TableItem;
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
        view->setItem(newRowIndex, 0, newItem);
        view->resizeColumnToContents(0);
    }
    
    
    int columnIndex = 1;
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = instanceSpecificKnobs.begin();it!=instanceSpecificKnobs.end();++it) {
        Int_Knob* isInt = dynamic_cast<Int_Knob*>(it->get());
        Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(it->get());
        Double_Knob* isDouble = dynamic_cast<Double_Knob*>(it->get());
        Color_Knob* isColor = dynamic_cast<Color_Knob*>(it->get());
        String_Knob* isString = dynamic_cast<String_Knob*>(it->get());
        
        
        ///Only these types are supported
        if (!isInt && !isBool && !isDouble && !isColor && !isString) {
            continue;
        }

        for (int i = 0; i < (*it)->getDimension(); ++i) {
            
            TableItem* newItem = new TableItem;
            Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

            if (isBool) {
                bool checked = isBool->getValue();
                AnimatedCheckBox* checkbox = new AnimatedCheckBox();
                checkbox->setChecked(checked);
                view->setCellWidget(newRowIndex, columnIndex, checkbox);
                flags |= Qt::ItemIsUserCheckable;
            } else if (isInt) {
                newItem->setData(Qt::DisplayRole, isInt->getValue());
            } else if (isDouble) {
                newItem->setData(Qt::DisplayRole, isDouble->getValue());
            } else if (isString) {
                newItem->setData(Qt::DisplayRole, isString->getValue().c_str());
            }
            newItem->setFlags(flags);

            view->setItem(newRowIndex, columnIndex, newItem);
            view->resizeColumnToContents(columnIndex);
            ++columnIndex;
            
        }
        
    }
    
    ///clear current selection
    view->selectionModel()->clear();
    
    ///select the new item
    QModelIndex newIndex = model->index(newRowIndex, 0);
    assert(newIndex.isValid());
    view->selectionModel()->select(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void MultiInstancePanel::onRemoveButtonClicked()
{

}

void MultiInstancePanel::onSelectAllButtonClicked()
{
    _imp->view->selectAll();
}

void MultiInstancePanel::onSelectionChanged(const QItemSelection& newSelection,const QItemSelection& oldSelection)
{
    std::list<std::pair<Node*,bool> > previouslySelectedInstances;
    QModelIndexList oldIndexes = oldSelection.indexes();
    for (int i = 0; i < oldIndexes.size(); ++i) {
        TableItem* item = _imp->model->item(oldIndexes[i]);
        if (item) {
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
    }
    _imp->getNodesFromSelection(oldIndexes, &previouslySelectedInstances);
    
    bool copyOnUnSlave = previouslySelectedInstances.size()  == 1;
    
    
    QModelIndexList rows = _imp->view->selectionModel()->selectedRows();
    bool setDirty = rows.count() > 1;
    
    for (std::list<std::pair<Node*,bool> >::iterator it = previouslySelectedInstances.begin(); it!=previouslySelectedInstances.end(); ++it) {
        ///disconnect all the knobs
        if (!it->second) {
            continue;
        }
        const std::vector<boost::shared_ptr<KnobI> >& knobs = it->first->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific()) {
                for (int j = 0; j < knobs[i]->getDimension();++j) {
                    knobs[i]->unSlave(j, copyOnUnSlave);
                }
            }
        }
        it->second = false;
    }
    
    ///now slave new selection
    std::list<std::pair<Node*,bool> > newlySelectedInstances;
    QModelIndexList newIndexes = newSelection.indexes();
    for (int i = 0; i < newIndexes.size(); ++i) {
        TableItem* item = _imp->model->item(newIndexes[i]);
        if (item) {
            item->setFlags(item->flags() | Qt::ItemIsEditable);
        }
    }
    _imp->getNodesFromSelection(newIndexes, &newlySelectedInstances);
 
    
    for (std::list<std::pair<Node*,bool> >::iterator it = newlySelectedInstances.begin(); it!=newlySelectedInstances.end(); ++it) {
        ///slave all the knobs
        if (it->second) {
            continue;
        }
        const std::vector<boost::shared_ptr<KnobI> >& knobs = it->first->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific()) {
                
                boost::shared_ptr<KnobI> otherKnob = getKnobByName(knobs[i]->getName());
                assert(otherKnob);
                for (int j = 0; j < knobs[i]->getDimension();++j) {
                    knobs[i]->slaveTo(j, otherKnob, j,true);
                }
                otherKnob->setAllDimensionsEnabled(true);
                otherKnob->setDirty(setDirty);
                
            }
        }
        it->second = true;
    }
    
    
    if (newlySelectedInstances.empty()) {
        ///disable knobs
        const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if (!knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific()) {
                knobs[i]->setAllDimensionsEnabled(false);
                knobs[i]->setDirty(false);
            }
        }
    }
}

void MultiInstancePanelPrivate::getNodesFromSelection(const QModelIndexList& indexes,std::list<std::pair<Node*,bool> >* nodes)
{
    std::set<int> rows;
    for (int i = 0; i < indexes.size(); ++i) {
        rows.insert(indexes[i].row());
    }
    
    for (std::set<int>::iterator it = rows.begin(); it!=rows.end(); ++it) {
        assert(*it >= 0 && *it < (int)instances.size());
        std::list< std::pair<boost::shared_ptr<Node>,bool > >::iterator it2 = instances.begin();
        std::advance(it2, *it);
        if (!it2->first->isNodeDisabled()) {
            nodes->push_back(std::make_pair(it2->first.get(), it2->second));
        }
    }

}

void MultiInstancePanel::onItemDataChanged(TableItem* item)
{
    QVariant data = item->data(Qt::DisplayRole);
    
}

/////////////// Tracker panel

TrackerPanel::TrackerPanel(const boost::shared_ptr<Natron::Node>& node)
: MultiInstancePanel(node)
{
    
}

TrackerPanel::~TrackerPanel()
{
    
}

void TrackerPanel::appendExtraGui(QVBoxLayout* /*layout*/)
{
    
}

void TrackerPanel::appendButtons(QHBoxLayout* /*buttonLayout*/)
{
    
}
