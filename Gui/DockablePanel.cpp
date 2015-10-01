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

#include "DockablePanel.h"

#include <QApplication> // qApp
#include <QColorDialog>
#include <QTimer>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <ofxNatron.h>

#include "Engine/Image.h" // Natron::clamp
#include "Engine/KnobTypes.h" // KnobButton
#include "Engine/GroupOutput.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeGuiI.h"
#include "Engine/Plugin.h"

#include "Gui/Button.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanelPrivate.h"
#include "Gui/DockablePanelTabWidget.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h" // triggerButtonIsRight...
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiColor.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/KnobUndoCommand.h" // RestoreDefaultsCommand
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/ManageUserParamsDialog.h"
#include "Gui/Menu.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h" // RenameNodeUndoRedoCommand
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/RotoPanel.h"
#include "Gui/TabGroup.h"
#include "Gui/TabWidget.h"
#include "Gui/Utils.h" // convertFromPlainText
#include "Gui/VerticalColorBar.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"


#define NATRON_VERTICAL_BAR_WIDTH 4
using std::make_pair;
using namespace Natron;


namespace {

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
        if (triggerButtonIsRight(e)) {
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

}


DockablePanel::DockablePanel(Gui* gui ,
                             KnobHolder* holder ,
                             QVBoxLayout* container,
                             HeaderModeEnum headerMode,
                             bool useScrollAreasForTabs,
                             const boost::shared_ptr<QUndoStack>& stack,
                             const QString & initialName,
                             const QString & helpToolTip,
                             bool createDefaultPage,
                             const QString & defaultPageName,
                             QWidget *parent)
: QFrame(parent)
, _imp(new DockablePanelPrivate(this,gui,holder,container,headerMode,useScrollAreasForTabs,defaultPageName,helpToolTip,stack))
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
                    int size = NATRON_MEDIUM_BUTTON_ICON_SIZE;
                    if (std::max(ic.width(), ic.height()) != size) {
                        ic = ic.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    _imp->_iconLabel->setPixmap(ic);
                } else {
                    _imp->_iconLabel->hide();
                }
                
            } else {
                _imp->_iconLabel->hide();
            }
            
            QPixmap pixCenter;
            appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixCenter);
            _imp->_centerNodeButton = new Button( QIcon(pixCenter),"",getHeaderWidget() );
            _imp->_centerNodeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_centerNodeButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
            _imp->_centerNodeButton->setToolTip(Natron::convertFromPlainText(tr("Centers the node graph on this item."), Qt::WhiteSpaceNormal));
            _imp->_centerNodeButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_centerNodeButton,SIGNAL( clicked() ),this,SLOT( onCenterButtonClicked() ) );
            _imp->_headerLayout->addWidget(_imp->_centerNodeButton);
            
            NodeGroup* isGroup = dynamic_cast<NodeGroup*>(iseffect);
            if (isGroup) {
                QPixmap enterPix;
                appPTR->getIcon(NATRON_PIXMAP_ENTER_GROUP, NATRON_MEDIUM_BUTTON_ICON_SIZE, &enterPix);
                _imp->_enterInGroupButton = new Button(QIcon(enterPix),"",_imp->_headerWidget);
                QObject::connect(_imp->_enterInGroupButton,SIGNAL(clicked(bool)),this,SLOT(onEnterInGroupClicked()));
                QObject::connect(isGroup, SIGNAL(graphEditableChanged(bool)), this, SLOT(onSubGraphEditionChanged(bool)));
                _imp->_enterInGroupButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
                _imp->_enterInGroupButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
                _imp->_enterInGroupButton->setFocusPolicy(Qt::NoFocus);
                _imp->_enterInGroupButton->setToolTip(Natron::convertFromPlainText(tr("Pressing this button will show the underlying node graph used for the implementation of this node."), Qt::WhiteSpaceNormal));
            }
            
            QPixmap pixHelp;
            appPTR->getIcon(NATRON_PIXMAP_HELP_WIDGET, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixHelp);
            _imp->_helpButton = new Button(QIcon(pixHelp),"",_imp->_headerWidget);
            
            const Natron::Plugin* plugin = iseffect->getNode()->getPlugin();
            assert(plugin);
            _imp->_pluginID = plugin->getPluginID();
            _imp->_pluginVersionMajor = plugin->getMajorVersion();
            _imp->_pluginVersionMinor = plugin->getMinorVersion();
            
            _imp->_helpButton->setToolTip(helpString());
            _imp->_helpButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_helpButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
            _imp->_helpButton->setFocusPolicy(Qt::NoFocus);
            
            QObject::connect( _imp->_helpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
            
            QPixmap pixHide,pixShow;
            appPTR->getIcon(NATRON_PIXMAP_UNHIDE_UNMODIFIED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixShow);
            appPTR->getIcon(NATRON_PIXMAP_HIDE_UNMODIFIED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixHide);
            QIcon icHideShow;
            icHideShow.addPixmap(pixShow,QIcon::Normal,QIcon::Off);
            icHideShow.addPixmap(pixHide,QIcon::Normal,QIcon::On);
            _imp->_hideUnmodifiedButton = new Button(icHideShow,"",_imp->_headerWidget);
            _imp->_hideUnmodifiedButton->setToolTip(Natron::convertFromPlainText(tr("Show/Hide all parameters without modifications."), Qt::WhiteSpaceNormal));
            _imp->_hideUnmodifiedButton->setFocusPolicy(Qt::NoFocus);
            _imp->_hideUnmodifiedButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_hideUnmodifiedButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
            _imp->_hideUnmodifiedButton->setCheckable(true);
            _imp->_hideUnmodifiedButton->setChecked(false);
            QObject::connect(_imp->_hideUnmodifiedButton,SIGNAL(clicked(bool)),this,SLOT(onHideUnmodifiedButtonClicked(bool)));
        }
        QPixmap pixM;
        appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixM);

        QPixmap pixC;
        appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixC);

        QPixmap pixF;
        appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixF);

        _imp->_minimize = new Button(QIcon(pixM),"",_imp->_headerWidget);
        _imp->_minimize->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_minimize->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _imp->_minimize->setCheckable(true);
        _imp->_minimize->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_minimize,SIGNAL( toggled(bool) ),this,SLOT( minimizeOrMaximize(bool) ) );

        _imp->_floatButton = new Button(QIcon(pixF),"",_imp->_headerWidget);
        _imp->_floatButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_floatButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _imp->_floatButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_floatButton,SIGNAL( clicked() ),this,SLOT( floatPanel() ) );


        _imp->_cross = new Button(QIcon(pixC),"",_imp->_headerWidget);
        _imp->_cross->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_cross->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _imp->_cross->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_cross,SIGNAL( clicked() ),this,SLOT( closePanel() ) );


        if (iseffect) {

            boost::shared_ptr<NodeGuiI> gui_i = iseffect->getNode()->getNodeGui();
            assert(gui_i);
            double r, g, b;
            gui_i->getColor(&r, &g, &b);
            currentColor.setRgbF(Natron::clamp(r, 0., 1.),
                                 Natron::clamp(g, 0., 1.),
                                 Natron::clamp(b, 0., 1.));
            QPixmap p(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE);
            p.fill(currentColor);


            _imp->_colorButton = new Button(QIcon(p),"",_imp->_headerWidget);
            _imp->_colorButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
            _imp->_colorButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
            _imp->_colorButton->setToolTip( Natron::convertFromPlainText(tr("Set here the color of the node in the nodegraph. "
                                                                        "By default the color of the node is the one set in the "
                                                                        "preferences of %1.").arg(NATRON_APPLICATION_NAME),
                                                                     Qt::WhiteSpaceNormal) );
            _imp->_colorButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_colorButton,SIGNAL( clicked() ),this,SLOT( onColorButtonClicked() ) );

            if ( iseffect && !iseffect->getNode()->isMultiInstance() ) {
                ///Show timeline keyframe markers to be consistent with the fact that the panel is opened by default
                iseffect->getNode()->showKeyframesOnTimeline(true);
            }
            
            
            if (iseffect && iseffect->hasOverlay()) {
                QPixmap pixOverlay;
                appPTR->getIcon(Natron::NATRON_PIXMAP_OVERLAY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixOverlay);
                _imp->_overlayColor.setRgbF(1., 1., 1.);
                _imp->_overlayButton = new OverlayColorButton(this,QIcon(pixOverlay),_imp->_headerWidget);
                _imp->_overlayButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
                _imp->_overlayButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
                _imp->_overlayButton->setToolTip(Natron::convertFromPlainText(tr("You can suggest here a color for the overlay on the viewer. "
                                                                             "Some plug-ins understand it and will use it to change the color of "
                                                                             "the overlay."), Qt::WhiteSpaceNormal));
                _imp->_overlayButton->setFocusPolicy(Qt::NoFocus);
                QObject::connect( _imp->_overlayButton,SIGNAL( clicked() ),this,SLOT( onOverlayButtonClicked() ) );
            }
            
        }
        QPixmap pixUndo;
        appPTR->getIcon(NATRON_PIXMAP_UNDO, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixUndo);
        QPixmap pixUndo_gray;
        appPTR->getIcon(NATRON_PIXMAP_UNDO_GRAYSCALE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixUndo_gray);
        QIcon icUndo;
        icUndo.addPixmap(pixUndo,QIcon::Normal);
        icUndo.addPixmap(pixUndo_gray,QIcon::Disabled);
        _imp->_undoButton = new Button(icUndo,"",_imp->_headerWidget);
        _imp->_undoButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_undoButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _imp->_undoButton->setToolTip( Natron::convertFromPlainText(tr("Undo the last change made to this operator."), Qt::WhiteSpaceNormal) );
        _imp->_undoButton->setEnabled(false);
        _imp->_undoButton->setFocusPolicy(Qt::NoFocus);
        QPixmap pixRedo;
        appPTR->getIcon(NATRON_PIXMAP_REDO, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixRedo);
        QPixmap pixRedo_gray;
        appPTR->getIcon(NATRON_PIXMAP_REDO_GRAYSCALE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixRedo_gray);
        QIcon icRedo;
        icRedo.addPixmap(pixRedo,QIcon::Normal);
        icRedo.addPixmap(pixRedo_gray,QIcon::Disabled);
        _imp->_redoButton = new Button(icRedo,"",_imp->_headerWidget);
        _imp->_redoButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_redoButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _imp->_redoButton->setToolTip( Natron::convertFromPlainText(tr("Redo the last change undone to this operator."), Qt::WhiteSpaceNormal) );
        _imp->_redoButton->setEnabled(false);
        _imp->_redoButton->setFocusPolicy(Qt::NoFocus);

        QPixmap pixRestore;
        appPTR->getIcon(NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixRestore);
        QIcon icRestore;
        icRestore.addPixmap(pixRestore);
        _imp->_restoreDefaultsButton = new Button(icRestore,"",_imp->_headerWidget);
        _imp->_restoreDefaultsButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _imp->_restoreDefaultsButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _imp->_restoreDefaultsButton->setToolTip( Natron::convertFromPlainText(tr("Restore default values for this operator."), Qt::WhiteSpaceNormal) );
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
        if (_imp->_enterInGroupButton) {
            _imp->_headerLayout->addWidget(_imp->_enterInGroupButton);
        }
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
    _imp->_horizLayout->setContentsMargins(NATRON_VERTICAL_BAR_WIDTH, 3, 3, 0);
    if (iseffect) {
        _imp->_verticalColorBar = new VerticalColorBar(_imp->_horizContainer);
        _imp->_verticalColorBar->setColor(currentColor);
        _imp->_horizLayout->addWidget(_imp->_verticalColorBar);
    }
    
    if (useScrollAreasForTabs) {
        _imp->_tabWidget = new QTabWidget(_imp->_horizContainer);
        _imp->_tabWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    } else {
        DockablePanelTabWidget* tabWidget = new DockablePanelTabWidget(gui,this);
        _imp->_tabWidget = tabWidget;
        tabWidget->getTabBar()->setObjectName("DockablePanelTabWidget");
        _imp->_tabWidget->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    }
    QObject::connect(_imp->_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onPageIndexChanged(int)));
    _imp->_horizLayout->addWidget(_imp->_tabWidget);
    _imp->_mainLayout->addWidget(_imp->_horizContainer);

    if (createDefaultPage) {
        _imp->getOrCreatePage(NULL);
    }
}

DockablePanel::~DockablePanel()
{
//    if (_imp->_holder) {
//        _imp->_holder->discardPanelPointer();
//    }
    getGui()->removeVisibleDockablePanel(this);

    ///Delete the knob gui if they weren't before
    ///normally the onKnobDeletion() function should have cleared them
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->second) {
            boost::shared_ptr<KnobI> knob = it->first.lock();
            if (knob) {
                knob->setKnobGuiPointer(0);
            }
            it->second->callDeleteLater();
        }
    }
}

void
DockablePanel::onPageIndexChanged(int index)
{
    assert(_imp->_tabWidget);
    QString name = _imp->_tabWidget->tabText(index);
    PageMap::iterator found = _imp->_pages.find(name);
    if (found == _imp->_pages.end()) {
        return;
    }
    
    std::string stdName = name.toStdString();
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->_holder->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        KnobPage* isPage = dynamic_cast<KnobPage*>(it->get());
        if (!isPage) {
            continue;
        }
        if (isPage->getDescription() == stdName) {
            isPage->setSecret(false);
        } else {
            isPage->setSecret(true);
        }
    }
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(_imp->_holder);
    if (isEffect && isEffect->getNode()->hasOverlay()) {
        isEffect->getApp()->redrawAllViewers();
    }
}

void
DockablePanel::turnOffPages()
{
    _imp->_pagesEnabled = false;
    delete _imp->_tabWidget;
    _imp->_tabWidget = 0;
    setFrameShape(QFrame::NoFrame);
    
    boost::shared_ptr<KnobPage> userPage = _imp->_holder->getOrCreateUserPageKnob();
    _imp->getOrCreatePage(userPage.get());
    
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
        _imp->_nameLineEdit->setToolTip("<p>Script name: <br/><b><font size=4>" + label + "</b></font></p>");
    } else if (_imp->_nameLabel) {
        _imp->_nameLabel->setToolTip("<p>Script name: <br/><b><font size=4>" + label + "</b></font></p>");
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
                KnobButton* isBtn = dynamic_cast<KnobButton*>( it2->get() );
                KnobPage* isPage = dynamic_cast<KnobPage*>( it2->get() );
                KnobGroup* isGroup = dynamic_cast<KnobGroup*>( it2->get() );
                KnobSeparator* isSeparator = dynamic_cast<KnobSeparator*>( it2->get() );
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
            KnobButton* isBtn = dynamic_cast<KnobButton*>( it->get() );
            KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( it->get() );
            KnobSeparator* isSeparator = dynamic_cast<KnobSeparator*>( it->get() );
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


void
DockablePanel::initializeKnobsInternal()
{
    std::vector< boost::shared_ptr<KnobI> > knobs = _imp->_holder->getKnobs();
    _imp->initializeKnobVector(knobs, NULL);

    ///add all knobs left  to the default page

    RotoPanel* roto = initializeRotoPanel();
    
    
    if (roto) {
//        boost::shared_ptr<KnobPage> page = _imp->ensureDefaultPageKnobCreated();
//        assert(page);
//        PageMap::iterator foundPage = _imp->_pages.find(page->getDescription().c_str());
//        assert(foundPage != _imp->_pages.end());
//        
//        QGridLayout* layout = 0;
//        if (_imp->_useScrollAreasForTabs) {
//            layout = dynamic_cast<QGridLayout*>( dynamic_cast<QScrollArea*>(foundPage->second.tab)->widget()->layout() );
//        } else {
//            layout = dynamic_cast<QGridLayout*>( foundPage->second.tab->layout() );
//        }
//        assert(layout);
//        layout->addWidget(roto, layout->rowCount(), 0 , 1, 2);
        _imp->_mainLayout->addWidget(roto);
    }

    initializeExtraGui(_imp->_mainLayout);
    
    NodeSettingsPanel* isNodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (isNodePanel) {
        boost::shared_ptr<NodeCollection> collec = isNodePanel->getNode()->getNode()->getGroup();
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>(collec.get());
        if (isGroup) {
            if (!isGroup->getNode()->hasPyPlugBeenEdited()) {
                setEnabled(false);
            }
        }
    }
    
}

void
DockablePanel::initializeKnobs()
{
    initializeKnobsInternal();
}

KnobGui*
DockablePanel::getKnobGui(const boost::shared_ptr<KnobI> & knob) const
{
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        if (it->first.lock() == knob) {
            return it->second;
        }
    }

    return NULL;
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
    QString tt = Natron::convertFromPlainText(_imp->_helpToolTip, Qt::WhiteSpaceNormal);

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
    
    setVisible(!c);
    
    setClosedInternal(c);

} // setClosed

void
DockablePanel::setClosedInternal(bool c)
{
 
    if (!_imp->_gui) {
        return;
    }
    
    if (_imp->_floating) {
        floatPanel();
        return;
    }
    {
        QMutexLocker l(&_imp->_isClosedMutex);
        if (c == _imp->_isClosed) {
            return;
            
        }
        _imp->_isClosed = c;
    }
    
    if (!c) {
        _imp->_gui->addVisibleDockablePanel(this);
    } else {
        _imp->_gui->removeVisibleDockablePanel(this);
        _imp->_gui->buildTabFocusOrderPropertiesBin();

    }
    
    ///Remove any color picker active
    const std::map<boost::weak_ptr<KnobI>,KnobGui*>& knobs = getKnobs();
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it = knobs.begin(); it!= knobs.end(); ++it) {
        KnobGuiColor* ck = dynamic_cast<KnobGuiColor*>(it->second);
        if (ck) {
            ck->setPickingEnabled(false);
        }
    }
    
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (nodePanel) {
        
        nodePanel->getNode()->getNode()->getLiveInstance()->refreshAfterTimeChange(getGui()->getApp()->getTimeLine()->currentFrame());
        
        
        boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
        boost::shared_ptr<Natron::Node> internalNode = nodeGui->getNode();
        boost::shared_ptr<MultiInstancePanel> panel = getMultiInstancePanel();
        Gui* gui = getGui();
        
        if (!c) {
            gui->addNodeGuiToCurveEditor(nodeGui);
            gui->addNodeGuiToDopeSheetEditor(nodeGui);
            
            NodeList children;
            internalNode->getChildrenMultiInstance(&children);
            for (NodeList::iterator it = children.begin() ; it != children.end(); ++it) {
                boost::shared_ptr<NodeGuiI> gui_i = (*it)->getNodeGui();
                assert(gui_i);
                boost::shared_ptr<NodeGui> childGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                assert(childGui);
                gui->addNodeGuiToCurveEditor(childGui);
                gui->addNodeGuiToDopeSheetEditor(childGui);
            }
        } else {
            gui->removeNodeGuiFromCurveEditor(nodeGui);
            gui->removeNodeGuiFromDopeSheetEditor(nodeGui);

            NodeList children;
            internalNode->getChildrenMultiInstance(&children);
            for (NodeList::iterator it = children.begin() ; it != children.end(); ++it) {
                boost::shared_ptr<NodeGuiI> gui_i = (*it)->getNodeGui();
                assert(gui_i);
                boost::shared_ptr<NodeGui> childGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                assert(childGui);
                gui->removeNodeGuiFromCurveEditor(childGui);
                gui->removeNodeGuiFromDopeSheetEditor(childGui);
            }
        }
        
        if (panel) {
            ///show all selected instances
            const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & childrenInstances = panel->getInstances();
            std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator next = childrenInstances.begin();
            if (next != childrenInstances.end()) {
                ++next;
            }
            for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = childrenInstances.begin();
                 it != childrenInstances.end();
                 ++it) {
                if (c) {
                    it->first.lock()->hideKeyframesFromTimeline( next == childrenInstances.end() );
                } else if (!c && it->second) {
                    it->first.lock()->showKeyframesOnTimeline( next == childrenInstances.end() );
                }
                
                // increment for next iteration
                if (next != childrenInstances.end()) {
                    ++next;
                }
            } // for(it)
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


}

void
DockablePanel::closePanel()
{
    
    close();
    setClosedInternal(true);

    ///Closing a panel always gives focus to some line-edit in the application which is quite annoying
    QWidget* hasFocus = qApp->focusWidget();
    if (hasFocus) {
        hasFocus->clearFocus();
    }
    
    const std::list<ViewerTab*>& viewers = getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->getViewer()->redraw();
    }
    
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
    {
        QMutexLocker k(&_imp->_isClosedMutex);
        _imp->_isClosed = false;
    }
    if (_imp->_floating) {
        assert(!_imp->_floatingWidget);
        QSize curSize = size();
        _imp->_floatingWidget = new FloatingWidget(_imp->_gui,_imp->_gui);
        QObject::connect( _imp->_floatingWidget,SIGNAL( closed() ),this,SLOT( closePanel() ) );
        _imp->_container->removeWidget(this);
        _imp->_floatingWidget->setWidget(this);
        _imp->_floatingWidget->resize(curSize);
        _imp->_gui->registerFloatingWindow(_imp->_floatingWidget);
    } else {
        assert(_imp->_floatingWidget);
        _imp->_gui->unregisterFloatingWindow(_imp->_floatingWidget);
        _imp->_floatingWidget->removeEmbeddedWidget();
        setParent( _imp->_container->parentWidget() );
        _imp->_container->insertWidget(0, this);
        _imp->_gui->addVisibleDockablePanel(this);
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
    KnobPage* isPage = dynamic_cast<KnobPage*>(knob.get());
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
            
            std::vector<boost::shared_ptr<KnobI> > children = isPage->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                deleteKnobGui(children[i]);
            }
        }
        
    } else {
        
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob.get());
        if (isGrp) {
            std::vector<boost::shared_ptr<KnobI> > children = isGrp->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                deleteKnobGui(children[i]);
            }
        }
        if (isGrp && isGrp->isTab()) {
            //find parent page
            boost::shared_ptr<KnobI> parent = knob->getParentKnob();
            KnobPage* isParentPage = dynamic_cast<KnobPage*>(parent.get());
            KnobGroup* isParentGroup = dynamic_cast<KnobGroup*>(parent.get());
            
            assert(isParentPage || isParentGroup);
            if (isParentPage) {
                PageMap::iterator page = _imp->_pages.find(isParentPage->getDescription().c_str());
                assert(page != _imp->_pages.end());
                TabGroup* groupAsTab = page->second.groupAsTab;
                if (groupAsTab) {
                    groupAsTab->removeTab(isGrp);
                    if (groupAsTab->isEmpty()) {
                        delete page->second.groupAsTab;
                        page->second.groupAsTab = 0;
                    }
                }
                
            } else if (isParentGroup) {
                std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator found  = _imp->_knobs.find(knob);
                assert(found != _imp->_knobs.end());
                KnobGuiGroup* parentGroupGui = dynamic_cast<KnobGuiGroup*>(found->second);
                assert(parentGroupGui);
                TabGroup* groupAsTab = parentGroupGui->getOrCreateTabWidget();
                if (groupAsTab) {
                    groupAsTab->removeTab(isGrp);
                    if (groupAsTab->isEmpty()) {
                        parentGroupGui->removeTabWidget();
                    }
                }
            }
            
            std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.find(knob);
            if (it != _imp->_knobs.end()) {
                it->first.lock()->setKnobGuiPointer(0);
                delete it->second;
                _imp->_knobs.erase(it);
            }
        
        } else {
            
            std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.find(knob);
            if (it != _imp->_knobs.end()) {
                it->second->removeGui();
                it->first.lock()->setKnobGuiPointer(0);
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

const std::map<boost::weak_ptr<KnobI>,KnobGui*> &
DockablePanel::getKnobs() const
{
    return _imp->_knobs;
}

QVBoxLayout*
DockablePanel::getContainer() const
{
    return _imp->_container;
}

boost::shared_ptr<QUndoStack>
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
            appPTR->getIcon(Natron::NATRON_PIXMAP_OVERLAY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixOverlay);
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
    appPTR->getIcon(Natron::NATRON_PIXMAP_OVERLAY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixOverlay);
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
        Natron::Menu menu(this);
        //menu.setFont( QFont(appFont,appFontSize) );

        QAction* userParams = new QAction(tr("Manage user parameters..."),&menu);
        menu.addAction(userParams);
        
      
        QAction* setKeys = new QAction(tr("Set key on all parameters"),&menu);
        menu.addAction(setKeys);
        
        QAction* removeAnimation = new QAction(tr("Remove animation on all parameters"),&menu);
        menu.addAction(removeAnimation);
        
        if (master || !_imp->_holder || !_imp->_holder->getApp() || _imp->_holder->getApp()->isGuiFrozen()) {
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
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        if (knob->isAnimationEnabled()) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(it->second,i);
                for (std::list<boost::shared_ptr<CurveGui> >::iterator it2 = curves.begin(); it2 != curves.end(); ++it2) {
                    
                    std::vector<KeyFrame> kVec;
                    KeyFrame kf;
                    kf.setTime(time);
                    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
                    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
                    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>( knob.get() );
                    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
                    
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
                    keys.insert(std::make_pair(*it2,kVec));
                }
                
                
            }
        }
    }
    pushUndoCommand( new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),keys) );

}

void
DockablePanel::removeAnimationOnAllParameters()
{
    std::map< boost::shared_ptr<CurveGui> ,std::vector<KeyFrame > > keysToRemove;
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        if (knob->isAnimationEnabled()) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(it->second,i);
                
                for (std::list<boost::shared_ptr<CurveGui> >::iterator it2 = curves.begin(); it2 != curves.end(); ++it2) {
                    KeyFrameSet keys = (*it2)->getInternalCurve()->getKeyFrames_mt_safe();
                    
                    std::vector<KeyFrame > vect;
                    for (KeyFrameSet::const_iterator it3 = keys.begin(); it3 != keys.end(); ++it3) {
                        vect.push_back(*it3);
                    }
                    keysToRemove.insert(std::make_pair(*it2,vect));
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
DockablePanel::onSubGraphEditionChanged(bool /*editable*/)
{
   // _imp->_enterInGroupButton->setVisible(editable);
}

void
DockablePanel::onEnterInGroupClicked()
{
    NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>(this);
    assert(panel);
    boost::shared_ptr<NodeGui> node = panel->getNode();
    assert(node);
    Natron::EffectInstance* effect = node->getNode()->getLiveInstance();
    assert(effect);
    NodeGroup* group = dynamic_cast<NodeGroup*>(effect);
    assert(group);
    NodeGraphI* graph_i = group->getNodeGraph();
    assert(graph_i);
    NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
    assert(graph);
    TabWidget* isParentTab = dynamic_cast<TabWidget*>(graph->parentWidget());
    if (isParentTab) {
        isParentTab->setCurrentWidget(graph);
    } else {
        NodeGraph* lastSelectedGraph = _imp->_gui->getLastSelectedGraph();
        if (!lastSelectedGraph) {
            const std::list<TabWidget*>& panes = _imp->_gui->getPanes();
            assert(panes.size() >= 1);
            isParentTab = panes.front();
        } else {
            isParentTab = dynamic_cast<TabWidget*>(lastSelectedGraph->parentWidget());
        }
        
        assert(isParentTab);
        isParentTab->appendTab(graph,graph);
        
    }
    QTimer::singleShot(25, graph, SLOT(centerOnAllNodes()));
}

void
DockablePanel::onHideUnmodifiedButtonClicked(bool checked)
{
    if (checked) {
        _imp->_knobsVisibilityBeforeHideModif.clear();
        for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator it = _imp->_knobs.begin(); it != _imp->_knobs.end(); ++it) {
            boost::shared_ptr<KnobI> knob = it->first.lock();
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>(knob.get());
            KnobParametric* isParametric = dynamic_cast<KnobParametric*>(knob.get());
            if (!isGroup && !isParametric) {
                _imp->_knobsVisibilityBeforeHideModif.insert(std::make_pair(it->second,it->second->isSecretRecursive()));
                if (!knob->hasModifications()) {
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
    
    std::list<KnobPage*> userPages;
    getUserPages(userPages);
    
    if (_imp->_pagesEnabled) {
        boost::shared_ptr<KnobPage> page = getUserPageKnob();
        userPages.push_back(page.get());
        for (std::list<KnobPage*>::iterator it = userPages.begin(); it != userPages.end(); ++it) {
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
    /*if (isNodePanel) {
        isNodePanel->getNode()->getNode()->declarePythonFields();
    }*/
    
    
    ///Refresh the curve editor with potential new animated knobs
    if (isNodePanel) {
        boost::shared_ptr<NodeGui> node = isNodePanel->getNode();
        getGui()->getCurveEditor()->removeNode(node.get());
        getGui()->getCurveEditor()->addNode(node);

        getGui()->removeNodeGuiFromDopeSheetEditor(node);
        getGui()->addNodeGuiToDopeSheetEditor(node);
    }
}

namespace {
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
    Button* pickButton;
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
    , pickButton(0)
    , editButton(0)
    , removeButton(0)
    , upButton(0)
    , downButton(0)
    , closeButton(0)
    {
        
    }
    
    boost::shared_ptr<KnobPage> getUserPageKnob() const;
    
    void initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs);
    
    void rebuildUserPages();
};
}

boost::shared_ptr<KnobPage>
DockablePanel::getUserPageKnob() const
{
    return _imp->_holder->getOrCreateUserPageKnob();
}

void
DockablePanel::getUserPages(std::list<KnobPage*>& userPages) const
{
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->isUserKnob()) {
            KnobPage* isPage = dynamic_cast<KnobPage*>(it->get());
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    }
}

