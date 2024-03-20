/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "MultiInstancePanel.h"

#include <stdexcept>
#include <sstream> // stringstream

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QUndoCommand>
#include <QtWidgets/QApplication>

#include <QtGui/QPixmap>
#include <QtGui/QPainter>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <ofxNatron.h>

#include "Engine/Curve.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackerContext.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

#include "Gui/AnimatedCheckBox.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Label.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/SpinBox.h"
#include "Gui/TableModelView.h"

#define kTrackCenterName "center"
#define kTrackInvertName "invert"

#define COL_ENABLED 0
#define COL_SCRIPT_NAME 1
#define COL_FIRST_KNOB 2

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

typedef std::list<std::pair<NodeWPtr, bool> > Nodes;

KnobDoublePtr
getCenterKnobForTracker(Node* node)
{
    KnobIPtr knob = node->getKnobByName(kTrackCenterName);

    assert(knob);
    KnobDoublePtr dblKnob = std::dynamic_pointer_cast<KnobDouble>(knob);
    assert(dblKnob);

    return dblKnob;
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct MultiInstancePanelPrivate
{
    Q_DECLARE_TR_FUNCTIONS(MultiInstancePanel)

public:
    MultiInstancePanel* publicInterface;
    bool guiCreated;
    NodeGuiWPtr mainInstance;
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

    ///same as above but when we're dealing with unslave/slaving parameters
    int knobValueRecursion;
    bool redrawOnSelectionChanged;

    MultiInstancePanelPrivate(MultiInstancePanel* publicI,
                              const NodeGuiPtr & node)
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
        , resetTracksButton(0)
        , executingKnobValueChanged(false)
        , knobValueRecursion(0)
        , redrawOnSelectionChanged(true)
    {
    }

    NodePtr getMainInstance() const
    {
        return mainInstance.lock()->getNode();
    }

    /**
     * @brief Called to make an exact copy of a main-instance's knob. The resulting copy will
     * be what is displayed on the GUI
     **/
    void createKnob(const KnobIPtr & ref)
    {
        if ( ref->isInstanceSpecific() ) {
            return;
        }

        bool declaredByPlugin = ref->isDeclaredByPlugin();
        KnobButton* isButton = dynamic_cast<KnobButton*>( ref.get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( ref.get() );
        KnobString* isString = dynamic_cast<KnobString*>( ref.get() );
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>( ref.get() );
        KnobInt* isInt = dynamic_cast<KnobInt*>( ref.get() );
        KnobHelperPtr ret;
        if (isInt) {
            KnobIntPtr intKnb = AppManager::createKnob<KnobInt>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
            intKnb->setMinimumsAndMaximums( isInt->getMinimums(), isInt->getMaximums() );
            intKnb->setDisplayMinimumsAndMaximums( isInt->getDisplayMinimums(), isInt->getDisplayMaximums() );
            ret = intKnb;
        } else if ( dynamic_cast<KnobBool*>( ref.get() ) ) {
            ret = AppManager::createKnob<KnobBool>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
        } else if (isDouble) {
            KnobDoublePtr dblKnob = AppManager::createKnob<KnobDouble>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
            dblKnob->setMinimumsAndMaximums( isDouble->getMinimums(), isDouble->getMaximums() );
            dblKnob->setDisplayMinimumsAndMaximums( isDouble->getDisplayMinimums(), isDouble->getDisplayMaximums() );
            ret = dblKnob;
        } else if (isChoice) {
            KnobChoicePtr choice = AppManager::createKnob<KnobChoice>(publicInterface,
                                                                                      ref->getLabel(), ref->getDimension(), declaredByPlugin);
            choice->populateChoices( isChoice->getEntries_mt_safe() );
            ret = choice;
        } else if (isString) {
            KnobStringPtr strKnob = AppManager::createKnob<KnobString>(publicInterface,
                                                                                       ref->getLabel(), ref->getDimension(), declaredByPlugin);
            if ( isString->isCustomKnob() ) {
                strKnob->setAsCustom();
            }
            if ( isString->isMultiLine() ) {
                strKnob->setAsMultiLine();
            }
            if ( isString->isLabel() ) {
                strKnob->setAsLabel();
            }
            if ( isString->usesRichText() ) {
                strKnob->setUsesRichText(true);
            }
            ret = strKnob;
        } else if ( dynamic_cast<KnobParametric*>( ref.get() ) ) {
            ret = AppManager::createKnob<KnobParametric>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
        } else if ( dynamic_cast<KnobColor*>( ref.get() ) ) {
            ret = AppManager::createKnob<KnobColor>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
        } else if ( dynamic_cast<KnobPath*>( ref.get() ) ) {
            ret = AppManager::createKnob<KnobPath>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
        } else if ( dynamic_cast<KnobFile*>( ref.get() ) ) {
            ret = AppManager::createKnob<KnobFile>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
        } else if ( dynamic_cast<KnobOutputFile*>( ref.get() ) ) {
            ret = AppManager::createKnob<KnobOutputFile>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
        } else if (isButton) {
            KnobButtonPtr btn = AppManager::createKnob<KnobButton>(publicInterface,
                                                                                   ref->getLabel(), ref->getDimension(), declaredByPlugin);
            ///set the name prior to calling setIconForButton
            btn->setName( ref->getName() );
            publicInterface->setIconForButton( btn.get() );
            ret = btn;
        } else if ( dynamic_cast<KnobPage*>( ref.get() ) ) {
            ret = AppManager::createKnob<KnobPage>(publicInterface, ref->getLabel(), ref->getDimension(), declaredByPlugin);
        } else {
            return;
        }
        assert(ret);
        ret->clone(ref);
        ret->setName( ref->getName() );
        ret->setAnimationEnabled( ref->isAnimationEnabled() );
        ret->setHintToolTip( ref->getHintToolTip() );
        ret->setEvaluateOnChange( ref->getEvaluateOnChange() );
        ret->setIsPersistent(false);
        ret->setAddNewLine( ref->isNewLineActivated() );
        bool refSecret = ref->getIsSecret();

        if (refSecret) {
            ret->setSecret(true);
        }
    } // createKnob

    void addTableRow(const NodePtr & node);

    void removeRow(int index);

    void getInstanceSpecificKnobs(const Node* node,
                                  std::list<KnobIPtr>* knobs) const
    {
        const std::vector<KnobIPtr> & instanceKnobs = node->getKnobs();

        for (U32 i = 0; i < instanceKnobs.size(); ++i) {
            KnobInt* isInt = dynamic_cast<KnobInt*>( instanceKnobs[i].get() );
            KnobBool* isBool = dynamic_cast<KnobBool*>( instanceKnobs[i].get() );
            KnobDouble* isDouble = dynamic_cast<KnobDouble*>( instanceKnobs[i].get() );
            KnobColor* isColor = dynamic_cast<KnobColor*>( instanceKnobs[i].get() );
            KnobString* isString = dynamic_cast<KnobString*>( instanceKnobs[i].get() );

            if ( instanceKnobs[i]->isInstanceSpecific() ) {
                if (!isInt && !isBool && !isDouble && !isColor && !isString) {
                    qDebug() << "Multi-instance panel doesn't support the following type of knob: " << instanceKnobs[i]->typeName().c_str();
                    continue;
                }

                knobs->push_back(instanceKnobs[i]);
            }
        }
    }

    void getNodesFromSelection(const QModelIndexList & indexes, std::list<std::pair<Node*, bool> >* nodes);

    void pushUndoCommand(QUndoCommand* cmd)
    {
        mainInstance.lock()->getSettingPanel()->pushUndoCommand(cmd);
    }

    NodePtr getInstanceFromItem(TableItem* item) const;
};

MultiInstancePanel::MultiInstancePanel(const NodeGuiPtr & node)
    : NamedKnobHolder( node->getNode()->getApp() )
    , _imp( new MultiInstancePanelPrivate(this, node) )
{
}

MultiInstancePanel::~MultiInstancePanel()
{
}

void
MultiInstancePanel::setRedrawOnSelectionChanged(bool redraw)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->redrawOnSelectionChanged = redraw;
}

////////////// TableView delegate

class TableItemDelegate
    : public QStyledItemDelegate
{
    TableView* _view;
    MultiInstancePanel* _panel;

public:

    explicit TableItemDelegate(TableView* view,
                               MultiInstancePanel* panel);

private:

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

TableItemDelegate::TableItemDelegate(TableView* view,
                                     MultiInstancePanel* panel)
    : QStyledItemDelegate(view)
    , _view(view)
    , _panel(panel)
{
}

void
TableItemDelegate::paint(QPainter * painter,
                         const QStyleOptionViewItem & option,
                         const QModelIndex & index) const
{
    assert( index.isValid() );
    if ( !index.isValid() ) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    TableModel* model = dynamic_cast<TableModel*>( _view->model() );
    assert(model);
    if (!model) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }
    TableItem* item = model->item(index);
    assert(item);
    if (!item) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    int dim;
    AnimationLevelEnum level = eAnimationLevelNone;
    KnobIPtr knob = _panel->getKnobForItem(item, &dim);
    if (knob) {
        level = knob->getAnimationLevel(dim);
    }

    bool fillRect = true;
    QBrush brush;
    if (option.state & QStyle::State_Selected) {
        brush = option.palette.highlight();
    } else if (level == eAnimationLevelInterpolatedValue) {
        double r, g, b;
        appPTR->getCurrentSettings()->getInterpolatedColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else if (level == eAnimationLevelOnKeyframe) {
        double r, g, b;
        appPTR->getCurrentSettings()->getKeyframeColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else {
        fillRect = false;
    }
    if (fillRect) {
        painter->fillRect( geom, brush);
    }

    QPen pen = painter->pen();
    if ( !item->flags().testFlag(Qt::ItemIsEditable) ) {
        pen.setColor(Qt::black);
    } else {
        double r, g, b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        pen.setColor(col);
    }
    painter->setPen(pen);

    QRect textRect( geom.x() + 5, geom.y(), geom.width() - 5, geom.height() );
    QRect r;
    QString data;
    QVariant var = item->data(Qt::DisplayRole);
    if ( var.canConvert(QVariant::String) ) {
        data = var.toString();
    } else if ( var.canConvert(QVariant::Double) ) {
        double d = var.toDouble();
        data = QString::number(d);
    } else if ( var.canConvert(QVariant::Int) ) {
        int i = var.toInt();
        data = QString::number(i);
    }

    painter->drawText(textRect, Qt::TextSingleLine, data, &r);
} // TableItemDelegate::paint

NodePtr
MultiInstancePanel::getMainInstance() const
{
    return _imp->getMainInstance();
}

NodeGuiPtr
MultiInstancePanel::getMainInstanceGui() const
{
    return _imp->mainInstance.lock();
}

Gui*
MultiInstancePanel::getGui() const
{
    return _imp->mainInstance.lock()->getDagGui()->getGui();
}

std::string
MultiInstancePanel::getScriptName_mt_safe() const
{
    return _imp->getMainInstance()->getScriptName_mt_safe();
}

std::string
MultiInstancePanel::getFullyQualifiedName() const
{
    return _imp->getMainInstance()->getFullyQualifiedName();
}

void
MultiInstancePanel::initializeKnobs()
{
    const std::vector<KnobIPtr> & mainInstanceKnobs = _imp->getMainInstance()->getKnobs();

    for (U32 i = 0; i < mainInstanceKnobs.size(); ++i) {
        _imp->createKnob(mainInstanceKnobs[i]);
    }
    ///copy page children
    const std::vector<KnobIPtr> & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobPage* isPage = dynamic_cast<KnobPage*>( knobs[i].get() );

        if (isPage) {
            ///find the corresponding knob in the main instance knobs
            KnobIPtr other = _imp->getMainInstance()->getKnobByName( isPage->getName() );
            assert(other);
            if (!other) {
                throw std::logic_error("MultiInstancePanel::initializeKnobs");
            }
            KnobPage* otherPage = dynamic_cast<KnobPage*>( other.get() );
            assert(otherPage);

            if (!otherPage) {
                continue;
            }
            KnobsVec otherChildren = otherPage->getChildren();
            bool isNodePage = otherPage->getName() == "Node";
            for (U32 j = 0; j < otherChildren.size(); ++j) {
                if ( !otherChildren[j]->isInstanceSpecific() ) {
                    KnobIPtr thisChild = getKnobByName( otherChildren[j]->getName() );
                    if (thisChild) {
                        isPage->addKnob(thisChild);
                        if ( isNodePage && !thisChild->isDeclaredByPlugin() ) {
                            thisChild->setAllDimensionsEnabled(false);
                        }
                    }
                }
            }
        }
    }
    initializeExtraKnobs();
}

bool
MultiInstancePanel::isGuiCreated() const
{
    return _imp->guiCreated;
}

void
MultiInstancePanel::createMultiInstanceGui(QVBoxLayout* layout)
{
    appendExtraGui(layout);
    layout->addSpacing(20);

    std::list<KnobIPtr> instanceSpecificKnobs;
    _imp->getInstanceSpecificKnobs(_imp->getMainInstance().get(), &instanceSpecificKnobs);

    _imp->view = new TableView( layout->parentWidget() );
    QObject::connect( _imp->view, SIGNAL(deleteKeyPressed()), this, SLOT(onDeleteKeyPressed()) );
    QObject::connect( _imp->view, SIGNAL(itemRightClicked(TableItem*)), this, SLOT(onItemRightClicked(TableItem*)) );
    TableItemDelegate* delegate = new TableItemDelegate(_imp->view, this);
    _imp->view->setItemDelegate(delegate);

    _imp->model = new TableModel(0, 0, _imp->view);
    QObject::connect( _imp->model, SIGNAL(s_itemChanged(TableItem*)), this, SLOT(onItemDataChanged(TableItem*)) );
    _imp->view->setTableModel(_imp->model);

    QItemSelectionModel *selectionModel = _imp->view->selectionModel();
    QObject::connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
                      SLOT(onSelectionChanged(QItemSelection,QItemSelection)) );
    QStringList dimensionNames;
    for (std::list<KnobIPtr>::iterator it = instanceSpecificKnobs.begin(); it != instanceSpecificKnobs.end(); ++it) {
        QString knobDesc( QString::fromUtf8( (*it)->getLabel().c_str() ) );
        int dims = (*it)->getDimension();
        for (int i = 0; i < dims; ++i) {
            QString dimName(knobDesc);
            if (dims > 1) {
                dimName += QLatin1Char(' ');
                dimName += QString::fromUtf8( (*it)->getDimensionName(i).c_str() );
            }
            dimensionNames.push_back(dimName);
        }
    }

    dimensionNames.prepend( QString::fromUtf8("Script-name") );
    dimensionNames.prepend( QString::fromUtf8("Enabled") );

    _imp->view->setColumnCount( dimensionNames.size() );
    _imp->view->setHorizontalHeaderLabels(dimensionNames);

    _imp->view->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _imp->view->setUniformRowHeights(true);

    _imp->view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _imp->view->header()->setStretchLastSection(true);


    layout->addWidget(_imp->view);

    _imp->buttonsContainer = new QWidget( layout->parentWidget() );
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->setContentsMargins(0, 0, 0, 0);
    _imp->addButton = new Button(QIcon(), QString::fromUtf8("+"), _imp->buttonsContainer);
    _imp->addButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _imp->addButton->setIconSize( QSize(NATRON_SMALL_BUTTON_ICON_SIZE, NATRON_SMALL_BUTTON_ICON_SIZE) );
    _imp->addButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add new."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonsLayout->addWidget(_imp->addButton);

    QObject::connect( _imp->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddButtonClicked()) );

    _imp->removeButton = new Button(QIcon(), QString::fromUtf8("-"), _imp->buttonsContainer);
    _imp->removeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove selection."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->removeButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _imp->removeButton->setIconSize( QSize(NATRON_SMALL_BUTTON_ICON_SIZE, NATRON_SMALL_BUTTON_ICON_SIZE) );
    _imp->buttonsLayout->addWidget(_imp->removeButton);

    QObject::connect( _imp->removeButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveButtonClicked()) );
    QPixmap selectAll;
    appPTR->getIcon(NATRON_PIXMAP_SELECT_ALL, NATRON_SMALL_BUTTON_ICON_SIZE, &selectAll);
    _imp->selectAll = new Button(QIcon(selectAll), QString(), _imp->buttonsContainer);
    _imp->selectAll->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _imp->selectAll->setIconSize( QSize(NATRON_SMALL_BUTTON_ICON_SIZE, NATRON_SMALL_BUTTON_ICON_SIZE) );
    _imp->selectAll->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Select all."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonsLayout->addWidget(_imp->selectAll);

    QObject::connect( _imp->selectAll, SIGNAL(clicked(bool)), this, SLOT(onSelectAllButtonClicked()) );

    _imp->resetTracksButton = new Button(QString::fromUtf8("Reset"), _imp->buttonsContainer);
    QObject::connect( _imp->resetTracksButton, SIGNAL(clicked(bool)), this, SLOT(resetSelectedInstances()) );
    _imp->buttonsLayout->addWidget(_imp->resetTracksButton);
    _imp->resetTracksButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Reset selected items."), NATRON_NAMESPACE::WhiteSpaceNormal) );

    layout->addWidget(_imp->buttonsContainer);
    appendButtons(_imp->buttonsLayout);
    _imp->buttonsLayout->addStretch();

    ///Deactivate the main-instance since this is more convenient this way for the user.
    //_imp->getMainInstance()->deactivate(std::list<Node* >(),false,false,false,false);
    _imp->guiCreated = true;
} // createMultiInstanceGui

class AddNodeCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddNodeCommand)

private:
    bool _firstRedoCalled;
    NodePtr _node;
    MultiInstancePanel* _panel;

public:

    AddNodeCommand(MultiInstancePanel* panel,
                   const NodePtr & node,
                   QUndoCommand* parent = 0)
        : QUndoCommand(parent)
        , _firstRedoCalled(false)
        , _node(node)
        , _panel(panel)
    {
    }

    virtual ~AddNodeCommand()
    {
    }

    virtual void undo() OVERRIDE FINAL
    {
        int index = _panel->getNodeIndex(_node);

        assert(index != -1);
        _panel->removeRow(index);
        _node->deactivate();
        _panel->getMainInstance()->getApp()->redrawAllViewers();
        setText( tr("Add %1").arg( QString::fromUtf8( _node->getLabel().c_str() ) ) );
    }

    virtual void redo() OVERRIDE FINAL
    {
        if (_firstRedoCalled) {
            _node->activate();
            _panel->addRow(_node);
            _panel->getMainInstance()->getApp()->redrawAllViewers();
        }
        _firstRedoCalled = true;
        setText( tr("Add %1").arg( QString::fromUtf8( _node->getLabel().c_str() ) ) );
    }
};

NodePtr
MultiInstancePanel::createNewInstance(bool useUndoRedoStack)
{
    return addInstanceInternal(useUndoRedoStack);
}

void
MultiInstancePanel::onAddButtonClicked()
{
    ignore_result( addInstanceInternal(true) );
}

NodePtr
MultiInstancePanel::addInstanceInternal(bool useUndoRedoStack)
{
    NodePtr mainInstance = _imp->getMainInstance();
    CreateNodeArgs args(mainInstance->getPluginID() ,mainInstance->getGroup() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName, mainInstance->getScriptName());
    NodePtr newInstance = _imp->getMainInstance()->getApp()->createNode(args);

    if (useUndoRedoStack) {
        _imp->pushUndoCommand( new AddNodeCommand(this, newInstance) );
    }

    return newInstance;
}

void
MultiInstancePanel::onChildCreated(const NodePtr& node)
{
    _imp->addTableRow(node);
}

const std::list<std::pair<NodeWPtr, bool> > &
MultiInstancePanel::getInstances() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->instances;
}

void
MultiInstancePanel::addRow(const NodePtr & node)
{
    _imp->addTableRow(node);
}

void
MultiInstancePanelPrivate::addTableRow(const NodePtr & node)
{
    for (Nodes::iterator it = instances.begin(); it != instances.end(); ++it) {
        if (it->first.lock() == node) {
            return;
        }
    }

    instances.push_back( std::make_pair(node, false) );
    int newRowIndex = view->rowCount();
    model->insertRow(newRowIndex);

    std::list<KnobIPtr> instanceSpecificKnobs;
    {
        const std::vector<KnobIPtr> & instanceKnobs = node->getKnobs();
        for (U32 i = 0; i < instanceKnobs.size(); ++i) {
            KnobSignalSlotHandlerPtr slotsHandler =
                instanceKnobs[i]->getSignalSlotHandler();
            if (slotsHandler) {
                QObject::connect( slotsHandler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), publicInterface, SLOT(onInstanceKnobValueChanged(ViewSpec,int,int)) );
            }

            if ( instanceKnobs[i]->isInstanceSpecific() ) {
                KnobInt* isInt = dynamic_cast<KnobInt*>( instanceKnobs[i].get() );
                KnobBool* isBool = dynamic_cast<KnobBool*>( instanceKnobs[i].get() );
                KnobDouble* isDouble = dynamic_cast<KnobDouble*>( instanceKnobs[i].get() );
                KnobColor* isColor = dynamic_cast<KnobColor*>( instanceKnobs[i].get() );
                KnobString* isString = dynamic_cast<KnobString*>( instanceKnobs[i].get() );
                if (!isInt && !isBool && !isDouble && !isColor && !isString) {
                    qDebug() << "Multi-instance panel doesn't support the following type of knob: " << instanceKnobs[i]->typeName().c_str();
                    continue;
                }

                instanceSpecificKnobs.push_back(instanceKnobs[i]);
            }
        }
    }


    ///first add the enabled column
    {
        QCheckBox* checkbox = new QCheckBox();
        checkbox->setChecked( !node->isNodeDisabled() );
        QObject::connect( checkbox, SIGNAL(toggled(bool)), publicInterface, SLOT(onCheckBoxChecked(bool)) );
        view->setCellWidget(newRowIndex, COL_ENABLED, checkbox);
        TableItem* newItem = new TableItem;
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
        view->setItem(newRowIndex, COL_ENABLED, newItem);
        view->resizeColumnToContents(COL_ENABLED);
    }

    ///Script name
    {
        TableItem* newItem = new TableItem;
        view->setItem(newRowIndex, COL_SCRIPT_NAME, newItem);
        newItem->setToolTip( tr("The script-name of the item as exposed to Python scripts") );
        newItem->setText( QString::fromUtf8( node->getScriptName().c_str() ) );
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_ENABLED);
    }
    int columnIndex = COL_FIRST_KNOB;
    for (std::list<KnobIPtr>::iterator it = instanceSpecificKnobs.begin(); it != instanceSpecificKnobs.end(); ++it) {
        KnobInt* isInt = dynamic_cast<KnobInt*>( it->get() );
        KnobBool* isBool = dynamic_cast<KnobBool*>( it->get() );
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>( it->get() );
        KnobColor* isColor = dynamic_cast<KnobColor*>( it->get() );
        KnobString* isString = dynamic_cast<KnobString*>( it->get() );


        ///Only these types are supported
        if (!isInt && !isBool && !isDouble && !isColor && !isString) {
            continue;
        }


        QString help = QString::fromUtf8("<b>Script-name: %1 </b><br/>").arg( QString::fromUtf8( (*it)->getName().c_str() ) );
        help.append( QString::fromUtf8( (*it)->getHintToolTip().c_str() ) );

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
                newItem->setData( Qt::DisplayRole, isInt->getValue(i) );
            } else if (isDouble) {
                newItem->setData( Qt::DisplayRole, isDouble->getValue(i) );
            } else if (isString) {
                newItem->setData( Qt::DisplayRole, QString::fromUtf8( isString->getValue(i).c_str() ) );
            }
            newItem->setFlags(flags);
            newItem->setToolTip(help);
            view->setItem(newRowIndex, columnIndex, newItem);
            view->resizeColumnToContents(columnIndex);
            ++columnIndex;
        }
    }

    ///clear current selection
    //view->selectionModel()->clear();

    ///select the new item
    QModelIndex newIndex = model->index(newRowIndex, COL_ENABLED);
    assert( newIndex.isValid() );
    view->selectionModel()->select(newIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
} // addTableRow

void
MultiInstancePanel::selectNode(const NodePtr & node,
                               bool addToSelection)
{
    if (!addToSelection) {
        _imp->view->selectionModel()->clear();
    }

    int index = -1;
    int i = 0;
    for (std::list<std::pair<NodeWPtr, bool > >::iterator it = _imp->instances.begin(); it != _imp->instances.end(); ++it, ++i) {
        if (it->first.lock() == node) {
            index = i;
            break;
        }
    }
    assert(index != -1);

    QItemSelection newSelection( _imp->model->index(index, 0), _imp->model->index(index, _imp->view->columnCount() - 1) );
    _imp->view->selectionModel()->select(newSelection, QItemSelectionModel::Select);
}

void
MultiInstancePanel::removeNodeFromSelection(const NodePtr & node)
{
    int index = -1;
    int i = 0;

    for (std::list<std::pair<NodeWPtr, bool > >::iterator it = _imp->instances.begin(); it != _imp->instances.end(); ++it, ++i) {
        if (it->first.lock() == node) {
            index = i;
            break;
        }
    }
    assert(index != -1);
    QItemSelection newSelection( _imp->model->index(index, 0), _imp->model->index(index, _imp->view->columnCount() - 1) );
    _imp->view->selectionModel()->select(newSelection, QItemSelectionModel::Deselect);
}

void
MultiInstancePanel::clearSelection()
{
    _imp->view->selectionModel()->clear();
}

void
MultiInstancePanel::selectNodes(const std::list<Node*> & nodes,
                                bool addToSelection)
{
    //_imp->view->selectionModel()->blockSignals(true);
    if (!addToSelection) {
        _imp->view->clearSelection();
    }
    //    for (std::list<std::pair<NodePtr,bool > >::iterator it2 = _imp->instances.begin();
    //         it2!=_imp->instances.end(); ++it2) {
    //        it2->second = false;
    //    }
    //    _imp->view->selectionModel()->blockSignals(false);
    if ( nodes.empty() ) {
        return;
    }


    QItemSelection newSelection;
    for (std::list<Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        int i = 0;
        for (std::list<std::pair<NodeWPtr, bool > >::iterator it2 = _imp->instances.begin();
             it2 != _imp->instances.end(); ++it2, ++i) {
            if (it2->first.lock().get() == *it) {
                QItemSelection sel( _imp->model->index(i, 0), _imp->model->index(i, _imp->view->columnCount() - 1) );
                newSelection.merge(sel, QItemSelectionModel::Select);
                break;
            }
        }
    }
    _imp->view->selectionModel()->select(newSelection, QItemSelectionModel::Select);
}

class RemoveNodeCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveNodeCommand)

private:
    MultiInstancePanel* _panel;
    std::list<NodePtr> _nodes;

public:

    RemoveNodeCommand(MultiInstancePanel* panel,
                      const std::list<NodePtr> & nodes,
                      QUndoCommand* parent = 0)
        : QUndoCommand(parent)
        , _panel(panel)
        , _nodes(nodes)
    {
        setText( tr("Remove instance(s)") );
    }

    virtual ~RemoveNodeCommand()
    {
    }

    virtual void undo() OVERRIDE FINAL
    {
        _panel->setRedrawOnSelectionChanged(false);
        _panel->addInstances(_nodes);
        _panel->setRedrawOnSelectionChanged(true);
        _panel->getMainInstance()->getApp()->triggerAutoSave();
        _panel->getMainInstance()->getApp()->redrawAllViewers();
    }

    virtual void redo() OVERRIDE FINAL
    {
        _panel->setRedrawOnSelectionChanged(false);
        _panel->removeInstances(_nodes);
        _panel->setRedrawOnSelectionChanged(true);
        _panel->getMainInstance()->getApp()->triggerAutoSave();
        _panel->getMainInstance()->getApp()->redrawAllViewers();
    }
};

void
MultiInstancePanel::removeInstances(const std::list<NodePtr>& instances)
{
    NodePtr mainInstance = getMainInstance();
    std::list<NodePtr>::const_iterator next = instances.begin();

    if ( next != instances.end() ) {
        ++next;
    }

    for (std::list<NodePtr>::const_iterator it = instances.begin();
         it != instances.end();
         ++it) {
        int index = getNodeIndex(*it);
        assert(index != -1);
        removeRow(index);
        bool isMainInstance = (*it) == mainInstance;
        (*it)->deactivate( std::list<NodePtr>(), false, false, !isMainInstance, next == instances.end() );

        // increment for next iteration
        if ( next != instances.end() ) {
            ++next;
        }
    } // for(it)
}

void
MultiInstancePanel::addInstances(const std::list<NodePtr>& instances)
{
    std::list<NodePtr>::const_iterator next = instances.begin();

    if ( next != instances.end() ) {
        ++next;
    }
    for (std::list<NodePtr>::const_iterator it = instances.begin();
         it != instances.end();
         ++it) {
        addRow(*it);
        (*it)->activate( std::list<NodePtr>(), false, next == instances.end() );

        // increment for next iteration
        if ( next != instances.end() ) {
            ++next;
        }
    } // for(it)
}

void
MultiInstancePanel::removeRow(int index)
{
    _imp->removeRow(index);
}

void
MultiInstancePanelPrivate::removeRow(int index)
{
    if ( (index < 0) || ( index >= (int)instances.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }
    model->removeRows(index);
    Nodes::iterator it = instances.begin();
    std::advance(it, index);
    instances.erase(it);
}

int
MultiInstancePanel::getNodeIndex(const NodePtr & node) const
{
    int i = 0;
    Nodes::iterator it = _imp->instances.begin();

    for (; it != _imp->instances.end(); ++it, ++i) {
        if (it->first.lock() == node) {
            return i;
        }
    }

    return -1;
}

void
MultiInstancePanel::onDeleteKeyPressed()
{
    removeInstancesInternal();
}

void
MultiInstancePanel::onRemoveButtonClicked()
{
    removeInstancesInternal();
}

void
MultiInstancePanel::removeInstancesInternal()
{
    const QItemSelection selection = _imp->view->selectionModel()->selection();
    std::list<NodePtr> instances;
    QModelIndexList indexes = selection.indexes();
    std::set<int> rows;

    for (int i = 0; i < indexes.size(); ++i) {
        rows.insert( indexes[i].row() );
    }

    for (std::set<int>::iterator it = rows.begin(); it != rows.end(); ++it) {
        assert( *it >= 0 && *it < (int)_imp->instances.size() );
        std::list<std::pair<NodeWPtr, bool > >::iterator it2 = _imp->instances.begin();
        std::advance(it2, *it);
        instances.push_back( it2->first.lock() );
    }
    _imp->pushUndoCommand( new RemoveNodeCommand(this, instances) );
}

void
MultiInstancePanel::onSelectAllButtonClicked()
{
    QItemSelectionModel* selectModel = _imp->view->selectionModel();
    QItemSelection sel;

    assert(selectModel);
    int rc = _imp->model->rowCount();
    int cc = _imp->model->columnCount();

    for (int i = 0; i < rc; ++i) {
        assert( i < (int)_imp->instances.size() );
        Nodes::iterator it = _imp->instances.begin();
        std::advance(it, i);
        bool disabled = it->first.lock()->isNodeDisabled();
        if (disabled) {
            continue;
        }

        QItemSelectionRange r( _imp->model->index(i, 0), _imp->model->index(i, cc - 1) );
        sel.append(r);
    }
    selectModel->select(sel, QItemSelectionModel::ClearAndSelect);
}

bool
MultiInstancePanel::isSettingsPanelVisible() const
{
    NodeSettingsPanel* panel = _imp->mainInstance.lock()->getSettingPanel();

    assert(panel);

    return !panel->isClosed();
}

void
MultiInstancePanel::onSettingsPanelClosed(bool closed)
{
    std::list<Node*> selection;

    getSelectedInstances(&selection);

    std::list<Node*>::iterator next = selection.begin();
    if ( next != selection.end() ) {
        ++next;
    }
    for (std::list<Node*>::iterator it = selection.begin();
         it != selection.end();
         ++it) {
        if (closed) {
            (*it)->hideKeyframesFromTimeline( next == selection.end() );
        } else {
            (*it)->showKeyframesOnTimeline( next == selection.end() );
        }

        // increment for next iteration
        if ( next != selection.end() ) {
            ++next;
        }
    } // for(it)
}

void
MultiInstancePanel::onSelectionChanged(const QItemSelection & newSelection,
                                       const QItemSelection & oldSelection)
{
    std::list<std::pair<Node*, bool> > previouslySelectedInstances;
    QModelIndexList oldIndexes = oldSelection.indexes();

    _imp->getNodesFromSelection(oldIndexes, &previouslySelectedInstances);

    bool copyOnUnSlave = previouslySelectedInstances.size()  <= 1;

    /// new selection
    std::list<std::pair<Node*, bool> > newlySelectedInstances;
    QModelIndexList newIndexes = newSelection.indexes();
    _imp->getNodesFromSelection(newIndexes, &newlySelectedInstances);

    ///Don't consider items that are in both previouslySelectedInstances && newlySelectedInstances

    QModelIndexList rows = _imp->view->selectionModel()->selectedRows();
    bool setDirty = rows.count() > 1;
    std::list<std::pair<Node*, bool> >::iterator nextPreviouslySelected = previouslySelectedInstances.begin();
    if ( nextPreviouslySelected != previouslySelectedInstances.end() ) {
        ++nextPreviouslySelected;
    }


    for (std::list<std::pair<Node*, bool> >::iterator it = previouslySelectedInstances.begin();
         it != previouslySelectedInstances.end(); ++it) {
        ///if the item is in the new selection, don't consider it
        bool skip = false;
        for (std::list<std::pair<Node*, bool> >::iterator it2 = newlySelectedInstances.begin();
             it2 != newlySelectedInstances.end(); ++it2) {
            if (it2->first == it->first) {
                skip = true;
                break;
            }
        } // for(it2)
          ///disconnect all the knobs
        if (!it->second || skip) {
            continue;
        }

        it->first->hideKeyframesFromTimeline( nextPreviouslySelected == previouslySelectedInstances.end() );

        it->first->getEffectInstance()->beginChanges();
        const std::vector<KnobIPtr> & knobs = it->first->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if ( knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific() && !knobs[i]->getIsSecret() ) {
                for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                    if ( knobs[i]->isSlave(j) ) {
                        knobs[i]->blockValueChanges();
                        knobs[i]->unSlave(j, copyOnUnSlave);
                        knobs[i]->unblockValueChanges();
                    }
                }
            }
        } // for(i)
        it->first->getEffectInstance()->endChanges();

        for (Nodes::iterator it2 = _imp->instances.begin(); it2 != _imp->instances.end(); ++it2) {
            if (it2->first.lock().get() == it->first) {
                it2->second = false;
                break;
            }
        } // for(it2)

        // increment for next iteration
        if ( nextPreviouslySelected != previouslySelectedInstances.end() ) {
            ++nextPreviouslySelected;
        }
    } // for(it)
    std::list<SequenceTime> allKeysToAdd;
    std::list<std::pair<Node*, bool> >::iterator nextNewlySelected = newlySelectedInstances.begin();
    if ( nextNewlySelected != newlySelectedInstances.end() ) {
        ++nextNewlySelected;
    }
    for (std::list<std::pair<Node*, bool> >::iterator it = newlySelectedInstances.begin();
         it != newlySelectedInstances.end(); ++it) {
        ///if the item is in the old selection, don't consider it
        bool skip = false;
        for (std::list<std::pair<Node*, bool> >::iterator it2 = previouslySelectedInstances.begin();
             it2 != previouslySelectedInstances.end(); ++it2) {
            if (it2->first == it->first) {
                skip = true;
                break;
            }
        }

        if (it->second || skip) {
            continue;
        }

        if ( isSettingsPanelVisible() ) {
            it->first->showKeyframesOnTimeline( nextNewlySelected == newlySelectedInstances.end() );
        }

        ///slave all the knobs that are declared by the plug-in (i.e: not the ones from the "Node" page)
        //and which are not instance specific (not the knob displayed in the table)
        const std::vector<KnobIPtr> & knobs = it->first->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if ( knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific() && !knobs[i]->getIsSecret() ) {
                KnobIPtr otherKnob = getKnobByName( knobs[i]->getName() );
                assert(otherKnob);

                ///Don't slave knobs when several are selected otherwise all the instances would then share the same values
                ///while being selected
                if (!setDirty) {
                    ///do not slave buttons, handle them separately in onButtonTriggered()
                    KnobButton* isButton = dynamic_cast<KnobButton*>( knobs[i].get() );
                    if (!isButton) {
                        otherKnob->clone(knobs[i]);
                        knobs[i]->beginChanges();
                        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                            knobs[i]->slaveTo(j, otherKnob, j, true);
                        }
                        knobs[i]->endChanges();
                    }
                }

                otherKnob->setAllDimensionsEnabled(true);
                otherKnob->setDirty(setDirty);
            }
        }
        for (Nodes::iterator it2 = _imp->instances.begin(); it2 != _imp->instances.end(); ++it2) {
            if (it2->first.lock().get() == it->first) {
                it2->second = true;
                break;
            }
        }

        if ( nextNewlySelected != newlySelectedInstances.end() ) {
            ++nextNewlySelected;
        }
    }


    if ( newlySelectedInstances.empty() ) {
        ///disable knobs
        const std::vector<KnobIPtr> & knobs = getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            if ( knobs[i]->isDeclaredByPlugin() && !knobs[i]->isInstanceSpecific() ) {
                knobs[i]->setAllDimensionsEnabled(false);
                knobs[i]->setDirty(false);
            }
        }
    }

    if (_imp->redrawOnSelectionChanged) {
        getGui()->redrawAllViewers();
    }
} // onSelectionChanged

void
MultiInstancePanelPrivate::getNodesFromSelection(const QModelIndexList & indexes,
                                                 std::list<std::pair<Node*, bool> >* nodes)
{
    std::set<int> rows;

    for (int i = 0; i < indexes.size(); ++i) {
        rows.insert( indexes[i].row() );
    }

    for (std::set<int>::iterator it = rows.begin(); it != rows.end(); ++it) {
        assert( *it >= 0 && *it < (int)instances.size() );
        std::list<std::pair<NodeWPtr, bool > >::iterator it2 = instances.begin();
        std::advance(it2, *it);
        NodePtr node = it2->first.lock();
        if ( !node->isNodeDisabled() ) {
            nodes->push_back( std::make_pair(node.get(), it2->second) );
        }
    }
}

NodePtr
MultiInstancePanelPrivate::getInstanceFromItem(TableItem* item) const
{
    assert( item->row() >= 0 && item->row() < (int)instances.size() );
    int i = 0;
    for (std::list<std::pair<NodeWPtr, bool > >::const_iterator it = instances.begin(); it != instances.end(); ++it, ++i) {
        if ( i == item->row() ) {
            return it->first.lock();
        }
    }

    return NodePtr();
}

KnobIPtr
MultiInstancePanel::getKnobForItem(TableItem* item,
                                   int* dimension) const
{
    QModelIndex modelIndex = _imp->model->index(item);

    assert( modelIndex.row() < (int)_imp->instances.size() );
    Nodes::iterator nIt = _imp->instances.begin();
    std::advance( nIt, modelIndex.row() );
    const std::vector<KnobIPtr> & knobs = nIt->first.lock()->getKnobs();
    int instanceSpecificIndex = COL_FIRST_KNOB;
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->isInstanceSpecific() ) {
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                if ( instanceSpecificIndex == modelIndex.column() ) {
                    *dimension = j;

                    return knobs[i];
                }
                ++instanceSpecificIndex;
            }
        }
    }
    *dimension = -1;

    return KnobIPtr();
}

void
MultiInstancePanel::onItemDataChanged(TableItem* item)
{
    if (_imp->executingKnobValueChanged) {
        return;
    }
    QVariant data = item->data(Qt::DisplayRole);
    QModelIndex modelIndex = _imp->model->index(item);

    ///The enabled cell is handled in onCheckBoxChecked
    if (modelIndex.column() == COL_ENABLED) {
        return;
    }

    double time = getApp()->getTimeLine()->currentFrame();

    assert( modelIndex.row() < (int)_imp->instances.size() );
    Nodes::iterator nIt = _imp->instances.begin();
    std::advance( nIt, modelIndex.row() );
    NodePtr node = nIt->first.lock();
    assert(node);
    const std::vector<KnobIPtr> & knobs = node->getKnobs();

    if (modelIndex.column() == COL_SCRIPT_NAME) {
        node->setLabel( data.toString().toStdString() );
    }

    int instanceSpecificIndex = COL_FIRST_KNOB;
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->isInstanceSpecific() ) {
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                if ( instanceSpecificIndex == modelIndex.column() ) {
                    KnobInt* isInt = dynamic_cast<KnobInt*>( knobs[i].get() );
                    KnobBool* isBool = dynamic_cast<KnobBool*>( knobs[i].get() );
                    KnobDouble* isDouble = dynamic_cast<KnobDouble*>( knobs[i].get() );
                    KnobColor* isColor = dynamic_cast<KnobColor*>( knobs[i].get() );
                    KnobString* isString = dynamic_cast<KnobString*>( knobs[i].get() );

                    if ( knobs[i]->isAnimationEnabled() && knobs[i]->isAnimated(j) ) {
                        if (isInt) {
                            isInt->setValueAtTime(time,    data.toInt(),    ViewSpec::all(), j);
                        } else if (isBool) {
                            isBool->setValueAtTime(time,   data.toBool(),   ViewSpec::all(), j);
                        } else if (isDouble) {
                            isDouble->setValueAtTime(time, data.toDouble(), ViewSpec::all(), j);
                        } else if (isColor) {
                            isColor->setValueAtTime(time,  data.toDouble(), ViewSpec::all(), j);
                        } else if (isString) {
                            isString->setValueAtTime(time, data.toString().toStdString(), ViewSpec::all(), j);
                        }
                    } else {
                        if (isInt) {
                            isInt->setValue(data.toInt(),       ViewSpec::all(), j, true);
                        } else if (isBool) {
                            isBool->setValue(data.toBool(),     ViewSpec::all(), j, true);
                        } else if (isDouble) {
                            isDouble->setValue(data.toDouble(), ViewSpec::all(), j, true);
                        } else if (isColor) {
                            isColor->setValue(data.toDouble(),  ViewSpec::all(), j, true);
                        } else if (isString) {
                            isString->setValue(data.toString().toStdString(), ViewSpec::all(), j, true);
                        }
                    }

                    return;
                }
                ++instanceSpecificIndex;
            }
        }
    }
} // MultiInstancePanel::onItemDataChanged

void
MultiInstancePanel::onItemRightClicked(TableItem* item)
{
    NodePtr instance = _imp->getInstanceFromItem(item);

    if (instance) {
        showMenuForInstance( instance.get() );
    }
}

///The checkbox interacts directly with the kDisableNodeKnobName knob of the node
///It doesn't call deactivate() on the node so calling isActivated() on a node
///will still return true even if you set the value of kDisableNodeKnobName to false.
void
MultiInstancePanel::onCheckBoxChecked(bool checked)
{
    QCheckBox* checkbox = qobject_cast<QCheckBox*>( sender() );

    if (!checkbox) {
        return;
    }


    ///find the row which owns this checkbox
    int rc = _imp->model->rowCount();
    int cc = _imp->model->columnCount();
    for (int i = 0; i < rc; ++i) {
        //TableItem* item = _imp->view->itemAt(i, COL_ENABLED);
        QWidget* w = _imp->view->cellWidget(i, COL_ENABLED);
        if (w == checkbox) {
            //assert(item);
            assert( i < (int)_imp->instances.size() );
            Nodes::iterator it = _imp->instances.begin();
            std::advance(it, i);
            KnobIPtr enabledKnob = it->first.lock()->getKnobByName(kDisableNodeKnobName);
            assert(enabledKnob);
            KnobBool* bKnob = dynamic_cast<KnobBool*>( enabledKnob.get() );
            assert(bKnob);
            bKnob->setValue(!checked);
            QItemSelection sel;
            QItemSelectionRange r( _imp->model->index(i, 0), _imp->model->index(i, cc - 1 ) );
            sel.append(r);

            if (!checked) {
                _imp->view->selectionModel()->select(sel, QItemSelectionModel::Clear);
            } else {
                _imp->view->selectionModel()->select(sel, QItemSelectionModel::Select);
            }
            break;
        }
    }
    getApp()->redrawAllViewers();
}

void
MultiInstancePanel::onInstanceKnobValueChanged(ViewSpec /*view*/,
                                               int dim,
                                               int reason)
{
    if ( (ValueChangedReasonEnum)reason == eValueChangedReasonSlaveRefresh ) {
        return;
    }

    KnobSignalSlotHandler* signalEmitter = qobject_cast<KnobSignalSlotHandler*>( sender() );
    if (!signalEmitter) {
        return;
    }
    KnobIPtr knob = signalEmitter->getKnob();
    if ( !knob->isDeclaredByPlugin() ) {
        return;
    }
    KnobHolder* holder = knob->getHolder();
    assert(holder);
    int rowIndex = 0;
    int colIndex = COL_FIRST_KNOB;
    for (Nodes::iterator it = _imp->instances.begin(); it != _imp->instances.end(); ++it, ++rowIndex) {
        NodePtr node = it->first.lock();
        if ( holder == node->getEffectInstance().get() ) {
            const std::vector<KnobIPtr> & knobs = node->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if ( knobs[i]->isInstanceSpecific() ) {
                    if (knobs[i] == knob) {
                        for (int k = 0; k < knob->getDimension(); ++k) {
                            if ( (k == dim) || (dim == -1) ) {
                                TableItem* item = _imp->model->item(rowIndex, colIndex);
                                if (item) {
                                    QVariant data;
                                    KnobInt* isInt = dynamic_cast<KnobInt*>( knobs[i].get() );
                                    KnobBool* isBool = dynamic_cast<KnobBool*>( knobs[i].get() );
                                    KnobDouble* isDouble = dynamic_cast<KnobDouble*>( knobs[i].get() );
                                    KnobColor* isColor = dynamic_cast<KnobColor*>( knobs[i].get() );
                                    KnobString* isString = dynamic_cast<KnobString*>( knobs[i].get() );
                                    if (isInt) {
                                        data.setValue<int>( isInt->getValue(k) );
                                    } else if (isBool) {
                                        data.setValue<bool>( isBool->getValue(k) );
                                    } else if (isDouble) {
                                        data.setValue<double>( isDouble->getValue(k) );
                                    } else if (isColor) {
                                        data.setValue<double>( isColor->getValue(k) );
                                    } else if (isString) {
                                        data.setValue<QString>( QString::fromUtf8( isString->getValue(k).c_str() ) );
                                    }
                                    _imp->executingKnobValueChanged = true;
                                    item->setData(Qt::DisplayRole, data);
                                    _imp->executingKnobValueChanged = false;
                                }
                                if (dim != -1) {
                                    colIndex += dim;
                                } else {
                                    ++colIndex;
                                }
                            }
                        }

                        return;
                    }
                    colIndex += knobs[i]->getDimension();
                } else if ( (knobs[i] == knob) && !_imp->knobValueRecursion ) {
                    ///If the knob is slaved to a knob used only for GUI, unslave it before updating value and reslave back

                    for (int k = 0; k < knob->getDimension(); ++k) {
                        if ( (k == dim) || (dim == 1) ) {
                            std::pair<int, KnobIPtr> master = knob->getMaster(k);
                            if (master.second) {
                                ++_imp->knobValueRecursion;
                                knob->unSlave(k, false);
                                KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( knob.get() );
                                KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( knob.get() );
                                KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( knob.get() );
                                KnobStringBase* isString = dynamic_cast<KnobStringBase*>( knob.get() );
                                if (isInt) {
                                    KnobIntBase* masterKnob = dynamic_cast<KnobIntBase*>( master.second.get() );
                                    assert(masterKnob);
                                    if (masterKnob) {
                                        masterKnob->clone( knob.get() );
                                    }
                                } else if (isBool) {
                                    KnobBoolBase* masterKnob = dynamic_cast<KnobBoolBase*>( master.second.get() );
                                    assert(masterKnob);
                                    if (masterKnob) {
                                        masterKnob->clone( knob.get() );
                                    }
                                } else if (isDouble) {
                                    KnobDoubleBase* masterKnob = dynamic_cast<KnobDoubleBase*>( master.second.get() );
                                    assert(masterKnob);
                                    if (masterKnob) {
                                        masterKnob->clone( knob.get() );
                                    }
                                } else if (isString) {
                                    KnobStringBase* masterKnob = dynamic_cast<KnobStringBase*>( master.second.get() );
                                    assert(masterKnob);
                                    if (masterKnob) {
                                        masterKnob->clone( knob.get() );
                                    }
                                }
                                knob->slaveTo(k, master.second, master.first, true);
                                --_imp->knobValueRecursion;
                            }
                        }
                    }
                }
            }

            return;
        }
    }
} // onInstanceKnobValueChanged

void
MultiInstancePanel::getSelectedInstances(std::list<Node*>* instances) const
{
    const QItemSelection selection = _imp->view->selectionModel()->selection();
    QModelIndexList indexes = selection.indexes();
    std::set<int> rows;

    for (int i = 0; i < indexes.size(); ++i) {
        rows.insert( indexes[i].row() );
    }

    for (std::set<int>::iterator it = rows.begin(); it != rows.end(); ++it) {
        assert( *it >= 0 && *it < (int)_imp->instances.size() );
        std::list<std::pair<NodeWPtr, bool > >::iterator it2 = _imp->instances.begin();
        std::advance(it2, *it);
        instances->push_back( it2->first.lock().get() );
    }
}

void
MultiInstancePanel::resetSelectedInstances()
{
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);
    _imp->view->selectionModel()->clear();
    resetInstances(selectedInstances);
}

void
MultiInstancePanel::resetAllInstances()
{
    _imp->view->selectionModel()->clear();
    std::list<Node*> all;
    for (Nodes::iterator it = _imp->instances.begin(); it != _imp->instances.end(); ++it) {
        all.push_back( it->first.lock().get() );
    }
    resetInstances(all);
}

void
MultiInstancePanel::resetInstances(const std::list<Node*> & instances)
{
    if ( instances.empty() ) {
        return;
    }

    std::list<Node*>::const_iterator next = instances.begin();
    if ( next != instances.end() ) {
        ++next;
    }
    for (std::list<Node*>::const_iterator it = instances.begin();
         it != instances.end();
         ++it) {
        //invalidate the cache by incrementing the age
        (*it)->incrementKnobsAge();
        if ( (*it)->areKeyframesVisibleOnTimeline() ) {
            (*it)->hideKeyframesFromTimeline( next == instances.end() );
        }
        const std::vector<KnobIPtr> & knobs = (*it)->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            KnobButton* isBtn = dynamic_cast<KnobButton*>( knobs[i].get() );

            if ( !isBtn && (knobs[i]->getName() != kUserLabelKnobName) && (knobs[i]->getName() != kNatronOfxParamStringSublabelName) ) {
                knobs[i]->beginChanges();
                int dims = knobs[i]->getDimension();
                for (int j = 0; j < dims; ++j) {
                    knobs[i]->resetToDefaultValue(j);
                }
                knobs[i]->endChanges();
            }
        }

        // increment for next iteration
        if ( next != instances.end() ) {
            ++next;
        }
    } // for(it)

    instances.front()->getEffectInstance()->incrHashAndEvaluate(true, true);

    ///To update interacts, kinda hack but can't figure out where else put this
    getMainInstance()->getApp()->redrawAllViewers();
}

void
MultiInstancePanel::onButtonTriggered(KnobButton* button)
{
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);

    ///Forward the button click event to all the selected instances
    double time = getApp()->getTimeLine()->currentFrame();
    for (std::list<Node*>::iterator it = selectedInstances.begin(); it != selectedInstances.end(); ++it) {
        KnobIPtr k = (*it)->getKnobByName( button->getName() );
        assert( k && dynamic_cast<KnobButton*>( k.get() ) );
        (*it)->getEffectInstance()->onKnobValueChanged_public(k.get(), eValueChangedReasonUserEdited, time, ViewIdx(0), true);
    }
}

bool
MultiInstancePanel::onKnobValueChanged(KnobI* k,
                                       ValueChangedReasonEnum reason,
                                       double time,
                                       ViewSpec view,
                                       bool /*originatedFromMainThread*/)
{
    bool ret = false;

    if ( !k->isDeclaredByPlugin() ) {
        if (k->getName() == kDisableNodeKnobName) {
            KnobBool* boolKnob = dynamic_cast<KnobBool*>(k);
            assert(boolKnob);
            if (boolKnob) {
                _imp->mainInstance.lock()->onDisabledKnobToggled( boolKnob->getValue(0, view) );
                ret = true;
            }
        }
    } else {
        if (reason == eValueChangedReasonUserEdited) {
            KnobButton* isButton = dynamic_cast<KnobButton*>(k);
            if ( isButton && (reason == eValueChangedReasonUserEdited) ) {
                onButtonTriggered(isButton);
                ret = true;
            } else {
                ///for all selected instances update the same knob because it might not be slaved (see
                ///onSelectionChanged for an explanation why)
                for (Nodes::iterator it = _imp->instances.begin(); it != _imp->instances.end(); ++it) {
                    if (it->second) {
                        KnobIPtr sameKnob = it->first.lock()->getKnobByName( k->getName() );
                        assert(sameKnob);
                        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( sameKnob.get() );
                        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( sameKnob.get() );
                        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( sameKnob.get() );
                        KnobStringBase* isString = dynamic_cast<KnobStringBase*>( sameKnob.get() );
                        if (isInt) {
                            isInt->clone(k);
                        } else if (isBool) {
                            isBool->clone(k);
                        } else if (isDouble) {
                            isDouble->clone(k);
                        } else if (isString) {
                            isString->clone(k);
                        }

                        sameKnob->getHolder()->onKnobValueChanged_public(sameKnob.get(), eValueChangedReasonPluginEdited, time, view, true);
                        ret = true;
                    }
                }
            }
        }
    }

    return ret;
} // MultiInstancePanel::onKnobValueChanged

namespace  {
enum ExportTransformTypeEnum
{
    eExportTransformTypeStabilize,
    eExportTransformTypeMatchMove
};
}


/////////////// Tracker panel
struct TrackerPanelPrivateV1
{
    Q_DECLARE_TR_FUNCTIONS(TrackerPanel)

public:
    TrackerPanelV1* publicInterface;
    Button* averageTracksButton;
    Label* exportLabel;
    QWidget* exportContainer;
    QHBoxLayout* exportLayout;
    ComboBox* exportChoice;
    Button* exportButton;
    KnobPagePtr transformPage;
    KnobIntPtr referenceFrame;
    TrackScheduler scheduler;


    TrackerPanelPrivateV1(TrackerPanelV1* publicInterface,
                          const NodeGuiPtr & node)
        : publicInterface(publicInterface)
        , averageTracksButton(0)
        , exportLabel(0)
        , exportContainer(0)
        , exportLayout(0)
        , exportChoice(0)
        , exportButton(0)
        , transformPage()
        , referenceFrame()
        , scheduler( publicInterface, node->getNode() )
    {
    }

    void createTransformFromSelection(const std::list<Node*> & selection, bool linked, ExportTransformTypeEnum type);

    void createCornerPinFromSelection(const std::list<Node*> & selection, bool linked, bool useTransformRefFrame, bool invert);

    bool getTrackInstancesForButton(std::vector<KnobButton*>* trackButtons, const std::string& buttonName);
};

TrackerPanelV1::TrackerPanelV1(const NodeGuiPtr & node)
    : MultiInstancePanel(node)
    , _imp( new TrackerPanelPrivateV1(this, node) )
{
    QObject::connect( &_imp->scheduler, SIGNAL(trackingStarted(int)), this, SLOT(onTrackingStarted(int)) );
    QObject::connect( &_imp->scheduler, SIGNAL(trackingFinished()), this, SLOT(onTrackingFinished()) );
}

TrackerPanelV1::~TrackerPanelV1()
{
    _imp->scheduler.quitThread(false);
}

void
TrackerPanelV1::appendExtraGui(QVBoxLayout* layout)
{
    if ( !getMainInstance()->isPointTrackerNode() ) {
        return;
    }

    _imp->exportLabel = new Label( tr("Export data"), layout->parentWidget() );
    layout->addWidget(_imp->exportLabel);
    layout->addSpacing(10);
    _imp->exportContainer = new QWidget( layout->parentWidget() );
    _imp->exportLayout = new QHBoxLayout(_imp->exportContainer);
    _imp->exportLayout->setContentsMargins(0, 0, 0, 0);

    _imp->exportChoice = new ComboBox(_imp->exportContainer);

    _imp->exportChoice->setToolTip(QString::fromUtf8("<p><b>") + tr("CornerPinOFX (Use current frame):") + QString::fromUtf8("</p></b>")
                                   + QString::fromUtf8("<p>") + tr("Warp the image according to the relative transform using the current frame as reference.") + QString::fromUtf8("</p>") +
                                   QString::fromUtf8("<p><b>") + tr("CornerPinOFX (Use transform ref frame):") + QString::fromUtf8("</p></b>") +
                                   QString::fromUtf8("<p>") + tr("Warp the image according to the relative transform using the "
                                                                 "reference frame specified in the transform tab.") + QString::fromUtf8("</p>") +
                                   QString::fromUtf8("<p><b>") + tr("CornerPinOFX (Stabilize):") + QString::fromUtf8("</p></b>") +
                                   QString::fromUtf8("<p>") + tr("Transform the image so that the tracked points do not move.") + QString::fromUtf8("</p>")
//                                      "<p><b>" + tr("Transform (Stabilize):</p></b>"
//                                      "<p>" + tr("Transform the image so that the tracked points do not move.") + "</p>"
//                                      "<p><b>" + tr("Transform (Match-move):</p></b>"
//                                      "<p>" + tr("Transform another image so that it moves to match the tracked points.") + "</p>"
//                                      "<p>" + tr("The linked versions keep a link between the new node and the track, the others just copy"
//                                      " the values.") + "</p>"
                                   );
    std::vector<std::string> choices;
    std::vector<std::string> helps;

    choices.push_back( tr("CornerPin (Use current frame. Linked)").toStdString() );
    helps.push_back( tr("Warp the image according to the relative transform using the current frame as reference.").toStdString() );
    //
    //    choices.push_back(tr("CornerPinOFX (Use transform ref frame. Linked)").toStdString());
    //    helps.push_back(tr("Warp the image according to the relative transform using the "
    //                       "reference frame specified in the transform tab.").toStdString());

    choices.push_back( tr("CornerPin (Stabilize. Linked)").toStdString() );
    helps.push_back( tr("Transform the image so that the tracked points do not move.").toStdString() );

    choices.push_back( tr("CornerPin (Use current frame. Copy)").toStdString() );
    helps.push_back( tr("Same as the linked version except that it copies values instead of "
                        "referencing them via a link to the track").toStdString() );

    choices.push_back( tr("CornerPin (Stabilize. Copy)").toStdString() );
    helps.push_back( tr("Same as the linked version except that it copies values instead of "
                        "referencing them via a link to the track").toStdString() );

    choices.push_back( tr("CornerPin (Use transform ref frame. Copy)").toStdString() );
    helps.push_back( tr("Same as the linked version except that it copies values instead of "
                        "referencing them via a link to the track").toStdString() );


    //    choices.push_back(tr("Transform (Stabilize. Linked)").toStdString());
    //    helps.push_back(tr("Transform the image so that the tracked points do not move.").toStdString());
    //
    //    choices.push_back(tr("Transform (Match-move. Linked)").toStdString());
    //    helps.push_back(tr("Transform another image so that it moves to match the tracked points.").toStdString());
    //
    //    choices.push_back(tr("Transform (Stabilize. Copy)").toStdString());
    //    helps.push_back(tr("Same as the linked version except that it copies values instead of "
    //                       "referencing them via a link to the track").toStdString());
    //
    //    choices.push_back(tr("Transform (Match-move. Copy)").toStdString());
    //    helps.push_back(tr("Same as the linked version except that it copies values instead of "
    //                       "referencing them via a link to the track").toStdString());
    for (U32 i = 0; i < choices.size(); ++i) {
        _imp->exportChoice->addItem( QString::fromUtf8( choices[i].c_str() ), QIcon(), QKeySequence(), QString::fromUtf8( helps[i].c_str() ) );
    }
    _imp->exportLayout->addWidget(_imp->exportChoice);

    _imp->exportButton = new Button(tr("Export"), _imp->exportContainer);
    QObject::connect( _imp->exportButton, SIGNAL(clicked(bool)), this, SLOT(onExportButtonClicked()) );
    _imp->exportLayout->addWidget(_imp->exportButton);
    _imp->exportLayout->addStretch();
    layout->addWidget(_imp->exportContainer);
} // appendExtraGui

void
TrackerPanelV1::appendButtons(QHBoxLayout* buttonLayout)
{
    if ( !getMainInstance()->isPointTrackerNode() ) {
        return;
    }
    _imp->averageTracksButton = new Button( tr("Average tracks"), buttonLayout->parentWidget() );
    _imp->averageTracksButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Make a new track which is the average of the selected tracks."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->averageTracksButton, SIGNAL(clicked(bool)), this, SLOT(onAverageTracksButtonClicked()) );
    buttonLayout->addWidget(_imp->averageTracksButton);
}

void
TrackerPanelV1::initializeExtraKnobs()
{
    if ( !getMainInstance()->isPointTrackerNode() ) {
        return;
    }
    _imp->transformPage = AppManager::createKnob<KnobPage>(this, tr("Transform"), 1, false);

    _imp->referenceFrame = AppManager::createKnob<KnobInt>(this, tr("Reference Frame"), 1, false);
    _imp->referenceFrame->setAnimationEnabled(false);
    _imp->referenceFrame->setHintToolTip( tr("This is the frame number at which the transform will be an identity.") );
    _imp->transformPage->addKnob(_imp->referenceFrame);
}

void
TrackerPanelV1::setIconForButton(KnobButton* knob)
{
    const std::string name = knob->getName();

    if (name == kNatronParamTrackingPrevious) {
        knob->setIconLabel(NATRON_IMAGES_PATH "back1.png");
    } else if (name == kNatronParamTrackingNext) {
        knob->setIconLabel(NATRON_IMAGES_PATH "forward1.png");
    } else if (name == kNatronParamTrackingBackward) {
        knob->setIconLabel(NATRON_IMAGES_PATH "rewind.png");
    } else if (name == kNatronParamTrackingForward) {
        knob->setIconLabel(NATRON_IMAGES_PATH "play.png");
    }
}

void
TrackerPanelV1::onAverageTracksButtonClicked()
{
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);
    if ( selectedInstances.empty() ) {
        Dialogs::warningDialog( tr("Average").toStdString(), tr("No tracks selected").toStdString() );

        return;
    }

    NodePtr newInstance = addInstanceInternal(true);
    ///give an appropriate name to the new instance
    int avgIndex = 0;
    const std::list<std::pair<NodeWPtr, bool > > & allInstances = getInstances();
    for (std::list<std::pair<NodeWPtr, bool > >::const_iterator it = allInstances.begin();
         it != allInstances.end(); ++it) {
        if ( QString::fromUtf8( it->first.lock()->getScriptName().c_str() ).contains(QString::fromUtf8("average"), Qt::CaseInsensitive) ) {
            ++avgIndex;
        }
    }
    QString newName = QString::fromUtf8("Average%1").arg(avgIndex + 1);
    try {
        newInstance->setScriptName( newName.toStdString() );
    } catch (...) {
    }

    newInstance->updateEffectLabelKnob(newName);

    KnobDoublePtr newInstanceCenter = getCenterKnobForTracker( newInstance.get() );
    std::list<KnobDoublePtr> centers;
    RangeD keyframesRange;
    keyframesRange.min = std::numeric_limits<int>::max();
    keyframesRange.max = std::numeric_limits<int>::min();

    for (std::list<Node*>::iterator it = selectedInstances.begin(); it != selectedInstances.end(); ++it) {
        KnobDoublePtr dblKnob = getCenterKnobForTracker(*it);
        centers.push_back(dblKnob);
        double mini, maxi;
        bool hasKey = dblKnob->getFirstKeyFrameTime(ViewIdx(0), 0, &mini);
        if (!hasKey) {
            continue;
        }
        if (mini < keyframesRange.min) {
            keyframesRange.min = mini;
        }

        hasKey = dblKnob->getLastKeyFrameTime(ViewIdx(0), 0, &maxi);

        ///both dimensions must have keyframes
        assert(hasKey);
        if (maxi > keyframesRange.max) {
            keyframesRange.max = maxi;
        }
    }
    if (keyframesRange.min == std::numeric_limits<int>::min()) {
        keyframesRange.min = 0;
    }
    if (keyframesRange.max == std::numeric_limits<int>::max()) {
        keyframesRange.max = 0;
    }

    newInstanceCenter->beginChanges();
    for (double t = keyframesRange.min; t <= keyframesRange.max; ++t) {
        std::pair<double, double> average;
        average.first = 0;
        average.second = 0;
        const size_t centersNb = centers.size();
        if (centersNb) {
            for (std::list<KnobDoublePtr>::iterator it = centers.begin(); it != centers.end(); ++it) {
                double x = (*it)->getValueAtTime(t, 0);
                double y = (*it)->getValueAtTime(t, 1);
                average.first += x;
                average.second += y;
            }
            average.first /= centersNb;
            average.second /= centersNb;
            newInstanceCenter->setValueAtTime(t, average.first,  ViewSpec::all(), 0);
            newInstanceCenter->setValueAtTime(t, average.second, ViewSpec::all(), 1);
        }
    }
    newInstanceCenter->endChanges();
} // onAverageTracksButtonClicked

void
TrackerPanelV1::onButtonTriggered(KnobButton* button)
{
    std::string name = button->getName();

    assert(button);

    EffectInstancePtr effect = getMainInstance()->getEffectInstance();
    assert(effect);
    std::list<ViewerInstance*> viewers;
    effect->getNode()->hasViewersConnected(&viewers);
    ViewerInstance* viewer = 0;
    if ( !viewers.empty() ) {
        viewer = viewers.front();
    }

    ///hack the trackBackward and trackForward buttons behaviour so they appear to progress simultaneously
    if (name == kNatronParamTrackingBackward) {
        trackBackward(viewer);
    } else if (name == kNatronParamTrackingForward) {
        trackForward(viewer);
    } else if (name == kNatronParamTrackingPrevious) {
        trackPrevious(viewer);
    } else if (name == kNatronParamTrackingNext) {
        trackNext(viewer);
    }
}

void
TrackerPanelV1::onTrackingStarted(int /*step*/)
{
    ///freeze the tracker node
    setKnobsFrozen(true);
    if ( getGui() ) {
        getGui()->progressStart(getMainInstance(), tr("Tracking...").toStdString(), "");
    }
}

void
TrackerPanelV1::onTrackingFinished()
{
    setKnobsFrozen(false);
    Q_EMIT trackingEnded();
    if ( getGui() ) {
        getGui()->progressEnd( getMainInstance() );
    }
}

bool
TrackerPanelPrivateV1::getTrackInstancesForButton(std::vector<KnobButton*>* trackButtons,
                                                  const std::string& buttonName)
{
    std::list<Node*> selectedInstances;

    publicInterface->getSelectedInstances(&selectedInstances);
    if ( selectedInstances.empty() ) {
        Dialogs::warningDialog( tr("Tracker").toStdString(), tr("You must select something to track first").toStdString() );

        return false;
    }

    KnobButton* prevBtn = dynamic_cast<KnobButton*>( publicInterface->getKnobByName(buttonName).get() );
    assert(prevBtn);
    if (prevBtn) {
        for (std::list<Node*>::const_iterator it = selectedInstances.begin(); it != selectedInstances.end(); ++it) {
            if ( !(*it)->getEffectInstance() ) {
                return false;
            }
            if ( (*it)->isNodeDisabled() ) {
                continue;
            }
            KnobIPtr k = (*it)->getKnobByName( prevBtn->getName() );
            KnobButton* bKnob = dynamic_cast<KnobButton*>( k.get() );
            assert(bKnob);
            trackButtons->push_back(bKnob);
        }
    }

    return true;
}

bool
TrackerPanelV1::trackBackward(ViewerInstance* /*viewer*/)
{
    Dialogs::errorDialog( tr("Tracker").toStdString(), tr("TrackerPM is now depecrated and can only be used to retrieve animation tracked from older projects, please use the Tracker node from now on.").toStdString() );

    return false;
#if 0
    assert( QThread::currentThread() == qApp->thread() );


    std::vector<KnobButton*> instanceButtons;
    if ( !_imp->getTrackInstancesForButton(&instanceButtons, kNatronParamTrackingPrevious) ) {
        return false;
    }

    double leftBound, rightBound;
    getApp()->getFrameRange(&leftBound, &rightBound);
    int end = leftBound - 1;
    int start = getApp()->getTimeLine()->currentFrame();

    _imp->scheduler.track( TrackArgsV1(start, end, -1, getApp()->getTimeLine(), viewer, instanceButtons) );

    return true;
#endif
} // trackBackward

bool
TrackerPanelV1::trackForward(ViewerInstance* /*viewer*/)
{
    Dialogs::errorDialog( tr("Tracker").toStdString(), tr("TrackerPM is now depecrated and can only be used to retrieve animation tracked from older projects, please use the Tracker node from now on.").toStdString() );

    return false;
#if 0
    assert( QThread::currentThread() == qApp->thread() );


    std::vector<KnobButton*> instanceButtons;
    if ( !_imp->getTrackInstancesForButton(&instanceButtons, kNatronParamTrackingNext) ) {
        return false;
    }

    double leftBound, rightBound;
    getApp()->getFrameRange(&leftBound, &rightBound);
    TimeLinePtr timeline = getApp()->getTimeLine();
    int end = rightBound + 1;
    int start = timeline->currentFrame();

    _imp->scheduler.track( TrackArgsV1(start, end, 1, getApp()->getTimeLine(), viewer, instanceButtons) );

    return true;
#endif
} // trackForward

void
TrackerPanelV1::stopTracking()
{
    _imp->scheduler.abortThreadedTask();
}

bool
TrackerPanelV1::isTracking() const
{
    return _imp->scheduler.isWorking();
}

bool
TrackerPanelV1::trackPrevious(ViewerInstance* /*viewer*/)
{
    Dialogs::errorDialog( tr("Tracker").toStdString(), tr("TrackerPM is now depecrated and can only be used to retrieve animation tracked from older projects, please use the Tracker node from now on.").toStdString() );

    return false;
#if 0
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);
    if ( selectedInstances.empty() ) {
        Dialogs::warningDialog( tr("Tracker").toStdString(), tr("You must select something to track first").toStdString() );

        return false;
    }
    std::vector<KnobButton*> instanceButtons;
    if ( !_imp->getTrackInstancesForButton(&instanceButtons, kNatronParamTrackingPrevious) ) {
        return false;
    }

    TimeLinePtr timeline = getApp()->getTimeLine();
    int start = timeline->currentFrame();
    int end = start - 1;

    _imp->scheduler.track( TrackArgsV1(start, end, -1, getApp()->getTimeLine(), viewer, instanceButtons) );

    return true;
#endif
}

bool
TrackerPanelV1::trackNext(ViewerInstance* /*viewer*/)
{
    Dialogs::errorDialog( tr("Tracker").toStdString(), tr("TrackerPM is now depecrated and can only be used to retrieve animation tracked from older projects, please use the Tracker node from now on.").toStdString() );

    return false;
#if 0
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);
    if ( selectedInstances.empty() ) {
        Dialogs::warningDialog( tr("Tracker").toStdString(), tr("You must select something to track first").toStdString() );

        return false;
    }
    std::vector<KnobButton*> instanceButtons;
    if ( !_imp->getTrackInstancesForButton(&instanceButtons, kNatronParamTrackingNext) ) {
        return false;
    }

    TimeLinePtr timeline = getApp()->getTimeLine();
    int start = timeline->currentFrame();
    int end = start + 1;

    _imp->scheduler.track( TrackArgsV1(start, end, 1, getApp()->getTimeLine(), viewer, instanceButtons) );

    return true;
#endif
}

void
TrackerPanelV1::clearAllAnimationForSelection()
{
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);
    for (std::list<Node*>::const_iterator it = selectedInstances.begin(); it != selectedInstances.end(); ++it) {
        const std::vector<KnobIPtr> & knobs = (*it)->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            for (int dim = 0; dim < knobs[i]->getDimension(); ++dim) {
                knobs[i]->removeAnimation(ViewSpec::all(), dim);
            }
        }
    }
}

void
TrackerPanelV1::clearBackwardAnimationForSelection()
{
    double time = getApp()->getTimeLine()->currentFrame();
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);
    for (std::list<Node*>::const_iterator it = selectedInstances.begin(); it != selectedInstances.end(); ++it) {
        const std::vector<KnobIPtr> & knobs = (*it)->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            for (int dim = 0; dim < knobs[i]->getDimension(); ++dim) {
                knobs[i]->deleteAnimationBeforeTime(time, ViewSpec::all(), dim, eValueChangedReasonPluginEdited);
            }
        }
    }
}

void
TrackerPanelV1::clearForwardAnimationForSelection()
{
    double time = getApp()->getTimeLine()->currentFrame();
    std::list<Node*> selectedInstances;

    getSelectedInstances(&selectedInstances);
    for (std::list<Node*>::const_iterator it = selectedInstances.begin(); it != selectedInstances.end(); ++it) {
        const std::vector<KnobIPtr> & knobs = (*it)->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            for (int dim = 0; dim < knobs[i]->getDimension(); ++dim) {
                knobs[i]->deleteAnimationAfterTime(time, ViewSpec::all(), dim, eValueChangedReasonPluginEdited);
            }
        }
    }
}

void
TrackerPanelV1::onExportButtonClicked()
{
    int index = _imp->exportChoice->activeIndex();
    std::list<Node*> selection;

    getSelectedInstances(&selection);
    ///This is the full list, decomment when everything will be possible to do
    //    switch (index) {
    //        case 0:
    //            _imp->createCornerPinFromSelection(selection, true, false);
    //            break;
    //        case 1:
    //            _imp->createCornerPinFromSelection(selection, true, true);
    //            break;
    //        case 2:
    //            _imp->createCornerPinFromSelection(selection, false, false);
    //            break;
    //        case 3:
    //            _imp->createCornerPinFromSelection(selection, false, true);
    //            break;
    //        case 4:
    //            _imp->createTransformFromSelection(selection, true, eExportTransformTypeStabilize);
    //            break;
    //        case 5:
    //            _imp->createTransformFromSelection(selection, true, eExportTransformTypeMatchMove);
    //            break;
    //        case 6:
    //            _imp->createTransformFromSelection(selection, false, eExportTransformTypeStabilize);
    //            break;
    //        case 7:
    //            _imp->createTransformFromSelection(selection, false, eExportTransformTypeMatchMove);
    //            break;
    //        default:
    //            break;
    //    }
    switch (index) {
    case 0:
        _imp->createCornerPinFromSelection(selection, true, false, false);
        break;
    case 1:
        _imp->createCornerPinFromSelection(selection, true, false, true);
        break;
    case 2:
        _imp->createCornerPinFromSelection(selection, false, false, false);
        break;
    case 3:
        _imp->createCornerPinFromSelection(selection, false, false, true);
        break;
    case 4:
        _imp->createCornerPinFromSelection(selection, false, true, false);
        break;
    default:
        break;
    }
} // TrackerPanelV1::onExportButtonClicked

void
TrackerPanelPrivateV1::createTransformFromSelection(const std::list<Node*> & /*selection*/,
                                                    bool /*linked*/,
                                                    ExportTransformTypeEnum /*type*/)
{
}

namespace  {
KnobDoublePtr
getCornerPinPoint(Node* node,
                  bool isFrom,
                  int index)
{
    assert(0 <= index && index < 4);
    QString name = isFrom ? QString::fromUtf8("from%1").arg(index + 1) : QString::fromUtf8("to%1").arg(index + 1);
    KnobIPtr knob = node->getKnobByName( name.toStdString() );
    assert(knob);
    KnobDoublePtr  ret = std::dynamic_pointer_cast<KnobDouble>(knob);
    assert(ret);

    return ret;
}
}

void
TrackerPanelPrivateV1::createCornerPinFromSelection(const std::list<Node*> & selection,
                                                    bool linked,
                                                    bool useTransformRefFrame,
                                                    bool invert)
{
    if ( (selection.size() > 4) || selection.empty() ) {
        Dialogs::errorDialog( tr("Export").toStdString(),
                              tr("Export to corner pin needs between 1 and 4 selected tracks.").toStdString() );

        return;
    }

    KnobDoublePtr centers[4];
    int i = 0;
    for (std::list<Node*>::const_iterator it = selection.begin(); it != selection.end(); ++it, ++i) {
        centers[i] = getCenterKnobForTracker(*it);
        assert(centers[i]);
    }
    GuiAppInstancePtr app = publicInterface->getGui()->getApp();
    CreateNodeArgs args(PLUGINID_OFX_CORNERPIN, publicInterface->getMainInstance()->getGroup() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    NodePtr cornerPin = app->createNode(args);

    if (!cornerPin) {
        return;
    }

    ///Move the node on the right of the tracker node
    NodeGuiIPtr cornerPinGui_i = cornerPin->getNodeGui();
    NodeGui* cornerPinGui = dynamic_cast<NodeGui*>( cornerPinGui_i.get() );
    assert(cornerPinGui);

    NodeGuiPtr mainInstanceGui = publicInterface->getMainInstanceGui();
    assert(mainInstanceGui);

    QPointF mainInstancePos = mainInstanceGui->scenePos();

    if (cornerPinGui) {
        mainInstancePos = cornerPinGui->mapToParent( cornerPinGui->mapFromScene(mainInstancePos) );
        cornerPinGui->refreshPosition( mainInstancePos.x() + mainInstanceGui->getSize().width() * 2, mainInstancePos.y() );
    }
    KnobDoublePtr toPoints[4];
    KnobDoublePtr  fromPoints[4];
    double timeForFromPoints = useTransformRefFrame ? referenceFrame->getValue() : app->getTimeLine()->currentFrame();

    for (unsigned int i = 0; i < selection.size(); ++i) {
        fromPoints[i] = getCornerPinPoint(cornerPin.get(), true, i);
        assert(fromPoints[i] && centers[i]);
        for (int j = 0; j < fromPoints[i]->getDimension(); ++j) {
            fromPoints[i]->setValue(centers[i]->getValueAtTime(timeForFromPoints, j), ViewSpec::all(), j);
        }

        toPoints[i] = getCornerPinPoint(cornerPin.get(), false, i);
        assert(toPoints[i]);
        if (!linked) {
            toPoints[i]->cloneAndUpdateGui( centers[i].get() );
        } else {
            EffectInstance* effect = dynamic_cast<EffectInstance*>( centers[i]->getHolder() );
            assert(effect);
            if (effect) {
                std::stringstream ss;
                ss << "thisGroup." << effect->getNode()->getFullyQualifiedName() << "." << centers[i]->getName() << ".get()[dimension]";
                std::string expr = ss.str();
                dynamic_cast<KnobI*>( toPoints[i].get() )->setExpression(0, expr, false, false);
                dynamic_cast<KnobI*>( toPoints[i].get() )->setExpression(1, expr, false, false);
            }
        }
    }

    ///Disable all non used points
    for (unsigned int i = selection.size(); i < 4; ++i) {
        QString enableName = QString::fromUtf8("enable%1").arg(i + 1);
        KnobIPtr knob = cornerPin->getKnobByName( enableName.toStdString() );
        assert(knob);
        KnobBool* enableKnob = dynamic_cast<KnobBool*>( knob.get() );
        assert(enableKnob);
        enableKnob->setValue(false);
    }

    if (invert) {
        KnobIPtr invertKnob = cornerPin->getKnobByName(kTrackInvertName);
        assert(invertKnob);
        KnobBool* isBool = dynamic_cast<KnobBool*>( invertKnob.get() );
        assert(isBool);
        isBool->setValue(true);
    }
} // createCornerPinFromSelection

void

TrackerPanelV1::showMenuForInstance(Node* instance)
{
    if ( !getMainInstance()->isPointTrackerNode() ) {
        return;
    }
    Menu menu( getGui() );

    //menu.setFont( QFont(appFont,appFontSize) );
    QAction* copyTrackAnimation = new QAction(tr("Copy track animation"), &menu);
    menu.addAction(copyTrackAnimation);

    QAction* ret = menu.exec( QCursor::pos() );
    if (ret == copyTrackAnimation) {
        KnobDoublePtr centerKnob = getCenterKnobForTracker(instance);
        assert(centerKnob);
        centerKnob->copyAnimationToClipboard();
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_MultiInstancePanel.cpp"
