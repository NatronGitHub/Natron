//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "DockablePanel.h"

#include <iostream>
#include <QLayout>
#include <QAction>
#include <QTabWidget>
#include <QStyle>
#include <QUndoStack>
#include <QFormLayout>
#include <QUndoCommand>
#include <QDebug>
#include <QToolTip>
#include <QMutex>
#include <QColorDialog>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QPaintEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QTextDocument> // for Qt::convertFromPlainText
#include <QPainter>
#include <QImage>

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiTypes.h" // for Group_KnobGui
#include "Gui/KnobGuiFactory.h"
#include "Gui/LineEdit.h"
#include "Gui/Button.h"
#include "Gui/NodeGraph.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/RotoPanel.h"
#include "Gui/NodeBackDrop.h"
#include "Gui/MultiInstancePanel.h"

using std::make_pair;
using namespace Natron;

namespace {

struct Page {
    QWidget* tab;
    int currentRow;
    QTabWidget* tabWidget; //< to gather group knobs that are set as a tab
    
    Page() : tab(0), currentRow(0),tabWidget(0)
    {}
    
    Page(const Page& other) : tab(other.tab), currentRow(other.currentRow) , tabWidget(other.tabWidget) {}
};
    
typedef std::map<QString,Page> PageMap;

}

struct DockablePanelPrivate
{
    DockablePanel* _publicInterface;
    
    Gui* _gui;
    
    QVBoxLayout* _container; /*!< ptr to the layout containing this DockablePanel*/
    
    /*global layout*/
    QVBoxLayout* _mainLayout;
    
    /*Header related*/
    QFrame* _headerWidget;
    QHBoxLayout *_headerLayout;
    
    LineEdit* _nameLineEdit; /*!< if the name is editable*/
    QLabel* _nameLabel; /*!< if the name is read-only*/
    
    /*Tab related*/
    QTabWidget* _tabWidget;
    
    Button* _helpButton;
    Button* _minimize;
    Button* _floatButton;
    Button* _cross;
    
    mutable QMutex _currentColorMutex; //< protects _currentColor
    QColor _currentColor; //< accessed by the serialization thread
    Button* _colorButton;
    Button* _undoButton;
    Button* _redoButton;
    Button* _restoreDefaultsButton;
    
    bool _minimized; /*!< true if the panel is minimized*/
    QUndoStack* _undoStack; /*!< undo/redo stack*/
    
    bool _floating; /*!< true if the panel is floating*/
    FloatingWidget* _floatingWidget;
    
    /*a map storing for each knob a pointer to their GUI.*/
    std::map<boost::shared_ptr<KnobI>,KnobGui*> _knobs;
    KnobHolder* _holder;
    
    /* map<tab name, pair<tab , row count> >*/
    PageMap _pages;
    
    QString _defaultPageName;
    
    bool _useScrollAreasForTabs;
    
    DockablePanel::HeaderMode _mode;
    
    mutable QMutex _isClosedMutex;
    bool _isClosed; //< accessed by serialization thread too
    
    DockablePanelPrivate(DockablePanel* publicI
                         ,Gui* gui
                         ,KnobHolder* holder
                         , QVBoxLayout* container
                         , DockablePanel::HeaderMode headerMode
                         ,bool useScrollAreasForTabs
                         ,const QString& defaultPageName)
    :_publicInterface(publicI)
    ,_gui(gui)
    ,_container(container)
    ,_mainLayout(NULL)
    ,_headerWidget(NULL)
    ,_headerLayout(NULL)
    ,_nameLineEdit(NULL)
    ,_nameLabel(NULL)
    ,_tabWidget(NULL)
    ,_helpButton(NULL)
    ,_minimize(NULL)
    ,_floatButton(NULL)
    ,_cross(NULL)
    , _currentColor()
    ,_colorButton(NULL)
    ,_undoButton(NULL)
    ,_redoButton(NULL)
    ,_restoreDefaultsButton(NULL)
    ,_minimized(false)
    ,_undoStack(new QUndoStack)
    ,_floating(false)
    ,_floatingWidget(NULL)
    ,_knobs()
    ,_holder(holder)
    ,_pages()
    ,_defaultPageName(defaultPageName)
    ,_useScrollAreasForTabs(useScrollAreasForTabs)
    ,_mode(headerMode)
    ,_isClosedMutex()
    ,_isClosed(false)
    {
        
    }
    
    
    /*inserts a new page to the dockable panel.*/
    PageMap::iterator addPage(const QString& name);
    
    
    void initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> >& knobs,
                              QWidget* lastRowWidget,
                              bool onlyTopLevelKnobs);
    
    KnobGui* createKnobGui(const boost::shared_ptr<KnobI> &knob);
    
    /*Search an existing knob GUI in the map, otherwise creates
     the gui for the knob.*/
    KnobGui* findKnobGuiOrCreate(const boost::shared_ptr<KnobI> &knob,
                                 bool makeNewLine,
                                 QWidget* lastRowWidget,
                    const std::vector< boost::shared_ptr< KnobI > >& knobsOnSameLine = std::vector< boost::shared_ptr< KnobI > >());
    
};

static QPixmap getColorButtonDefaultPixmap()
{
    QImage img(15,15,QImage::Format_ARGB32);
    QColor gray(Qt::gray);
    img.fill(gray.rgba());
    QPainter p(&img);
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(2);
    p.setPen(pen);
    p.drawLine(0, 0, 14, 14);
    return QPixmap::fromImage(img);
}

DockablePanel::DockablePanel(Gui* gui
                             , KnobHolder* holder
                             , QVBoxLayout* container
                             , HeaderMode headerMode
                             , bool useScrollAreasForTabs
                             , const QString& initialName
                             , const QString& helpToolTip
                             , bool createDefaultPage
                             , const QString& defaultPageName
                             , QWidget *parent)
: QFrame(parent)
, _imp(new DockablePanelPrivate(this,gui,holder,container,headerMode,useScrollAreasForTabs,defaultPageName))

{
    _imp->_mainLayout = new QVBoxLayout(this);
    _imp->_mainLayout->setSpacing(0);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_imp->_mainLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameShape(QFrame::Box);
    
    if(headerMode != NO_HEADER){
        
        _imp->_headerWidget = new QFrame(this);
        _imp->_headerWidget->setFrameShape(QFrame::Box);
        _imp->_headerLayout = new QHBoxLayout(_imp->_headerWidget);
        _imp->_headerLayout->setContentsMargins(0, 0, 0, 0);
        _imp->_headerLayout->setSpacing(2);
        _imp->_headerWidget->setLayout(_imp->_headerLayout);
        
        
        QPixmap pixHelp ;
        appPTR->getIcon(NATRON_PIXMAP_HELP_WIDGET,&pixHelp);
        _imp->_helpButton = new Button(QIcon(pixHelp),"",_imp->_headerWidget);
        if (!helpToolTip.isEmpty()) {
            _imp->_helpButton->setToolTip(Qt::convertFromPlainText(helpToolTip, Qt::WhiteSpaceNormal));
        }
        _imp->_helpButton->setFixedSize(15, 15);
        QObject::connect(_imp->_helpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
        
        
        QPixmap pixM;
        appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET,&pixM);
        
        QPixmap pixC;
        appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);
        
        QPixmap pixF;
        appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, &pixF);
        
        _imp->_minimize=new Button(QIcon(pixM),"",_imp->_headerWidget);
        _imp->_minimize->setFixedSize(15,15);
        _imp->_minimize->setCheckable(true);
        QObject::connect(_imp->_minimize,SIGNAL(toggled(bool)),this,SLOT(minimizeOrMaximize(bool)));
        
        _imp->_floatButton = new Button(QIcon(pixF),"",_imp->_headerWidget);
        _imp->_floatButton->setFixedSize(15, 15);
        QObject::connect(_imp->_floatButton,SIGNAL(clicked()),this,SLOT(floatPanel()));
        
        
        _imp->_cross=new Button(QIcon(pixC),"",_imp->_headerWidget);
        _imp->_cross->setFixedSize(15,15);
        QObject::connect(_imp->_cross,SIGNAL(clicked()),this,SLOT(closePanel()));
        
        if (headerMode != READ_ONLY_NAME) {
            boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
            float r,g,b;
            Natron::EffectInstance* iseffect = dynamic_cast<Natron::EffectInstance*>(holder);
            NodeBackDrop* backdrop = dynamic_cast<NodeBackDrop*>(holder);
            MultiInstancePanel* isMultiInstance = dynamic_cast<MultiInstancePanel*>(holder);
            if (isMultiInstance) {
                iseffect = isMultiInstance->getMainInstance()->getLiveInstance();
                assert(iseffect);
            }
            if (iseffect) {
                std::list<std::string> grouping;
                iseffect->pluginGrouping(&grouping);
                std::string majGroup = grouping.empty() ? "" : grouping.front();
                
                if (iseffect->isReader()) {
                    settings->getReaderColor(&r, &g, &b);
                } else if (iseffect->isWriter()) {
                    settings->getWriterColor(&r, &g, &b);
                } else if (iseffect->isGenerator()) {
                    settings->getGeneratorColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_COLOR) {
                    settings->getColorGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_FILTER) {
                    settings->getFilterGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_CHANNEL) {
                    settings->getChannelGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_KEYER) {
                    settings->getKeyerGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_MERGE) {
                    settings->getMergeGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_PAINT) {
                    settings->getDrawGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_TIME) {
                    settings->getTimeGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_TRANSFORM) {
                    settings->getTransformGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_MULTIVIEW) {
                    settings->getViewsGroupColor(&r, &g, &b);
                } else if (majGroup == PLUGIN_GROUP_DEEP) {
                    settings->getDeepGroupColor(&r, &g, &b);
                } else {
                    settings->getDefaultNodeColor(&r, &g, &b);
                }
            } else if(backdrop) {
                appPTR->getCurrentSettings()->getDefaultBackDropColor(&r, &g, &b);
            } else {
                r = g = b = 0.6;
            }
            _imp->_currentColor.setRgbF(Natron::clamp(r), Natron::clamp(g), Natron::clamp(b));
            _imp->_colorButton = new Button(QIcon(getColorButtonDefaultPixmap()),"",_imp->_headerWidget);
            _imp->_colorButton->setToolTip(Qt::convertFromPlainText(tr("Set here the color of the node in the nodegraph. "
                                                                    "By default the color of the node is the one set in the "
                                                                    "preferences of %1").arg(NATRON_APPLICATION_NAME)
                                                                    ,Qt::WhiteSpaceNormal));
            QObject::connect(_imp->_colorButton,SIGNAL(clicked()),this,SLOT(onColorButtonClicked()));
            _imp->_colorButton->setFixedSize(15,15);
            
            if (iseffect) {
                ///Show timeline keyframe markers to be consistent with the fact that the panel is opened by default
                iseffect->getNode()->showKeyframesOnTimeline(true);
            }
            
        }
        QPixmap pixUndo ;
        appPTR->getIcon(NATRON_PIXMAP_UNDO,&pixUndo);
        QPixmap pixUndo_gray ;
        appPTR->getIcon(NATRON_PIXMAP_UNDO_GRAYSCALE,&pixUndo_gray);
        QIcon icUndo;
        icUndo.addPixmap(pixUndo,QIcon::Normal);
        icUndo.addPixmap(pixUndo_gray,QIcon::Disabled);
        _imp->_undoButton = new Button(icUndo,"",_imp->_headerWidget);
        _imp->_undoButton->setToolTip(Qt::convertFromPlainText(tr("Undo the last change made to this operator"), Qt::WhiteSpaceNormal));
        _imp->_undoButton->setEnabled(false);
        _imp->_undoButton->setFixedSize(20, 20);
        
        QPixmap pixRedo ;
        appPTR->getIcon(NATRON_PIXMAP_REDO,&pixRedo);
        QPixmap pixRedo_gray;
        appPTR->getIcon(NATRON_PIXMAP_REDO_GRAYSCALE,&pixRedo_gray);
        QIcon icRedo;
        icRedo.addPixmap(pixRedo,QIcon::Normal);
        icRedo.addPixmap(pixRedo_gray,QIcon::Disabled);
        _imp->_redoButton = new Button(icRedo,"",_imp->_headerWidget);
        _imp->_redoButton->setToolTip(Qt::convertFromPlainText(tr("Redo the last change undone to this operator"), Qt::WhiteSpaceNormal));
        _imp->_redoButton->setEnabled(false);
        _imp->_redoButton->setFixedSize(20, 20);
        
        QPixmap pixRestore;
        appPTR->getIcon(NATRON_PIXMAP_RESTORE_DEFAULTS, &pixRestore);
        QIcon icRestore;
        icRestore.addPixmap(pixRestore);
        _imp->_restoreDefaultsButton = new Button(icRestore,"",_imp->_headerWidget);
        _imp->_restoreDefaultsButton->setToolTip(Qt::convertFromPlainText(tr("Restore default values for this operator."
                                                                    " This cannot be undone!"),Qt::WhiteSpaceNormal));
        _imp->_restoreDefaultsButton->setFixedSize(20, 20);
        QObject::connect(_imp->_restoreDefaultsButton,SIGNAL(clicked()),this,SLOT(onRestoreDefaultsButtonClicked()));
    
        
        QObject::connect(_imp->_undoButton, SIGNAL(clicked()),this, SLOT(onUndoClicked()));
        QObject::connect(_imp->_redoButton, SIGNAL(clicked()),this, SLOT(onRedoPressed()));
        
        if(headerMode != READ_ONLY_NAME){
            _imp->_nameLineEdit = new LineEdit(_imp->_headerWidget);
            _imp->_nameLineEdit->setText(initialName);
            QObject::connect(_imp->_nameLineEdit,SIGNAL(editingFinished()),this,SLOT(onLineEditNameEditingFinished()));
            _imp->_headerLayout->addWidget(_imp->_nameLineEdit);
        }else{
            _imp->_nameLabel = new QLabel(initialName,_imp->_headerWidget);
            _imp->_headerLayout->addWidget(_imp->_nameLabel);
        }
        
        _imp->_headerLayout->addStretch();
        
        if (headerMode != READ_ONLY_NAME) {
            _imp->_headerLayout->addWidget(_imp->_colorButton);
        }
        _imp->_headerLayout->addWidget(_imp->_undoButton);
        _imp->_headerLayout->addWidget(_imp->_redoButton);
        _imp->_headerLayout->addWidget(_imp->_restoreDefaultsButton);
        
        _imp->_headerLayout->addStretch();
        _imp->_headerLayout->addWidget(_imp->_helpButton);
        _imp->_headerLayout->addWidget(_imp->_minimize);
        _imp->_headerLayout->addWidget(_imp->_floatButton);
        _imp->_headerLayout->addWidget(_imp->_cross);
        
        _imp->_mainLayout->addWidget(_imp->_headerWidget);
        
    }
    
    if (useScrollAreasForTabs) {
        _imp->_tabWidget = new QTabWidget(this);
    } else {
        _imp->_tabWidget = new DockablePanelTabWidget(this);
    }
    _imp->_tabWidget->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    _imp->_tabWidget->setObjectName("QTabWidget");
    _imp->_mainLayout->addWidget(_imp->_tabWidget);
    
    if(createDefaultPage){
        _imp->addPage(defaultPageName);
    }
}

DockablePanel::~DockablePanel(){
    delete _imp->_undoStack;
    
    ///Delete the knob gui if they weren't before
    ///normally the onKnobDeletion() function should have cleared them
    for(std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _imp->_knobs.begin();it!=_imp->_knobs.end();++it){
        if(it->second){
            KnobHelper* helper = dynamic_cast<KnobHelper*>(it->first.get());
            QObject::disconnect(helper->getSignalSlotHandler().get(),SIGNAL(deleted()),this,SLOT(onKnobDeletion()));
            delete it->second;
        }
    }
}

KnobHolder* DockablePanel::getHolder() const
{
    return _imp->_holder;
}

DockablePanelTabWidget::DockablePanelTabWidget(QWidget* parent)
: QTabWidget(parent)
{
}

QSize DockablePanelTabWidget::sizeHint() const
{
    return currentWidget() ? currentWidget()->sizeHint() + QSize(0,20) : QSize(300,100);
}

QSize DockablePanelTabWidget::minimumSizeHint() const
{
    return currentWidget() ? currentWidget()->minimumSizeHint() + QSize(0,20): QSize(300,100);
}

void DockablePanel::onRestoreDefaultsButtonClicked() {
    
    Natron::StandardButton reply = Natron::questionDialog(tr("Reset").toStdString(), tr("Are you sure you want to restore default settings for this operator ? "
                                                          "This cannot be undone.").toStdString(),Natron::StandardButtons(Natron::Yes | Natron::No),
                                                          Natron::Yes);
    if (reply != Natron::Yes) {
        return;
    }
    boost::shared_ptr<MultiInstancePanel> multiPanel = getMultiInstancePanel();
    if (multiPanel) {
        multiPanel->resetAllInstances();
        return;
    }
    _imp->_holder->restoreDefaultValues();
   
}

void DockablePanel::onLineEditNameEditingFinished() {
    
    if (_imp->_gui->getApp()->isClosing()) {
        return;
    }
    NodeBackDrop* bd = dynamic_cast<NodeBackDrop*>(_imp->_holder);
    if (bd) {
        QString newName = _imp->_nameLineEdit->text();
        if (_imp->_gui->getNodeGraph()->checkIfBackDropNameExists(newName,bd)) {
            Natron::errorDialog(tr("Backdrop name").toStdString(), tr("A backdrop node with the same name already exists in the project.").toStdString());
            _imp->_nameLineEdit->setText(bd->getName());
            return;
        }
        
    }
    
    Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(_imp->_holder);
    if (effect) {
        
        std::string newName = _imp->_nameLineEdit->text().toStdString();
        if (newName.empty()) {
            _imp->_nameLineEdit->blockSignals(true);
            Natron::errorDialog(tr("Node name").toStdString(), tr("A node must have a unique name.").toStdString());
            _imp->_nameLineEdit->setText(effect->getName().c_str());
            _imp->_nameLineEdit->blockSignals(false);
            return;
        }
        
        ///if the node name hasn't changed return
        if (effect->getName() == newName) {
            return;
        }
        
        NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
        assert(nodePanel);
        if (_imp->_gui->getNodeGraph()->checkIfNodeNameExists(newName, nodePanel->getNode().get())) {
            _imp->_nameLineEdit->blockSignals(true);
            Natron::errorDialog(tr("Node name").toStdString(), tr("A node with the same name already exists in the project.").toStdString());
            _imp->_nameLineEdit->setText(effect->getName().c_str());
            _imp->_nameLineEdit->blockSignals(false);
            return;
        }
    }
    emit nameChanged(_imp->_nameLineEdit->text());
}

void DockablePanelPrivate::initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> >& knobs,
                                                QWidget* lastRowWidget,
                                         bool onlyTopLevelKnobs) {
    for(U32 i = 0 ; i < knobs.size(); ++i) {
        
        ///we create only top level knobs, they will in-turn create their children if they have any
        if ((!onlyTopLevelKnobs) || (onlyTopLevelKnobs && !knobs[i]->getParentKnob())) {
            bool makeNewLine = true;
            
            boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knobs[i]);
            
            ////The knob  will have a vector of all other knobs on the same line.
            std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;
            
            if (!isGroup) { //< a knob with children (i.e a group) cannot have children on the same line
                if (i > 0 && knobs[i-1]->isNewLineTurnedOff()) {
                    makeNewLine = false;
                }
                ///find all knobs backward that are on the same line.
                int k = i -1;
                while (k >= 0 && knobs[k]->isNewLineTurnedOff()) {
                    knobsOnSameLine.push_back(knobs[k]);
                    --k;
                }
                
                ///find all knobs forward that are on the same line.
                k = i;
                while (k < (int)(knobs.size() - 1) && knobs[k]->isNewLineTurnedOff()) {
                    knobsOnSameLine.push_back(knobs[k + 1]);
                    ++k;
                }
            }
            
            KnobGui* newGui = findKnobGuiOrCreate(knobs[i],makeNewLine,lastRowWidget,knobsOnSameLine);
            ///childrens cannot be on the same row than their parent
            if (!isGroup && newGui) {
                lastRowWidget = newGui->getFieldContainer();
            }

        }
    }
}

void DockablePanel::initializeKnobsInternal( const std::vector< boost::shared_ptr<KnobI> >& knobs)
{
    _imp->initializeKnobVector(knobs,NULL, false);
    
    ///add all knobs left  to the default page
    
    RotoPanel* roto = initializeRotoPanel();
    if (roto) {
        PageMap::iterator parentTab = _imp->_pages.find(_imp->_defaultPageName);
        ///the top level parent is not a page, i.e the plug-in didn't specify any page
        ///for this param, put it in the first page that is not the default page.
        ///If there is still no page, put it in the default tab.
        for (PageMap::iterator it = _imp->_pages.begin(); it!=_imp->_pages.end(); ++it) {
            if (it->first != _imp->_defaultPageName) {
                parentTab = it;
                break;
            }
        }
        if (parentTab == _imp->_pages.end()) {
            ///find in all knobs a page param (that is not the extra one added by Natron) to set this param into
            for (U32 i = 0; i < knobs.size(); ++i) {
                Page_Knob* p = dynamic_cast<Page_Knob*>(knobs[i].get());
                if (p && p->getDescription() != NATRON_EXTRA_PARAMETER_PAGE_NAME) {
                    parentTab = _imp->addPage(p->getDescription().c_str());
                    break;
                }
            }
            
            ///Last resort: The plug-in didn't specify ANY page, just put it into the default page
            if (parentTab == _imp->_pages.end()) {
                parentTab = _imp->addPage(_imp->_defaultPageName);
            }
        }
        
        assert(parentTab != _imp->_pages.end());
        QFormLayout* layout;
        if (_imp->_useScrollAreasForTabs) {
            layout = dynamic_cast<QFormLayout*>(dynamic_cast<QScrollArea*>(parentTab->second.tab)->widget()->layout());
        } else {
            layout = dynamic_cast<QFormLayout*>(parentTab->second.tab->layout());
        }
        assert(layout);
        layout->addRow(roto);
    }
    
    initializeExtraGui(_imp->_mainLayout);
}

void DockablePanel::initializeKnobs() {
    
    /// function called to create the gui for each knob. It can be called several times in a row
    /// without any damage
    const std::vector< boost::shared_ptr<KnobI> >& knobs = _imp->_holder->getKnobs();
    initializeKnobsInternal(knobs);
}


KnobGui* DockablePanel::getKnobGui(const boost::shared_ptr<KnobI>& knob) const
{
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _imp->_knobs.begin(); it!=_imp->_knobs.end(); ++it) {
        if(it->first == knob){
            return it->second;
        }
    }
    return NULL;
}

KnobGui* DockablePanelPrivate::createKnobGui(const boost::shared_ptr<KnobI> &knob)
{
    std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator found = _knobs.find(knob);
    if (found != _knobs.end()) {
        return found->second;
    }
    
    KnobHelper* helper = dynamic_cast<KnobHelper*>(knob.get());
    QObject::connect(helper->getSignalSlotHandler().get(),SIGNAL(deleted()),_publicInterface,SLOT(onKnobDeletion()));
    
    KnobGui* ret =  appPTR->createGuiForKnob(knob,_publicInterface);
    if (!ret) {
        qDebug() << "Failed to create Knob GUI";
        return NULL;
    }
    _knobs.insert(make_pair(knob, ret));
    return ret;
}

KnobGui* DockablePanelPrivate::findKnobGuiOrCreate(const boost::shared_ptr<KnobI>& knob,
                                                   bool makeNewLine,
                                                   QWidget* lastRowWidget,
                                            const std::vector< boost::shared_ptr< KnobI > >& knobsOnSameLine) {
    
    assert(knob);
    boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knob);
    boost::shared_ptr<Page_Knob> isPage = boost::dynamic_pointer_cast<Page_Knob>(knob);
    
    KnobGui* ret = 0;
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
        if(it->first == knob && it->second) {
            if (isPage) {
                return it->second;
            } else if (isGroup && ((!isGroup->isTab() && it->second->hasWidgetBeenCreated()) || isGroup->isTab())) {
                return it->second;
            } else if (it->second->hasWidgetBeenCreated()) {
                return it->second;
            } else {
                break;
            }
        }
    }
    

    if (isPage) {
        addPage(isPage->getDescription().c_str());
    } else {
        

        
        ret = createKnobGui(knob);
        
        boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
        boost::shared_ptr<Group_Knob> parentIsGroup = boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
        Group_KnobGui* parentGui = 0;
        /// if this knob is within a group, make sure the group is created so far
        if (parentIsGroup) {
            parentGui = dynamic_cast<Group_KnobGui*>(findKnobGuiOrCreate(parentKnob,true,ret->getFieldContainer()));
        }
        

        
        ///if widgets for the KnobGui have already been created, don't the following
        ///For group only create the gui if it is not  a tab.
        if (!ret->hasWidgetBeenCreated() && (!isGroup || !isGroup->isTab())) {
    
            KnobI* parentKnobTmp = parentKnob.get();
            while (parentKnobTmp) {
                boost::shared_ptr<KnobI> parent = parentKnobTmp->getParentKnob();
                if (!parent) {
                    break;
                } else {
                    parentKnobTmp = parent.get();
                }
            }
            
            ////find in which page the knob should be
            Page_Knob* isTopLevelParentAPage = dynamic_cast<Page_Knob*>(parentKnobTmp);
        
            PageMap::iterator page = _pages.end();
            
            if (isTopLevelParentAPage) {
                page = addPage(isTopLevelParentAPage->getDescription().c_str());
            } else {
                ///the top level parent is not a page, i.e the plug-in didn't specify any page
                ///for this param, put it in the first page that is not the default page.
                ///If there is still no page, put it in the default tab.
                for (PageMap::iterator it = _pages.begin(); it!=_pages.end(); ++it) {
                    if (it->first != _defaultPageName) {
                        page = it;
                        break;
                    }
                }
                if (page == _pages.end()) {
                    const std::vector< boost::shared_ptr<KnobI> >& knobs = _holder->getKnobs();
                    ///find in all knobs a page param to set this param into
                    for (U32 i = 0; i < knobs.size(); ++i) {
                        Page_Knob* p = dynamic_cast<Page_Knob*>(knobs[i].get());
                        if (p && p->getDescription() != NATRON_EXTRA_PARAMETER_PAGE_NAME) {
                            page = addPage(p->getDescription().c_str());
                            break;
                        }
                    }
                    
                    ///Last resort: The plug-in didn't specify ANY page, just put it into the default page
                    if (page == _pages.end()) {
                        page = addPage(_defaultPageName);
                    }
                }
            }
            
            assert(page != _pages.end());
            
            ///retrieve the form layout
            QFormLayout* layout;
            if (_useScrollAreasForTabs) {
                layout = dynamic_cast<QFormLayout*>(
                                                    dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout());
            } else {
                layout = dynamic_cast<QFormLayout*>(page->second.tab->layout());
            }
            assert(layout);
            
            
            ///if the knob has specified that it didn't want to trigger a new line, decrement the current row
            /// index of the tab
            
            if (!makeNewLine) {
                --page->second.currentRow;
            }
            
            QWidget* fieldContainer = 0;
            QHBoxLayout* fieldLayout = 0;
            
            if (makeNewLine) {
                ///if new line is not turned off, create a new line
                fieldContainer = new QWidget(page->second.tab);
                fieldLayout = new QHBoxLayout(fieldContainer);
                fieldLayout->setContentsMargins(3,0,0,0);
                fieldLayout->setSpacing(0);
                fieldContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            } else {
                ///otherwise re-use the last row's widget and layout
                assert(lastRowWidget);
                fieldContainer = lastRowWidget;
                fieldLayout = dynamic_cast<QHBoxLayout*>(fieldContainer->layout());
                
                ///the knobs use this value to know whether we should remove the row or not
                //fieldContainer->setObjectName("multi-line");
                
            }
            assert(fieldContainer);
            assert(fieldLayout);
            ClickableLabel* label = new ClickableLabel("",page->second.tab);
            
            
            if (ret->showDescriptionLabel() && label) {
                label->setText_overload(QString(QString(ret->getKnob()->getDescription().c_str()) + ":"));
                QObject::connect(label, SIGNAL(clicked(bool)), ret, SIGNAL(labelClicked(bool)));
            }
            
            
            if (parentIsGroup && parentIsGroup->isTab()) {
                ///The group is a tab, check if the tab widget is created
                if (!page->second.tabWidget) {
                    QFrame* frame = new QFrame(_publicInterface);
                    frame->setFrameShadow(QFrame::Raised);
                    frame->setFrameShape(QFrame::Box);
                    QHBoxLayout* frameLayout = new QHBoxLayout(frame);
                    page->second.tabWidget = new QTabWidget(frame);
                    frameLayout->addWidget(page->second.tabWidget);
                    layout->addRow(frame);
                }
                QString parentTabName(parentIsGroup->getDescription().c_str());
                
                ///now check if the tab exists
                QWidget* tab = 0;
                QFormLayout* tabLayout = 0;
                for (int i = 0; i < page->second.tabWidget->count(); ++i) {
                    if (page->second.tabWidget->tabText(i) == parentTabName) {
                        tab = page->second.tabWidget->widget(i);
                        tabLayout = qobject_cast<QFormLayout*>(tab->layout());
                        break;
                    }
                }
                
                if (!tab) {
                    tab = new QWidget(page->second.tabWidget);
                    tabLayout = new QFormLayout(tab);
                    tabLayout->setContentsMargins(0, 0, 0, 0);
                    tabLayout->setSpacing(3);
                    page->second.tabWidget->addTab(tab,parentTabName);
                }
                
                ret->createGUI(tabLayout,fieldContainer,label,fieldLayout,page->second.currentRow,makeNewLine,knobsOnSameLine);
            } else {
                
                ///fill the fieldLayout with the widgets
                ret->createGUI(layout,fieldContainer,label,fieldLayout,page->second.currentRow,makeNewLine,knobsOnSameLine);
                
            }
            
            ///increment the row count
            ++page->second.currentRow;
            
            /// if this knob is within a group, check that the group is visible, i.e. the toplevel group is unfolded
            if (parentIsGroup) {
                
                assert(parentGui);
                ///FIXME: this offsetColumn is never really used. Shall we use this anyway? It seems
                ///to work fine without it.
                int offsetColumn = knob->determineHierarchySize();
                parentGui->addKnob(ret,page->second.currentRow,offsetColumn);
                
                bool showit = !ret->getKnob()->getIsSecret();
                // see KnobGui::setSecret() for a very similar code
                while (showit && parentIsGroup) {
                    assert(parentGui);
                    // check for secretness and visibility of the group
                    if (parentKnob->getIsSecret() || (parentGui && !parentGui->isChecked())) {
                        showit = false; // one of the including groups is folded, so this item is hidden
                    }
                    // prepare for next loop iteration
                    parentKnob = parentKnob->getParentKnob();
                    parentIsGroup =  boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
                    if (parentKnob) {
                        parentGui = dynamic_cast<Group_KnobGui*>(findKnobGuiOrCreate(parentKnob,true,NULL));
                    }
                }
                if (showit) {
                    ret->show();
                } else {
                    //ret->hide(); // already hidden? please comment if it's not.
                }
            }
        }
    } // !isPage
    
    ///if the knob is a group, create all the children
    if (isGroup) {
        initializeKnobVector(isGroup->getChildren(),lastRowWidget, false);
    } else if (isPage) {
        initializeKnobVector(isPage->getChildren(),lastRowWidget, false);
    }
    
    return ret;
    
}



PageMap::iterator DockablePanelPrivate::addPage(const QString& name)
{
    PageMap::iterator found = _pages.find(name);
    if (found != _pages.end()) {
        return found;
    }
    
    QWidget* newTab;
    QWidget* layoutContainer;
    if (_useScrollAreasForTabs) {
        QScrollArea* sa = new QScrollArea(_tabWidget);
        layoutContainer = new QWidget(sa);
        sa->setWidgetResizable(true);
        sa->setWidget(layoutContainer);
        newTab = sa;
    } else {
        newTab = new QWidget(_tabWidget);
        layoutContainer = newTab;
    }
    newTab->setObjectName(name);
    QFormLayout *tabLayout = new QFormLayout(layoutContainer);
    tabLayout->setObjectName("formLayout");
    layoutContainer->setLayout(tabLayout);
    //tabLayout->setVerticalSpacing(2); // unfortunately, this leaves extra space when parameters are hidden
    tabLayout->setVerticalSpacing(3);
    tabLayout->setContentsMargins(3, 0, 0, 0);
    tabLayout->setHorizontalSpacing(3);
    tabLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    tabLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    tabLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    _tabWidget->addTab(newTab,name);
    Page p;
    p.tab = newTab;
    p.currentRow = 0;
    return _pages.insert(make_pair(name,p)).first;
}

const QUndoCommand* DockablePanel::getLastUndoCommand() const{
    return _imp->_undoStack->command(_imp->_undoStack->index()-1);
}

void DockablePanel::pushUndoCommand(QUndoCommand* cmd){
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(cmd);
    if(_imp->_undoButton && _imp->_redoButton){
        _imp->_undoButton->setEnabled(_imp->_undoStack->canUndo());
        _imp->_redoButton->setEnabled(_imp->_undoStack->canRedo());
    }
}

void DockablePanel::onUndoClicked(){
    _imp->_undoStack->undo();
    if(_imp->_undoButton && _imp->_redoButton){
        _imp->_undoButton->setEnabled(_imp->_undoStack->canUndo());
        _imp->_redoButton->setEnabled(_imp->_undoStack->canRedo());
    }
    emit undoneChange();
}

void DockablePanel::onRedoPressed(){
    _imp->_undoStack->redo();
    if(_imp->_undoButton && _imp->_redoButton){
        _imp->_undoButton->setEnabled(_imp->_undoStack->canUndo());
        _imp->_redoButton->setEnabled(_imp->_undoStack->canRedo());
    }
    emit redoneChange();
}

void DockablePanel::showHelp(){
    QToolTip::showText(QCursor::pos(), _imp->_helpButton->toolTip());
}

void DockablePanel::setClosed2(bool c,bool setTimelineKeys)
{
    if (_imp->_floating) {
        floatPanel();
    }
    setVisible(!c);
    {
        QMutexLocker l(&_imp->_isClosedMutex);
        if (c == _imp->_isClosed) {
            return;
        }
        _imp->_isClosed = c;
    }
    emit closeChanged(c);
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (nodePanel && setTimelineKeys) {
        if (c) {
            nodePanel->getNode()->getNode()->hideKeyframesFromTimeline(true);
        } else {
            nodePanel->getNode()->getNode()->showKeyframesOnTimeline(true);
        }
    }
    if (!c) {
        _imp->_gui->addVisibleDockablePanel(this);
    } else {
        _imp->_gui->removeVisibleDockablePanel(this);
    }
}

void DockablePanel::closePanel() {
    if (_imp->_floating) {
        floatPanel();
    }
    close();
    {
        QMutexLocker l(&_imp->_isClosedMutex);
        _imp->_isClosed = true;
    }
    emit closeChanged(true);
    _imp->_gui->removeVisibleDockablePanel(this);
    
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (nodePanel) {
        nodePanel->getNode()->getNode()->hideKeyframesFromTimeline(true);
    }
    getGui()->getApp()->redrawAllViewers();
    
}
void DockablePanel::minimizeOrMaximize(bool toggled){
    _imp->_minimized=toggled;
    if(_imp->_minimized){
        emit minimized();
    }else{
        emit maximized();
    }
    _imp->_tabWidget->setVisible(!_imp->_minimized);
    std::vector<QWidget*> _panels;
    for(int i =0 ; i < _imp->_container->count(); ++i) {
        if (QWidget *myItem = dynamic_cast <QWidget*>(_imp->_container->itemAt(i))){
            _panels.push_back(myItem);
            _imp->_container->removeWidget(myItem);
        }
    }
    for (U32 i =0 ; i < _panels.size(); ++i) {
        _imp->_container->addWidget(_panels[i]);
    }
    update();
}

void DockablePanel::floatPanel() {
    _imp->_floating = !_imp->_floating;
    if (_imp->_floating) {
        assert(!_imp->_floatingWidget);
        _imp->_floatingWidget = new FloatingWidget(_imp->_gui);
        QObject::connect(_imp->_floatingWidget,SIGNAL(closed()),this,SLOT(closePanel()));
        _imp->_container->removeWidget(this);
        _imp->_floatingWidget->setWidget(size(),this);
    } else {
        assert(_imp->_floatingWidget);
        _imp->_floatingWidget->removeWidget();
        setParent(_imp->_container->parentWidget());
        _imp->_container->insertWidget(1, this);
        delete _imp->_floatingWidget;
        _imp->_floatingWidget = 0;
    }
}


void DockablePanel::onNameChanged(const QString& str){
    if(_imp->_nameLabel){
        _imp->_nameLabel->setText(str);
    }else if(_imp->_nameLineEdit){
        _imp->_nameLineEdit->setText(str);
    }
}


Button* DockablePanel::insertHeaderButton(int headerPosition){
    Button* ret = new Button(_imp->_headerWidget);
    _imp->_headerLayout->insertWidget(headerPosition, ret);
    return ret;
}

void DockablePanel::onKnobDeletion(){
    
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (handler) {
        for(std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin();it!=_imp->_knobs.end();++it){
            KnobHelper* helper = dynamic_cast<KnobHelper*>(it->first.get());
            if (helper->getSignalSlotHandler().get() == handler) {
                if(it->second){
                    delete it->second;
                }
                _imp->_knobs.erase(it);
                return;
            }
        }
    }
    
}


Gui* DockablePanel::getGui() const {
    return _imp->_gui;
}

void DockablePanel::insertHeaderWidget(int index,QWidget* widget) {
    if (_imp->_mode != NO_HEADER) {
        _imp->_headerLayout->insertWidget(index, widget);
    }
}

void DockablePanel::appendHeaderWidget(QWidget* widget) {
    if (_imp->_mode != NO_HEADER) {
        _imp->_headerLayout->addWidget(widget);
    }
}

QWidget* DockablePanel::getHeaderWidget() const {
    return _imp->_headerWidget;
}

bool DockablePanel::isMinimized() const {return _imp->_minimized;}

const std::map<boost::shared_ptr<KnobI>,KnobGui*>& DockablePanel::getKnobs() const { return _imp->_knobs; }

QVBoxLayout* DockablePanel::getContainer() const {return _imp->_container;}

QUndoStack* DockablePanel::getUndoStack() const { return _imp->_undoStack; }

bool DockablePanel::isClosed() const { QMutexLocker l(&_imp->_isClosedMutex); return _imp->_isClosed; }

bool DockablePanel::isFloating() const { return _imp->_floating; }

void DockablePanel::onColorDialogColorChanged(const QColor& color)
{
    if (_imp->_mode != READ_ONLY_NAME) {
        QPixmap p(15,15);
        p.fill(color);
        _imp->_colorButton->setIcon(QIcon(p));
    }
}

void DockablePanel::onColorButtonClicked()
{
    QColorDialog dialog(this);
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        dialog.setCurrentColor(_imp->_currentColor);
    }
    QObject::connect(&dialog,SIGNAL(currentColorChanged(QColor)),this,SLOT(onColorDialogColorChanged(QColor)));
    if (dialog.exec()) {
        QColor c = dialog.currentColor();
        {
            QMutexLocker locker(&_imp->_currentColorMutex);
            _imp->_currentColor = c;
        }

        emit colorChanged(c);
    }
}

QColor DockablePanel::getCurrentColor() const
{
    QMutexLocker locker(&_imp->_currentColorMutex);
    return _imp->_currentColor;
}

void DockablePanel::setCurrentColor(const QColor& c)
{
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        _imp->_currentColor = c;
    }
    onColorDialogColorChanged(c);
}

NodeSettingsPanel::NodeSettingsPanel(const boost::shared_ptr<MultiInstancePanel>& multiPanel,
                                     Gui* gui,boost::shared_ptr<NodeGui> NodeUi ,QVBoxLayout* container,QWidget *parent)
: DockablePanel(gui,
                multiPanel.get() != NULL ? dynamic_cast<KnobHolder*>(multiPanel.get()) : NodeUi->getNode()->getLiveInstance(),
               container,
               DockablePanel::FULLY_FEATURED,
               false,
               NodeUi->getNode()->getName().c_str(),
               NodeUi->getNode()->description().c_str(),
               false,
               "Settings",
               parent)
, _nodeGUI(NodeUi)
, _multiPanel(multiPanel)
{
    if (multiPanel) {
        multiPanel->initializeKnobsPublic();
    }
    
    QPixmap pixC;
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER,&pixC);
    _centerNodeButton = new Button(QIcon(pixC),"",getHeaderWidget());
    _centerNodeButton->setToolTip(tr("Centers the node graph on this node."));
    _centerNodeButton->setFixedSize(15, 15);
    QObject::connect(_centerNodeButton,SIGNAL(clicked()),this,SLOT(centerNode()));
    insertHeaderWidget(0, _centerNodeButton);
    QObject::connect(this,SIGNAL(closeChanged(bool)),NodeUi.get(),SLOT(onSettingsPanelClosedChanged(bool)));
}

NodeSettingsPanel::~NodeSettingsPanel(){
    _nodeGUI->removeSettingsPanel();
}


void NodeSettingsPanel::setSelected(bool s){
    _selected = s;
    style()->unpolish(this);
    style()->polish(this);
}

void NodeSettingsPanel::centerNode() {
    _nodeGUI->centerGraphOnIt();
}

RotoPanel* NodeSettingsPanel::initializeRotoPanel()
{
    if (_nodeGUI->getNode()->isRotoNode()) {
        return new RotoPanel(_nodeGUI.get(),this);
    } else {
        return NULL;
    }
}

void NodeSettingsPanel::initializeExtraGui(QVBoxLayout* layout)
{
    if (_multiPanel && !_multiPanel->isGuiCreated()) {
        _multiPanel->createMultiInstanceGui(layout);
    }
}
