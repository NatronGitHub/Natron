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
#include <QStyledItemDelegate>
#include <QUndoCommand>
#include <QPainter>

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/SpinBox.h"
#include "Gui/TableModelView.h"
#include "Gui/NodeGui.h"
#include "Gui/DockablePanel.h"

#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/EffectInstance.h"
#include "Engine/Curve.h"
using namespace Natron;

namespace {
typedef std::list < std::pair<boost::shared_ptr<Node>,bool> > Nodes;
}

struct MultiInstancePanelPrivate
{
    
    MultiInstancePanel* publicInterface;
    bool guiCreated;
    
    boost::shared_ptr<NodeGui> mainInstance;
    //pair <pointer,selected?>
    Nodes instances;
    
    TableView* view;
    TableModel* model;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    
    Button* addButton;
    Button* removeButton;
    Button* selectAll;
    Button* resetTracksButton;

    ///Set to true when we receive a signal from a knob value change
    ///this is to avoid infinite recursion with the dataChanged signal from the TableItem
    bool executingKnobValueChanged;
    
    MultiInstancePanelPrivate(MultiInstancePanel* publicI,const boost::shared_ptr<NodeGui>& node)
    : publicInterface(publicI)
    , guiCreated(false)
    , mainInstance(node)
    , instances()
    , view(0)
    , model(0)
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , removeButton(0)
    , selectAll(0)
    , executingKnobValueChanged(false)
    {
    }
    
    boost::shared_ptr<Natron::Node> getMainInstance() const { return mainInstance->getNode(); }
    
    void createKnob(const boost::shared_ptr<KnobI>& ref)
    {
        if (ref->isInstanceSpecific()) {
            return;
        }
        
        bool declaredByPlugin = ref->isDeclaredByPlugin();
        Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(ref.get());
        String_Knob* isString = dynamic_cast<String_Knob*>(ref.get());
        
        boost::shared_ptr<KnobI> ret;
        if (dynamic_cast<Int_Knob*>(ref.get())) {
            ret = Natron::createKnob<Int_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<Bool_Knob*>(ref.get())) {
            ret = Natron::createKnob<Bool_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<Double_Knob*>(ref.get())) {
            ret = Natron::createKnob<Double_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (isChoice) {
            boost::shared_ptr<Choice_Knob> choice = Natron::createKnob<Choice_Knob>(publicInterface,
                                                                                    ref->getDescription(),ref->getDimension(),declaredByPlugin);
            choice->populateChoices(isChoice->getEntries(),isChoice->getEntriesHelp());
            ret = choice;
        }
        else if (isString) {
            boost::shared_ptr<String_Knob> strKnob = Natron::createKnob<String_Knob>(publicInterface,
                                                                            ref->getDescription(),ref->getDimension(),declaredByPlugin);
            if (isString->isCustomKnob()) {
                strKnob->setAsCustom();
            }
            if (isString->isMultiLine()) {
                strKnob->setAsMultiLine();
            }
            if (isString->isLabel()) {
                strKnob->setAsLabel();
            }
            if (isString->usesRichText()) {
                strKnob->setUsesRichText(true);
            }
            ret = strKnob;
        }
        else if (dynamic_cast<Parametric_Knob*>(ref.get())) {
            ret = Natron::createKnob<Parametric_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<Color_Knob*>(ref.get())) {
            ret = Natron::createKnob<Color_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<Path_Knob*>(ref.get())) {
            ret = Natron::createKnob<Path_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<File_Knob*>(ref.get())) {
            ret = Natron::createKnob<File_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<OutputFile_Knob*>(ref.get())) {
            ret = Natron::createKnob<OutputFile_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<Button_Knob*>(ref.get())) {
            ret = Natron::createKnob<Button_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        else if (dynamic_cast<Page_Knob*>(ref.get())) {
            ret = Natron::createKnob<Page_Knob>(publicInterface, ref->getDescription(),ref->getDimension(),declaredByPlugin);
        }
        assert(ret);
        ret->clone(ref);
        ret->setName(ref->getName());
        ret->setAnimationEnabled(ref->isAnimationEnabled());
        ret->setHintToolTip(ref->getHintToolTip());
        ret->setEvaluateOnChange(ref->getEvaluateOnChange());
        ret->setIsPersistant(false);
        if (ref->isNewLineTurnedOff()) {
            ret->turnOffNewLine();
        }
        bool refSecret = ref->getIsSecret();
        if (!refSecret && !declaredByPlugin && ref->getName() != "disable_natron") {
            for (int i = 0; i < ref->getDimension();++i) {
                ref->slaveTo(i, ret, i,true);
            }
        } else {
            ret->setAllDimensionsEnabled(false);
        }
        if (refSecret) {
            ret->setSecret(true);
        }
    }
    
    void addTableRow(const boost::shared_ptr<Natron::Node>& node);
    
    void removeRow(int index);
    
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
    
    void pushUndoCommand(QUndoCommand* cmd) {
        mainInstance->getSettingPanel()->pushUndoCommand(cmd);
    }
};

MultiInstancePanel::MultiInstancePanel(const boost::shared_ptr<NodeGui>& node)
: QObject()
, NamedKnobHolder(node->getNode()->getApp())
, _imp(new MultiInstancePanelPrivate(this,node))
{
}

MultiInstancePanel::~MultiInstancePanel()
{
    delete _imp->model;
}

////////////// TableView delegate

class TableItemDelegate : public QStyledItemDelegate {
    
    TableView* _view;
    MultiInstancePanel* _panel;
    
public:
    
    explicit TableItemDelegate(TableView* view,MultiInstancePanel* panel);
    
private:
    
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
    
};



TableItemDelegate::TableItemDelegate(TableView* view,MultiInstancePanel* panel)
: QStyledItemDelegate(view)
, _view(view)
, _panel(panel)
{
    
}


void TableItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QStyledItemDelegate::paint(painter,option,index);
    if (!index.isValid() || index.column() == 0 || option.state & QStyle::State_Selected) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    TableItem* item = dynamic_cast<TableModel*>(_view->model())->item(index);
    assert(item);
    int dim;
    boost::shared_ptr<KnobI> knob = _panel->getKnobForItem(item, &dim);
    assert(knob);
    Natron::AnimationLevel level = knob->getAnimationLevel(dim);
    if (level == NO_ANIMATION) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    
    const QWidget* widget = _view->cellWidget(index.row(), index.column());
    if ( !widget ) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    QColor bgColor;
    if (level == ON_KEYFRAME) {
        bgColor.setRgb(21,97,248);
    } else if (level == INTERPOLATED_VALUE) {
        bgColor.setRgb(86,117,156);
    }

    
    
    //   widget->render(painter);
    
}



boost::shared_ptr<Natron::Node> MultiInstancePanel::getMainInstance() const
{
    return _imp->getMainInstance();
}

std::string MultiInstancePanel::getName_mt_safe() const
{
    return _imp->getMainInstance()->getName_mt_safe();
}

void MultiInstancePanel::initializeKnobs()
{
    const std::vector<boost::shared_ptr<KnobI> >& mainInstanceKnobs = _imp->getMainInstance()->getKnobs();
    for (U32 i = 0; i < mainInstanceKnobs.size(); ++i) {
        _imp->createKnob(mainInstanceKnobs[i]);
    }
    ///copy page children
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        Page_Knob* isPage = dynamic_cast<Page_Knob*>(knobs[i].get());
        
        if (isPage) {
            ///find the corresponding knob in the main instance knobs
            boost::shared_ptr<KnobI> other = _imp->getMainInstance()->getKnobByName(isPage->getName());
            assert(other);
            Page_Knob* otherPage = dynamic_cast<Page_Knob*>(other.get());
            assert(otherPage);
            const std::vector<boost::shared_ptr<KnobI> >& otherChildren = otherPage->getChildren();
            for (U32 j = 0; j < otherChildren.size(); ++j) {
                if (!otherChildren[j]->isInstanceSpecific()) {
                    boost::shared_ptr<KnobI> thisChild = getKnobByName(otherChildren[j]->getName());
                    assert(thisChild);
                    isPage->addKnob(thisChild);
                }
            }
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
    QObject::connect(_imp->view,SIGNAL(deleteKeyPressed()),this,SLOT(onDeleteKeyPressed()));
    TableItemDelegate* delegate = new TableItemDelegate(_imp->view,this);
    _imp->view->setItemDelegate(delegate);
    
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
    _imp->addButton->setToolTip("Add new");
    _imp->buttonsLayout->addWidget(_imp->addButton);
    _imp->addButton->setFixedSize(18,18);
    QObject::connect(_imp->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddButtonClicked()));
    
    _imp->removeButton = new Button(QIcon(),"-",_imp->buttonsContainer);
    _imp->removeButton->setToolTip("Remove selection");
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
    
    _imp->resetTracksButton = new Button("Reset",_imp->buttonsContainer);
    QObject::connect(_imp->resetTracksButton, SIGNAL(clicked(bool)), this, SLOT(resetSelectedInstances()));
    _imp->buttonsLayout->addWidget(_imp->resetTracksButton);
    _imp->resetTracksButton->setToolTip("Reset selected items");
    
    layout->addWidget(_imp->buttonsContainer);
    appendExtraGui(layout);
    appendButtons(_imp->buttonsLayout);
    _imp->buttonsLayout->addStretch();
    ///finally insert the main instance in the table
    _imp->addTableRow(_imp->getMainInstance());
    
    _imp->guiCreated = true;
}

class AddNodeCommand : public QUndoCommand
{
    bool _firstRedoCalled;
    boost::shared_ptr<Node> _node;
    MultiInstancePanel* _panel;
public:
    
    AddNodeCommand(MultiInstancePanel* panel,const boost::shared_ptr<Node>& node,QUndoCommand* parent = 0)
    : QUndoCommand(parent)
    , _firstRedoCalled(false)
    , _node(node)
    , _panel(panel)
    {
        
    }
    
    virtual ~AddNodeCommand() {}
    
    virtual void undo() OVERRIDE FINAL
    {
        int index = _panel->getNodeIndex(_node);
        assert(index != -1);
        _panel->removeRow(index);
        _node->deactivate();
        setText(QString("Add %1").arg(_node->getName().c_str()));
    }
    
    virtual void redo() OVERRIDE FINAL
    {
        if (_firstRedoCalled) {
            _node->activate();
            _panel->addRow(_node);
        }
        _firstRedoCalled = true;
        setText(QString("Add %1").arg(_node->getName().c_str()));
    }
    
    
};

void MultiInstancePanel::onAddButtonClicked()
{
    boost::shared_ptr<Natron::Node> mainInstance = _imp->getMainInstance();
    
    CreateNodeArgs args(mainInstance->pluginID().c_str(),
                   mainInstance->getName(),
                   -1,-1,true,
                   (int)_imp->instances.size());
    boost::shared_ptr<Node> newInstance = _imp->getMainInstance()->getApp()->createNode(args);
    _imp->addTableRow(newInstance);
    _imp->pushUndoCommand(new AddNodeCommand(this,newInstance));
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
        QObject::connect(checkbox,SIGNAL(toggled(bool)),publicInterface,SLOT(onCheckBoxChecked(bool)));
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
                bool checked = isBool->getValue(i);
                AnimatedCheckBox* checkbox = new AnimatedCheckBox();
                checkbox->setChecked(checked);
                view->setCellWidget(newRowIndex, columnIndex, checkbox);
                flags |= Qt::ItemIsUserCheckable;
            } else if (isInt) {
                newItem->setData(Qt::DisplayRole, isInt->getValue(i));
            } else if (isDouble) {
                newItem->setData(Qt::DisplayRole, isDouble->getValue(i));
            } else if (isString) {
                newItem->setData(Qt::DisplayRole, isString->getValue(i).c_str());
            }
            newItem->setFlags(flags);

            view->setItem(newRowIndex, columnIndex, newItem);
            view->resizeColumnToContents(columnIndex);
            ++columnIndex;
            
        }
        boost::shared_ptr<KnobSignalSlotHandler> slotsHandler = dynamic_cast<KnobHelper*>(it->get())->getSignalSlotHandler();
        if (slotsHandler) {
            QObject::connect(slotsHandler.get(), SIGNAL(valueChanged(int)), publicInterface,SLOT(onInstanceKnobValueChanged(int)));
        }
        
    }
    
    ///clear current selection
    view->selectionModel()->clear();
    
    ///select the new item
    QModelIndex newIndex = model->index(newRowIndex, 0);
    assert(newIndex.isValid());
    view->selectionModel()->select(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

class RemoveNodeCommand : public QUndoCommand
{
    MultiInstancePanel* _panel;
    std::list<boost::shared_ptr<Natron::Node> > _nodes;
public:
    
    RemoveNodeCommand(MultiInstancePanel* panel,const std::list<boost::shared_ptr<Natron::Node> >& nodes,QUndoCommand* parent = 0)
    : QUndoCommand(parent)
    , _panel(panel)
    , _nodes(nodes)
    {
        
    }
    
    virtual ~RemoveNodeCommand() {}
    
    virtual void undo() OVERRIDE FINAL
    {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
            _panel->addRow(*it);
            (*it)->activate();
        }
        setText(QString("Remove instance(s)"));
    }
    
    virtual void redo() OVERRIDE FINAL
    {
        boost::shared_ptr<Node> mainInstance = _panel->getMainInstance();
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
            int index = _panel->getNodeIndex(*it);
            assert(index != -1);
            _panel->removeRow(index);
            bool isMainInstance = (*it) == mainInstance;
            (*it)->deactivate(!isMainInstance);
        }
        setText(QString("Remove instance(s)"));
    }
};

void MultiInstancePanel::removeRow(int index)
{
    _imp->removeRow(index);
}

void MultiInstancePanelPrivate::removeRow(int index)
{
    if (index < 0 || index >= (int)instances.size()) {
        throw std::invalid_argument("Index out of range");
    }
    model->removeRows(index);
    Nodes::iterator it = instances.begin();
    std::advance(it, index);
    instances.erase(it);
}

int MultiInstancePanel::getNodeIndex(const boost::shared_ptr<Natron::Node>& node) const
{
    int i = 0;
    Nodes::iterator it = _imp->instances.begin();
    for (; it!=_imp->instances.end(); ++it,++i) {
        if (it->first == node) {
            return i;
        }
    }
    return -1;
}

void MultiInstancePanel::onDeleteKeyPressed()
{
    removeInstancesInternal();
}

void MultiInstancePanel::onRemoveButtonClicked()
{
    removeInstancesInternal();
}

void MultiInstancePanel::removeInstancesInternal()
{
    const QItemSelection selection = _imp->view->selectionModel()->selection();
    std::list<boost::shared_ptr<Node> > instances;
    QModelIndexList indexes = selection.indexes();
    std::set<int> rows;
    for (int i = 0; i < indexes.size(); ++i) {
        rows.insert(indexes[i].row());
    }
    
    for (std::set<int>::iterator it = rows.begin(); it!=rows.end(); ++it) {
        assert(*it >= 0 && *it < (int)_imp->instances.size());
        std::list< std::pair<boost::shared_ptr<Node>,bool > >::iterator it2 = _imp->instances.begin();
        std::advance(it2, *it);
        instances.push_back(it2->first);
    }
    _imp->pushUndoCommand(new RemoveNodeCommand(this,instances));

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
    
    bool copyOnUnSlave = previouslySelectedInstances.size()  <= 1;
    
    
    QModelIndexList rows = _imp->view->selectionModel()->selectedRows();
    bool setDirty = rows.count() > 1;
    
    for (std::list<std::pair<Node*,bool> >::iterator it = previouslySelectedInstances.begin(); it!=previouslySelectedInstances.end(); ++it) {
        ///disconnect all the knobs
        if (!it->second) {
            continue;
        }
        const std::vector<boost::shared_ptr<KnobI> >& knobs = it->first->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific() && !knobs[i]->getIsSecret()) {
                for (int j = 0; j < knobs[i]->getDimension();++j) {
                    knobs[i]->unSlave(j, copyOnUnSlave);
                }
            }
        }
        for (Nodes::iterator it2 = _imp->instances.begin(); it2!=_imp->instances.end(); ++it2) {
            if (it2->first.get() == it->first) {
                it2->second = false;
                break;
            }
        }
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
            if (knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific() && !knobs[i]->getIsSecret()) {
                
                boost::shared_ptr<KnobI> otherKnob = getKnobByName(knobs[i]->getName());
                assert(otherKnob);
                otherKnob->clone(knobs[i]);
                for (int j = 0; j < knobs[i]->getDimension();++j) {
                    knobs[i]->slaveTo(j, otherKnob, j,true);
                }
                otherKnob->setAllDimensionsEnabled(true);
                otherKnob->setDirty(setDirty);
                
            }
        }
        for (Nodes::iterator it2 = _imp->instances.begin(); it2!=_imp->instances.end(); ++it2) {
            if (it2->first.get() == it->first) {
                it2->second = true;
                break;
            }
        }
    }
    
    
    if (newlySelectedInstances.empty()) {
        ///disable knobs
        const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific()) {
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

boost::shared_ptr<KnobI> MultiInstancePanel::getKnobForItem(TableItem* item,int* dimension) const
{
    QModelIndex modelIndex = _imp->model->index(item);
    assert(modelIndex.row() < (int)_imp->instances.size());
    Nodes::iterator nIt = _imp->instances.begin();
    std::advance(nIt, modelIndex.row());
    const std::vector<boost::shared_ptr<KnobI> >& knobs = nIt->first->getKnobs();
    int instanceSpecificIndex = 1; //< 1 because we skip the enable cell
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->isInstanceSpecific()) {
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                if (instanceSpecificIndex == modelIndex.column()) {
                    *dimension = j;
                    return knobs[i];
                }
                ++instanceSpecificIndex;
            }
        }
    }
    return boost::shared_ptr<KnobI>();
}

void MultiInstancePanel::onItemDataChanged(TableItem* item)
{
    
    if (_imp->executingKnobValueChanged) {
        return;
    }
    QVariant data = item->data(Qt::DisplayRole);
    QModelIndex modelIndex = _imp->model->index(item);
    
    ///The enabled cell is handled in onCheckBoxChecked
    if (modelIndex.column() == 0) {
        return;
    }
    assert(modelIndex.row() < (int)_imp->instances.size());
    Nodes::iterator nIt = _imp->instances.begin();
    std::advance(nIt, modelIndex.row());
    const std::vector<boost::shared_ptr<KnobI> >& knobs = nIt->first->getKnobs();
    int instanceSpecificIndex = 1; //< 1 because we skip the enable cell
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->isInstanceSpecific()) {
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                if (instanceSpecificIndex == modelIndex.column()) {
                    Int_Knob* isInt = dynamic_cast<Int_Knob*>(knobs[i].get());
                    Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knobs[i].get());
                    Double_Knob* isDouble = dynamic_cast<Double_Knob*>(knobs[i].get());
                    Color_Knob* isColor = dynamic_cast<Color_Knob*>(knobs[i].get());
                    String_Knob* isString = dynamic_cast<String_Knob*>(knobs[i].get());
                    if (isInt) {
                        isInt->setValue(data.toInt(), j);
                    } else if (isBool) {
                        isBool->setValue(data.toBool(), j);
                    } else if (isDouble) {
                        isDouble->setValue(data.toDouble(), j);
                    } else if (isColor) {
                        isColor->setValue(data.toDouble(), j);
                    } else if (isString) {
                        isString->setValue(data.toString().toStdString(), j);
                    }
                    return;
                }
                ++instanceSpecificIndex;
            }
        }
    }
    
}

void MultiInstancePanel::onCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* checkbox = qobject_cast<AnimatedCheckBox*>(sender());
    if (!checkbox) {
        return;
    }
    
    ///find the row which owns this checkbox
    for (int i = 0; i < _imp->model->rowCount(); ++i) {
        QWidget* w = _imp->view->cellWidget(i, 0);
        if (w == checkbox) {
            assert(i < (int)_imp->instances.size());
            Nodes::iterator it = _imp->instances.begin();
            std::advance(it, i);
            boost::shared_ptr<KnobI> enabledKnob = it->first->getKnobByName("disable_natron");
            assert(enabledKnob);
            Bool_Knob* bKnob = dynamic_cast<Bool_Knob*>(enabledKnob.get());
            assert(bKnob);
            bKnob->setValue(!checked, 0);
            break;
        }
    }
}

void MultiInstancePanel::onInstanceKnobValueChanged(int dim)
{
    KnobSignalSlotHandler* signalEmitter = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (!signalEmitter) {
        return;
    }
    boost::shared_ptr<KnobI> knob = signalEmitter->getKnob();
    KnobHolder* holder = knob->getHolder();
    assert(holder);
    int rowIndex = 0;
    int colIndex = 1;
    for (Nodes::iterator it = _imp->instances.begin(); it!=_imp->instances.end(); ++it,++rowIndex) {
        if (holder == it->first->getLiveInstance()) {
            const std::vector<boost::shared_ptr<KnobI> >& knobs = it->first->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if (knobs[i]->isInstanceSpecific()) {
                    if (knobs[i] == knob) {
                        colIndex += dim;
                        TableItem* item = _imp->model->item(rowIndex, colIndex);
                        QVariant data;
                        Int_Knob* isInt = dynamic_cast<Int_Knob*>(knobs[i].get());
                        Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knobs[i].get());
                        Double_Knob* isDouble = dynamic_cast<Double_Knob*>(knobs[i].get());
                        Color_Knob* isColor = dynamic_cast<Color_Knob*>(knobs[i].get());
                        String_Knob* isString = dynamic_cast<String_Knob*>(knobs[i].get());
                        if (isInt) {
                            data.setValue<int>(isInt->getValue(dim));
                        } else if (isBool) {
                            data.setValue<bool>(isBool->getValue(dim));
                        } else if (isDouble) {
                            data.setValue<double>(isDouble->getValue(dim));
                        } else if (isColor) {
                            data.setValue<double>(isColor->getValue(dim));
                        } else if (isString) {
                            data.setValue<QString>(isString->getValue(dim).c_str());
                        }
                        _imp->executingKnobValueChanged = true;
                        item->setData(Qt::DisplayRole,data);
                        _imp->executingKnobValueChanged = false;
                        return;
                    }
                    colIndex += knobs[i]->getDimension();
                }
            }
            return;
        }
    }
}

void MultiInstancePanel::getSelectedInstances(std::list<Natron::Node*>* instances) const
{
    const QItemSelection selection = _imp->view->selectionModel()->selection();
    QModelIndexList indexes = selection.indexes();
    std::set<int> rows;
    for (int i = 0; i < indexes.size(); ++i) {
        rows.insert(indexes[i].row());
    }
    
    for (std::set<int>::iterator it = rows.begin(); it!=rows.end(); ++it) {
        assert(*it >= 0 && *it < (int)_imp->instances.size());
        std::list< std::pair<boost::shared_ptr<Node>,bool > >::iterator it2 = _imp->instances.begin();
        std::advance(it2, *it);
        instances->push_back(it2->first.get());
    }

}

void MultiInstancePanel::resetSelectedInstances()
{
    std::list<Natron::Node*> selectedInstances;
    getSelectedInstances(&selectedInstances);
    _imp->view->selectionModel()->clear();
    resetInstances(selectedInstances);
}

void MultiInstancePanel::resetAllInstances()
{
    _imp->view->selectionModel()->clear();
    std::list<Natron::Node*> all;
    for (Nodes::iterator it = _imp->instances.begin(); it!=_imp->instances.end(); ++it) {
        all.push_back(it->first.get());
    }
    resetInstances(all);
}

void MultiInstancePanel::resetInstances(const std::list<Natron::Node*>& instances)
{
    for (std::list<Natron::Node*>::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        
//        Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>((*it)->getLiveInstance());
//        assert(isEffect);
//        std::list <SequenceTime> keys;
//        isEffect->getNode()->getAllKnobsKeyframes(&keys);
//        _imp->_gui->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(keys);
        
        notifyProjectBeginKnobsValuesChanged(Natron::USER_EDITED);
        const std::vector<boost::shared_ptr<KnobI> >& knobs = (*it)->getKnobs();
        for (U32 i = 0; i < knobs.size();++i) {
            Button_Knob* isBtn = dynamic_cast<Button_Knob*>(knobs[i].get());
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                if (!isBtn && knobs[i]->getName() != "label_natron") {
                    knobs[i]->resetToDefaultValue(j);
                }
            }
        }
        notifyProjectEndKnobsValuesChanged();
    }
    
    ///To update interacts, kinda hack but can't figure out where else put this
    getMainInstance()->getApp()->redrawAllViewers();
}

/////////////// Tracker panel
struct TrackerPanelPrivate
{
    Button* averageTracksButton;
    
    TrackerPanelPrivate()
    : averageTracksButton(0)
    {
        
    }
};

TrackerPanel::TrackerPanel(const boost::shared_ptr<NodeGui>& node)
: MultiInstancePanel(node)
, _imp(new TrackerPanelPrivate())
{
    
}

TrackerPanel::~TrackerPanel()
{
    
}

void TrackerPanel::appendExtraGui(QVBoxLayout* /*layout*/)
{
    
}

void TrackerPanel::appendButtons(QHBoxLayout* buttonLayout)
{
    _imp->averageTracksButton = new Button("Average tracks",buttonLayout->parentWidget());
    _imp->averageTracksButton->setToolTip("Make a new track which is the average of the selected tracks");
    QObject::connect(_imp->averageTracksButton, SIGNAL(clicked(bool)), this, SLOT(onAverageTracksButtonClicked()));
    buttonLayout->addWidget(_imp->averageTracksButton);
}

void TrackerPanel::onAverageTracksButtonClicked()
{
    std::list<Natron::Node*> selectedInstances;
    getSelectedInstances(&selectedInstances);
    if (selectedInstances.empty()) {
        Natron::warningDialog("Average", "No tracks selected");
        return;
    }
    for (std::list<Natron::Node*>::iterator it = selectedInstances.begin(); it!=selectedInstances.end(); ++it) {
        //average
    }
}
