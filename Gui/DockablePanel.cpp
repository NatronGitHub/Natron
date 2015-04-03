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

#include "DockablePanel.h"
#include <cfloat>
#include <iostream>
#include <fstream>
#include <QLayout>
#include <QAction>
#include <QApplication>
#include <QTabWidget>
#include <QStyle>
#include <QUndoStack>
#include <QGridLayout>
#include <QUndoCommand>
#include <QFormLayout>
#include <QDebug>
#include <QToolTip>
#include <QHeaderView>
#include <QMutex>
#include <QTreeWidget>
#include <QCheckBox>
#include <QHeaderView>
#include <QColorDialog>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QPaintEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QTextDocument> // for Qt::convertFromPlainText
#include <QPainter>
#include <QImage>
#include <QToolButton>
#include <QMenu>
#include <QDialogButtonBox>

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
#include "Engine/Plugin.h"
#include "Engine/Image.h"
#include "Engine/BackDrop.h"
#include "Engine/NoOp.h"
#include "Engine/NodeSerialization.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"
#include "Gui/ComboBox.h"
#include "Gui/Histogram.h"
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
#include "Gui/MultiInstancePanel.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/SpinBox.h"

#define NATRON_FORM_LAYOUT_LINES_SPACING 0
#define NATRON_SETTINGS_VERTICAL_SPACING_PIXELS 3


#define NATRON_VERTICAL_BAR_WIDTH 4
using std::make_pair;
using namespace Natron;

namespace {
struct Page
{
    QWidget* tab;
    int currentRow;
    TabGroup* groupAsTab; //< to gather group knobs that are set as a tab

    Page()
        : tab(0), currentRow(0),groupAsTab(0)
    {
    }

    Page(const Page & other)
        : tab(other.tab), currentRow(other.currentRow), groupAsTab(other.groupAsTab)
    {
    }
};

typedef std::map<QString,Page> PageMap;
}

static Page_Knob* getTopLevelPageForKnob(KnobI* knob)
{
    boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
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
    return isTopLevelParentAPage;
}

TabGroup::TabGroup(QWidget* parent)
: QFrame(parent)
{
    setFrameShadow(QFrame::Raised);
    setFrameShape(QFrame::Box);
    QHBoxLayout* frameLayout = new QHBoxLayout(this);
    _tabWidget = new QTabWidget(this);
    frameLayout->addWidget(_tabWidget);
}

QGridLayout*
TabGroup::addTab(const boost::shared_ptr<Group_Knob>& group,const QString& name)
{
    
    QWidget* tab = 0;
    QGridLayout* tabLayout = 0;
    for (U32 i = 0 ; i < _tabs.size(); ++i) {
        if (_tabs[i].lock() == group) {
            tab = _tabWidget->widget(i);
            assert(tab);
            tabLayout = qobject_cast<QGridLayout*>( tab->layout() );
            assert(tabLayout);
        }
    }
   
    if (!tab) {
        tab = new QWidget(_tabWidget);
        tabLayout = new QGridLayout(tab);
        tabLayout->setColumnStretch(1, 1);
        tabLayout->setContentsMargins(0, 0, 0, 0);
        tabLayout->setSpacing(NATRON_FORM_LAYOUT_LINES_SPACING); // unfortunately, this leaves extra space when parameters are hidden
        _tabWidget->addTab(tab,name);
        _tabs.push_back(group);
    }
    assert(tabLayout);
    return tabLayout;
}

void
TabGroup::removeTab(Group_Knob* group)
{
    int i = 0;
    for (std::vector<boost::weak_ptr<Group_Knob> >::iterator it = _tabs.begin(); it != _tabs.end(); ++it,++i) {
        if (it->lock().get() == group) {
            _tabWidget->removeTab(i);
            _tabs.erase(it);
            break;
        }
    }
}

class OverlayColorButton: public Button
{
    
    DockablePanel* _panel;
    
public:
    
    
    OverlayColorButton(DockablePanel* panel,const QIcon& icon,QWidget* parent)
    : Button(icon,"",parent)
    , _panel(panel)
    {
        
    }
    
private:
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        if (triggerButtonisRight(e)) {
            Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Warning").toStdString(),
                                                                    tr("Are you sure you want to reset the overlay color ?").toStdString(),
                                                                    false);
            if (rep == Natron::eStandardButtonYes) {
                _panel->resetDefaultOverlayColor();
            }
        } else {
            Button::mousePressEvent(e);
        }
    }
};

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
    Natron::Label* _nameLabel; /*!< if the name is read-only*/

    QHBoxLayout* _horizLayout;
    QWidget* _horizContainer;
    VerticalColorBar* _verticalColorBar;
    
    /*Tab related*/
    QTabWidget* _tabWidget;
    Button* _centerNodeButton;
    Button* _helpButton;
    Button* _minimize;
    Button* _hideUnmodifiedButton;
    Button* _floatButton;
    Button* _cross;
    mutable QMutex _currentColorMutex;
    QColor _overlayColor;
    bool _hasOverlayColor;
    Button* _colorButton;
    Button* _overlayButton;
    Button* _undoButton;
    Button* _redoButton;
    Button* _restoreDefaultsButton;
    bool _minimized; /*!< true if the panel is minimized*/
    QUndoStack* _undoStack; /*!< undo/redo stack*/
    bool _floating; /*!< true if the panel is floating*/
    FloatingWidget* _floatingWidget;

    /*a map storing for each knob a pointer to their GUI.*/
    std::map<boost::shared_ptr<KnobI>,KnobGui*> _knobs;
    
    ///THe visibility of the knobs before the hide/show unmodified button is clicked
    ///to show only the knobs that need to afterwards
    std::map<KnobGui*,bool> _knobsVisibilityBeforeHideModif;
    KnobHolder* _holder;

    /* map<tab name, pair<tab , row count> >*/
    PageMap _pages;
    QString _defaultPageName;
    bool _useScrollAreasForTabs;
    DockablePanel::HeaderModeEnum _mode;
    mutable QMutex _isClosedMutex;
    bool _isClosed; //< accessed by serialization thread too
    
    QString _helpToolTip;
    QString _pluginID;
    unsigned _pluginVersionMajor,_pluginVersionMinor;
    
    
    bool _pagesEnabled;
    
    Natron::Label* _iconLabel;
    
    DockablePanelPrivate(DockablePanel* publicI,
                         Gui* gui,
                         KnobHolder* holder,
                         QVBoxLayout* container,
                         DockablePanel::HeaderModeEnum headerMode,
                         bool useScrollAreasForTabs,
                         const QString & defaultPageName,
                         const QString& helpToolTip)
    : _publicInterface(publicI)
    ,_gui(gui)
    ,_container(container)
    ,_mainLayout(NULL)
    ,_headerWidget(NULL)
    ,_headerLayout(NULL)
    ,_nameLineEdit(NULL)
    ,_nameLabel(NULL)
    , _horizLayout(0)
    , _horizContainer(0)
    , _verticalColorBar(0)
    ,_tabWidget(NULL)
    , _centerNodeButton(NULL)
    ,_helpButton(NULL)
    ,_minimize(NULL)
    ,_hideUnmodifiedButton(NULL)
    ,_floatButton(NULL)
    ,_cross(NULL)
    ,_overlayColor()
    ,_hasOverlayColor(false)
    ,_colorButton(NULL)
    ,_overlayButton(NULL)
    ,_undoButton(NULL)
    ,_redoButton(NULL)
    ,_restoreDefaultsButton(NULL)
    ,_minimized(false)
    ,_undoStack(new QUndoStack)
    ,_floating(false)
    ,_floatingWidget(NULL)
    ,_knobs()
    ,_knobsVisibilityBeforeHideModif()
    ,_holder(holder)
    ,_pages()
    ,_defaultPageName(defaultPageName)
    ,_useScrollAreasForTabs(useScrollAreasForTabs)
    ,_mode(headerMode)
    ,_isClosedMutex()
    ,_isClosed(false)
    ,_helpToolTip(helpToolTip)
    ,_pluginID()
    ,_pluginVersionMajor(0)
    ,_pluginVersionMinor(0)
    ,_pagesEnabled(true)
    ,_iconLabel(0)
    {
    }

    /*inserts a new page to the dockable panel.*/
    PageMap::iterator addPage(Page_Knob* page,const QString & name);
    
    boost::shared_ptr<Page_Knob> ensureDefaultPageKnobCreated() ;


    void initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> > & knobs,
                              QWidget* lastRowWidget);

    KnobGui* createKnobGui(const boost::shared_ptr<KnobI> &knob);

    /*Search an existing knob GUI in the map, otherwise creates
       the gui for the knob.*/
    KnobGui* findKnobGuiOrCreate( const boost::shared_ptr<KnobI> &knob,
                                  bool makeNewLine,
                                  QWidget* lastRowWidget,
                                  const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine = std::vector< boost::shared_ptr< KnobI > >() );
    
    PageMap::iterator getDefaultPage(const boost::shared_ptr<KnobI> &knob);
};



DockablePanel::DockablePanel(Gui* gui ,
                             KnobHolder* holder ,
                             QVBoxLayout* container,
                             HeaderModeEnum headerMode,
                             bool useScrollAreasForTabs,
                             const QString & initialName,
                             const QString & helpToolTip,
                             bool createDefaultPage,
                             const QString & defaultPageName,
                             QWidget *parent)
: QFrame(parent)
, _imp(new DockablePanelPrivate(this,gui,holder,container,headerMode,useScrollAreasForTabs,defaultPageName,helpToolTip))
{
    assert(holder);
    holder->setPanelPointer(this);
    
    _imp->_mainLayout = new QVBoxLayout(this);
    _imp->_mainLayout->setSpacing(0);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_imp->_mainLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameShape(QFrame::Box);
    setFocusPolicy(Qt::NoFocus);
    
    Natron::EffectInstance* iseffect = dynamic_cast<Natron::EffectInstance*>(holder);
    QString pluginLabelVersioned;
    if (iseffect) {
        
        if (dynamic_cast<GroupOutput*>(iseffect)) {
            headerMode = eHeaderModeReadOnlyName;
        }
        
        const Natron::Plugin* plugin = iseffect->getNode()->getPlugin();
        pluginLabelVersioned = plugin->getPluginLabel();
        QString toAppend = QString(" version %1.%2").arg(plugin->getMajorVersion()).arg(plugin->getMinorVersion());
        pluginLabelVersioned.append(toAppend);
        
        
    }
    MultiInstancePanel* isMultiInstance = dynamic_cast<MultiInstancePanel*>(holder);
    if (isMultiInstance) {
        iseffect = isMultiInstance->getMainInstance()->getLiveInstance();
        assert(iseffect);
    }
    
    QColor currentColor;
    if (headerMode != eHeaderModeNoHeader) {
        _imp->_headerWidget = new QFrame(this);
        _imp->_headerWidget->setFrameShape(QFrame::Box);
        _imp->_headerLayout = new QHBoxLayout(_imp->_headerWidget);
        _imp->_headerLayout->setContentsMargins(0, 0, 0, 0);
        _imp->_headerLayout->setSpacing(2);
        _imp->_headerWidget->setLayout(_imp->_headerLayout);
        
        if (iseffect) {
            
            _imp->_iconLabel = new Natron::Label(getHeaderWidget());
            _imp->_iconLabel->setContentsMargins(2, 2, 2, 2);
            _imp->_iconLabel->setToolTip(pluginLabelVersioned);
            _imp->_headerLayout->addWidget(_imp->_iconLabel);


            std::string iconFilePath = iseffect->getNode()->getPluginIconFilePath();
            if (!iconFilePath.empty()) {
               
                QPixmap ic;
                if (ic.load(iconFilePath.c_str())) {
                    ic = ic.scaled(NATRON_MEDIUM_BUTTON_SIZE - 2,NATRON_MEDIUM_BUTTON_SIZE - 2,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
                    _imp->_iconLabel->setPixmap(ic);
                } else {
                    _imp->_iconLabel->hide();
                }
                
            } else {
                _imp->_iconLabel->hide();
            }
            
            QPixmap pixCenter;
            appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER,&pixCenter);
            _imp->_centerNodeButton = new Button( QIcon(pixCenter),"",getHeaderWidget() );
            _imp->_centerNodeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_centerNodeButton->setToolTip( tr("Centers the node graph on this item.") );
            _imp->_centerNodeButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_centerNodeButton,SIGNAL( clicked() ),this,SLOT( onCenterButtonClicked() ) );
            _imp->_headerLayout->addWidget(_imp->_centerNodeButton);
            
            QPixmap pixHelp;
            appPTR->getIcon(NATRON_PIXMAP_HELP_WIDGET,&pixHelp);
            _imp->_helpButton = new Button(QIcon(pixHelp),"",_imp->_headerWidget);
            
            const Natron::Plugin* plugin = iseffect->getNode()->getPlugin();
            assert(plugin);
            _imp->_pluginID = plugin->getPluginID();
            _imp->_pluginVersionMajor = plugin->getMajorVersion();
            _imp->_pluginVersionMinor = plugin->getMinorVersion();
            
            _imp->_helpButton->setToolTip(helpString());
            _imp->_helpButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_helpButton->setFocusPolicy(Qt::NoFocus);
            
            QObject::connect( _imp->_helpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
            
            QPixmap pixHide,pixShow;
            appPTR->getIcon(NATRON_PIXMAP_UNHIDE_UNMODIFIED, &pixShow);
            appPTR->getIcon(NATRON_PIXMAP_HIDE_UNMODIFIED,&pixHide);
            QIcon icHideShow;
            icHideShow.addPixmap(pixShow,QIcon::Normal,QIcon::Off);
            icHideShow.addPixmap(pixHide,QIcon::Normal,QIcon::On);
            _imp->_hideUnmodifiedButton = new Button(icHideShow,"",_imp->_headerWidget);
            _imp->_hideUnmodifiedButton->setToolTip(tr("Show/Hide all parameters without modifications"));
            _imp->_hideUnmodifiedButton->setFocusPolicy(Qt::NoFocus);
            _imp->_hideUnmodifiedButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_hideUnmodifiedButton->setCheckable(true);
            _imp->_hideUnmodifiedButton->setChecked(false);
            QObject::connect(_imp->_hideUnmodifiedButton,SIGNAL(clicked(bool)),this,SLOT(onHideUnmodifiedButtonClicked(bool)));
        }
        QPixmap pixM;
        appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET,&pixM);

        QPixmap pixC;
        appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);

        QPixmap pixF;
        appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, &pixF);

        _imp->_minimize = new Button(QIcon(pixM),"",_imp->_headerWidget);
        _imp->_minimize->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_minimize->setCheckable(true);
        _imp->_minimize->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_minimize,SIGNAL( toggled(bool) ),this,SLOT( minimizeOrMaximize(bool) ) );

        _imp->_floatButton = new Button(QIcon(pixF),"",_imp->_headerWidget);
        _imp->_floatButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_floatButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_floatButton,SIGNAL( clicked() ),this,SLOT( floatPanel() ) );


        _imp->_cross = new Button(QIcon(pixC),"",_imp->_headerWidget);
        _imp->_cross->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_cross->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_cross,SIGNAL( clicked() ),this,SLOT( closePanel() ) );


        if (iseffect) {

            boost::shared_ptr<NodeGuiI> gui_i = iseffect->getNode()->getNodeGui();
            assert(gui_i);
            double r,g,b;
            gui_i->getColor(&r, &g, &b);
            currentColor.setRgbF(Natron::clamp(r), Natron::clamp(g), Natron::clamp(b));
            QPixmap p(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
            p.fill(currentColor);


            _imp->_colorButton = new Button(QIcon(p),"",_imp->_headerWidget);
            _imp->_colorButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_colorButton->setToolTip( Qt::convertFromPlainText(tr("Set here the color of the node in the nodegraph. "
                                                                        "By default the color of the node is the one set in the "
                                                                        "preferences of %1.").arg(NATRON_APPLICATION_NAME)
                                                                     ,Qt::WhiteSpaceNormal) );
            _imp->_colorButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_colorButton,SIGNAL( clicked() ),this,SLOT( onColorButtonClicked() ) );

            if ( iseffect && !iseffect->getNode()->isMultiInstance() ) {
                ///Show timeline keyframe markers to be consistent with the fact that the panel is opened by default
                iseffect->getNode()->showKeyframesOnTimeline(true);
            }
            
            
            if (iseffect && iseffect->hasOverlay()) {
                QPixmap pixOverlay;
                appPTR->getIcon(Natron::NATRON_PIXMAP_OVERLAY,&pixOverlay);
                _imp->_overlayColor.setRgbF(1., 1., 1.);
                _imp->_overlayButton = new OverlayColorButton(this,QIcon(pixOverlay),_imp->_headerWidget);
                _imp->_overlayButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
                _imp->_overlayButton->setToolTip(Qt::convertFromPlainText(tr("You can suggest here a color for the overlay on the viewer. "
                                                                             "Some plug-ins understand it and will use it to change the color of "
                                                                             "the overlay."),Qt::WhiteSpaceNormal));
                _imp->_overlayButton->setFocusPolicy(Qt::NoFocus);
                QObject::connect( _imp->_overlayButton,SIGNAL( clicked() ),this,SLOT( onOverlayButtonClicked() ) );
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
        _imp->_undoButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_undoButton->setToolTip( Qt::convertFromPlainText(tr("Undo the last change made to this operator."), Qt::WhiteSpaceNormal) );
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
        _imp->_redoButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_redoButton->setToolTip( Qt::convertFromPlainText(tr("Redo the last change undone to this operator."), Qt::WhiteSpaceNormal) );
        _imp->_redoButton->setEnabled(false);
        _imp->_redoButton->setFocusPolicy(Qt::NoFocus);

        QPixmap pixRestore;
        appPTR->getIcon(NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED, &pixRestore);
        QIcon icRestore;
        icRestore.addPixmap(pixRestore);
        _imp->_restoreDefaultsButton = new Button(icRestore,"",_imp->_headerWidget);
        _imp->_restoreDefaultsButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_restoreDefaultsButton->setToolTip( Qt::convertFromPlainText(tr("Restore default values for this operator."),Qt::WhiteSpaceNormal) );
        _imp->_restoreDefaultsButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_restoreDefaultsButton,SIGNAL( clicked() ),this,SLOT( onRestoreDefaultsButtonClicked() ) );
        QObject::connect( _imp->_undoButton, SIGNAL( clicked() ),this, SLOT( onUndoClicked() ) );
        QObject::connect( _imp->_redoButton, SIGNAL( clicked() ),this, SLOT( onRedoPressed() ) );

        if (headerMode != eHeaderModeReadOnlyName) {
            _imp->_nameLineEdit = new LineEdit(_imp->_headerWidget);
            if (iseffect) {
                onNodeScriptChanged(iseffect->getScriptName().c_str());
                QObject::connect(iseffect->getNode().get(),SIGNAL(scriptNameChanged(QString)),this, SLOT(onNodeScriptChanged(QString)));
            }
            _imp->_nameLineEdit->setText(initialName);
            QObject::connect( _imp->_nameLineEdit,SIGNAL( editingFinished() ),this,SLOT( onLineEditNameEditingFinished() ) );
            _imp->_headerLayout->addWidget(_imp->_nameLineEdit);
        } else {
            _imp->_nameLabel = new Natron::Label(initialName,_imp->_headerWidget);
            if (iseffect) {
                onNodeScriptChanged(iseffect->getScriptName().c_str());
            }
            _imp->_headerLayout->addWidget(_imp->_nameLabel);
        }

        _imp->_headerLayout->addStretch();

        if (headerMode != eHeaderModeReadOnlyName && _imp->_colorButton) {
            _imp->_headerLayout->addWidget(_imp->_colorButton);
        }
        if (headerMode != eHeaderModeReadOnlyName && _imp->_overlayButton) {
            _imp->_headerLayout->addWidget(_imp->_overlayButton);
        }
        _imp->_headerLayout->addWidget(_imp->_undoButton);
        _imp->_headerLayout->addWidget(_imp->_redoButton);
        _imp->_headerLayout->addWidget(_imp->_restoreDefaultsButton);

        _imp->_headerLayout->addStretch();
        if (_imp->_helpButton) {
            _imp->_headerLayout->addWidget(_imp->_helpButton);
        }
        if (_imp->_hideUnmodifiedButton) {
            _imp->_headerLayout->addWidget(_imp->_hideUnmodifiedButton);
        }
        _imp->_headerLayout->addWidget(_imp->_minimize);
        _imp->_headerLayout->addWidget(_imp->_floatButton);
        _imp->_headerLayout->addWidget(_imp->_cross);

        _imp->_mainLayout->addWidget(_imp->_headerWidget);
    }
    
    
    _imp->_horizContainer = new QWidget(this);
    _imp->_horizLayout = new QHBoxLayout(_imp->_horizContainer);
    _imp->_horizLayout->setContentsMargins(NATRON_VERTICAL_BAR_WIDTH, 3, 3, 3);
    if (iseffect) {
        _imp->_verticalColorBar = new VerticalColorBar(_imp->_horizContainer);
        _imp->_verticalColorBar->setColor(currentColor);
        _imp->_horizLayout->addWidget(_imp->_verticalColorBar);
    }
    
    if (useScrollAreasForTabs) {
        _imp->_tabWidget = new QTabWidget(_imp->_horizContainer);
    } else {
        _imp->_tabWidget = new DockablePanelTabWidget(gui,this);
    }
    _imp->_horizLayout->addWidget(_imp->_tabWidget);
    _imp->_tabWidget->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    _imp->_tabWidget->setObjectName("QTabWidget");
    _imp->_mainLayout->addWidget(_imp->_horizContainer);

    if (createDefaultPage) {
        _imp->addPage(NULL,defaultPageName);
    }
}

DockablePanel::~DockablePanel()
{
//    if (_imp->_holder) {
//        _imp->_holder->discardPanelPointer();
//    }
    getGui()->removeVisibleDockablePanel(this);

    delete _imp->_undoStack;

    ///Delete the knob gui if they weren't before
    ///normally the onKnobDeletion() function should have cleared them
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->second) {
            it->first->setKnobGuiPointer(0);
            it->second->callDeleteLater();
        }
    }
}

void
DockablePanel::turnOffPages()
{
    _imp->_pagesEnabled = false;
    delete _imp->_tabWidget;
    _imp->_tabWidget = 0;
    setFrameShape(QFrame::NoFrame);
    
    boost::shared_ptr<Page_Knob> userPage = _imp->_holder->getOrCreateUserPageKnob();
    _imp->addPage(userPage.get(), userPage->getDescription().c_str());
    
}

void
DockablePanel::setPluginIDAndVersion(const std::string& pluginLabel,const std::string& pluginID,unsigned int version)
{
    if (_imp->_iconLabel) {
        QString pluginLabelVersioned(pluginLabel.c_str());
        QString toAppend = QString(" version %1").arg(version);
        pluginLabelVersioned.append(toAppend);
        _imp->_iconLabel->setToolTip(pluginLabelVersioned);
    }
    if (_imp->_helpButton) {
        
        
        
        Natron::EffectInstance* iseffect = dynamic_cast<Natron::EffectInstance*>(_imp->_holder);
        if (iseffect) {
            _imp->_pluginID = pluginID.c_str();
            _imp->_pluginVersionMajor = version;
            _imp->_pluginVersionMinor = 0;
            _imp->_helpButton->setToolTip(helpString());
        }
        
    }
}

void
DockablePanel::setPluginIcon(const QPixmap& pix)
{
    if (_imp->_iconLabel) {
        _imp->_iconLabel->setPixmap(pix);
        if (!_imp->_iconLabel->isVisible()) {
            _imp->_iconLabel->show();
        }
    }
}

void
DockablePanel::setPluginDescription(const std::string& description)
{
    _imp->_helpToolTip = description.c_str();
    _imp->_helpButton->setToolTip(helpString());
}


void
DockablePanel::onNodeScriptChanged(const QString& label)
{
    if (_imp->_nameLineEdit) {
        _imp->_nameLineEdit->setToolTip("Script name: <br/><b><font size=4>" + label + "</b></font>");
    } else if (_imp->_nameLabel) {
        _imp->_nameLabel->setToolTip("Script name: <br/><b><font size=4>" + label + "</b></font>");
    }
}

void
DockablePanel::setUserPageActiveIndex()
{
    for (int i = 0; i < _imp->_tabWidget->count(); ++i) {
        if (_imp->_tabWidget->tabText(i) == NATRON_USER_MANAGED_KNOBS_PAGE_LABEL) {
            _imp->_tabWidget->setCurrentIndex(i);
            break;
        }
    }
}

int
DockablePanel::getPagesCount() const
{
    return _imp->_pages.size();
}


void
DockablePanel::onGuiClosing()
{
    if (_imp->_holder) {
        _imp->_holder->discardPanelPointer();
    }
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

class NoWheelTabBar : public QTabBar
{
public:
    
    NoWheelTabBar(QWidget* parent) : QTabBar(parent) {}
    
private:
    
    virtual void wheelEvent(QWheelEvent* event) OVERRIDE FINAL
    {
        //ignore wheel events so it doesn't scroll the tabs
        QWidget::wheelEvent(event);
    }
};


DockablePanelTabWidget::DockablePanelTabWidget(Gui* gui,QWidget* parent)
    : QTabWidget(parent)
    , _gui(gui)
{
    setFocusPolicy(Qt::ClickFocus);
    QTabBar* tabbar = new NoWheelTabBar(this);
    tabbar->setFocusPolicy(Qt::ClickFocus);
    setTabBar(tabbar);
}

void
DockablePanelTabWidget::keyPressEvent(QKeyEvent* event)
{
    Qt::Key key = (Qt::Key)event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    
    if (isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key)) {
        if ( _gui->getNodeGraph()->getLastSelectedViewer() ) {
            _gui->getNodeGraph()->getLastSelectedViewer()->previousFrame();
        }
    } else if (isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( _gui->getNodeGraph()->getLastSelectedViewer() ) {
            _gui->getNodeGraph()->getLastSelectedViewer()->nextFrame();
        }
    } else {
        QTabWidget::keyPressEvent(event);
    }
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
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = multiPanel->getInstances();
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            const std::vector<boost::shared_ptr<KnobI> > & knobs = it->first.lock()->getKnobs();
            for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
                Button_Knob* isBtn = dynamic_cast<Button_Knob*>( it2->get() );
                Page_Knob* isPage = dynamic_cast<Page_Knob*>( it2->get() );
                Group_Knob* isGroup = dynamic_cast<Group_Knob*>( it2->get() );
                Separator_Knob* isSeparator = dynamic_cast<Separator_Knob*>( it2->get() );
                if ( !isBtn && !isPage && !isGroup && !isSeparator && ( (*it2)->getName() != kUserLabelKnobName ) &&
                     ( (*it2)->getName() != kNatronOfxParamStringSublabelName ) ) {
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
                 ( (*it)->getName() != kNatronOfxParamStringSublabelName ) ) {
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
    
    QString newName = _imp->_nameLineEdit->text();
    
    QString oldName;
    if (panel) {
        node = panel->getNode();
        assert(node);
        oldName = QString(node->getNode()->getLabel().c_str());
        
    }
    
    if (oldName == newName) {
        return;
    }

    assert(node);
    
    if (node) {
        pushUndoCommand(new RenameNodeUndoRedoCommand(node, oldName, newName));
    } 
   
}

static void findKnobsOnSameLine(const std::vector<boost::shared_ptr<KnobI> >& knobs,
                                const boost::shared_ptr<KnobI>& ref,
                                std::vector<boost::shared_ptr<KnobI> >& knobsOnSameLine)
{
    int idx = -1;
    for (U32 k = 0; k < knobs.size() ; ++k) {
        if (knobs[k] == ref) {
            idx = k;
            break;
        }
    }
    assert(idx != -1);

    ///find all knobs backward that are on the same line.
    int k = idx - 1;
    boost::shared_ptr<KnobI> parent = ref->getParentKnob();
    
    while ( k >= 0 && !knobs[k]->isNewLineActivated()) {
        if (parent) {
            assert(knobs[k]->getParentKnob() == parent);
            knobsOnSameLine.push_back(knobs[k]);
        } else {
            if (!knobs[k]->getParentKnob() &&
                !dynamic_cast<Page_Knob*>(knobs[k].get()) &&
                !dynamic_cast<Group_Knob*>(knobs[k].get())) {
                knobsOnSameLine.push_back(knobs[k]);
            }
        }
        --k;
    }
    
    ///find all knobs forward that are on the same line.
    k = idx;
    while ( k < (int)(knobs.size() - 1) && !knobs[k]->isNewLineActivated()) {
        if (parent) {
            assert(knobs[k + 1]->getParentKnob() == parent);
            knobsOnSameLine.push_back(knobs[k + 1]);
        } else {
            if (!knobs[k + 1]->getParentKnob() &&
                !dynamic_cast<Page_Knob*>(knobs[k + 1].get()) &&
                !dynamic_cast<Group_Knob*>(knobs[k + 1].get())) {
                knobsOnSameLine.push_back(knobs[k + 1]);
            }
        }
        ++k;
    }

}

void
DockablePanelPrivate::initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> > & knobs,
                                           QWidget* lastRowWidget)
{
    for (U32 i = 0; i < knobs.size(); ++i) {
        
        bool makeNewLine = true;
        Group_Knob *isGroup = dynamic_cast<Group_Knob*>(knobs[i].get());
        Page_Knob *isPage = dynamic_cast<Page_Knob*>(knobs[i].get());
        
        ////The knob  will have a vector of all other knobs on the same line.
        std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;
        
        
        //If the knob is dynamic (i:e created after the initial creation of knobs)
        //it can be added as part of a group defined earlier hence we have to insert it at the proper index.
        boost::shared_ptr<KnobI> parentKnob = knobs[i]->getParentKnob();
        Group_Knob* isParentGroup = dynamic_cast<Group_Knob*>(parentKnob.get());

        
        if (!isPage && !isGroup) {
            if ( (i > 0) && !knobs[i - 1]->isNewLineActivated() ) {
                makeNewLine = false;
            }
            
            Page_Knob* isParentPage = dynamic_cast<Page_Knob*>(parentKnob.get());
            if (isParentPage) {
                const std::vector<boost::shared_ptr<KnobI> > & children = isParentPage->getChildren();
                findKnobsOnSameLine(children, knobs[i], knobsOnSameLine);
            } else if (isParentGroup) {
                const std::vector<boost::shared_ptr<KnobI> > & children = isParentGroup->getChildren();
                findKnobsOnSameLine(children, knobs[i], knobsOnSameLine);
            } else {
                findKnobsOnSameLine(knobs, knobs[i], knobsOnSameLine);
            }
            
        }
        
        KnobGui* newGui = findKnobGuiOrCreate(knobs[i],makeNewLine,lastRowWidget,knobsOnSameLine);
        ///childrens cannot be on the same row than their parent
        if (!isGroup && newGui) {
            lastRowWidget = newGui->getFieldContainer();
        }
    }
    
}

void
DockablePanel::initializeKnobsInternal()
{
    std::vector< boost::shared_ptr<KnobI> > knobs = _imp->_holder->getKnobs();
    _imp->initializeKnobVector(knobs, NULL);

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
                if ( p && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_INFO) && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
                    parentTab = _imp->addPage(p, p->getDescription().c_str() );
                    break;
                }
            }

            ///Last resort: The plug-in didn't specify ANY page, just put it into the default page
            if ( parentTab == _imp->_pages.end() ) {
                parentTab = _imp->addPage(NULL, _imp->_defaultPageName);
            }
        }

        assert( parentTab != _imp->_pages.end() );
        
        QGridLayout* layout = 0;
        if (_imp->_useScrollAreasForTabs) {
            layout = dynamic_cast<QGridLayout*>( dynamic_cast<QScrollArea*>(parentTab->second.tab)->widget()->layout() );
        } else {
            layout = dynamic_cast<QGridLayout*>( parentTab->second.tab->layout() );
        }
        assert(layout);
        layout->addWidget(roto, layout->rowCount(), 0 , 1, 2);
    }

    initializeExtraGui(_imp->_mainLayout);
    
}

void
DockablePanel::initializeKnobs()
{
    initializeKnobsInternal();
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

    KnobGui* ret =  appPTR->createGuiForKnob(knob,_publicInterface);
    if (!ret) {
        qDebug() << "Failed to create Knob GUI";

        return NULL;
    }
    _knobs.insert( make_pair(knob, ret) );

    return ret;
}

boost::shared_ptr<Page_Knob>
DockablePanelPrivate::ensureDefaultPageKnobCreated()
{
    const std::vector< boost::shared_ptr<KnobI> > & knobs = _holder->getKnobs();
    ///find in all knobs a page param to set this param into
    for (U32 i = 0; i < knobs.size(); ++i) {
        boost::shared_ptr<Page_Knob> p = boost::dynamic_pointer_cast<Page_Knob>(knobs[i]);
        if ( p && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_INFO) && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
            addPage(p.get(),  p->getDescription().c_str() );
            return p;
        }
    }

    
    boost::shared_ptr<KnobI> knob = _holder->getKnobByName(_defaultPageName.toStdString());
    boost::shared_ptr<Page_Knob> pk;
    if (!knob) {
        pk = Natron::createKnob<Page_Knob>(_holder, _defaultPageName.toStdString());
    } else {
        pk = boost::dynamic_pointer_cast<Page_Knob>(knob);
    }
    assert(pk);
    addPage(pk.get(), _defaultPageName);
    return pk;
}

PageMap::iterator
DockablePanelPrivate::getDefaultPage(const boost::shared_ptr<KnobI> &knob)
{
    PageMap::iterator page = _pages.end();
    const std::vector< boost::shared_ptr<KnobI> > & knobs = _holder->getKnobs();
    ///find in all knobs a page param to set this param into
    for (U32 i = 0; i < knobs.size(); ++i) {
        Page_Knob* p = dynamic_cast<Page_Knob*>( knobs[i].get() );
        if ( p && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_INFO) && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
            page = addPage(p,  p->getDescription().c_str() );
            p->addKnob(knob);
            break;
        }
    }
    
    if ( page == _pages.end() ) {
        ///the plug-in didn't specify any page
        ///for this param, put it in the first page that is not the default page.
        ///If there is still no page, put it in the default tab.
        for (PageMap::iterator it = _pages.begin(); it != _pages.end(); ++it) {
            if (it->first != _defaultPageName) {
                page = it;
                break;
            }
        }
        
        ///Last resort: The plug-in didn't specify ANY page, just put it into the default page
        if ( page == _pages.end() ) {
            page = addPage(NULL, _defaultPageName);
        }
    }
    return page;
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
        if (isPage->getChildren().empty()) {
            return 0;
        }
        QString pageName(isPage->getDescription().c_str());
        addPage(isPage.get(), pageName);

    } else {
        
        
        ret = createKnobGui(knob);

        boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
        boost::shared_ptr<Group_Knob> parentIsGroup = boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
        Group_KnobGui* parentGui = 0;
        /// if this knob is within a group, make sure the group is created so far
        if (parentIsGroup) {
            parentGui = dynamic_cast<Group_KnobGui*>( findKnobGuiOrCreate( parentKnob,true,ret->getFieldContainer() ) );
        }

        ///So far the knob could have no parent, in which case we force it to be in the default page.
        if (!parentKnob) {
            boost::shared_ptr<Page_Knob> defPage = ensureDefaultPageKnobCreated();
            defPage->addKnob(knob);
            parentKnob = defPage;
        }

        ///if widgets for the KnobGui have already been created, don't do the following
        ///For group only create the gui if it is not  a tab.
        if (isGroup  && isGroup->isTab()) {
            
            Page_Knob* parentIsPage = dynamic_cast<Page_Knob*>(parentKnob.get());
            if (!parentKnob || parentIsPage) {
                PageMap::iterator page = _pages.end();
                if (!parentKnob) {
                    page = getDefaultPage(knob);
                } else {
                    page = addPage(parentIsPage, parentIsPage->getDescription().c_str() );
                }
                bool existed = true;
                if (!page->second.groupAsTab) {
                    existed = false;
                    page->second.groupAsTab = new TabGroup(_publicInterface);
                }
                page->second.groupAsTab->addTab(isGroup, isGroup->getDescription().c_str());
                
                ///retrieve the form layout
                QGridLayout* layout;
                if (_useScrollAreasForTabs) {
                    layout = dynamic_cast<QGridLayout*>(
                                                        dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout() );
                } else {
                    layout = dynamic_cast<QGridLayout*>( page->second.tab->layout() );
                }
                assert(layout);
                if (!existed) {
                    layout->addWidget(page->second.groupAsTab, page->second.currentRow, 0, 1, 2);
                }
            } else {
                assert(parentIsGroup);
                assert(parentGui);
                TabGroup* groupAsTab = parentGui->getOrCreateTabWidget();
                
                groupAsTab->addTab(isGroup, isGroup->getDescription().c_str());
                
                if (parentIsGroup && parentIsGroup->isTab()) {
                    ///insert the tab in the layout of the parent
                    ///Find the page in the parentParent group
                    boost::shared_ptr<KnobI> parentParent = parentKnob->getParentKnob();
                    assert(parentParent);
                    boost::shared_ptr<Group_Knob> parentParentIsGroup = boost::dynamic_pointer_cast<Group_Knob>(parentParent);
                    boost::shared_ptr<Page_Knob> parentParentIsPage = boost::dynamic_pointer_cast<Page_Knob>(parentParent);
                    assert(parentParentIsGroup || parentParentIsPage);
                    TabGroup* parentTabGroup = 0;
                    if (parentParentIsPage) {
                        PageMap::iterator page = addPage(parentParentIsPage.get(),parentParentIsPage->getDescription().c_str());
                        assert(page != _pages.end());
                        parentTabGroup = page->second.groupAsTab;
                    } else {
                        std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _knobs.find(parentParent);
                        assert(it != _knobs.end());
                        Group_KnobGui* parentParentGroupGui = dynamic_cast<Group_KnobGui*>(it->second);
                        assert(parentParentGroupGui);
                        parentTabGroup = parentParentGroupGui->getOrCreateTabWidget();
                    }
                    
                    QGridLayout* layout = parentTabGroup->addTab(parentIsGroup, parentIsGroup->getDescription().c_str());
                    assert(layout);
                    layout->addWidget(groupAsTab, 0, 0, 1, 2);
                    
                } else {
                    Page_Knob* topLevelPage = getTopLevelPageForKnob(knob.get());
                    PageMap::iterator page = addPage(topLevelPage,topLevelPage->getDescription().c_str());
                    assert(page != _pages.end());
                    ///retrieve the form layout
                    QGridLayout* layout;
                    if (_useScrollAreasForTabs) {
                        layout = dynamic_cast<QGridLayout*>(
                                                            dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout() );
                    } else {
                        layout = dynamic_cast<QGridLayout*>( page->second.tab->layout() );
                    }
                    assert(layout);
                    
                    layout->addWidget(groupAsTab, page->second.currentRow, 0, 1, 2);
                }
                
            }
            
        } else if (!ret->hasWidgetBeenCreated()) {
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
            assert(isTopLevelParentAPage);
            
            PageMap::iterator page = addPage(isTopLevelParentAPage, isTopLevelParentAPage->getDescription().c_str());
            assert( page != _pages.end() );

            ///retrieve the form layout
            QGridLayout* layout;
            if (_useScrollAreasForTabs) {
                layout = dynamic_cast<QGridLayout*>(
                    dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout() );
            } else {
                layout = dynamic_cast<QGridLayout*>( page->second.tab->layout() );
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
                fieldLayout->setContentsMargins(3,0,0,NATRON_SETTINGS_VERTICAL_SPACING_PIXELS);
                fieldLayout->setSpacing(2);
            } else {
                ///otherwise re-use the last row's widget and layout
                assert(lastRowWidget);
                fieldContainer = lastRowWidget;
                fieldLayout = dynamic_cast<QHBoxLayout*>( fieldContainer->layout() );
            }
            
            assert(fieldContainer);
            assert(fieldLayout);
            
            ///Create the label if needed
            ClickableLabel* label = 0;
            
            if (ret->showDescriptionLabel() && !knob->getDescription().empty()) {
                label = new ClickableLabel("",page->second.tab);
                label->setText_overload( QString(QString( ret->getKnob()->getDescription().c_str() ) + ":") );
                QObject::connect( label, SIGNAL( clicked(bool) ), ret, SIGNAL( labelClicked(bool) ) );
            }

            /*
             * Find out in which layout the knob should be: either in the layout of the page or in the layout of
             * the nearest parent group tab in the hierarchy
             */
            boost::shared_ptr<Group_Knob> closestParentGroupTab;
            boost::shared_ptr<KnobI> parentTmp = parentKnob;
            assert(parentKnobTmp);
            while (!closestParentGroupTab) {
                boost::shared_ptr<Group_Knob> parentGroup = boost::dynamic_pointer_cast<Group_Knob>(parentTmp);
                if (parentGroup && parentGroup->isTab()) {
                    closestParentGroupTab = parentGroup;
                }
                parentTmp = parentTmp->getParentKnob();
                if (!parentTmp) {
                    break;
                }
                
            }
            
            if (closestParentGroupTab) {
                
                /*
                 * At this point we know that the parent group (which is a tab in the TabWidget) will have at least 1 knob
                 * so ensure it is added to the TabWidget.
                 * There are 2 possibilities, either the parent of the group tab is another group, in which case we have to 
                 * make sure the TabWidget is visible in the parent TabWidget of the group, otherwise we just add the TabWidget
                 * to the on of the page.
                 */
                
                boost::shared_ptr<KnobI> parentParent = closestParentGroupTab->getParentKnob();
                Group_Knob* parentParentIsGroup = dynamic_cast<Group_Knob*>(parentParent.get());
                Page_Knob* parentParentIsPage = dynamic_cast<Page_Knob*>(parentParent.get());
                
                assert(parentParentIsGroup || parentParentIsPage);
                if (parentParentIsGroup) {
                    Group_KnobGui* parentParentGroupGui = dynamic_cast<Group_KnobGui*>(findKnobGuiOrCreate(parentParent, true,
                                                                                                           ret->getFieldContainer()));
                    assert(parentParentGroupGui);
                    TabGroup* groupAsTab = parentParentGroupGui->getOrCreateTabWidget();
                    assert(groupAsTab);
                    layout = groupAsTab->addTab(closestParentGroupTab, closestParentGroupTab->getDescription().c_str());
                    
                } else if (parentParentIsPage) {
                    PageMap::iterator page = addPage(parentParentIsPage, parentParentIsPage->getDescription().c_str());
                    assert(page != _pages.end());
                    assert(page->second.groupAsTab);
                    layout = page->second.groupAsTab->addTab(closestParentGroupTab, closestParentGroupTab->getDescription().c_str());
                }
                assert(layout);
                
            }
            
            ///fill the fieldLayout with the widgets
            ret->createGUI(layout,fieldContainer,label,fieldLayout,makeNewLine,knobsOnSameLine);
            
            
            ret->setEnabledSlot();
            
            ///Must add the row to the layout before calling setSecret()
            if (makeNewLine) {
                int rowIndex;
                if (closestParentGroupTab) {
                    rowIndex = layout->rowCount();
                } else if (parentGui && knob->isDynamicallyCreated()) {
                    const std::list<KnobGui*>& children = parentGui->getChildren();
                    if (children.empty()) {
                        rowIndex = parentGui->getActualIndexInLayout();
                    } else {
                        rowIndex = children.back()->getActualIndexInLayout();
                    }
                    ++rowIndex;
                } else {
                    rowIndex = page->second.currentRow;
                }
                
                fieldContainer->layout()->setAlignment(Qt::AlignLeft);
                
                
                if (!label || !ret->showDescriptionLabel() || label->text().isEmpty()) {
                    layout->addWidget(fieldContainer,rowIndex,0, 1, 2);
                } else {
                    
                    layout->addWidget(fieldContainer,rowIndex,1, 1, 1);
                    layout->addWidget(label, rowIndex, 0, 1, 1, Qt::AlignRight);
        
                }
                
                
                if (closestParentGroupTab) {
                    ///See http://stackoverflow.com/questions/14033902/qt-qgridlayout-automatically-centers-moves-items-to-the-middle for
                    ///a bug of QGridLayout: basically all items are centered, but we would like to add stretch in the bottom of the layout.
                    ///To do this we add an empty widget with an expanding vertical size policy.
                    QWidget* foundSpacer = 0;
                    for (int i = 0; i < layout->rowCount(); ++i) {
                        QLayoutItem* item = layout->itemAtPosition(i, 0);
                        if (!item) {
                            continue;
                        }
                        QWidget* w = item->widget();
                        if (!w) {
                            continue;
                        }
                        if (w->objectName() == "emptyWidget") {
                            foundSpacer = w;
                            break;
                        }
                    }
                    if (foundSpacer) {
                        layout->removeWidget(foundSpacer);
                    } else {
                        foundSpacer = new QWidget(layout->parentWidget());
                        foundSpacer->setObjectName("emptyWidget");
                        foundSpacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

                    }
                    
                    ///And add our stretch
                    layout->addWidget(foundSpacer,layout->rowCount(), 0, 1, 2);
                }
                
            }

            ret->setSecret();
            
            if (knob->isNewLineActivated() && ret->shouldAddStretch()) {
                fieldLayout->addStretch();
            }


            
            ///increment the row count
            ++page->second.currentRow;
            
            if (parentIsGroup) {
                assert(parentGui);
                parentGui->addKnob(ret);
            }

        } //  if ( !ret->hasWidgetBeenCreated() && ( !isGroup || !isGroup->isTab() ) ) {
        
    } // !isPage

    ///if the knob is a group, create all the children
    if (isGroup) {
        initializeKnobVector(isGroup->getChildren(), lastRowWidget);
    } else if (isPage) {
        initializeKnobVector(isPage->getChildren(), lastRowWidget);
    }

    return ret;
} // findKnobGuiOrCreate

void
RightClickableWidget::mousePressEvent(QMouseEvent* e)
{
    if (buttonDownIsRight(e)) {
        QWidget* underMouse = qApp->widgetAt(e->globalPos());
        if (underMouse == this) {
            Q_EMIT rightClicked(e->pos());
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
        Q_EMIT escapePressed();
    }
    QWidget::keyPressEvent(e);
}

void
RightClickableWidget::enterEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    QWidget* currentFocus = qApp->focusWidget();
    
    bool canSetFocus = !currentFocus ||
    dynamic_cast<ViewerGL*>(currentFocus) ||
    dynamic_cast<CurveWidget*>(currentFocus) ||
    dynamic_cast<Histogram*>(currentFocus) ||
    dynamic_cast<NodeGraph*>(currentFocus) ||
    dynamic_cast<QToolButton*>(currentFocus) ||
    currentFocus->objectName() == "Properties";
    
    if (canSetFocus) {
        setFocus();
    }
    QWidget::enterEvent(e);
}

PageMap::iterator
DockablePanelPrivate::addPage(Page_Knob* page,const QString & name)
{
    if (!_pagesEnabled && _pages.size() > 0) {
        return _pages.begin();
    }
    
    PageMap::iterator found = _pages.find(name);

    if ( found != _pages.end() ) {
        return found;
    }
    
    
    QWidget* newTab;
    QWidget* layoutContainer;
    if (_useScrollAreasForTabs) {
        assert(_tabWidget);
        QScrollArea* sa = new QScrollArea(_tabWidget);
        layoutContainer = new QWidget(sa);
        layoutContainer->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        sa->setWidgetResizable(true);
        sa->setWidget(layoutContainer);
        newTab = sa;
    } else {
        QWidget* parent;
        
        if (_tabWidget) {
            parent = _tabWidget;
        } else {
            parent = _publicInterface;
        };
        
        RightClickableWidget* clickableWidget = new RightClickableWidget(_publicInterface,parent);
        QObject::connect(clickableWidget,SIGNAL(rightClicked(QPoint)),_publicInterface,SLOT( onRightClickMenuRequested(QPoint) ) );
        QObject::connect(clickableWidget,SIGNAL(escapePressed()),_publicInterface,SLOT( closePanel() ) );
        clickableWidget->setFocusPolicy(Qt::NoFocus);
        newTab = clickableWidget;
        layoutContainer = newTab;
    }
    QGridLayout *tabLayout = new QGridLayout(layoutContainer);
    tabLayout->setObjectName("formLayout");
    layoutContainer->setLayout(tabLayout);
    tabLayout->setContentsMargins(1, 1, 1, 1);
    tabLayout->setColumnStretch(1, 1);
    tabLayout->setSpacing(NATRON_FORM_LAYOUT_LINES_SPACING);
    
    if (_tabWidget) {
        if (name == NATRON_USER_MANAGED_KNOBS_PAGE_LABEL || (page && page->isUserKnob())) {
            _tabWidget->insertTab(0,newTab,name);
            _tabWidget->setCurrentIndex(0);
        } else {
            _tabWidget->addTab(newTab,name);
        }
    } else {
        if (name == NATRON_USER_MANAGED_KNOBS_PAGE_LABEL || (page && page->isUserKnob())) {
            _horizLayout->insertWidget(0, newTab);
        } else {
            _horizLayout->addWidget(newTab);
        }
    }
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
    Q_EMIT undoneChange();
}

void
DockablePanel::onRedoPressed()
{
    _imp->_undoStack->redo();
    if (_imp->_undoButton && _imp->_redoButton) {
        _imp->_undoButton->setEnabled( _imp->_undoStack->canUndo() );
        _imp->_redoButton->setEnabled( _imp->_undoStack->canRedo() );
    }
    Q_EMIT redoneChange();
}

QString
DockablePanel::helpString() const
{
    //Base help
    QString tt = Qt::convertFromPlainText(_imp->_helpToolTip, Qt::WhiteSpaceNormal);

    Natron::EffectInstance* iseffect = dynamic_cast<Natron::EffectInstance*>(_imp->_holder);
    if (iseffect) {
        //Prepend the plugin ID
        if (!_imp->_pluginID.isEmpty()) {
            QString pluginLabelVersioned(_imp->_pluginID);
            QString toAppend = QString(" version %1.%2").arg(_imp->_pluginVersionMajor).arg(_imp->_pluginVersionMinor);
            pluginLabelVersioned.append(toAppend);

            if (!pluginLabelVersioned.isEmpty()) {
                QString toPrepend("<p><b>");
                toPrepend.append(pluginLabelVersioned);
                toPrepend.append("</b></p>");
                tt.prepend(toPrepend);
            }
        }
    }
    return tt;
}

void
DockablePanel::showHelp()
{
    Natron::EffectInstance* iseffect = dynamic_cast<Natron::EffectInstance*>(_imp->_holder);
    if (iseffect) {
        const Natron::Plugin* plugin = iseffect->getNode()->getPlugin();
        assert(plugin);
        if (plugin) {
            Natron::informationDialog(plugin->getPluginLabel().toStdString(), helpString().toStdString(), true);
        }
    }
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
        return;
    }
    setVisible(!c);
    {
        QMutexLocker l(&_imp->_isClosedMutex);
        if (c == _imp->_isClosed) {
            return;

        }
        _imp->_isClosed = c;
    }
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (nodePanel) {
        boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
        boost::shared_ptr<Natron::Node> internalNode = nodeGui->getNode();
        boost::shared_ptr<MultiInstancePanel> panel = getMultiInstancePanel();
        Gui* gui = getGui();

        if (!c) {
            gui->addNodeGuiToCurveEditor(nodeGui);
            
            NodeList children;
            internalNode->getChildrenMultiInstance(&children);
            for (NodeList::iterator it = children.begin() ; it != children.end(); ++it) {
                boost::shared_ptr<NodeGuiI> gui_i = (*it)->getNodeGui();
                assert(gui_i);
                boost::shared_ptr<NodeGui> childGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                assert(childGui);
                gui->addNodeGuiToCurveEditor(childGui);
            }
        } else {
            gui->removeNodeGuiFromCurveEditor(nodeGui);
            
            NodeList children;
            internalNode->getChildrenMultiInstance(&children);
            for (NodeList::iterator it = children.begin() ; it != children.end(); ++it) {
                boost::shared_ptr<NodeGuiI> gui_i = (*it)->getNodeGui();
                assert(gui_i);
                boost::shared_ptr<NodeGui> childGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                assert(childGui);
                gui->removeNodeGuiFromCurveEditor(childGui);
            }
        }
        
        if (panel) {
            ///show all selected instances
            const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & childrenInstances = panel->getInstances();
            std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator next = childrenInstances.begin();
			if (!childrenInstances.empty()) {
				++next;
			}
            for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = childrenInstances.begin();
                 it != childrenInstances.end(); ++it,++next) {
                if (c) {
                    it->first.lock()->hideKeyframesFromTimeline( next == childrenInstances.end() );
                } else if (!c && it->second) {
                    it->first.lock()->showKeyframesOnTimeline( next == childrenInstances.end() );
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
    Q_EMIT closeChanged(c);

} // setClosed

void
DockablePanel::closePanel()
{
    if (_imp->_floating) {
        floatPanel();
        return;
    }
    {
        QMutexLocker l(&_imp->_isClosedMutex);
        if (_imp->_isClosed) {
            return;
        }
        _imp->_isClosed = true;
    }
    close();

    _imp->_gui->removeVisibleDockablePanel(this);
    _imp->_gui->buildTabFocusOrderPropertiesBin();
    
    
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (nodePanel) {
        
        boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
        boost::shared_ptr<Natron::Node> internalNode = nodeGui->getNode();
        _imp->_gui->removeNodeGuiFromCurveEditor(nodeGui);
        
        NodeList children;
        internalNode->getChildrenMultiInstance(&children);
        for (NodeList::iterator it = children.begin() ; it != children.end(); ++it) {
            boost::shared_ptr<NodeGuiI> gui_i = (*it)->getNodeGui();
            assert(gui_i);
            boost::shared_ptr<NodeGui> childGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
            assert(childGui);
            _imp->_gui->removeNodeGuiFromCurveEditor(childGui);
        }
        
        
        internalNode->hideKeyframesFromTimeline(true);
        boost::shared_ptr<MultiInstancePanel> panel = getMultiInstancePanel();
        if (panel) {
            const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & childrenInstances = panel->getInstances();
            std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator next = childrenInstances.begin();
            if (!childrenInstances.empty()) {
                ++next;
            }
            for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = childrenInstances.begin();
                 it != childrenInstances.end(); ++it,++next) {
                
                NodePtr node = it->first.lock();
                if ( it->second && (node != internalNode) ) {
                    node->hideKeyframesFromTimeline( next == childrenInstances.end() );
                }
				if (next == childrenInstances.end()) {
					--next;
				}
            }
        } else {
            internalNode->showKeyframesOnTimeline(false);
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
    Q_EMIT closeChanged(true);

    
}

void
DockablePanel::minimizeOrMaximize(bool toggled)
{
    _imp->_minimized = toggled;
    if (_imp->_minimized) {
        Q_EMIT minimized();
    } else {
        Q_EMIT maximized();
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
DockablePanel::deleteKnobGui(const boost::shared_ptr<KnobI>& knob)
{
    Page_Knob* isPage = dynamic_cast<Page_Knob*>(knob.get());
    if (isPage && _imp->_pagesEnabled) {
        PageMap::iterator found = _imp->_pages.find(isPage->getDescription().c_str());
        if (found != _imp->_pages.end()) {
            if (_imp->_tabWidget) {
                int index = _imp->_tabWidget->indexOf(found->second.tab);
                if (index != -1) {
                    _imp->_tabWidget->removeTab(index);
                }
            }
            found->second.tab->deleteLater();
            found->second.currentRow = 0;
            _imp->_pages.erase(found);
            
            const std::vector<boost::shared_ptr<KnobI> >& children = isPage->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                deleteKnobGui(children[i]);
            }
        }
        
    } else {
        
        Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob.get());
        if (isGrp) {
            const std::vector<boost::shared_ptr<KnobI> >& children = isGrp->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                deleteKnobGui(children[i]);
            }
        }
        if (isGrp && isGrp->isTab()) {
            //find parent page
            boost::shared_ptr<KnobI> parent = knob->getParentKnob();
            Page_Knob* isParentPage = dynamic_cast<Page_Knob*>(parent.get());
            Group_Knob* isParentGroup = dynamic_cast<Group_Knob*>(parent.get());
            
            assert(isParentPage || isParentGroup);
            TabGroup* groupAsTab = 0;
            if (isParentPage) {
                PageMap::iterator page = _imp->_pages.find(isParentPage->getDescription().c_str());
                assert(page != _imp->_pages.end());
                assert(page->second.groupAsTab);
                groupAsTab = page->second.groupAsTab;
                
            } else {
                std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator found  = _imp->_knobs.find(knob);
                assert(found != _imp->_knobs.end());
                Group_KnobGui* parentGroupGui = dynamic_cast<Group_KnobGui*>(found->second);
                assert(parentGroupGui);
                groupAsTab = parentGroupGui->getOrCreateTabWidget();
            }
            assert(groupAsTab);
            groupAsTab->removeTab(isGrp);
            
            std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.find(knob);
            if (it != _imp->_knobs.end()) {
                it->first->setKnobGuiPointer(0);
                delete it->second;
                _imp->_knobs.erase(it);
            }
        
        } else {
            
            std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.find(knob);
            if (it != _imp->_knobs.end()) {
                it->second->removeGui();
                it->first->setKnobGuiPointer(0);
                delete it->second;
                _imp->_knobs.erase(it);
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
    if (_imp->_mode != eHeaderModeNoHeader) {
        _imp->_headerLayout->insertWidget(index, widget);
    }
}

void
DockablePanel::appendHeaderWidget(QWidget* widget)
{
    if (_imp->_mode != eHeaderModeNoHeader) {
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
    if (_imp->_mode != eHeaderModeReadOnlyName && _imp->_colorButton) {
        QPixmap p(15,15);
        p.fill(color);
        _imp->_colorButton->setIcon( QIcon(p) );
        if (_imp->_verticalColorBar) {
            _imp->_verticalColorBar->setColor(color);
        }
    }
}

void
DockablePanel::onOverlayColorDialogColorChanged(const QColor& color)
{
    
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (!nodePanel) {
        return;
    }
    boost::shared_ptr<Natron::Node> node = nodePanel->getNode()->getNode();
    if (!node) {
        return;
    }

    
    if (_imp->_mode  != eHeaderModeReadOnlyName && _imp->_overlayButton) {
        QPixmap p(15,15);
        p.fill(color);
        _imp->_overlayButton->setIcon( QIcon(p) );
        {
            QMutexLocker k(&_imp->_currentColorMutex);
            _imp->_overlayColor = color;
            _imp->_hasOverlayColor = true;
        }

        std::list<boost::shared_ptr<Natron::Node> > overlayNodes;
        getGui()->getNodesEntitledForOverlays(overlayNodes);
        std::list<boost::shared_ptr<Natron::Node> >::iterator found = std::find(overlayNodes.begin(),overlayNodes.end(),node);
        if (found != overlayNodes.end()) {
            getGui()->getApp()->redrawAllViewers();
        }
    }
}

void
DockablePanel::onColorButtonClicked()
{
    QColorDialog dialog(this);
    QColor oldColor;
    {
        oldColor = getCurrentColor();
        dialog.setCurrentColor(oldColor);
    }
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onColorDialogColorChanged(QColor) ) );

    if ( dialog.exec() ) {
        QColor c = dialog.currentColor();
        Q_EMIT colorChanged(c);
    } else {
        onColorDialogColorChanged(oldColor);
    }
}

void
DockablePanel::onOverlayButtonClicked()
{
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (!nodePanel) {
        return;
    }
    boost::shared_ptr<Natron::Node> node = nodePanel->getNode()->getNode();
    if (!node) {
        return;
    }
    QColorDialog dialog(this);
    QColor oldColor;
    bool hadOverlayColor;
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        dialog.setCurrentColor(_imp->_overlayColor);
        oldColor = _imp->_overlayColor;
        hadOverlayColor = _imp->_hasOverlayColor;
    }
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onOverlayColorDialogColorChanged(QColor) ) );
    
    if ( dialog.exec() ) {
        QColor c = dialog.currentColor();
        {
            QMutexLocker locker(&_imp->_currentColorMutex);
            _imp->_overlayColor = c;
            _imp->_hasOverlayColor = true;
        }
    } else {
        if (!hadOverlayColor) {
            {
                QMutexLocker locker(&_imp->_currentColorMutex);
                _imp->_hasOverlayColor = false;
            }
            QPixmap pixOverlay;
            appPTR->getIcon(Natron::NATRON_PIXMAP_OVERLAY,&pixOverlay);
            _imp->_overlayButton->setIcon(QIcon(pixOverlay));
        }
    }
    std::list<boost::shared_ptr<Natron::Node> > overlayNodes;
    getGui()->getNodesEntitledForOverlays(overlayNodes);
    std::list<boost::shared_ptr<Natron::Node> >::iterator found = std::find(overlayNodes.begin(),overlayNodes.end(),node);
    if (found != overlayNodes.end()) {
        getGui()->getApp()->redrawAllViewers();
    }

}


QColor
DockablePanel::getOverlayColor() const
{
    QMutexLocker locker(&_imp->_currentColorMutex);
    return _imp->_overlayColor;
}

bool
DockablePanel::hasOverlayColor() const
{
    QMutexLocker locker(&_imp->_currentColorMutex);
    return _imp->_hasOverlayColor;
}

void
DockablePanel::setCurrentColor(const QColor & c)
{
    onColorDialogColorChanged(c);
}

void
DockablePanel::resetDefaultOverlayColor()
{
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (!nodePanel) {
        return;
    }
    boost::shared_ptr<Natron::Node> node = nodePanel->getNode()->getNode();
    if (!node) {
        return;
    }
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        _imp->_hasOverlayColor = false;
    }
    QPixmap pixOverlay;
    appPTR->getIcon(Natron::NATRON_PIXMAP_OVERLAY,&pixOverlay);
    _imp->_overlayButton->setIcon(QIcon(pixOverlay));
    
    std::list<boost::shared_ptr<Natron::Node> > overlayNodes;
    getGui()->getNodesEntitledForOverlays(overlayNodes);
    std::list<boost::shared_ptr<Natron::Node> >::iterator found = std::find(overlayNodes.begin(),overlayNodes.end(),node);
    if (found != overlayNodes.end()) {
        getGui()->getApp()->redrawAllViewers();
    }

}

void
DockablePanel::setOverlayColor(const QColor& c)
{
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        _imp->_overlayColor = c;
        _imp->_hasOverlayColor = true;
    }
    onOverlayColorDialogColorChanged(c);

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
        //menu.setFont( QFont(appFont,appFontSize) );

        QAction* userParams = new QAction(tr("Manage user parameters..."),&menu);
        menu.addAction(userParams);
        
      
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
        } else if (ret == userParams) {
            onManageUserParametersActionTriggered();
        } 
    }
} // onRightClickMenuRequested

void
DockablePanel::onManageUserParametersActionTriggered()
{
    ManageUserParamsDialog dialog(this,this);
    ignore_result(dialog.exec());
}

void
DockablePanel::setKeyOnAllParameters()
{
    int time = getGui()->getApp()->getTimeLine()->currentFrame();

    AddKeysCommand::KeysToAddList keys;
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->first->isAnimationEnabled()) {
            for (int i = 0; i < it->first->getDimension(); ++i) {
                std::list<CurveGui*> curves = getGui()->getCurveEditor()->findCurve(it->second,i);
                for (std::list<CurveGui*>::iterator it2 = curves.begin(); it2 != curves.end(); ++it2) {
                    boost::shared_ptr<AddKeysCommand::KeysForCurve> curveKeys(new AddKeysCommand::KeysForCurve);
                    curveKeys->curve = *it2;
                    
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
                std::list<CurveGui*> curves = getGui()->getCurveEditor()->findCurve(it->second,i);
                
                for (std::list<CurveGui*>::iterator it2 = curves.begin(); it2 != curves.end(); ++it2) {
                    KeyFrameSet keys = (*it2)->getInternalCurve()->getKeyFrames_mt_safe();
                    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                        keysToRemove.push_back( std::make_pair(*it2,*it) );
                    }
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

void
DockablePanel::onHideUnmodifiedButtonClicked(bool checked)
{
    if (checked) {
        _imp->_knobsVisibilityBeforeHideModif.clear();
        for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>(it->first.get());
            Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(it->first.get());
            if (!isGroup && !isParametric) {
                _imp->_knobsVisibilityBeforeHideModif.insert(std::make_pair(it->second,it->second->isSecretRecursive()));
                if (!it->first->hasModifications()) {
                    it->second->hide();
                }
            }
        }
    } else {
        for (std::map<KnobGui*,bool>::iterator it = _imp->_knobsVisibilityBeforeHideModif.begin();
             it != _imp->_knobsVisibilityBeforeHideModif.end(); ++it) {
            if (!it->second) {
                it->first->show();
            }
        }
    }
}

void
DockablePanel::scanForNewKnobs()
{
    
    std::list<Page_Knob*> userPages;
    getUserPages(userPages);
    
    if (_imp->_pagesEnabled) {
        boost::shared_ptr<Page_Knob> page = getUserPageKnob();
        userPages.push_back(page.get());
        for (std::list<Page_Knob*>::iterator it = userPages.begin(); it != userPages.end(); ++it) {
            PageMap::iterator foundPage = _imp->_pages.find((*it)->getDescription().c_str());
            if (foundPage != _imp->_pages.end()) {
                foundPage->second.currentRow = 0;
                std::vector<boost::shared_ptr<KnobI> > children = (*it)->getChildren();
                for (std::vector<boost::shared_ptr<KnobI> >::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                    deleteKnobGui(*it2);
                }
            }
        }
    } else {
        
        std::vector<boost::shared_ptr<KnobI> > knobs = _imp->_holder->getKnobs();
        for (std::vector<boost::shared_ptr<KnobI> >::iterator it = knobs.begin(); it != knobs.end(); ++it) {
            deleteKnobGui(*it);
        }
        
    }
    
    _imp->initializeKnobVector(_imp->_holder->getKnobs(),NULL);
    NodeSettingsPanel* isNodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (isNodePanel) {
        isNodePanel->getNode()->getNode()->declarePythonFields();
    }
    
    
    ///Refresh the curve editor with potential new animated knobs
    if (isNodePanel) {
        boost::shared_ptr<NodeGui> node = isNodePanel->getNode();
        getGui()->getCurveEditor()->removeNode(node.get());
        getGui()->getCurveEditor()->addNode(node);
    }
}

VerticalColorBar::VerticalColorBar(QWidget* parent)
: QWidget(parent)
, _color(Qt::black)
{
    setFixedWidth(NATRON_VERTICAL_BAR_WIDTH);
}

void
VerticalColorBar::setColor(const QColor& color)
{
    _color = color;
    update();
}

QSize
VerticalColorBar::sizeHint() const
{
    return QWidget::sizeHint();
    //return QSize(5,1000);
}

void
VerticalColorBar::paintEvent(QPaintEvent* /*e*/)
{
    QPainter p(this);
    QPen pen;
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(NATRON_VERTICAL_BAR_WIDTH);
    pen.setColor(_color);
    p.setPen(pen);
    p.drawLine( 0, NATRON_VERTICAL_BAR_WIDTH, 0, height() - NATRON_VERTICAL_BAR_WIDTH);
}

NodeSettingsPanel::NodeSettingsPanel(const boost::shared_ptr<MultiInstancePanel> & multiPanel,
                                     Gui* gui,
                                     const boost::shared_ptr<NodeGui> &NodeUi,
                                     QVBoxLayout* container,
                                     QWidget *parent)
    : DockablePanel(gui,
                    multiPanel.get() != NULL ? dynamic_cast<KnobHolder*>( multiPanel.get() ) : NodeUi->getNode()->getLiveInstance(),
                    container,
                    DockablePanel::eHeaderModeFullyFeatured,
                    false,
                    NodeUi->getNode()->getLabel().c_str(),
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
    getNode()->removeSettingsPanel();
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
    getNode()->centerGraphOnIt();
}

RotoPanel*
NodeSettingsPanel::initializeRotoPanel()
{
    if ( getNode()->getNode()->isRotoNode() ) {
        return new RotoPanel(_nodeGUI.lock(),this);
    } else {
        return NULL;
    }
}

QColor
NodeSettingsPanel::getCurrentColor() const
{
    return getNode()->getCurrentColor();
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
    //menu.setFont(QFont(appFont,appFontSize));
    
    boost::shared_ptr<NodeGui> node = getNode();
    boost::shared_ptr<Natron::Node> master = node->getNode()->getMasterNode();
    
    QAction* importPresets = new QAction(tr("Import presets"),&menu);
    QObject::connect(importPresets,SIGNAL(triggered()),this,SLOT(onImportPresetsActionTriggered()));
    QAction* exportAsPresets = new QAction(tr("Export as presets"),&menu);
    QObject::connect(exportAsPresets,SIGNAL(triggered()),this,SLOT(onExportPresetsActionTriggered()));
    
    menu.addAction(importPresets);
    menu.addAction(exportAsPresets);
    menu.addSeparator();
    
    QAction* manageUserParams = new QAction(tr("Manage user parameters..."),&menu);
    QObject::connect(manageUserParams,SIGNAL(triggered()),this,SLOT(onManageUserParametersActionTriggered()));
    menu.addAction(manageUserParams);
   
    menu.addSeparator();

    
    QAction* setKeyOnAll = new QAction(tr("Set key on all parameters"),&menu);
    QObject::connect(setKeyOnAll,SIGNAL(triggered()),this,SLOT(setKeyOnAllParameters()));
    QAction* removeAnimationOnAll = new QAction(tr("Remove animation on all parameters"),&menu);
    QObject::connect(removeAnimationOnAll,SIGNAL(triggered()),this,SLOT(removeAnimationOnAllParameters()));
    menu.addAction(setKeyOnAll);
    menu.addAction(removeAnimationOnAll);
    
    if (master || node->getDagGui()->getGui()->isGUIFrozen()) {
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
            boost::shared_ptr<NodeSerialization> node(new NodeSerialization());
            iArchive >> boost::serialization::make_nvp("Node",*node);
            nodeSerialization.push_back(node);
        }
        
        
    } catch (const std::exception & e) {
        ifile.close();
        Natron::errorDialog("Presets",e.what());
        return;
    }
    boost::shared_ptr<NodeGui> node = getNode();
    if (nodeSerialization.front()->getPluginID() != node->getNode()->getPluginID()) {
        QString err = QString(tr("You cannot load ") + filename.c_str()  + tr(" which are presets for the plug-in ") +
                              nodeSerialization.front()->getPluginID().c_str() + tr(" on the plug-in ") +
                              node->getNode()->getPluginID().c_str());
        Natron::errorDialog(tr("Presets").toStdString(),err.toStdString());
        return;
    }
    
    node->restoreInternal(node,nodeSerialization);
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

    boost::shared_ptr<NodeGui> node = getNode();
    std::list<boost::shared_ptr<NodeSerialization> > nodeSerialization;
    node->serializeInternal(nodeSerialization);
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

struct TreeItem
{
    QTreeWidgetItem* item;
    boost::shared_ptr<KnobI> knob;
};

struct ManageUserParamsDialogPrivate
{
    DockablePanel* panel;
    
    QHBoxLayout* mainLayout;
    QTreeWidget* tree;
    
    std::list<TreeItem> items;
    
    
    QWidget* buttonsContainer;
    QVBoxLayout* buttonsLayout;
    
    Button* addButton;
    Button* editButton;
    Button* removeButton;
    Button* upButton;
    Button* downButton;
    Button* closeButton;

    
    ManageUserParamsDialogPrivate(DockablePanel* panel)
    : panel(panel)
    , mainLayout(0)
    , tree(0)
    , items()
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , editButton(0)
    , removeButton(0)
    , upButton(0)
    , downButton(0)
    , closeButton(0)
    {
        
    }
    
    boost::shared_ptr<Page_Knob> getUserPageKnob() const;
    
    void initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs);
    
    void rebuildUserPages();
};

ManageUserParamsDialog::ManageUserParamsDialog(DockablePanel* panel,QWidget* parent)
: QDialog(parent)
, _imp(new ManageUserParamsDialogPrivate(panel))
{
    _imp->mainLayout = new QHBoxLayout(this);
    
    _imp->tree = new QTreeWidget(this);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    //_imp->tree->setRootIsDecorated(false);
    _imp->tree->setItemsExpandable(true);
    _imp->tree->header()->setStretchLastSection(false);
    _imp->tree->setTextElideMode(Qt::ElideMiddle);
    _imp->tree->setContextMenuPolicy(Qt::CustomContextMenu);
    _imp->tree->header()->setStretchLastSection(true);
    _imp->tree->header()->hide();
    
    TreeItem userPageItem;
    userPageItem.item = new QTreeWidgetItem(_imp->tree);
    userPageItem.item->setText(0, NATRON_USER_MANAGED_KNOBS_PAGE);
    userPageItem.item->setExpanded(true);
    _imp->items.push_back(userPageItem);
    
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()),this,SLOT(onSelectionChanged()));
    
    std::list<KnobI*> markedKnobs;
    const std::vector<boost::shared_ptr<KnobI> >& knobs = panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->isUserKnob()) {
            
            Page_Knob* page = dynamic_cast<Page_Knob*>(it->get());
            if (page) {
                
                TreeItem pageItem;
                if (page->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                    pageItem.item = userPageItem.item;
                } else {
                    pageItem.item = new QTreeWidgetItem(_imp->tree);
                    pageItem.item->setText(0, page->getName().c_str());
                    pageItem.knob = *it;
                    pageItem.item->setExpanded(true);
                }
                _imp->items.push_back(pageItem);
                
                const std::vector<boost::shared_ptr<KnobI> >& children = page->getChildren();
                _imp->initializeKnobs(children, pageItem.item, markedKnobs);
                
            }
        }
    }
    
    _imp->mainLayout->addWidget(_imp->tree);
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QVBoxLayout(_imp->buttonsContainer);
    
    _imp->addButton = new Button(tr("Add"),_imp->buttonsContainer);
    _imp->addButton->setToolTip(Qt::convertFromPlainText(tr("Add a new parameter, group or page")));
    QObject::connect(_imp->addButton,SIGNAL(clicked(bool)),this,SLOT(onAddClicked()));
    _imp->buttonsLayout->addWidget(_imp->addButton);
    
    _imp->editButton = new Button(tr("Edit"),_imp->buttonsContainer);
    _imp->editButton->setToolTip(Qt::convertFromPlainText(tr("Edit the selected parameter")));
    QObject::connect(_imp->editButton,SIGNAL(clicked(bool)),this,SLOT(onEditClicked()));
    _imp->buttonsLayout->addWidget(_imp->editButton);
    
    _imp->removeButton = new Button(tr("Delete"),_imp->buttonsContainer);
    _imp->removeButton->setToolTip(Qt::convertFromPlainText(tr("Delete the selected parameter")));
    QObject::connect(_imp->removeButton,SIGNAL(clicked(bool)),this,SLOT(onDeleteClicked()));
    _imp->buttonsLayout->addWidget(_imp->removeButton);
    
    _imp->upButton = new Button(tr("Up"),_imp->buttonsContainer);
    _imp->upButton->setToolTip(Qt::convertFromPlainText(tr("Move the selected parameter one level up in the layout")));
    QObject::connect(_imp->upButton,SIGNAL(clicked(bool)),this,SLOT(onUpClicked()));
    _imp->buttonsLayout->addWidget(_imp->upButton);
    
    _imp->downButton = new Button(tr("Down"),_imp->buttonsContainer);
    _imp->downButton->setToolTip(Qt::convertFromPlainText(tr("Move the selected parameter one level down in the layout")));
    QObject::connect(_imp->downButton,SIGNAL(clicked(bool)),this,SLOT(onDownClicked()));
    _imp->buttonsLayout->addWidget(_imp->downButton);
    
    _imp->closeButton = new Button(tr("Close"),_imp->buttonsContainer);
    _imp->closeButton->setToolTip(Qt::convertFromPlainText(tr("Close this dialog")));
    QObject::connect(_imp->closeButton,SIGNAL(clicked(bool)),this,SLOT(onCloseClicked()));
    _imp->buttonsLayout->addWidget(_imp->closeButton);
    
    _imp->mainLayout->addWidget(_imp->buttonsContainer);
    onSelectionChanged();
    _imp->panel->setUserPageActiveIndex();

}

ManageUserParamsDialog::~ManageUserParamsDialog()
{
//    for (std::list<TreeItem>::iterator it = _imp->items.begin() ;it != _imp->items.end(); ++it) {
//        delete it->item;
//    }
}

void
ManageUserParamsDialogPrivate::initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs)
{
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2!=knobs.end(); ++it2) {
        
        if (std::find(markedKnobs.begin(),markedKnobs.end(),it2->get()) != markedKnobs.end()) {
            continue;
        }
        
        markedKnobs.push_back(it2->get());
        
        TreeItem i;
        i.knob = *it2;
        i.item = new QTreeWidgetItem(parent);
        i.item->setText(0, (*it2)->getName().c_str());
        i.item->setExpanded(true);
        items.push_back(i);
        
        Group_Knob* isGrp = dynamic_cast<Group_Knob*>(it2->get());
        if (isGrp) {
            initializeKnobs(isGrp->getChildren(), i.item, markedKnobs);
        }
    }
}

boost::shared_ptr<Page_Knob>
DockablePanel::getUserPageKnob() const
{
    return _imp->_holder->getOrCreateUserPageKnob();
}

boost::shared_ptr<Page_Knob>
ManageUserParamsDialogPrivate::getUserPageKnob() const
{
    return panel->getUserPageKnob();
}

void
DockablePanel::getUserPages(std::list<Page_Knob*>& userPages) const
{
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->isUserKnob()) {
            Page_Knob* isPage = dynamic_cast<Page_Knob*>(it->get());
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    }
}

void
ManageUserParamsDialogPrivate::rebuildUserPages()
{
    panel->scanForNewKnobs();
}

void
ManageUserParamsDialog::onAddClicked()
{
    AddKnobDialog dialog(_imp->panel,boost::shared_ptr<KnobI>(),this);
    if (dialog.exec()) {
        //Ensure the user page knob exists
        boost::shared_ptr<Page_Knob> userPageKnob = _imp->panel->getUserPageKnob();
        
        boost::shared_ptr<KnobI> knob = dialog.getKnob();
        QTreeWidgetItem* parent = 0;
        
        boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
        if (parentKnob) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->knob.get() == parentKnob.get()) {
                    parent = it->item;
                    break;
                }
            }
        }
        if (!parent) {
            Page_Knob* isPage = dynamic_cast<Page_Knob*>(knob.get());
            if (!isPage) {
                //Default to user page
                for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                    if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                        parent = it->item;
                        break;
                    }
                }
            }

        }
        TreeItem i;
        i.knob = knob;
        i.item = new QTreeWidgetItem;
        if (parent) {
            parent->addChild(i.item);
            parent->setExpanded(true);
        } else {
            _imp->tree->addTopLevelItem(i.item);
        }
        i.item->setText(0, knob->getName().c_str());
        _imp->items.push_back(i);
    }
    _imp->rebuildUserPages();
    _imp->panel->getGui()->getApp()->triggerAutoSave();
}

void
ManageUserParamsDialog::onDeleteClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end();++it) {
                if (it->item == selection[i]) {
                    it->knob->getHolder()->removeDynamicKnob(it->knob.get());
                    delete it->item;
                    _imp->items.erase(it);
                    
                    boost::shared_ptr<Page_Knob> userPage = _imp->getUserPageKnob();
                    if (userPage->getChildren().empty()) {
                        userPage->getHolder()->removeDynamicKnob(userPage.get());
                    }
                    _imp->panel->getGui()->getApp()->triggerAutoSave();
                    break;
                }
            }
        }
    }
}

void
ManageUserParamsDialog::onEditClicked()
{
    
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end();++it) {
                if (it->item == selection[i]) {
                    
                    std::list<KnobI*> listeners;
                    it->knob->getListeners(listeners);
                    if (!listeners.empty()) {
                        Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Edit parameter").toStdString(),
                                                                                tr("This parameter has one or several "
                                                                                   "parameters from which other parameters "
                                                                                   "of the project rely on through expressions "
                                                                                   "or links. Editing this parameter will "
                                                                                   "remove these expressions. Do you wish to continue ?").toStdString(), false);
                        if (rep == Natron::eStandardButtonNo) {
                            return;
                        }
                    }
                    
                    AddKnobDialog dialog(_imp->panel,it->knob,this);
                    if (dialog.exec()) {
                        it->knob = dialog.getKnob();
                        it->item->setText(0, it->knob->getName().c_str());
                        _imp->rebuildUserPages();
                        _imp->panel->getGui()->getApp()->triggerAutoSave();
                        break;
                    }
                }
                
            }
        }
    }
    
}

void
ManageUserParamsDialog::onUpClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end();++it) {
                if (it->item == selection[i]) {
                    
                    if (dynamic_cast<Page_Knob*>(it->knob.get())) {
                        break;
                    }
                    QTreeWidgetItem* parent = 0;
                    boost::shared_ptr<KnobI> parentKnob = it->knob->getParentKnob();
                    if (parentKnob) {
                        if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        } else {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->knob == parentKnob) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        }
                    }
                    
                    int index;
                    if (!parent) {
                        index = _imp->tree->indexOfTopLevelItem(it->item);
                    } else {
                        index = parent->indexOfChild(it->item);
                    }
                    if (index == 0) {
                        break;
                    }
                    _imp->panel->getHolder()->moveKnobOneStepUp(it->knob.get());
                    QList<QTreeWidgetItem*> children = it->item->takeChildren();
                    delete it->item;
                    
                    it->item = new QTreeWidgetItem;
                    it->item->setText(0, it->knob->getName().c_str());
                    if (!parent) {
                        _imp->tree->insertTopLevelItem(index - 1, it->item);
                    } else {
                        parent->insertChild(index - 1, it->item);
                    }
                    it->item->insertChildren(0, children);
                    it->item->setExpanded(true);
                    _imp->tree->clearSelection();
                    it->item->setSelected(true);
                    break;
                }
            }
            
        }
        _imp->rebuildUserPages();
    }
}

void
ManageUserParamsDialog::onDownClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end();++it) {
                if (it->item == selection[i]) {
                    if (dynamic_cast<Page_Knob*>(it->knob.get())) {
                        break;
                    }

                    QTreeWidgetItem* parent = 0;
                    boost::shared_ptr<KnobI> parentKnob = it->knob->getParentKnob();
                    if (parentKnob) {
                        if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        } else {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->knob == parentKnob) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        }
                    }
                    int index;
                    
                    if (!parent) {
                        index = _imp->tree->indexOfTopLevelItem(it->item);
                        if (index == _imp->tree->topLevelItemCount() - 1) {
                            break;
                        }
                    } else {
                        index = parent->indexOfChild(it->item);
                        if (index == parent->childCount() - 1) {
                            break;
                        }
                    }
                    
                    _imp->panel->getHolder()->moveKnobOneStepDown(it->knob.get());
                    QList<QTreeWidgetItem*> children = it->item->takeChildren();
                    delete it->item;
                    
                    
                    it->item = new QTreeWidgetItem;
                    if (parent) {
                        parent->insertChild(index + 1, it->item);
                    } else {
                        _imp->tree->insertTopLevelItem(index + 1, it->item);
                    }
                    it->item->setText(0, it->knob->getName().c_str());
                    it->item->insertChildren(0, children);
                    it->item->setExpanded(true);
                    _imp->tree->clearSelection();
                    it->item->setSelected(true);
                    break;
                }
            }
            
        }
        _imp->rebuildUserPages();
    }
}

void
ManageUserParamsDialog::onCloseClicked()
{
    accept();
}

void
ManageUserParamsDialog::onSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    bool canEdit = true;
    bool canMove = true;
    bool canDelete = true;
    if (!selection.isEmpty()) {
        QTreeWidgetItem* item = selection[0];
        if (item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
            canEdit = false;
            canDelete = false;
            canMove = false;
        }
        for (std::list<TreeItem>::iterator it = _imp->items.begin(); it!=_imp->items.end(); ++it) {
            if (it->item == item) {
                Page_Knob* isPage = dynamic_cast<Page_Knob*>(it->knob.get());
                Group_Knob* isGroup = dynamic_cast<Group_Knob*>(it->knob.get());
                if (isPage) {
                    canMove = false;
                    canEdit = false;
                } else if (isGroup) {
                    canEdit = false;
                }
                break;
            }
        }
    }
    
    _imp->removeButton->setEnabled(selection.size() == 1 && canDelete);
    _imp->editButton->setEnabled(selection.size() == 1 && canEdit);
    _imp->upButton->setEnabled(selection.size() == 1 && canMove);
    _imp->downButton->setEnabled(selection.size() == 1 && canMove);
}

struct AddKnobDialogPrivate
{
    boost::shared_ptr<KnobI> knob;
    boost::shared_ptr<KnobSerialization> originalKnobSerialization;
    
    DockablePanel* panel;
    
    QVBoxLayout* vLayout;
    
    QWidget* mainContainer;
    QFormLayout* mainLayout;
    
    Natron::Label* typeLabel;
    ComboBox* typeChoice;
    Natron::Label* nameLabel;
    LineEdit* nameLineEdit;

    
    Natron::Label* labelLabel;
    LineEdit* labelLineEdit;
    
    Natron::Label* hideLabel;
    QCheckBox* hideBox;
    
    Natron::Label* startNewLineLabel;
    QCheckBox* startNewLineBox;
    
    Natron::Label* animatesLabel;
    QCheckBox* animatesCheckbox;
    
    Natron::Label* evaluatesLabel;
    QCheckBox* evaluatesOnChange;
    
    Natron::Label* tooltipLabel;
    QTextEdit* tooltipArea;
    
    Natron::Label* minLabel;
    SpinBox* minBox;
    
    Natron::Label* maxLabel;
    SpinBox* maxBox;
    
    Natron::Label* dminLabel;
    SpinBox* dminBox;
    
    Natron::Label* dmaxLabel;
    SpinBox* dmaxBox;
    
    enum DefaultValueType
    {
        eDefaultValueTypeInt,
        eDefaultValueTypeDouble,
        eDefaultValueTypeBool,
        eDefaultValueTypeString
    };
    
    
    Natron::Label* defaultValueLabel;
    SpinBox* default0;
    SpinBox* default1;
    SpinBox* default2;
    SpinBox* default3;
    LineEdit* defaultStr;
    QCheckBox* defaultBool;
    
    Natron::Label* menuItemsLabel;
    QTextEdit* menuItemsEdit;
    
    Natron::Label* multiLineLabel;
    QCheckBox* multiLine;
    
    Natron::Label* richTextLabel;
    QCheckBox* richText;
    
    Natron::Label* sequenceDialogLabel;
    QCheckBox* sequenceDialog;
    
    Natron::Label* multiPathLabel;
    QCheckBox* multiPath;
    
    Natron::Label* groupAsTabLabel;
    QCheckBox* groupAsTab;
    
    Natron::Label* parentGroupLabel;
    ComboBox* parentGroup;
    
    Natron::Label* parentPageLabel;
    ComboBox* parentPage;
    
    std::list<Group_Knob*> userGroups;
    std::list<Page_Knob*> userPages; //< all user pages except the "User" one
    
    AddKnobDialogPrivate(DockablePanel* panel)
    : knob()
    , originalKnobSerialization()
    , panel(panel)
    , vLayout(0)
    , mainContainer(0)
    , mainLayout(0)
    , typeLabel(0)
    , typeChoice(0)
    , nameLabel(0)
    , nameLineEdit(0)
    , labelLabel(0)
    , labelLineEdit(0)
    , hideLabel(0)
    , hideBox(0)
    , startNewLineLabel(0)
    , startNewLineBox(0)
    , animatesLabel(0)
    , animatesCheckbox(0)
    , evaluatesLabel(0)
    , evaluatesOnChange(0)
    , tooltipLabel(0)
    , tooltipArea(0)
    , minLabel(0)
    , minBox(0)
    , maxLabel(0)
    , maxBox(0)
    , dminLabel(0)
    , dminBox(0)
    , dmaxLabel(0)
    , dmaxBox(0)
    , defaultValueLabel(0)
    , default0(0)
    , default1(0)
    , default2(0)
    , default3(0)
    , defaultStr(0)
    , defaultBool(0)
    , menuItemsLabel(0)
    , menuItemsEdit(0)
    , multiLineLabel(0)
    , multiLine(0)
    , richTextLabel(0)
    , richText(0)
    , sequenceDialogLabel(0)
    , sequenceDialog(0)
    , multiPathLabel(0)
    , multiPath(0)
    , groupAsTabLabel(0)
    , groupAsTab(0)
    , parentGroupLabel(0)
    , parentGroup(0)
    , parentPageLabel(0)
    , parentPage(0)
    , userGroups()
    , userPages()
    {
        
    }
    
    void setVisibleMinMax(bool visible);
    
    void setVisibleMenuItems(bool visible);
    
    void setVisibleAnimates(bool visible);
    
    void setVisibleEvaluate(bool visible);
    
    void setVisibleStartNewLine(bool visible);
    
    void setVisibleHide(bool visible);
    
    void setVisibleMultiLine(bool visible);
    
    void setVisibleRichText(bool visible);
    
    void setVisibleSequence(bool visible);
    
    void setVisibleMultiPath(bool visible);
    
    void setVisibleGrpAsTab(bool visible);
    
    void setVisibleParent(bool visible);
    
    void setVisiblePage(bool visible);
    
    void setVisibleDefaultValues(bool visible,AddKnobDialogPrivate::DefaultValueType type,int dimensions);
    
    void createKnobFromSelection(int type,int optionalGroupIndex = -1);
    
    Group_Knob* getSelectedGroup() const;
};

static int getChoiceIndexFromKnobType(KnobI* knob)
{
    
    int dim = knob->getDimension();
    
    Int_Knob* isInt = dynamic_cast<Int_Knob*>(knob);
    Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob);
    Color_Knob* isColor = dynamic_cast<Color_Knob*>(knob);
    Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob);
    Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob);
    String_Knob* isStr = dynamic_cast<String_Knob*>(knob);
    File_Knob* isFile = dynamic_cast<File_Knob*>(knob);
    OutputFile_Knob* isOutputFile = dynamic_cast<OutputFile_Knob*>(knob);
    Path_Knob* isPath = dynamic_cast<Path_Knob*>(knob);
    Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob);
    Page_Knob* isPage = dynamic_cast<Page_Knob*>(knob);
    Button_Knob* isBtn = dynamic_cast<Button_Knob*>(knob);
    
    if (isInt) {
        if (dim == 1) {
            return 0;
        } else if (dim == 2) {
            return 1;
        } else if (dim == 3) {
            return 2;
        }
    } else if (isDbl) {
        if (dim == 1) {
            return 3;
        } else if (dim == 2) {
            return 4;
        } else if (dim == 3) {
            return 5;
        }
    } else if (isColor) {
        if (dim == 3) {
            return 6;
        } else if (dim == 4) {
            return 7;
        }
    } else if (isChoice) {
        return 8;
    } else if (isBool) {
        return 9;
    } else if (isStr) {
        if (isStr->isLabel()) {
            return 10;
        } else  {
            return 11;
        }
    } else if (isFile) {
        return 12;
    } else if (isOutputFile) {
        return 13;
    } else if (isPath) {
        return 14;
    } else if (isGrp) {
        return 15;
    } else if (isPage) {
        return 16;
    } else if (isBtn) {
        return 17;
    }
    return -1;
}

AddKnobDialog::AddKnobDialog(DockablePanel* panel,const boost::shared_ptr<KnobI>& knob,QWidget* parent)
: QDialog(parent)
, _imp(new AddKnobDialogPrivate(panel))
{
    
    _imp->knob = knob;
    assert(!knob || knob->isUserKnob());
    
    //QFont font(NATRON_FONT,NATRON_FONT_SIZE_11);
    
    _imp->vLayout = new QVBoxLayout(this);
    _imp->vLayout->setContentsMargins(0, 0, 15, 0);
    
    _imp->mainContainer = new QWidget(this);
    _imp->mainLayout = new QFormLayout(_imp->mainContainer);
    _imp->mainLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    _imp->mainLayout->setFormAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _imp->mainLayout->setSpacing(3);
    _imp->mainLayout->setContentsMargins(0, 0, 15, 0);
    
    _imp->vLayout->addWidget(_imp->mainContainer);
    
    {
        QWidget* firstRowContainer = new QWidget(this);
        QHBoxLayout* firstRowLayout = new QHBoxLayout(firstRowContainer);
        firstRowLayout->setContentsMargins(0, 0, 0, 0);
        
        _imp->nameLabel = new Natron::Label(tr("Script name:"),this);
        _imp->nameLineEdit = new LineEdit(firstRowContainer);
        _imp->nameLineEdit->setToolTip(Qt::convertFromPlainText(tr("The name of the parameter as it will be used in Python scripts")));
        
        if (knob) {
            _imp->nameLineEdit->setText(knob->getName().c_str());
        }
        firstRowLayout->addWidget(_imp->nameLineEdit);
        firstRowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->nameLabel, firstRowContainer);
        
    }
    
    {
        QWidget* secondRowContainer = new QWidget(this);
        QHBoxLayout* secondRowLayout = new QHBoxLayout(secondRowContainer);
        secondRowLayout->setContentsMargins(0, 0, 15, 0);
        _imp->labelLabel = new Natron::Label(tr("Label:"),secondRowContainer);
        _imp->labelLineEdit = new LineEdit(secondRowContainer);
        _imp->labelLineEdit->setToolTip(Qt::convertFromPlainText(tr("The label of the parameter as displayed on the graphical user interface")));
        if (knob) {
            _imp->labelLineEdit->setText(knob->getDescription().c_str());
        }
        secondRowLayout->addWidget(_imp->labelLineEdit);
        _imp->hideLabel = new Natron::Label(tr("Hide:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->hideLabel);
        _imp->hideBox = new QCheckBox(secondRowContainer);
        _imp->hideBox->setToolTip(Qt::convertFromPlainText(tr("If checked the parameter will not be visible on the user interface")));
        if (knob) {
            _imp->hideBox->setChecked(knob->getIsSecret());
        }
        secondRowLayout->addWidget(_imp->hideBox);
        _imp->startNewLineLabel = new Natron::Label(tr("Start new line:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->startNewLineLabel);
        _imp->startNewLineBox = new QCheckBox(secondRowContainer);
        _imp->startNewLineBox->setToolTip(tr("If checked the <b><i>next</i></b> parameter defined will be on the same line as this parameter"));
        if (knob) {
            _imp->startNewLineBox->setChecked(knob->isNewLineActivated());
        }
        secondRowLayout->addWidget(_imp->startNewLineBox);
        secondRowLayout->addStretch();
        
        _imp->mainLayout->addRow(_imp->labelLabel, secondRowContainer);
    }
    
    {
        QWidget* thirdRowContainer = new QWidget(this);
        QHBoxLayout* thirdRowLayout = new QHBoxLayout(thirdRowContainer);
        thirdRowLayout->setContentsMargins(0, 0, 15, 0);
        
        if (!knob) {
            _imp->typeLabel = new Natron::Label(tr("Type:"),thirdRowContainer);
            _imp->typeChoice = new ComboBox(thirdRowContainer);
            _imp->typeChoice->setToolTip(Qt::convertFromPlainText(tr("The data type of the parameter")));
            _imp->typeChoice->addItem("Integer");
            _imp->typeChoice->addItem("Integer 2D");
            _imp->typeChoice->addItem("Integer 3D");
            _imp->typeChoice->addItem("Floating point");
            _imp->typeChoice->addItem("Floating point 2D");
            _imp->typeChoice->addItem("Floating point 3D");
            _imp->typeChoice->addItem("Color RGB");
            _imp->typeChoice->addItem("Color RGBA");
            _imp->typeChoice->addItem("Choice (Pulldown)");
            _imp->typeChoice->addItem("Checkbox");
            _imp->typeChoice->addItem("Label");
            _imp->typeChoice->addItem("Text input");
            _imp->typeChoice->addItem("Input file");
            _imp->typeChoice->addItem("Output file");
            _imp->typeChoice->addItem("Directory");
            _imp->typeChoice->addItem("Group");
            _imp->typeChoice->addItem("Page");
            _imp->typeChoice->addItem("Button");
            QObject::connect(_imp->typeChoice, SIGNAL(currentIndexChanged(int)),this, SLOT(onTypeCurrentIndexChanged(int)));
            
            thirdRowLayout->addWidget(_imp->typeChoice);
        }
        _imp->animatesLabel = new Natron::Label(Qt::convertFromPlainText(tr("Animates:")),thirdRowContainer);

        if (!knob) {
            thirdRowLayout->addWidget(_imp->animatesLabel);
        }
        _imp->animatesCheckbox = new QCheckBox(thirdRowContainer);
        _imp->animatesCheckbox->setToolTip(Qt::convertFromPlainText(tr("When checked this parameter will be able to animate with keyframes")));
        if (knob) {
            _imp->animatesCheckbox->setChecked(knob->isAnimationEnabled());
        }
        thirdRowLayout->addWidget(_imp->animatesCheckbox);
        _imp->evaluatesLabel = new Natron::Label(Qt::convertFromPlainText(tr("Render on change:")),thirdRowContainer);
        thirdRowLayout->addWidget(_imp->evaluatesLabel);
        _imp->evaluatesOnChange = new QCheckBox(thirdRowContainer);
        _imp->evaluatesOnChange->setToolTip(Qt::convertFromPlainText(tr("If checked, when the value of this parameter changes a new render will be triggered")));
        if (knob) {
            _imp->evaluatesOnChange->setChecked(knob->getEvaluateOnChange());
        }
        thirdRowLayout->addWidget(_imp->evaluatesOnChange);
        thirdRowLayout->addStretch();
        
        if (!knob) {
            _imp->mainLayout->addRow(_imp->typeLabel, thirdRowContainer);
        } else {
            _imp->mainLayout->addRow(_imp->animatesLabel, thirdRowContainer);
        }
    }
    {
        _imp->tooltipLabel = new Natron::Label(tr("Tooltip:"),this);
        _imp->tooltipArea = new QTextEdit(this);
        _imp->tooltipArea->setToolTip(Qt::convertFromPlainText(tr("The help tooltip that will appear when hovering the parameter with the mouse")));
        _imp->mainLayout->addRow(_imp->tooltipLabel,_imp->tooltipArea);
        if (knob) {
            _imp->tooltipArea->setPlainText(knob->getHintToolTip().c_str());
        }
    }
    {
        _imp->menuItemsLabel = new Natron::Label(tr("Menu items:"),this);
        _imp->menuItemsEdit = new QTextEdit(this);
        QString tt = Qt::convertFromPlainText(tr("The entries that will be available in the drop-down menu. \n"
                                                 "Each line defines a new menu entry. You can specify a specific help tooltip for each entry "
                                                 "by separating the entry text from the help with the following characters on the line: "
                                                 "<?> \n\n"
                                                 "E.g: Special function<?>Will use our very own special function"),Qt::WhiteSpaceNormal);
        _imp->menuItemsEdit->setToolTip(tt);
        _imp->mainLayout->addRow(_imp->menuItemsLabel,_imp->menuItemsEdit);
        
        Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob.get());
        if (isChoice) {
            std::vector<std::string> entries = isChoice->getEntries_mt_safe();
            std::vector<std::string> entriesHelp = isChoice->getEntriesHelp_mt_safe();
            QString data;
            for (U32 i = 0; i < entries.size(); ++i) {
                QString line(entries[i].c_str());
                if (i < entriesHelp.size() && !entriesHelp[i].empty()) {
                    line.append("<?>");
                    line.append(entriesHelp[i].c_str());
                }
                data.append(line);
                data.append('\n');
            }
            _imp->menuItemsEdit->setPlainText(data);
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->multiLineLabel = new Natron::Label(tr("Multi-line:"),optContainer);
        _imp->multiLine = new QCheckBox(optContainer);
        _imp->multiLine->setToolTip(Qt::convertFromPlainText(tr("Should this text be multi-line or single-line ?")));
        optLayout->addWidget(_imp->multiLine);
        _imp->mainLayout->addRow(_imp->multiLineLabel, optContainer);
        
        String_Knob* isStr = dynamic_cast<String_Knob*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiLine()) {
                _imp->multiLine->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->richTextLabel = new Natron::Label(tr("Rich text:"),optContainer);
        _imp->richText = new QCheckBox(optContainer);
        QString tt = Qt::convertFromPlainText(tr("If checked, the text area will be able to use rich text encoding with a sub-set of html.\n "
                                                 "This property is only valid for multi-line input text only"),Qt::WhiteSpaceNormal);

        _imp->richText->setToolTip(tt);
        optLayout->addWidget(_imp->richText);
        _imp->mainLayout->addRow(_imp->richTextLabel, optContainer);
        
        String_Knob* isStr = dynamic_cast<String_Knob*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiLine() && isStr->usesRichText()) {
                _imp->richText->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->sequenceDialogLabel = new Natron::Label(tr("Use sequence dialog:"),optContainer);
        _imp->sequenceDialog = new QCheckBox(optContainer);
        _imp->sequenceDialog->setToolTip(Qt::convertFromPlainText(tr("If checked the file dialog for this parameter will be able to decode image sequences")));
        optLayout->addWidget(_imp->sequenceDialog);
        _imp->mainLayout->addRow(_imp->sequenceDialogLabel, optContainer);
        
        File_Knob* isFile = dynamic_cast<File_Knob*>(knob.get());
        OutputFile_Knob* isOutFile = dynamic_cast<OutputFile_Knob*>(knob.get());
        if (isFile) {
            if (isFile->isInputImageFile()) {
                _imp->sequenceDialog->setChecked(true);
            }
        } else if (isOutFile) {
            if (isOutFile->isOutputImageFile()) {
                _imp->sequenceDialog->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->multiPathLabel = new Natron::Label(Qt::convertFromPlainText(tr("Multiple paths:")),optContainer);
        _imp->multiPath = new QCheckBox(optContainer);
        _imp->multiPath->setToolTip(Qt::convertFromPlainText(tr("If checked the parameter will be a table where each entry points to a different path")));
        optLayout->addWidget(_imp->multiPath);
        _imp->mainLayout->addRow(_imp->multiPathLabel, optContainer);
        
        Path_Knob* isStr = dynamic_cast<Path_Knob*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiPath()) {
                _imp->multiPath->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->groupAsTabLabel = new Natron::Label(tr("Group as tab:"),optContainer);
        _imp->groupAsTab = new QCheckBox(optContainer);
        _imp->groupAsTab->setToolTip(Qt::convertFromPlainText(tr("If checked the group will be a tab instead")));
        optLayout->addWidget(_imp->groupAsTab);
        _imp->mainLayout->addRow(_imp->groupAsTabLabel, optContainer);
        
        Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob.get());
        if (isGrp) {
            if (isGrp && isGrp->isTab()) {
                _imp->groupAsTab->setChecked(true);
            }
        }

    }
    {
        QWidget* minMaxContainer = new QWidget(this);
        QWidget* dminMaxContainer = new QWidget(this);
        QHBoxLayout* minMaxLayout = new QHBoxLayout(minMaxContainer);
        QHBoxLayout* dminMaxLayout = new QHBoxLayout(dminMaxContainer);
        minMaxLayout->setContentsMargins(0, 0, 0, 0);
        dminMaxLayout->setContentsMargins(0, 0, 0, 0);
        _imp->minLabel = new Natron::Label(tr("Minimum:"),minMaxContainer);

        _imp->minBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->minBox->setToolTip(Qt::convertFromPlainText(tr("Set the minimum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval.")));
        minMaxLayout->addWidget(_imp->minBox);
        
        _imp->maxLabel = new Natron::Label(tr("Maximum:"),minMaxContainer);
        _imp->maxBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->maxBox->setToolTip(Qt::convertFromPlainText(tr("Set the maximum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval.")));
        minMaxLayout->addWidget(_imp->maxLabel);
        minMaxLayout->addWidget(_imp->maxBox);
        minMaxLayout->addStretch();
        
        _imp->dminLabel = new Natron::Label(tr("Display Minimum:"),dminMaxContainer);
        _imp->dminBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dminBox->setToolTip(Qt::convertFromPlainText(tr("Set the display minimum value for the parameter. This is a hint that is typically "
                                                              "used to set the range of the slider.")));
        dminMaxLayout->addWidget(_imp->dminBox);
        
        _imp->dmaxLabel = new Natron::Label(tr("Display Maximum:"),dminMaxContainer);
        _imp->dmaxBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dmaxBox->setToolTip(Qt::convertFromPlainText(tr("Set the display maximum value for the parameter. This is a hint that is typically "
                                                              "used to set the range of the slider.")));
        dminMaxLayout->addWidget(_imp->dmaxLabel);
        dminMaxLayout->addWidget(_imp->dmaxBox);
       
        dminMaxLayout->addStretch();
        
        Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob.get());
        Int_Knob* isInt = dynamic_cast<Int_Knob*>(knob.get());
        Color_Knob* isColor = dynamic_cast<Color_Knob*>(knob.get());
        

        if (isDbl) {
            double min = isDbl->getMinimum(0);
            double max = isDbl->getMaximum(0);
            double dmin = isDbl->getDisplayMinimum(0);
            double dmax = isDbl->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);
        } else if (isInt) {
            int min = isInt->getMinimum(0);
            int max = isInt->getMaximum(0);
            int dmin = isInt->getDisplayMinimum(0);
            int dmax = isInt->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);

        } else if (isColor) {
            double min = isColor->getMinimum(0);
            double max = isColor->getMaximum(0);
            double dmin = isColor->getDisplayMinimum(0);
            double dmax = isColor->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);

        }
        
        _imp->mainLayout->addRow(_imp->minLabel, minMaxContainer);
        _imp->mainLayout->addRow(_imp->dminLabel, dminMaxContainer);
    }
    
    {
        QWidget* defValContainer = new QWidget(this);
        QHBoxLayout* defValLayout = new QHBoxLayout(defValContainer);
        defValLayout->setContentsMargins(0, 0, 0, 0);
        _imp->defaultValueLabel = new Natron::Label(tr("Default value:"),defValContainer);

        _imp->default0 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default0->setValue(0);
        _imp->default0->setToolTip(Qt::convertFromPlainText(tr("Set the default value for the parameter (dimension 0)")));
        defValLayout->addWidget(_imp->default0);
        
        _imp->default1 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default1->setValue(0);
        _imp->default1->setToolTip(Qt::convertFromPlainText(tr("Set the default value for the parameter (dimension 1)")));
        defValLayout->addWidget(_imp->default1);
        
        _imp->default2 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default2->setValue(0);
        _imp->default2->setToolTip(Qt::convertFromPlainText(tr("Set the default value for the parameter (dimension 2)")));
        defValLayout->addWidget(_imp->default2);
        
        _imp->default3 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default3->setValue(0);
        _imp->default3->setToolTip(Qt::convertFromPlainText(tr("Set the default value for the parameter (dimension 3)")));
        defValLayout->addWidget(_imp->default3);

        
        _imp->defaultStr = new LineEdit(defValContainer);
        _imp->defaultStr->setToolTip(Qt::convertFromPlainText(tr("Set the default value for the parameter")));
        defValLayout->addWidget(_imp->defaultStr);
        
        
        _imp->defaultBool = new QCheckBox(defValContainer);
        _imp->defaultBool->setToolTip(Qt::convertFromPlainText(tr("Set the default value for the parameter")));
        _imp->defaultBool->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
        defValLayout->addWidget(_imp->defaultBool);

        defValLayout->addStretch();
        
        _imp->mainLayout->addRow(_imp->defaultValueLabel, defValContainer);
        
        
        Knob<double>* isDbl = dynamic_cast<Knob<double>*>(knob.get());
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
        Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob.get());
        Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(knob.get());
        
        if (isDbl) {
            _imp->default0->setValue(isDbl->getDefaultValue(0));
            if (isDbl->getDimension() >= 2) {
                _imp->default1->setValue(isDbl->getDefaultValue(1));
            }
            if (isDbl->getDimension() >= 3) {
                _imp->default2->setValue(isDbl->getDefaultValue(2));
            }
            if (isDbl->getDimension() >= 4) {
                _imp->default3->setValue(isDbl->getDefaultValue(3));
            }
        } else if (isInt) {
            _imp->default0->setValue(isInt->getDefaultValue(0));
            if (isInt->getDimension() >= 2) {
                _imp->default1->setValue(isInt->getDefaultValue(1));
            }
            if (isInt->getDimension() >= 3) {
                _imp->default2->setValue(isInt->getDefaultValue(2));
            }

        } else if (isBool) {
            _imp->defaultBool->setChecked(isBool->getDefaultValue(0));
        } else if (isStr) {
            _imp->defaultStr->setText(isStr->getDefaultValue(0).c_str());
        }
        
    }
    
    
    const std::map<boost::shared_ptr<KnobI>,KnobGui*>& knobs = _imp->panel->getKnobs();
    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (it->first->isUserKnob()) {
            Group_Knob* isGrp = dynamic_cast<Group_Knob*>(it->first.get());
            if (isGrp) {
                _imp->userGroups.push_back(isGrp);
            }
        }
    }
    
    
    
    if (!_imp->userGroups.empty()) {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->parentGroupLabel = new Natron::Label(tr("Group:"),optContainer);
        _imp->parentGroup = new ComboBox(optContainer);
        
        _imp->parentGroup->setToolTip(Qt::convertFromPlainText(tr("The name of the group under which this parameter will appear")));
        optLayout->addWidget(_imp->parentGroup);
        
        _imp->mainLayout->addRow(_imp->parentGroupLabel, optContainer);
    }
    
    if (!knob) {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        _imp->parentPageLabel = new Natron::Label(tr("Page:"),optContainer);
        _imp->parentPage = new ComboBox(optContainer);
        _imp->parentPage->addItem(NATRON_USER_MANAGED_KNOBS_PAGE);
        QObject::connect(_imp->parentPage,SIGNAL(currentIndexChanged(int)),this,SLOT(onPageCurrentIndexChanged(int)));
        const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->panel->getHolder()->getKnobs();
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin() ;it != knobs.end(); ++it) {
            if ((*it)->isUserKnob()) {
                Page_Knob* isPage = dynamic_cast<Page_Knob*>(it->get());
                if (isPage && isPage->getName() != std::string(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                    _imp->userPages.push_back(isPage);
                }
            }
        }
        
        for (std::list<Page_Knob*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
            _imp->parentPage->addItem((*it)->getName().c_str());
        }
        _imp->parentPage->setToolTip(Qt::convertFromPlainText(tr("The tab  under which this parameter will appear")));
        optLayout->addWidget(_imp->parentPage);
        if (knob) {
            ////find in which page the knob should be
            Page_Knob* isTopLevelParentAPage = getTopLevelPageForKnob(knob.get());
            assert(isTopLevelParentAPage);
            if (isTopLevelParentAPage->getName() != std::string(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                int index = 1; // 1 because of the "User" item
                bool found = false;
                for (std::list<Page_Knob*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it, ++index) {
                    if ((*it) == isTopLevelParentAPage) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    _imp->parentPage->setCurrentIndex(index);
                }
            }
            
            
        }
        
        _imp->mainLayout->addRow(_imp->parentPageLabel, optContainer);
        onPageCurrentIndexChanged(0);
    } else { // if(!knob)
        
        if (_imp->parentGroup) {
            Page_Knob* topLvlPage = getTopLevelPageForKnob(knob.get());
            assert(topLvlPage);
            boost::shared_ptr<KnobI> parent = knob->getParentKnob();
            Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
            _imp->parentGroup->addItem("-");
            int idx = 1;
            for (std::list<Group_Knob*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it, ++idx) {
                Page_Knob* page = getTopLevelPageForKnob(*it);
                assert(page);
                
                ///add only grps whose parent page is the selected page
                if (page == topLvlPage) {
                    _imp->parentGroup->addItem((*it)->getName().c_str());
                    if (isParentGrp && isParentGrp == *it) {
                        _imp->parentGroup->setCurrentIndex(idx);
                    }
                }
                
            }
        }
    }
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),Qt::Horizontal,this);
    QObject::connect(buttons,SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(buttons,SIGNAL(accepted()), this, SLOT(onOkClicked()));
    _imp->vLayout->addWidget(buttons);
    
    int type;
    if (!knob) {
        type = _imp->typeChoice->activeIndex();
    } else {
        type = getChoiceIndexFromKnobType(knob.get());
        assert(type != -1);
    }
    onTypeCurrentIndexChanged(type);
    _imp->panel->setUserPageActiveIndex();
    
    if (knob) {
        _imp->originalKnobSerialization.reset(new KnobSerialization(knob));
    }
}

void
AddKnobDialog::onPageCurrentIndexChanged(int index)
{
    if (!_imp->parentGroup) {
        return;
    }
    _imp->parentGroup->clear();
    _imp->parentGroup->addItem("-");
    
    std::string selectedPage = _imp->parentPage->itemText(index).toStdString();
    Page_Knob* parentPage = 0;
    
    if (selectedPage == NATRON_USER_MANAGED_KNOBS_PAGE) {
        parentPage = _imp->panel->getUserPageKnob().get();
    } else {
        for (std::list<Page_Knob*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
            if ((*it)->getName() == selectedPage) {
                parentPage = *it;
                break;
            }
        }
    }
    
    for (std::list<Group_Knob*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it) {
        Page_Knob* page = getTopLevelPageForKnob(*it);
        assert(page);
        
        ///add only grps whose parent page is the selected page
        if (page == parentPage) {
            _imp->parentGroup->addItem((*it)->getName().c_str());
        }
        
    }
}

void
AddKnobDialog::onTypeCurrentIndexChanged(int index)
{
    _imp->setVisiblePage(index != 16);
    switch (index) {
        case 0: // int
        case 1: // int 2D
        case 2: // int 3D
        case 3: // fp
        case 4: // fp 2D
        case 5: // fp 3D
        case 6: // RGB
        case 7: // RGBA
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(true);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            if (index == 0 || index == 3) {
                _imp->setVisibleDefaultValues(true,
                                              index == 0 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              1);
            } else if (index == 1 || index == 4) {
                _imp->setVisibleDefaultValues(true,
                                              index == 1 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              2);
            } else if (index == 2 || index == 5 || index == 6) {
                _imp->setVisibleDefaultValues(true,
                                              index == 2 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              3);
            } else if (index == 7) {
                _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeDouble,4);
            }
            break;
        case 8: // choice
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(true);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 9: // bool
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeBool,1);
            break;
        case 10: // label
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 11: // text input
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(true);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(true);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 12: // input file
        case 13: // output file
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(true);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 14: // path
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(true);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 15: // grp
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(true);
            _imp->setVisibleParent(false);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 16: // page
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(false);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(false);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 17: // button
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(false);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        default:
            break;
    }
}

AddKnobDialog::~AddKnobDialog()
{
    
}

boost::shared_ptr<KnobI>
AddKnobDialog::getKnob() const
{
    return _imp->knob;
}

void
AddKnobDialogPrivate::createKnobFromSelection(int index,int optionalGroupIndex)
{
    assert(!knob);
    std::string label = labelLineEdit->text().toStdString();
    
    switch (index) {
        case 0:
        case 1:
        case 2: {
            //int
            int dim = index + 1;
            boost::shared_ptr<Int_Knob> k = Natron::createKnob<Int_Knob>(panel->getHolder(), label, dim, false);
            std::vector<int> mins(dim),dmins(dim);
            std::vector<int> maxs(dim),dmaxs(dim);
            
            for (int i = 0; i < dim; ++i) {
                mins[i] = std::floor(minBox->value() + 0.5);
                dmins[i] = std::floor(dminBox->value() + 0.5);
                maxs[i] = std::floor(maxBox->value() + 0.5);
                dmaxs[i] = std::floor(dmaxBox->value() + 0.5);
            }
            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            std::vector<int> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }
            knob = k;
        } break;
        case 3:
        case 4:
        case 5: {
            //double
            int dim = index - 2;
            boost::shared_ptr<Double_Knob> k = Natron::createKnob<Double_Knob>(panel->getHolder(), label, dim, false);
            std::vector<double> mins(dim),dmins(dim);
            std::vector<double> maxs(dim),dmaxs(dim);
            for (int i = 0; i < dim; ++i) {
                mins[i] = minBox->value();
                dmins[i] = dminBox->value();
                maxs[i] = maxBox->value();
                dmaxs[i] = dmaxBox->value();
            }
            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            std::vector<double> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }

            
            knob = k;
        } break;
        case 6:
        case 7: {
            // color
            int dim = index - 3;
            boost::shared_ptr<Color_Knob> k = Natron::createKnob<Color_Knob>(panel->getHolder(), label, dim, false);
            std::vector<double> mins(dim),dmins(dim);
            std::vector<double> maxs(dim),dmaxs(dim);
            for (int i = 0; i < dim; ++i) {
                mins[i] = minBox->value();
                dmins[i] = dminBox->value();
                maxs[i] = maxBox->value();
                dmaxs[i] = dmaxBox->value();
            }
            std::vector<double> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            if (dim >= 4) {
                defValues.push_back(default3->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }

            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            knob = k;
        }  break;
        case 8: {

            boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(panel->getHolder(), label, 1, false);
            QString entriesRaw = menuItemsEdit->toPlainText();
            QTextStream stream(&entriesRaw);
            std::vector<std::string> entries,helps;

            while (!stream.atEnd()) {
                QString line = stream.readLine();
                int foundHelp = line.indexOf("<?>");
                if (foundHelp != -1) {
                    QString entry = line.mid(0,foundHelp);
                    QString help = line.mid(foundHelp + 3,-1);
                    for (int i = 0; i < (int)entries.size() - (int)helps.size(); ++i) {
                        helps.push_back("");
                    }
                    entries.push_back(entry.toStdString());
                    helps.push_back(help.toStdString());
                } else {
                    entries.push_back(line.toStdString());
                    if (!helps.empty()) {
                        helps.push_back("");
                    }
                }
            }
            k->populateChoices(entries,helps);
            
            int defValue = default0->value();
            if (defValue < (int)entries.size() && defValue >= 0) {
                k->setDefaultValue(defValue);
            }
            
            knob = k;
        } break;
        case 9: {
            boost::shared_ptr<Bool_Knob> k = Natron::createKnob<Bool_Knob>(panel->getHolder(), label, 1, false);
            bool defValue = defaultBool->isChecked();
            k->setDefaultValue(defValue);
            knob = k;
        }   break;
        case 10:
        case 11: {
            boost::shared_ptr<String_Knob> k = Natron::createKnob<String_Knob>(panel->getHolder(), label, 1, false);
            if (multiLine->isChecked()) {
                k->setAsMultiLine();
                if (richText->isChecked()) {
                    k->setUsesRichText(true);
                }
            } else {
                if (index == 10) {
                    k->setAsLabel();
                }
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        }   break;
        case 12: {
            boost::shared_ptr<File_Knob> k = Natron::createKnob<File_Knob>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsInputImage();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 13: {
            boost::shared_ptr<OutputFile_Knob> k = Natron::createKnob<OutputFile_Knob>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsOutputImageFile();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 14: {
            boost::shared_ptr<Path_Knob> k = Natron::createKnob<Path_Knob>(panel->getHolder(), label, 1, false);
            if (multiPath->isChecked()) {
                k->setMultiPath(true);
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 15: {
            boost::shared_ptr<Group_Knob> k = Natron::createKnob<Group_Knob>(panel->getHolder(), label, 1, false);
            if (groupAsTab->isChecked()) {
                k->setAsTab();
            }
            k->setDefaultValue(true); //< default to opened
            knob = k;
        } break;
        case 16: {
            boost::shared_ptr<Page_Knob> k = Natron::createKnob<Page_Knob>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        case 17: {
            boost::shared_ptr<Button_Knob> k = Natron::createKnob<Button_Knob>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        default:
            break;
    }
    assert(knob);
    knob->setAsUserKnob();
    if (knob->canAnimate()) {
        knob->setAnimationEnabled(animatesCheckbox->isChecked());
    }
    knob->setEvaluateOnChange(evaluatesOnChange->isChecked());
    knob->setAddNewLine(startNewLineBox->isChecked());
    knob->setSecret(hideBox->isChecked());
    knob->setName(nameLineEdit->text().toStdString());
    knob->setHintToolTip(tooltipArea->toPlainText().toStdString());
    bool addedInGrp = false;
    Group_Knob* selectedGrp = getSelectedGroup();
    if (selectedGrp) {
        if (optionalGroupIndex != -1) {
            selectedGrp->insertKnob(optionalGroupIndex, knob);
        } else {
            selectedGrp->addKnob(knob);
        }
        addedInGrp = true;
    }
    
    
    if (index != 16 && parentPage && !addedInGrp) {
        std::string selectedItem = parentPage->getCurrentIndexText().toStdString();
        if (selectedItem == std::string(NATRON_USER_MANAGED_KNOBS_PAGE)) {
            boost::shared_ptr<Page_Knob> userPage = panel->getUserPageKnob();
            userPage->addKnob(knob);
            panel->setUserPageActiveIndex();
        } else {
            for (std::list<Page_Knob*>::iterator it = userPages.begin(); it != userPages.end(); ++it) {
                if ((*it)->getName() == selectedItem) {
                    (*it)->addKnob(knob);
                    break;
                }
            }

        }
    }
}

Group_Knob*
AddKnobDialogPrivate::getSelectedGroup() const
{
    if (parentGroup) {
        std::string selectedItem = parentGroup->getCurrentIndexText().toStdString();
        if (selectedItem != "-") {
            for (std::list<Group_Knob*>::const_iterator it = userGroups.begin(); it != userGroups.end(); ++it) {
                if ((*it)->getName() == selectedItem) {
                    return *it;
                }
            }
        }
    }
    return 0;
}

void
AddKnobDialog::onOkClicked()
{
    QString name = _imp->nameLineEdit->text();
    bool badFormat = false;
    if (name.isEmpty()) {
        badFormat = true;
    }
    if (!badFormat && !name[0].isLetter()) {
        badFormat = true;
    }
    
    if (!badFormat) {
        //make sure everything is alphaNumeric without spaces
        for (int i = 0; i < name.size(); ++i) {
            if (name[i] == QChar(' ') || !name[i].isLetterOrNumber()) {
                badFormat = true;
                break;
            }
        }
    }
    
    //check if another knob has the same script name
    std::string stdName = name.toStdString();
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin() ;it != knobs.end(); ++it) {
        if ((*it)->getName() == stdName && (*it) != _imp->knob) {
            badFormat = true;
            break;
        }
    }
    
    if (badFormat) {
        Natron::errorDialog(tr("Error").toStdString(), tr("A parameter must have a unique script name composed only of characters from "
                                                          "[a - z / A- Z] and digits [0 - 9]. This name cannot contain spaces for scripting purposes.")
                            .toStdString());
        return;
        
    } else {
        ///Remove the previous knob, and recreate it.
        
        ///Index of the type in the combo
        int index;
        
        ///Remember the old page in which to insert the knob
        Page_Knob* oldParentPage = 0;
        
        ///If the knob was in a group, we need to place it at the same index
        int oldIndexInGroup = -1;
        
        if (_imp->knob) {
            
            oldParentPage = getTopLevelPageForKnob(_imp->knob.get());
            index = getChoiceIndexFromKnobType(_imp->knob.get());
            boost::shared_ptr<KnobI> parent = _imp->knob->getParentKnob();
            Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
            if (isParentGrp && isParentGrp == _imp->getSelectedGroup()) {
                const std::vector<boost::shared_ptr<KnobI> >& children = isParentGrp->getChildren();
                for (U32 i = 0; i < children.size(); ++i) {
                    if (children[i] == _imp->knob) {
                        oldIndexInGroup = i;
                        break;
                    }
                }
            } else {
                const std::vector<boost::shared_ptr<KnobI> >& children = oldParentPage->getChildren();
                for (U32 i = 0; i < children.size(); ++i) {
                    if (children[i] == _imp->knob) {
                        oldIndexInGroup = i;
                        break;
                    }
                }
            }
            _imp->panel->getHolder()->removeDynamicKnob(_imp->knob.get());
            _imp->knob.reset();
        } else {
            assert(_imp->typeChoice);
            index = _imp->typeChoice->activeIndex();
        }
        _imp->createKnobFromSelection(index, oldIndexInGroup);
        assert(_imp->knob);
        if (oldParentPage && !_imp->knob->getParentKnob()) {
            if (oldIndexInGroup == -1) {
                oldParentPage->addKnob(_imp->knob);
            } else {
                oldParentPage->insertKnob(oldIndexInGroup, _imp->knob);
            }
        }
        if (_imp->originalKnobSerialization) {
            _imp->knob->clone(_imp->originalKnobSerialization->getKnob().get());
        }
    }
    
    accept();
}

void
AddKnobDialogPrivate::setVisibleMinMax(bool visible)
{
    minLabel->setVisible(visible);
    minBox->setVisible(visible);
    maxLabel->setVisible(visible);
    maxBox->setVisible(visible);
    dminLabel->setVisible(visible);
    dminBox->setVisible(visible);
    dmaxLabel->setVisible(visible);
    dmaxBox->setVisible(visible);
    if (typeChoice) {
        int type = typeChoice->activeIndex();
        
        if (type == 6 || type == 7) {
            // color range to 0-1
            minBox->setValue(INT_MIN);
            maxBox->setValue(INT_MAX);
            dminBox->setValue(0.);
            dmaxBox->setValue(1.);
        } else {
            minBox->setValue(INT_MIN);
            maxBox->setValue(INT_MAX);
            dminBox->setValue(0);
            dmaxBox->setValue(100);
        }
    }
}

void
AddKnobDialogPrivate::setVisibleMenuItems(bool visible)
{
    menuItemsLabel->setVisible(visible);
    menuItemsEdit->setVisible(visible);
}

void
AddKnobDialogPrivate::setVisibleAnimates(bool visible)
{
    animatesLabel->setVisible(visible);
    animatesCheckbox->setVisible(visible);
    if (!knob) {
        animatesCheckbox->setChecked(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleEvaluate(bool visible)
{
    evaluatesLabel->setVisible(visible);
    evaluatesOnChange->setVisible(visible);
    if (!knob) {
        evaluatesOnChange->setChecked(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleStartNewLine(bool visible)
{
    startNewLineLabel->setVisible(visible);
    startNewLineBox->setVisible(visible);
    if (!knob) {
        startNewLineBox->setChecked(true);
    }
}

void
AddKnobDialogPrivate::setVisibleHide(bool visible)
{
    hideLabel->setVisible(visible);
    hideBox->setVisible(visible);
    if (!knob) {
        hideBox->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleMultiLine(bool visible)
{
    multiLineLabel->setVisible(visible);
    multiLine->setVisible(visible);
    if (!knob) {
        multiLine->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleRichText(bool visible)
{
    richTextLabel->setVisible(visible);
    richText->setVisible(visible);
    if (!knob) {
        richText->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleSequence(bool visible)
{
    sequenceDialogLabel->setVisible(visible);
    sequenceDialog->setVisible(visible);
    if (!knob) {
        sequenceDialog->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleMultiPath(bool visible)
{
    multiPathLabel->setVisible(visible);
    multiPath->setVisible(visible);
    if (!knob) {
        multiPath->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleGrpAsTab(bool visible)
{
    groupAsTabLabel->setVisible(visible);
    groupAsTab->setVisible(visible);
    if (!knob) {
        groupAsTab->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleParent(bool visible)
{
    if (!userGroups.empty()) {
        assert(parentGroup);
        parentGroup->setVisible(visible);
        parentGroupLabel->setVisible(visible);
    }
}

void
AddKnobDialogPrivate::setVisiblePage(bool visible)
{
    if (parentPage) {
        parentPage->setVisible(visible);
        parentPageLabel->setVisible(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleDefaultValues(bool visible,AddKnobDialogPrivate::DefaultValueType type,int dimensions)
{
    if (!visible) {
        defaultStr->setVisible(false);
        default0->setVisible(false);
        default1->setVisible(false);
        default2->setVisible(false);
        default3->setVisible(false);
        defaultValueLabel->setVisible(false);
    } else {
        defaultValueLabel->setVisible(true);

        if (type == eDefaultValueTypeInt || type == eDefaultValueTypeDouble) {
            defaultStr->setVisible(false);
            defaultBool->setVisible(false);
            if (dimensions == 1) {
                default0->setVisible(true);
                default1->setVisible(false);
                default2->setVisible(false);
                default3->setVisible(false);
            } else if (dimensions == 2) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(false);
                default3->setVisible(false);
            } else if (dimensions == 3) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(true);
                default3->setVisible(false);
            } else if (dimensions == 4) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(true);
                default3->setVisible(true);
            } else {
                assert(false);
            }
            if (type == eDefaultValueTypeDouble) {
                default0->setType(SpinBox::eSpinBoxTypeDouble);
                default1->setType(SpinBox::eSpinBoxTypeDouble);
                default2->setType(SpinBox::eSpinBoxTypeDouble);
                default3->setType(SpinBox::eSpinBoxTypeDouble);
            } else {
                default0->setType(SpinBox::eSpinBoxTypeInt);
                default1->setType(SpinBox::eSpinBoxTypeInt);
                default2->setType(SpinBox::eSpinBoxTypeInt);
                default3->setType(SpinBox::eSpinBoxTypeInt);
            }
        } else if (type == eDefaultValueTypeString) {
            defaultStr->setVisible(true);
            default0->setVisible(false);
            default1->setVisible(false);
            default2->setVisible(false);
            default3->setVisible(false);
            defaultBool->setVisible(false);
        } else {
            defaultStr->setVisible(false);
            default0->setVisible(false);
            default1->setVisible(false);
            default2->setVisible(false);
            default3->setVisible(false);
            defaultBool->setVisible(true);
        }
    }
}

