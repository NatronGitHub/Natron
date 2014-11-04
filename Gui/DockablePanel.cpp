//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "DockablePanel.h"

#include <iostream>
#include <fstream>
#include <QLayout>
#include <QAction>
#include <QApplication>
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
#include <QMenu>

#include <ofxNatron.h>

CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/utility.hpp>
CLANG_DIAG_ON(unused-parameter)

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"
#include "Engine/NodeSerialization.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiTypes.h" // for Group_KnobGui
#include "Gui/KnobGuiFactory.h"
#include "Gui/LineEdit.h"
#include "Gui/Button.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/RotoPanel.h"
#include "Gui/NodeBackDrop.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/GuiMacros.h"

using std::make_pair;
using namespace Natron;

namespace {
struct Page
{
    QWidget* tab;
    int currentRow;
    QTabWidget* tabWidget; //< to gather group knobs that are set as a tab

    Page()
        : tab(0), currentRow(0),tabWidget(0)
    {
    }

    Page(const Page & other)
        : tab(other.tab), currentRow(other.currentRow), tabWidget(other.tabWidget)
    {
    }
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
    Button* _centerNodeButton;
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
                         ,
                         Gui* gui
                         ,
                         KnobHolder* holder
                         ,
                         QVBoxLayout* container
                         ,
                         DockablePanel::HeaderMode headerMode
                         ,
                         bool useScrollAreasForTabs
                         ,
                         const QString & defaultPageName)
        : _publicInterface(publicI)
          ,_gui(gui)
          ,_container(container)
          ,_mainLayout(NULL)
          ,_headerWidget(NULL)
          ,_headerLayout(NULL)
          ,_nameLineEdit(NULL)
          ,_nameLabel(NULL)
          ,_tabWidget(NULL)
          , _centerNodeButton(NULL)
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
    PageMap::iterator addPage(const QString & name);


    void initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> > & knobs,
                              QWidget* lastRowWidget,
                              bool onlyTopLevelKnobs);

    KnobGui* createKnobGui(const boost::shared_ptr<KnobI> &knob);

    /*Search an existing knob GUI in the map, otherwise creates
       the gui for the knob.*/
    KnobGui* findKnobGuiOrCreate( const boost::shared_ptr<KnobI> &knob,
                                  bool makeNewLine,
                                  QWidget* lastRowWidget,
                                  const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine = std::vector< boost::shared_ptr< KnobI > >() );
};



DockablePanel::DockablePanel(Gui* gui
                             ,
                             KnobHolder* holder
                             ,
                             QVBoxLayout* container
                             ,
                             HeaderMode headerMode
                             ,
                             bool useScrollAreasForTabs
                             ,
                             const QString & initialName
                             ,
                             const QString & helpToolTip
                             ,
                             bool createDefaultPage
                             ,
                             const QString & defaultPageName
                             ,
                             QWidget *parent)
    : QFrame(parent)
      , _imp( new DockablePanelPrivate(this,gui,holder,container,headerMode,useScrollAreasForTabs,defaultPageName) )

{
    _imp->_mainLayout = new QVBoxLayout(this);
    _imp->_mainLayout->setSpacing(0);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_imp->_mainLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameShape(QFrame::Box);

    if (headerMode != NO_HEADER) {
        _imp->_headerWidget = new QFrame(this);
        _imp->_headerWidget->setFrameShape(QFrame::Box);
        _imp->_headerLayout = new QHBoxLayout(_imp->_headerWidget);
        _imp->_headerLayout->setContentsMargins(0, 0, 0, 0);
        _imp->_headerLayout->setSpacing(2);
        _imp->_headerWidget->setLayout(_imp->_headerLayout);
        
        if (!holder->isProject()) {
            QPixmap pixCenter;
            appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER,&pixCenter);
            _imp->_centerNodeButton = new Button( QIcon(pixCenter),"",getHeaderWidget() );
            _imp->_centerNodeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_centerNodeButton->setToolTip( tr("Centers the node graph on this item.") );
            _imp->_centerNodeButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_centerNodeButton,SIGNAL( clicked() ),this,SLOT( onCenterButtonClicked() ) );
            _imp->_headerLayout->addWidget(_imp->_centerNodeButton);
        }
        
        QPixmap pixHelp;
        appPTR->getIcon(NATRON_PIXMAP_HELP_WIDGET,&pixHelp);
        _imp->_helpButton = new Button(QIcon(pixHelp),"",_imp->_headerWidget);
        _imp->_helpButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
        _imp->_helpButton->setFocusPolicy(Qt::NoFocus);
        if ( !helpToolTip.isEmpty() ) {
            _imp->_helpButton->setToolTip( Qt::convertFromPlainText(helpToolTip, Qt::WhiteSpaceNormal) );
        }
        QObject::connect( _imp->_helpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
        QPixmap pixM;
        appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET,&pixM);

        QPixmap pixC;
        appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);

        QPixmap pixF;
        appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, &pixF);

        _imp->_minimize = new Button(QIcon(pixM),"",_imp->_headerWidget);
        _imp->_minimize->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
        _imp->_minimize->setCheckable(true);
        _imp->_minimize->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_minimize,SIGNAL( toggled(bool) ),this,SLOT( minimizeOrMaximize(bool) ) );

        _imp->_floatButton = new Button(QIcon(pixF),"",_imp->_headerWidget);
        _imp->_floatButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
        _imp->_floatButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_floatButton,SIGNAL( clicked() ),this,SLOT( floatPanel() ) );


        _imp->_cross = new Button(QIcon(pixC),"",_imp->_headerWidget);
        _imp->_cross->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
        _imp->_cross->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_cross,SIGNAL( clicked() ),this,SLOT( closePanel() ) );

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
                iseffect->getPluginGrouping(&grouping);
                std::string majGroup = grouping.empty() ? "" : grouping.front();

                if ( iseffect->isReader() ) {
                    settings->getReaderColor(&r, &g, &b);
                } else if ( iseffect->isWriter() ) {
                    settings->getWriterColor(&r, &g, &b);
                } else if ( iseffect->isGenerator() ) {
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
            } else if (backdrop) {
                appPTR->getCurrentSettings()->getDefaultBackDropColor(&r, &g, &b);
            } else {
                r = g = b = 0.6;
            }
            
            
            _imp->_currentColor.setRgbF( Natron::clamp(r), Natron::clamp(g), Natron::clamp(b) );
            QPixmap p(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
            p.fill(_imp->_currentColor);

            
            _imp->_colorButton = new Button(QIcon(p),"",_imp->_headerWidget);
            _imp->_colorButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
            _imp->_colorButton->setToolTip( Qt::convertFromPlainText(tr("Set here the color of the node in the nodegraph. "
                                                                        "By default the color of the node is the one set in the "
                                                                        "preferences of %1").arg(NATRON_APPLICATION_NAME)
                                                                     ,Qt::WhiteSpaceNormal) );
            _imp->_colorButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_colorButton,SIGNAL( clicked() ),this,SLOT( onColorButtonClicked() ) );

            if ( iseffect && !iseffect->getNode()->isMultiInstance() ) {
                ///Show timeline keyframe markers to be consistent with the fact that the panel is opened by default
                iseffect->getNode()->showKeyframesOnTimeline(true);
            }
        }
        QPixmap pixUndo;
        appPTR->getIcon(NATRON_PIXMAP_UNDO,&pixUndo);
        QPixmap pixUndo_gray;
        appPTR->getIcon(NATRON_PIXMAP_UNDO_GRAYSCALE,&pixUndo_gray);
        QIcon icUndo;
        icUndo.addPixmap(pixUndo,QIcon::Normal);
        icUndo.addPixmap(pixUndo_gray,QIcon::Disabled);
        _imp->_undoButton = new Button(icUndo,"",_imp->_headerWidget);
        _imp->_undoButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
        _imp->_undoButton->setToolTip( Qt::convertFromPlainText(tr("Undo the last change made to this operator"), Qt::WhiteSpaceNormal) );
        _imp->_undoButton->setEnabled(false);
        _imp->_undoButton->setFocusPolicy(Qt::NoFocus);
        QPixmap pixRedo;
        appPTR->getIcon(NATRON_PIXMAP_REDO,&pixRedo);
        QPixmap pixRedo_gray;
        appPTR->getIcon(NATRON_PIXMAP_REDO_GRAYSCALE,&pixRedo_gray);
        QIcon icRedo;
        icRedo.addPixmap(pixRedo,QIcon::Normal);
        icRedo.addPixmap(pixRedo_gray,QIcon::Disabled);
        _imp->_redoButton = new Button(icRedo,"",_imp->_headerWidget);
        _imp->_redoButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
        _imp->_redoButton->setToolTip( Qt::convertFromPlainText(tr("Redo the last change undone to this operator"), Qt::WhiteSpaceNormal) );
        _imp->_redoButton->setEnabled(false);
        _imp->_redoButton->setFocusPolicy(Qt::NoFocus);

        QPixmap pixRestore;
        appPTR->getIcon(NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED, &pixRestore);
        QIcon icRestore;
        icRestore.addPixmap(pixRestore);
        _imp->_restoreDefaultsButton = new Button(icRestore,"",_imp->_headerWidget);
        _imp->_restoreDefaultsButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
        _imp->_restoreDefaultsButton->setToolTip( Qt::convertFromPlainText(tr("Restore default values for this operator."
                                                                              " This cannot be undone!"),Qt::WhiteSpaceNormal) );
        _imp->_restoreDefaultsButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_restoreDefaultsButton,SIGNAL( clicked() ),this,SLOT( onRestoreDefaultsButtonClicked() ) );
        QObject::connect( _imp->_undoButton, SIGNAL( clicked() ),this, SLOT( onUndoClicked() ) );
        QObject::connect( _imp->_redoButton, SIGNAL( clicked() ),this, SLOT( onRedoPressed() ) );

        if (headerMode != READ_ONLY_NAME) {
            _imp->_nameLineEdit = new LineEdit(_imp->_headerWidget);
            _imp->_nameLineEdit->setText(initialName);
            QObject::connect( _imp->_nameLineEdit,SIGNAL( editingFinished() ),this,SLOT( onLineEditNameEditingFinished() ) );
            _imp->_headerLayout->addWidget(_imp->_nameLineEdit);
        } else {
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

    if (createDefaultPage) {
        _imp->addPage(defaultPageName);
    }
}

DockablePanel::~DockablePanel()
{
    delete _imp->_undoStack;

    ///Delete the knob gui if they weren't before
    ///normally the onKnobDeletion() function should have cleared them
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->second) {
            KnobHelper* helper = dynamic_cast<KnobHelper*>( it->first.get() );
            QObject::disconnect( helper->getSignalSlotHandler().get(),SIGNAL( deleted() ),this,SLOT( onKnobDeletion() ) );
            it->first->setKnobGuiPointer(0);
            it->second->deleteLater();
        }
    }
}

void
DockablePanel::onGuiClosing()
{
    if (_imp->_nameLineEdit) {
        QObject::disconnect( _imp->_nameLineEdit,SIGNAL( editingFinished() ),this,SLOT( onLineEditNameEditingFinished() ) );
    }
    _imp->_gui = 0;
}

KnobHolder*
DockablePanel::getHolder() const
{
    return _imp->_holder;
}

DockablePanelTabWidget::DockablePanelTabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

QSize
DockablePanelTabWidget::sizeHint() const
{
    return currentWidget() ? currentWidget()->sizeHint() + QSize(0,20) : QSize(300,100);
}

QSize
DockablePanelTabWidget::minimumSizeHint() const
{
    return currentWidget() ? currentWidget()->minimumSizeHint() + QSize(0,20) : QSize(300,100);
}

void
DockablePanel::onRestoreDefaultsButtonClicked()
{
    std::list<boost::shared_ptr<KnobI> > knobsList;
    boost::shared_ptr<MultiInstancePanel> multiPanel = getMultiInstancePanel();

    if (multiPanel) {
        const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> > & instances = multiPanel->getInstances();
        for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            const std::vector<boost::shared_ptr<KnobI> > & knobs = it->first->getKnobs();
            for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
                Button_Knob* isBtn = dynamic_cast<Button_Knob*>( it2->get() );
                Page_Knob* isPage = dynamic_cast<Page_Knob*>( it2->get() );
                Group_Knob* isGroup = dynamic_cast<Group_Knob*>( it2->get() );
                Separator_Knob* isSeparator = dynamic_cast<Separator_Knob*>( it2->get() );
                if ( !isBtn && !isPage && !isGroup && !isSeparator && ( (*it2)->getName() != kUserLabelKnobName ) &&
                     ( (*it2)->getName() != kOfxParamStringSublabelName ) ) {
                    knobsList.push_back(*it2);
                }
            }
        }
        multiPanel->clearSelection();
    } else {
        const std::vector<boost::shared_ptr<KnobI> > & knobs = _imp->_holder->getKnobs();
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            Button_Knob* isBtn = dynamic_cast<Button_Knob*>( it->get() );
            Page_Knob* isPage = dynamic_cast<Page_Knob*>( it->get() );
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>( it->get() );
            Separator_Knob* isSeparator = dynamic_cast<Separator_Knob*>( it->get() );
            if ( !isBtn && !isPage && !isGroup && !isSeparator && ( (*it)->getName() != kUserLabelKnobName ) &&
                 ( (*it)->getName() != kOfxParamStringSublabelName ) ) {
                knobsList.push_back(*it);
            }
        }
    }
    pushUndoCommand( new RestoreDefaultsCommand(knobsList) );
}

void
DockablePanel::onLineEditNameEditingFinished()
{
    if ( _imp->_gui->getApp()->isClosing() ) {
        return;
    }
    
    NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>(this);
    boost::shared_ptr<NodeGui> node;
    if (panel) {
        node = panel->getNode();
    }
    NodeBackDrop* bd = dynamic_cast<NodeBackDrop*>(_imp->_holder);
    assert(node || bd);
    pushUndoCommand(new RenameNodeUndoRedoCommand(node,bd,_imp->_nameLineEdit->text()));
   
}

void
DockablePanelPrivate::initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> > & knobs,
                                           QWidget* lastRowWidget,
                                           bool onlyTopLevelKnobs)
{
    for (U32 i = 0; i < knobs.size(); ++i) {
        ///we create only top level knobs, they will in-turn create their children if they have any
        if ( (!onlyTopLevelKnobs) || ( onlyTopLevelKnobs && !knobs[i]->getParentKnob() ) ) {
            bool makeNewLine = true;
            boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knobs[i]);

            ////The knob  will have a vector of all other knobs on the same line.
            std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;

            if (!isGroup) { //< a knob with children (i.e a group) cannot have children on the same line
                if ( (i > 0) && knobs[i - 1]->isNewLineTurnedOff() ) {
                    makeNewLine = false;
                }
                ///find all knobs backward that are on the same line.
                int k = i - 1;
                while ( k >= 0 && knobs[k]->isNewLineTurnedOff() ) {
                    knobsOnSameLine.push_back(knobs[k]);
                    --k;
                }

                ///find all knobs forward that are on the same line.
                k = i;
                while ( k < (int)(knobs.size() - 1) && knobs[k]->isNewLineTurnedOff() ) {
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

void
DockablePanel::initializeKnobsInternal( const std::vector< boost::shared_ptr<KnobI> > & knobs)
{
    _imp->initializeKnobVector(knobs,NULL, false);

    ///add all knobs left  to the default page

    RotoPanel* roto = initializeRotoPanel();
    if (roto) {
        PageMap::iterator parentTab = _imp->_pages.find(_imp->_defaultPageName);
        ///the top level parent is not a page, i.e the plug-in didn't specify any page
        ///for this param, put it in the first page that is not the default page.
        ///If there is still no page, put it in the default tab.
        for (PageMap::iterator it = _imp->_pages.begin(); it != _imp->_pages.end(); ++it) {
            if (it->first != _imp->_defaultPageName) {
                parentTab = it;
                break;
            }
        }
        if ( parentTab == _imp->_pages.end() ) {
            ///find in all knobs a page param (that is not the extra one added by Natron) to set this param into
            for (U32 i = 0; i < knobs.size(); ++i) {
                Page_Knob* p = dynamic_cast<Page_Knob*>( knobs[i].get() );
                if ( p && (p->getDescription() != NATRON_EXTRA_PARAMETER_PAGE_NAME) ) {
                    parentTab = _imp->addPage( p->getDescription().c_str() );
                    break;
                }
            }

            ///Last resort: The plug-in didn't specify ANY page, just put it into the default page
            if ( parentTab == _imp->_pages.end() ) {
                parentTab = _imp->addPage(_imp->_defaultPageName);
            }
        }

        assert( parentTab != _imp->_pages.end() );
        QFormLayout* layout;
        if (_imp->_useScrollAreasForTabs) {
            layout = dynamic_cast<QFormLayout*>( dynamic_cast<QScrollArea*>(parentTab->second.tab)->widget()->layout() );
        } else {
            layout = dynamic_cast<QFormLayout*>( parentTab->second.tab->layout() );
        }
        assert(layout);
        layout->addRow(roto);
    }

    initializeExtraGui(_imp->_mainLayout);
}

void
DockablePanel::initializeKnobs()
{
    /// function called to create the gui for each knob. It can be called several times in a row
    /// without any damage
    const std::vector< boost::shared_ptr<KnobI> > & knobs = _imp->_holder->getKnobs();

    initializeKnobsInternal(knobs);
}

KnobGui*
DockablePanel::getKnobGui(const boost::shared_ptr<KnobI> & knob) const
{
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->first == knob) {
            return it->second;
        }
    }

    return NULL;
}

KnobGui*
DockablePanelPrivate::createKnobGui(const boost::shared_ptr<KnobI> &knob)
{
    std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator found = _knobs.find(knob);

    if ( found != _knobs.end() ) {
        return found->second;
    }

    KnobHelper* helper = dynamic_cast<KnobHelper*>( knob.get() );
    QObject::connect( helper->getSignalSlotHandler().get(),SIGNAL( deleted() ),_publicInterface,SLOT( onKnobDeletion() ) );
    KnobGui* ret =  appPTR->createGuiForKnob(knob,_publicInterface);
    if (!ret) {
        qDebug() << "Failed to create Knob GUI";

        return NULL;
    }
    _knobs.insert( make_pair(knob, ret) );

    return ret;
}

KnobGui*
DockablePanelPrivate::findKnobGuiOrCreate(const boost::shared_ptr<KnobI> & knob,
                                          bool makeNewLine,
                                          QWidget* lastRowWidget,
                                          const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine)
{
    assert(knob);
    boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knob);
    boost::shared_ptr<Page_Knob> isPage = boost::dynamic_pointer_cast<Page_Knob>(knob);
    KnobGui* ret = 0;
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        if ( (it->first == knob) && it->second ) {
            if (isPage) {
                return it->second;
            } else if ( isGroup && ( ( !isGroup->isTab() && it->second->hasWidgetBeenCreated() ) || isGroup->isTab() ) ) {
                return it->second;
            } else if ( it->second->hasWidgetBeenCreated() ) {
                return it->second;
            } else {
                break;
            }
        }
    }


    if (isPage) {
        addPage( isPage->getDescription().c_str() );
    } else {
        ret = createKnobGui(knob);

        boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
        boost::shared_ptr<Group_Knob> parentIsGroup = boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
        Group_KnobGui* parentGui = 0;
        /// if this knob is within a group, make sure the group is created so far
        if (parentIsGroup) {
            parentGui = dynamic_cast<Group_KnobGui*>( findKnobGuiOrCreate( parentKnob,true,ret->getFieldContainer() ) );
        }


        ///if widgets for the KnobGui have already been created, don't the following
        ///For group only create the gui if it is not  a tab.
        if ( !ret->hasWidgetBeenCreated() && ( !isGroup || !isGroup->isTab() ) ) {
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
                page = addPage( isTopLevelParentAPage->getDescription().c_str() );
            } else {
                ///the top level parent is not a page, i.e the plug-in didn't specify any page
                ///for this param, put it in the first page that is not the default page.
                ///If there is still no page, put it in the default tab.
                for (PageMap::iterator it = _pages.begin(); it != _pages.end(); ++it) {
                    if (it->first != _defaultPageName) {
                        page = it;
                        break;
                    }
                }
                if ( page == _pages.end() ) {
                    const std::vector< boost::shared_ptr<KnobI> > & knobs = _holder->getKnobs();
                    ///find in all knobs a page param to set this param into
                    for (U32 i = 0; i < knobs.size(); ++i) {
                        Page_Knob* p = dynamic_cast<Page_Knob*>( knobs[i].get() );
                        if ( p && (p->getDescription() != NATRON_EXTRA_PARAMETER_PAGE_NAME) ) {
                            page = addPage( p->getDescription().c_str() );
                            break;
                        }
                    }

                    ///Last resort: The plug-in didn't specify ANY page, just put it into the default page
                    if ( page == _pages.end() ) {
                        page = addPage(_defaultPageName);
                    }
                }
            }

            assert( page != _pages.end() );

            ///retrieve the form layout
            QFormLayout* layout;
            if (_useScrollAreasForTabs) {
                layout = dynamic_cast<QFormLayout*>(
                    dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout() );
            } else {
                layout = dynamic_cast<QFormLayout*>( page->second.tab->layout() );
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
                fieldLayout = dynamic_cast<QHBoxLayout*>( fieldContainer->layout() );

                ///the knobs use this value to know whether we should remove the row or not
                //fieldContainer->setObjectName("multi-line");
            }
            assert(fieldContainer);
            assert(fieldLayout);
            ClickableLabel* label = new ClickableLabel("",page->second.tab);


            if (ret->showDescriptionLabel() && !knob->getDescription().empty() && label) {
                label->setText_overload( QString(QString( ret->getKnob()->getDescription().c_str() ) + ":") );
                QObject::connect( label, SIGNAL( clicked(bool) ), ret, SIGNAL( labelClicked(bool) ) );
            }


            if ( parentIsGroup && parentIsGroup->isTab() ) {
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
                QString parentTabName( parentIsGroup->getDescription().c_str() );

                ///now check if the tab exists
                QWidget* tab = 0;
                QFormLayout* tabLayout = 0;
                for (int i = 0; i < page->second.tabWidget->count(); ++i) {
                    if (page->second.tabWidget->tabText(i) == parentTabName) {
                        tab = page->second.tabWidget->widget(i);
                        tabLayout = qobject_cast<QFormLayout*>( tab->layout() );
                        break;
                    }
                }

                if (!tab) {
                    tab = new QWidget(page->second.tabWidget);
                    tabLayout = new QFormLayout(tab);
                    tabLayout->setContentsMargins(0, 0, 0, 0);
                    tabLayout->setSpacing(0); // unfortunately, this leaves extra space when parameters are hidden
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
                    if ( parentKnob->getIsSecret() || ( parentGui && !parentGui->isChecked() ) ) {
                        showit = false; // one of the including groups is folded, so this item is hidden
                    }
                    // prepare for next loop iteration
                    parentKnob = parentKnob->getParentKnob();
                    parentIsGroup =  boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
                    if (parentKnob) {
                        parentGui = dynamic_cast<Group_KnobGui*>( findKnobGuiOrCreate(parentKnob,true,NULL) );
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
} // findKnobGuiOrCreate

void
RightClickableWidget::mousePressEvent(QMouseEvent* e)
{
    if (buttonDownIsRight(e)) {
        QWidget* underMouse = qApp->widgetAt(e->globalPos());
        if (underMouse == this) {
            emit rightClicked(e->pos());
            e->accept();
        }
    } else {
        QWidget::mousePressEvent(e);
    }
}

void
RightClickableWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        emit escapePressed();
    }
    QWidget::keyPressEvent(e);
}

void
RightClickableWidget::enterEvent(QEvent* e)
{
    //setFocus();
    QWidget::enterEvent(e);
}

PageMap::iterator
DockablePanelPrivate::addPage(const QString & name)
{
    PageMap::iterator found = _pages.find(name);

    if ( found != _pages.end() ) {
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
        RightClickableWidget* clickableWidget = new RightClickableWidget(_publicInterface,_tabWidget);
        QObject::connect(clickableWidget,SIGNAL(rightClicked(QPoint)),_publicInterface,SLOT( onRightClickMenuRequested(QPoint) ) );
        QObject::connect(clickableWidget,SIGNAL(escapePressed()),_publicInterface,SLOT( closePanel() ) );
        newTab = clickableWidget;
        layoutContainer = newTab;
    }
    QFormLayout *tabLayout = new QFormLayout(layoutContainer);
    tabLayout->setObjectName("formLayout");
    layoutContainer->setLayout(tabLayout);
    tabLayout->setContentsMargins(3, 0, 0, 0);
    tabLayout->setSpacing(3); // unfortunately, this leaves extra space when parameters are hidden
    tabLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    tabLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    tabLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    _tabWidget->addTab(newTab,name);
    Page p;
    p.tab = newTab;
    p.currentRow = 0;

    return _pages.insert( make_pair(name,p) ).first;
}

const QUndoCommand*
DockablePanel::getLastUndoCommand() const
{
    return _imp->_undoStack->command(_imp->_undoStack->index() - 1);
}

void
DockablePanel::pushUndoCommand(QUndoCommand* cmd)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(cmd);
    if (_imp->_undoButton && _imp->_redoButton) {
        _imp->_undoButton->setEnabled( _imp->_undoStack->canUndo() );
        _imp->_redoButton->setEnabled( _imp->_undoStack->canRedo() );
    }
}

void
DockablePanel::onUndoClicked()
{
    _imp->_undoStack->undo();
    if (_imp->_undoButton && _imp->_redoButton) {
        _imp->_undoButton->setEnabled( _imp->_undoStack->canUndo() );
        _imp->_redoButton->setEnabled( _imp->_undoStack->canRedo() );
    }
    emit undoneChange();
}

void
DockablePanel::onRedoPressed()
{
    _imp->_undoStack->redo();
    if (_imp->_undoButton && _imp->_redoButton) {
        _imp->_undoButton->setEnabled( _imp->_undoStack->canUndo() );
        _imp->_redoButton->setEnabled( _imp->_undoStack->canRedo() );
    }
    emit redoneChange();
}

void
DockablePanel::showHelp()
{
    QToolTip::showText( QCursor::pos(), _imp->_helpButton->toolTip() );
}

void
DockablePanel::setClosed(bool c)
{
    if (!_imp->_gui) {
        return;
    }
    if (!c) {
        _imp->_gui->addVisibleDockablePanel(this);
    } else {
        _imp->_gui->removeVisibleDockablePanel(this);
    }
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
    if (nodePanel) {
        boost::shared_ptr<Natron::Node> internalNode = nodePanel->getNode()->getNode();
        boost::shared_ptr<MultiInstancePanel> panel = getMultiInstancePanel();

        if (panel) {
            ///show all selected instances
            const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> > & childrenInstances = panel->getInstances();
            std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator next = childrenInstances.begin();
			if (!childrenInstances.empty()) {
				++next;
			}
            for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = childrenInstances.begin();
                 it != childrenInstances.end(); ++it,++next) {
                if (c) {
                    it->first->hideKeyframesFromTimeline( next == childrenInstances.end() );
                } else if (!c && it->second) {
                    it->first->showKeyframesOnTimeline( next == childrenInstances.end() );
                }
				if (next == childrenInstances.end()) {
					--next;
				}
            }
        } else {
            ///Regular show/hide
            if (c) {
                internalNode->hideKeyframesFromTimeline(true);
            } else {
                internalNode->showKeyframesOnTimeline(true);
            }
        }
    }
} // setClosed

void
DockablePanel::closePanel()
{
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
        boost::shared_ptr<Natron::Node> internalNode = nodePanel->getNode()->getNode();
        internalNode->hideKeyframesFromTimeline(true);
        boost::shared_ptr<MultiInstancePanel> panel = getMultiInstancePanel();
        if (panel) {
            const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> > & childrenInstances = panel->getInstances();
            std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator next = childrenInstances.begin();
            ++next;
            for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = childrenInstances.begin();
                 it != childrenInstances.end(); ++it,++next) {
                if ( it->second && (it->first != internalNode) ) {
                    it->first->hideKeyframesFromTimeline( next == childrenInstances.end() );
                }
            }
        }
    }

    ///Closing a panel always gives focus to some line-edit in the application which is quite annoying
    QWidget* hasFocus = qApp->focusWidget();
    if (hasFocus) {
        hasFocus->clearFocus();
    }
    
    const std::list<ViewerTab*>& viewers = getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
        (*it)->getViewer()->redraw();
    }
    
}

void
DockablePanel::minimizeOrMaximize(bool toggled)
{
    _imp->_minimized = toggled;
    if (_imp->_minimized) {
        emit minimized();
    } else {
        emit maximized();
    }
    _imp->_tabWidget->setVisible(!_imp->_minimized);
    std::vector<QWidget*> _panels;
    for (int i = 0; i < _imp->_container->count(); ++i) {
        if ( QWidget * myItem = dynamic_cast <QWidget*>( _imp->_container->itemAt(i) ) ) {
            _panels.push_back(myItem);
            _imp->_container->removeWidget(myItem);
        }
    }
    for (U32 i = 0; i < _panels.size(); ++i) {
        _imp->_container->addWidget(_panels[i]);
    }
    getGui()->buildTabFocusOrderPropertiesBin();
    update();
}

void
DockablePanel::floatPanel()
{
    _imp->_floating = !_imp->_floating;
    if (_imp->_floating) {
        assert(!_imp->_floatingWidget);
        _imp->_floatingWidget = new FloatingWidget(_imp->_gui,_imp->_gui);
        QObject::connect( _imp->_floatingWidget,SIGNAL( closed() ),this,SLOT( closePanel() ) );
        _imp->_container->removeWidget(this);
        _imp->_floatingWidget->setWidget(this);
        _imp->_gui->registerFloatingWindow(_imp->_floatingWidget);
    } else {
        assert(_imp->_floatingWidget);
        _imp->_gui->unregisterFloatingWindow(_imp->_floatingWidget);
        _imp->_floatingWidget->removeEmbeddedWidget();
        setParent( _imp->_container->parentWidget() );
        _imp->_container->insertWidget(0, this);
        _imp->_floatingWidget->deleteLater();
        _imp->_floatingWidget = 0;
    }
    getGui()->buildTabFocusOrderPropertiesBin();
}

void
DockablePanel::setName(const QString & str)
{
    if (_imp->_nameLabel) {
        _imp->_nameLabel->setText(str);
    } else if (_imp->_nameLineEdit) {
        _imp->_nameLineEdit->setText(str);
    }
}


Button*
DockablePanel::insertHeaderButton(int headerPosition)
{
    Button* ret = new Button(_imp->_headerWidget);

    _imp->_headerLayout->insertWidget(headerPosition, ret);

    return ret;
}

void
DockablePanel::onKnobDeletion()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    if (handler) {
        for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
            KnobHelper* helper = dynamic_cast<KnobHelper*>( it->first.get() );
            if (helper->getSignalSlotHandler().get() == handler) {
                if (it->second) {
                    it->second->deleteLater();
                }
                _imp->_knobs.erase(it);

                return;
            }
        }
    }
}

Gui*
DockablePanel::getGui() const
{
    return _imp->_gui;
}

void
DockablePanel::insertHeaderWidget(int index,
                                  QWidget* widget)
{
    if (_imp->_mode != NO_HEADER) {
        _imp->_headerLayout->insertWidget(index, widget);
    }
}

void
DockablePanel::appendHeaderWidget(QWidget* widget)
{
    if (_imp->_mode != NO_HEADER) {
        _imp->_headerLayout->addWidget(widget);
    }
}

QWidget*
DockablePanel::getHeaderWidget() const
{
    return _imp->_headerWidget;
}

bool
DockablePanel::isMinimized() const
{
    return _imp->_minimized;
}

const std::map<boost::shared_ptr<KnobI>,KnobGui*> &
DockablePanel::getKnobs() const
{
    return _imp->_knobs;
}

QVBoxLayout*
DockablePanel::getContainer() const
{
    return _imp->_container;
}

QUndoStack*
DockablePanel::getUndoStack() const
{
    return _imp->_undoStack;
}

bool
DockablePanel::isClosed() const
{
    QMutexLocker l(&_imp->_isClosedMutex); return _imp->_isClosed;
}

bool
DockablePanel::isFloating() const
{
    return _imp->_floating;
}

void
DockablePanel::onColorDialogColorChanged(const QColor & color)
{
    if (_imp->_mode != READ_ONLY_NAME) {
        QPixmap p(15,15);
        p.fill(color);
        _imp->_colorButton->setIcon( QIcon(p) );
    }
}

void
DockablePanel::onColorButtonClicked()
{
    QColorDialog dialog(this);
    QColor oldColor;
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        dialog.setCurrentColor(_imp->_currentColor);
        oldColor = _imp->_currentColor;
    }
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onColorDialogColorChanged(QColor) ) );

    if ( dialog.exec() ) {
        QColor c = dialog.currentColor();
        {
            QMutexLocker locker(&_imp->_currentColorMutex);
            _imp->_currentColor = c;
        }
        emit colorChanged(c);
    } else {
        onColorDialogColorChanged(oldColor);
    }
}

QColor
DockablePanel::getCurrentColor() const
{
    QMutexLocker locker(&_imp->_currentColorMutex);

    return _imp->_currentColor;
}

void
DockablePanel::setCurrentColor(const QColor & c)
{
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        _imp->_currentColor = c;
    }
    onColorDialogColorChanged(c);
}

void
DockablePanel::focusInEvent(QFocusEvent* e)
{
    QFrame::focusInEvent(e);

    _imp->_undoStack->setActive();
}

void
DockablePanel::onRightClickMenuRequested(const QPoint & pos)
{
    QWidget* emitter = qobject_cast<QWidget*>( sender() );

    assert(emitter);
    
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(_imp->_holder);
    if (isEffect) {
        
        boost::shared_ptr<Natron::Node> master = isEffect->getNode()->getMasterNode();
        QMenu menu(this);
        menu.setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );
        QAction* setKeys = new QAction(tr("Set key on all parameters"),&menu);
        menu.addAction(setKeys);
        QAction* removeAnimation = new QAction(tr("Remove animation on all parameters"),&menu);
        menu.addAction(removeAnimation);
        
        if (master || _imp->_holder->getApp()->isGuiFrozen()) {
            setKeys->setEnabled(false);
            removeAnimation->setEnabled(false);
        }

        QAction* ret = menu.exec( emitter->mapToGlobal(pos) );
        if (ret == setKeys) {
            setKeyOnAllParameters();
        } else if (ret == removeAnimation) {
            removeAnimationOnAllParameters();
        }
    }
} // onRightClickMenuRequested

void
DockablePanel::setKeyOnAllParameters()
{
    int time = getGui()->getApp()->getTimeLine()->currentFrame();

    AddKeysCommand::KeysToAddList keys;
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->first->isAnimationEnabled()) {
            for (int i = 0; i < it->first->getDimension(); ++i) {
                CurveGui* curve = getGui()->getCurveEditor()->findCurve(it->second,i);
                if (!curve) {
                    continue;
                }
                boost::shared_ptr<AddKeysCommand::KeysForCurve> curveKeys(new AddKeysCommand::KeysForCurve);
                curveKeys->curve = curve;
                
                std::vector<KeyFrame> kVec;
                KeyFrame kf;
                kf.setTime(time);
                Knob<int>* isInt = dynamic_cast<Knob<int>*>( it->first.get() );
                Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( it->first.get() );
                AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>( it->first.get() );
                Knob<double>* isDouble = dynamic_cast<Knob<double>*>( it->first.get() );
                
                if (isInt) {
                    kf.setValue( isInt->getValueAtTime(time,i) );
                } else if (isBool) {
                    kf.setValue( isBool->getValueAtTime(time,i) );
                } else if (isDouble) {
                    kf.setValue( isDouble->getValueAtTime(time,i) );
                } else if (isString) {
                    std::string v = isString->getValueAtTime(time,i);
                    double dv;
                    isString->stringToKeyFrameValue(time, v, &dv);
                    kf.setValue(dv);
                }
                
                kVec.push_back(kf);
                curveKeys->keys = kVec;
                keys.push_back(curveKeys);
            }
        }
    }
    pushUndoCommand( new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),keys) );

}

void
DockablePanel::removeAnimationOnAllParameters()
{
    std::vector< std::pair<CurveGui*,KeyFrame > > keysToRemove;
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->first->isAnimationEnabled()) {
            for (int i = 0; i < it->first->getDimension(); ++i) {
                CurveGui* curve = getGui()->getCurveEditor()->findCurve(it->second,i);
                if (!curve) {
                    continue;
                }
                KeyFrameSet keys = curve->getInternalCurve()->getKeyFrames_mt_safe();
                for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                    keysToRemove.push_back( std::make_pair(curve,*it) );
                }
            }
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),keysToRemove) );
}


void
DockablePanel::onCenterButtonClicked()
{
    centerOnItem();
}

NodeSettingsPanel::NodeSettingsPanel(const boost::shared_ptr<MultiInstancePanel> & multiPanel,
                                     Gui* gui,
                                     boost::shared_ptr<NodeGui> NodeUi,
                                     QVBoxLayout* container,
                                     QWidget *parent)
    : DockablePanel(gui,
                    multiPanel.get() != NULL ? dynamic_cast<KnobHolder*>( multiPanel.get() ) : NodeUi->getNode()->getLiveInstance(),
                    container,
                    DockablePanel::FULLY_FEATURED,
                    false,
                    NodeUi->getNode()->getName().c_str(),
                    NodeUi->getNode()->getDescription().c_str(),
                    false,
                    "Settings",
                    parent)
      , _nodeGUI(NodeUi)
      , _selected(false)
      , _settingsButton(0)
      , _multiPanel(multiPanel)
{
    if (multiPanel) {
        multiPanel->initializeKnobsPublic();
    }

    
    QObject::connect( this,SIGNAL( closeChanged(bool) ),NodeUi.get(),SLOT( onSettingsPanelClosedChanged(bool) ) );
    
    QPixmap pixSettings;
    appPTR->getIcon(NATRON_PIXMAP_SETTINGS,&pixSettings);
    _settingsButton = new Button( QIcon(pixSettings),"",getHeaderWidget() );
    _settingsButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _settingsButton->setToolTip( tr("Settings and presets") );
    _settingsButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _settingsButton,SIGNAL( clicked() ),this,SLOT( onSettingsButtonClicked() ) );
    insertHeaderWidget(1, _settingsButton);
}

NodeSettingsPanel::~NodeSettingsPanel()
{
    _nodeGUI->removeSettingsPanel();
}

void
NodeSettingsPanel::setSelected(bool s)
{
    _selected = s;
    style()->unpolish(this);
    style()->polish(this);
}

void
NodeSettingsPanel::centerOnItem()
{
    _nodeGUI->centerGraphOnIt();
}

RotoPanel*
NodeSettingsPanel::initializeRotoPanel()
{
    if ( _nodeGUI->getNode()->isRotoNode() ) {
        return new RotoPanel(_nodeGUI.get(),this);
    } else {
        return NULL;
    }
}

void
NodeSettingsPanel::initializeExtraGui(QVBoxLayout* layout)
{
    if ( _multiPanel && !_multiPanel->isGuiCreated() ) {
        _multiPanel->createMultiInstanceGui(layout);
    }
}

void
NodeSettingsPanel::onSettingsButtonClicked()
{
    QMenu menu(this);
    menu.setFont(QFont(NATRON_FONT,NATRON_FONT_SIZE_11));
    
    boost::shared_ptr<Natron::Node> master = _nodeGUI->getNode()->getMasterNode();
    
    QAction* importPresets = new QAction(tr("Import presets"),&menu);
    QObject::connect(importPresets,SIGNAL(triggered()),this,SLOT(onImportPresetsActionTriggered()));
    QAction* exportAsPresets = new QAction(tr("Export as presets"),&menu);
    QObject::connect(exportAsPresets,SIGNAL(triggered()),this,SLOT(onExportPresetsActionTriggered()));
    
    menu.addAction(importPresets);
    menu.addAction(exportAsPresets);
    menu.addSeparator();
    
    QAction* setKeyOnAll = new QAction(tr("Set key on all parameters"),&menu);
    QObject::connect(setKeyOnAll,SIGNAL(triggered()),this,SLOT(setKeyOnAllParameters()));
    QAction* removeAnimationOnAll = new QAction(tr("Remove animation on all parameters"),&menu);
    QObject::connect(removeAnimationOnAll,SIGNAL(triggered()),this,SLOT(removeAnimationOnAllParameters()));
    menu.addAction(setKeyOnAll);
    menu.addAction(removeAnimationOnAll);
    
    if (master || _nodeGUI->getDagGui()->getGui()->isGUIFrozen()) {
        importPresets->setEnabled(false);
        exportAsPresets->setEnabled(false);
        setKeyOnAll->setEnabled(false);
        removeAnimationOnAll->setEnabled(false);
    }
    
    menu.exec(_settingsButton->mapToGlobal(_settingsButton->pos()));
}

void
NodeSettingsPanel::onImportPresetsActionTriggered()
{
    std::vector<std::string> filters;
    filters.push_back(NATRON_PRESETS_FILE_EXT);
    std::string filename = getGui()->popOpenFileDialog(false, filters, getGui()->getLastLoadProjectDirectory().toStdString(), false);
    if (filename.empty()) {
        return;
    }
    
    std::ifstream ifile;
    try {
        ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifile.open(filename.c_str(),std::ifstream::in);
    } catch (const std::ifstream::failure & e) {
        Natron::errorDialog("Presets",e.what());
        return;
    }
    
    std::list<boost::shared_ptr<NodeSerialization> > nodeSerialization;
    try {

        int nNodes;
        boost::archive::xml_iarchive iArchive(ifile);
        iArchive >> boost::serialization::make_nvp("NodesCount",nNodes);
        for (int i = 0; i < nNodes ;++i) {
            boost::shared_ptr<NodeSerialization> node(new NodeSerialization(getHolder()->getApp()));
            iArchive >> boost::serialization::make_nvp("Node",*node);
            nodeSerialization.push_back(node);
        }
        
        
    } catch (const std::exception & e) {
        ifile.close();
        Natron::errorDialog("Presets",e.what());
        return;
    }
    if (nodeSerialization.front()->getPluginID() != _nodeGUI->getNode()->getPluginID()) {
        QString err = QString(tr("You cannot load ") + filename.c_str()  + tr(" which are presets for the plug-in ") +
                              nodeSerialization.front()->getPluginID().c_str() + tr(" on the plug-in ") +
                              _nodeGUI->getNode()->getPluginID().c_str());
        Natron::errorDialog(tr("Presets").toStdString(),err.toStdString());
        return;
    }
    
    _nodeGUI->restoreInternal(_nodeGUI,nodeSerialization);
}


static bool endsWith(const std::string &str, const std::string &suffix)
{
    return ((str.size() >= suffix.size()) &&
            (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0));
}

void
NodeSettingsPanel::onExportPresetsActionTriggered()
{
    std::vector<std::string> filters;
    filters.push_back(NATRON_PRESETS_FILE_EXT);
    std::string filename = getGui()->popSaveFileDialog(false, filters, getGui()->getLastSaveProjectDirectory().toStdString(), false);
    if (filename.empty()) {
        return;
    }
    
    if (!endsWith(filename, "." NATRON_PRESETS_FILE_EXT)) {
        filename.append("." NATRON_PRESETS_FILE_EXT);
    }
    
    std::ofstream ofile;
    try {
        ofile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        ofile.open(filename.c_str(),std::ofstream::out);
    } catch (const std::ofstream::failure & e) {
        Natron::errorDialog("Presets",e.what());
        return;
    }

    std::list<boost::shared_ptr<NodeSerialization> > nodeSerialization;
    _nodeGUI->serializeInternal(nodeSerialization,true);
    try {
         
        int nNodes = nodeSerialization.size();
        boost::archive::xml_oarchive oArchive(ofile);
        oArchive << boost::serialization::make_nvp("NodesCount",nNodes);
        for (std::list<boost::shared_ptr<NodeSerialization> >::iterator it = nodeSerialization.begin();
             it != nodeSerialization.end(); ++it) {
            oArchive << boost::serialization::make_nvp("Node",**it);
        }
        
        
    }  catch (const std::exception & e) {
        ofile.close();
        Natron::errorDialog("Presets",e.what());
        return;
    }
 
}

NodeBackDropSettingsPanel::NodeBackDropSettingsPanel(NodeBackDrop* backdrop,
                                                     Gui* gui,
                                                     QVBoxLayout* container,
                                                     const QString& name,
                                                     QWidget* parent)
: DockablePanel(gui,
                backdrop,
                container,
                DockablePanel::FULLY_FEATURED,
                false,
                name,
                QObject::tr("The node backdrop is useful to group nodes and identify them in the node graph. You can also "
                   "move all the nodes inside the backdrop."),
                false, //< no default page
                QObject::tr("BackDrop"), //< default page name
                parent)
, _backdrop(backdrop)
{
    
}

NodeBackDropSettingsPanel::~NodeBackDropSettingsPanel()
{
    
}

void
NodeBackDropSettingsPanel::centerOnItem()
{
    _backdrop->centerOnIt();
}