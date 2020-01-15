/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <stdexcept>

#include <QApplication> // qApp
#include <QColorDialog>
#include <QtCore/QSize>
#include <QtCore/QTimer>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QDesktopServices>
#include <QTextDocument>

#include <ofxNatron.h>

#include "Engine/Image.h" // Image::clamp
#include "Engine/KnobTypes.h" // KnobButton
#include "Engine/GroupOutput.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeGuiI.h"
#include "Engine/Plugin.h"
#include "Engine/ReadNode.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/WriteNode.h"

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
#include "Gui/RightClickableWidget.h"
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/RotoPanel.h"
#include "Gui/TabGroup.h"
#include "Gui/TabWidget.h"
#include "Gui/TrackerPanel.h"
#include "Gui/VerticalColorBar.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

using std::make_pair;


NATRON_NAMESPACE_ENTER

// called by NodeSettingsPanel::NodeSettingsPanel()
DockablePanel::DockablePanel(Gui* gui,
                             KnobHolder* holder,
                             QVBoxLayout* container,
                             HeaderModeEnum headerMode,
                             bool useScrollAreasForTabs,
                             const QUndoStackPtr& stack,
                             const QString & initialName,
                             const QString & helpToolTip,
                             QWidget *parent)
    : QFrame(parent)
    , KnobGuiContainerHelper(holder, stack)
    , _imp( new DockablePanelPrivate(this, gui, holder, container, headerMode, useScrollAreasForTabs, helpToolTip) )
{
    setContainerWidget(this);

    assert(holder);
    holder->setPanelPointer(this);

    _imp->_mainLayout = new QVBoxLayout(this);
    _imp->_mainLayout->setSpacing(0);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_imp->_mainLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameShape(QFrame::Box);
    setFocusPolicy(Qt::NoFocus);

    NodePtr node;
    NodePtr nodeForDocumentation;
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    QString pluginLabelVersioned;
    if (isEffect) {
        if ( dynamic_cast<GroupOutput*>(isEffect) ) {
            headerMode = eHeaderModeReadOnlyName;
        }
        // if this is a Read or Write plugin, get the info from the embedded plugin
        node = isEffect->getNode();
        nodeForDocumentation = node;
        if (node) {
            const std::string pluginID = isEffect->getPluginID();
            if (pluginID == PLUGINID_NATRON_READ ||
                pluginID == PLUGINID_NATRON_WRITE) {
                EffectInstancePtr effectInstance = node->getEffectInstance();
                if ( effectInstance && effectInstance->isReader() ) {
                    ReadNode* isReadNode = dynamic_cast<ReadNode*>( effectInstance.get() );

                    if (isReadNode) {
                        NodePtr subnode = isReadNode->getEmbeddedReader();
                        if (subnode) {
                            nodeForDocumentation = subnode;
                        }
                    }
                } else if ( effectInstance && effectInstance->isWriter() ) {
                    WriteNode* isWriteNode = dynamic_cast<WriteNode*>( effectInstance.get() );

                    if (isWriteNode) {
                        NodePtr subnode = isWriteNode->getEmbeddedWriter();
                        if (subnode) {
                            nodeForDocumentation = subnode;
                        }
                    }
                }
            }

            const Plugin* plugin = nodeForDocumentation->getPlugin();
            if (plugin) {
                _imp->_helpToolTip = QString::fromUtf8( nodeForDocumentation->getPluginDescription().c_str() );
                _imp->_pluginLabel = plugin->getPluginLabel();
                _imp->_pluginID = plugin->getPluginID();
                _imp->_pluginVersionMajor = plugin->getMajorVersion();
                _imp->_pluginVersionMinor = plugin->getMinorVersion();
                pluginLabelVersioned = tr("%1 version %2.%3").arg(_imp->_pluginLabel).arg(_imp->_pluginVersionMajor).arg(_imp->_pluginVersionMinor);
            }
        }
    }
    MultiInstancePanel* isMultiInstance = dynamic_cast<MultiInstancePanel*>(holder);
    if (isMultiInstance) {
        isEffect = isMultiInstance->getMainInstance()->getEffectInstance().get();
        assert(isEffect);
        if (!isEffect) {
            throw std::logic_error("");
        }
    }

    const QSize mediumBSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    const QSize mediumIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );
    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QColor currentColor;
    if (headerMode != eHeaderModeNoHeader) {
        _imp->_headerWidget = new QFrame(this);
        _imp->_headerWidget->setFrameShape(QFrame::Box);
        _imp->_headerLayout = new QHBoxLayout(_imp->_headerWidget);
        _imp->_headerLayout->setContentsMargins(0, 0, 0, 0);
        _imp->_headerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _imp->_headerLayout->setSpacing(2);
        _imp->_headerWidget->setLayout(_imp->_headerLayout);

        if (isEffect) {
            _imp->_iconLabel = new Label( getHeaderWidget() );
            _imp->_iconLabel->setContentsMargins(2, 2, 2, 2);
            _imp->_iconLabel->setToolTip(pluginLabelVersioned);
            _imp->_headerLayout->addWidget(_imp->_iconLabel);


            std::string iconFilePath;
            if (nodeForDocumentation) {
                iconFilePath = nodeForDocumentation->getPluginIconFilePath();
            }
            if ( !iconFilePath.empty() ) {
                QPixmap ic;
                if ( ic.load( QString::fromUtf8( iconFilePath.c_str() ) ) ) {
                    int size = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
                    if (std::max( ic.width(), ic.height() ) != size) {
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
            appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER, iconSize, &pixCenter);
            _imp->_centerNodeButton = new Button( QIcon(pixCenter), QString(), getHeaderWidget() );
            _imp->_centerNodeButton->setFixedSize(mediumBSize);
            _imp->_centerNodeButton->setIconSize(mediumIconSize);
            _imp->_centerNodeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Centers the node graph on this item."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _imp->_centerNodeButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_centerNodeButton, SIGNAL(clicked()), this, SLOT(onCenterButtonClicked()) );
            _imp->_headerLayout->addWidget(_imp->_centerNodeButton);

            NodeGroup* isGroup = dynamic_cast<NodeGroup*>(isEffect);
            if ( isGroup && (isGroup->getPluginID() == PLUGINID_NATRON_GROUP) ) {
                QPixmap enterPix;
                appPTR->getIcon(NATRON_PIXMAP_ENTER_GROUP, iconSize, &enterPix);
                _imp->_enterInGroupButton = new Button(QIcon(enterPix), QString(), _imp->_headerWidget);
                QObject::connect( _imp->_enterInGroupButton, SIGNAL(clicked(bool)), this, SLOT(onEnterInGroupClicked()) );
                QObject::connect( isGroup, SIGNAL(graphEditableChanged(bool)), this, SLOT(onSubGraphEditionChanged(bool)) );
                _imp->_enterInGroupButton->setFixedSize(mediumBSize);
                _imp->_enterInGroupButton->setIconSize(mediumIconSize);
                _imp->_enterInGroupButton->setFocusPolicy(Qt::NoFocus);
                _imp->_enterInGroupButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Pressing this button will show the underlying node graph used for the implementation of this node."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            }

            QPixmap pixHelp;
            appPTR->getIcon(NATRON_PIXMAP_HELP_WIDGET, iconSize, &pixHelp);
            _imp->_helpButton = new Button(QIcon(pixHelp), QString(), _imp->_headerWidget);

            _imp->_helpButton->setToolTip( helpString() );
            _imp->_helpButton->setFixedSize(mediumBSize);
            _imp->_helpButton->setIconSize(mediumIconSize);
            _imp->_helpButton->setFocusPolicy(Qt::NoFocus);

            QObject::connect( _imp->_helpButton, SIGNAL(clicked()), this, SLOT(showHelp()) );
            QPixmap pixHide, pixShow;
            appPTR->getIcon(NATRON_PIXMAP_UNHIDE_UNMODIFIED, iconSize, &pixShow);
            appPTR->getIcon(NATRON_PIXMAP_HIDE_UNMODIFIED, iconSize, &pixHide);
            QIcon icHideShow;
            icHideShow.addPixmap(pixShow, QIcon::Normal, QIcon::Off);
            icHideShow.addPixmap(pixHide, QIcon::Normal, QIcon::On);
            _imp->_hideUnmodifiedButton = new Button(icHideShow, QString(), _imp->_headerWidget);
            _imp->_hideUnmodifiedButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Show/Hide all parameters without modifications."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            _imp->_hideUnmodifiedButton->setFocusPolicy(Qt::NoFocus);
            _imp->_hideUnmodifiedButton->setFixedSize(mediumBSize);
            _imp->_hideUnmodifiedButton->setIconSize(mediumIconSize);
            _imp->_hideUnmodifiedButton->setCheckable(true);
            _imp->_hideUnmodifiedButton->setChecked(false);
            QObject::connect( _imp->_hideUnmodifiedButton, SIGNAL(clicked(bool)), this, SLOT(onHideUnmodifiedButtonClicked(bool)) );
        }
        QPixmap pixM;
        appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET, iconSize, &pixM);

        QPixmap pixC;
        appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET, iconSize, &pixC);

        QPixmap pixF;
        appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, iconSize, &pixF);

        _imp->_minimize = new Button(QIcon(pixM), QString(), _imp->_headerWidget);
        _imp->_minimize->setFixedSize(mediumBSize);
        _imp->_minimize->setIconSize(mediumIconSize);
        _imp->_minimize->setCheckable(true);
        _imp->_minimize->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_minimize, SIGNAL(toggled(bool)), this, SLOT(minimizeOrMaximize(bool)) );

        _imp->_floatButton = new Button(QIcon(pixF), QString(), _imp->_headerWidget);
        _imp->_floatButton->setFixedSize(mediumBSize);
        _imp->_floatButton->setIconSize(mediumIconSize);
        _imp->_floatButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_floatButton, SIGNAL(clicked()), this, SLOT(floatPanel()) );


        _imp->_cross = new Button(QIcon(pixC), QString(), _imp->_headerWidget);
        _imp->_cross->setFixedSize(mediumBSize);
        _imp->_cross->setIconSize(mediumIconSize);
        _imp->_cross->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_cross, SIGNAL(clicked()), this, SLOT(closePanel()) );


        if (node) {
            NodeGuiIPtr gui_i = node->getNodeGui();
            assert(gui_i);
            double r, g, b;
            gui_i->getColor(&r, &g, &b);
            currentColor.setRgbF( Image::clamp(r, 0., 1.),
                                  Image::clamp(g, 0., 1.),
                                  Image::clamp(b, 0., 1.) );
            QPixmap p(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE);
            p.fill(currentColor);


            _imp->_colorButton = new Button(QIcon(p), QString(), _imp->_headerWidget);
            _imp->_colorButton->setFixedSize(mediumBSize);
            _imp->_colorButton->setIconSize(mediumIconSize);
            _imp->_colorButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set here the color of the node in the nodegraph. "
                                                                              "By default the color of the node is the one set in the "
                                                                              "preferences of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ),
                                                                           NATRON_NAMESPACE::WhiteSpaceNormal) );
            _imp->_colorButton->setFocusPolicy(Qt::NoFocus);
            QObject::connect( _imp->_colorButton, SIGNAL(clicked()), this, SLOT(onColorButtonClicked()) );

            if ( node && !node->isMultiInstance() ) {
                ///Show timeline keyframe markers to be consistent with the fact that the panel is opened by default
                node->showKeyframesOnTimeline(true);
            }


            if ( node && node->hasOverlay() ) {
                QPixmap pixOverlay;
                appPTR->getIcon(NATRON_PIXMAP_OVERLAY, iconSize, &pixOverlay);
                _imp->_overlayColor.setRgbF(1., 1., 1.);
                _imp->_overlayButton = new OverlayColorButton(this, QIcon(pixOverlay), _imp->_headerWidget);
                _imp->_overlayButton->setFixedSize(mediumBSize);
                _imp->_overlayButton->setIconSize(mediumIconSize);
                _imp->_overlayButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("You can suggest here a color for the overlay on the viewer. "
                                                                                    "Some plug-ins understand it and will use it to change the color of "
                                                                                    "the overlay."), NATRON_NAMESPACE::WhiteSpaceNormal) );
                _imp->_overlayButton->setFocusPolicy(Qt::NoFocus);
                QObject::connect( _imp->_overlayButton, SIGNAL(clicked()), this, SLOT(onOverlayButtonClicked()) );
            }
        }
        QPixmap pixUndo;
        appPTR->getIcon(NATRON_PIXMAP_UNDO, iconSize, &pixUndo);
        QPixmap pixUndo_gray;
        appPTR->getIcon(NATRON_PIXMAP_UNDO_GRAYSCALE, iconSize, &pixUndo_gray);
        QIcon icUndo;
        icUndo.addPixmap(pixUndo, QIcon::Normal);
        icUndo.addPixmap(pixUndo_gray, QIcon::Disabled);
        _imp->_undoButton = new Button(icUndo, QString(), _imp->_headerWidget);
        _imp->_undoButton->setFixedSize(mediumBSize);
        _imp->_undoButton->setIconSize(mediumIconSize);
        _imp->_undoButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Undo the last change made to this operator."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->_undoButton->setEnabled(false);
        _imp->_undoButton->setFocusPolicy(Qt::NoFocus);
        QPixmap pixRedo;
        appPTR->getIcon(NATRON_PIXMAP_REDO, iconSize, &pixRedo);
        QPixmap pixRedo_gray;
        appPTR->getIcon(NATRON_PIXMAP_REDO_GRAYSCALE, iconSize, &pixRedo_gray);
        QIcon icRedo;
        icRedo.addPixmap(pixRedo, QIcon::Normal);
        icRedo.addPixmap(pixRedo_gray, QIcon::Disabled);
        _imp->_redoButton = new Button(icRedo, QString(), _imp->_headerWidget);
        _imp->_redoButton->setFixedSize(mediumBSize);
        _imp->_redoButton->setIconSize(mediumIconSize);
        _imp->_redoButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Redo the last change undone to this operator."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->_redoButton->setEnabled(false);
        _imp->_redoButton->setFocusPolicy(Qt::NoFocus);

        QPixmap pixRestore;
        appPTR->getIcon(NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED, iconSize, &pixRestore);
        QIcon icRestore;
        icRestore.addPixmap(pixRestore);
        _imp->_restoreDefaultsButton = new Button(icRestore, QString(), _imp->_headerWidget);
        _imp->_restoreDefaultsButton->setFixedSize(mediumBSize);
        _imp->_restoreDefaultsButton->setIconSize(mediumIconSize);
        _imp->_restoreDefaultsButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Restore default values for this operator."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->_restoreDefaultsButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _imp->_restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(onRestoreDefaultsButtonClicked()) );
        QObject::connect( _imp->_undoButton, SIGNAL(clicked()), this, SLOT(onUndoClicked()) );
        QObject::connect( _imp->_redoButton, SIGNAL(clicked()), this, SLOT(onRedoPressed()) );

        if (headerMode != eHeaderModeReadOnlyName) {
            _imp->_nameLineEdit = new LineEdit(_imp->_headerWidget);
            if (isEffect) {
                onNodeScriptChanged( QString::fromUtf8( isEffect->getScriptName().c_str() ) );
                QObject::connect( node.get(), SIGNAL(scriptNameChanged(QString)), this, SLOT(onNodeScriptChanged(QString)) );
            }
            _imp->_nameLineEdit->setText(initialName);
            QObject::connect( _imp->_nameLineEdit, SIGNAL(editingFinished()), this, SLOT(onLineEditNameEditingFinished()) );
            _imp->_headerLayout->addWidget(_imp->_nameLineEdit);
        } else {
            _imp->_nameLabel = new Label(initialName, _imp->_headerWidget);
            if (isEffect) {
                onNodeScriptChanged( QString::fromUtf8( isEffect->getScriptName().c_str() ) );
            }
            _imp->_headerLayout->addWidget(_imp->_nameLabel);
        }

        _imp->_headerLayout->addStretch();

        if ( (headerMode != eHeaderModeReadOnlyName) && _imp->_colorButton ) {
            _imp->_headerLayout->addWidget(_imp->_colorButton);
        }
        if ( (headerMode != eHeaderModeReadOnlyName) && _imp->_overlayButton ) {
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
    _imp->_horizLayout->setContentsMargins(0, 3, 3, 0);
    _imp->_horizLayout->setSpacing(2);

    _imp->_rightContainer = new QWidget(_imp->_horizContainer);
    _imp->_rightContainerLayout = new QVBoxLayout(_imp->_rightContainer);
    _imp->_rightContainerLayout->setSpacing(0);
    _imp->_rightContainerLayout->setContentsMargins(0, 0, 0, 0);

    if (isEffect) {
        _imp->_verticalColorBar = new VerticalColorBar(_imp->_horizContainer);
        _imp->_verticalColorBar->setColor(currentColor);
        _imp->_horizLayout->addWidget(_imp->_verticalColorBar);
    }

    if (useScrollAreasForTabs) {
        _imp->_tabWidget = new QTabWidget(_imp->_horizContainer);
        _imp->_tabWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    } else {
        DockablePanelTabWidget* tabWidget = new DockablePanelTabWidget(gui, this);
        _imp->_tabWidget = tabWidget;
        tabWidget->getTabBar()->setObjectName( QString::fromUtf8("DockablePanelTabWidget") );
        _imp->_tabWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }
    QObject::connect( _imp->_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onPageIndexChanged(int)) );
    _imp->_horizLayout->addWidget(_imp->_rightContainer);
    _imp->_mainLayout->addWidget(_imp->_horizContainer);
}

DockablePanel::~DockablePanel()
{
    
}

bool
DockablePanel::isPagingEnabled() const
{
    return _imp->_pagesEnabled;
}

bool
DockablePanel::useScrollAreaForTabs() const
{
    return _imp->_useScrollAreasForTabs;
}

QWidget*
DockablePanel::createKnobHorizontalFieldContainer(QWidget* parent) const
{
    RightClickableWidget* clickableWidget = new RightClickableWidget(this, parent);
    QObject::connect( clickableWidget, SIGNAL(rightClicked(QPoint)), this, SLOT(onRightClickMenuRequested(QPoint)) );
    QObject::connect( clickableWidget, SIGNAL(escapePressed()), this, SLOT(closePanel()) );
    clickableWidget->setFocusPolicy(Qt::NoFocus);
    return clickableWidget;
}

QWidget*
DockablePanel::getPagesContainer() const
{
    if (_imp->_tabWidget) {
        return _imp->_tabWidget;
    }

    return (QWidget*)_imp->_publicInterface;
}

QWidget*
DockablePanel::createPageMainWidget(QWidget* parent) const
{
    RightClickableWidget* clickableWidget = new RightClickableWidget(this, parent);
    QObject::connect( clickableWidget, SIGNAL(rightClicked(QPoint)), this, SLOT(onRightClickMenuRequested(QPoint)) );
    QObject::connect( clickableWidget, SIGNAL(escapePressed()), this, SLOT(closePanel()) );

    clickableWidget->setFocusPolicy(Qt::NoFocus);

    return clickableWidget;
}

void
DockablePanel::onPageLabelChanged(const KnobPageGuiPtr& page)
{
    if (_imp->_tabWidget) {
        int nTabs = _imp->_tabWidget->count();
        for (int i = 0; i < nTabs; ++i) {
            if (_imp->_tabWidget->widget(i) == page->tab) {
                QString newLabel = QString::fromUtf8( page->pageKnob.lock()->getLabel().c_str() );
                _imp->_tabWidget->setTabText(i, newLabel);
                break;
            }
        }
    }
}

void
DockablePanel::addPageToPagesContainer(const KnobPageGuiPtr& page)
{
    if (_imp->_tabWidget) {
        QString name = QString::fromUtf8( page->pageKnob.lock()->getLabel().c_str() );
        _imp->_tabWidget->addTab(page->tab, name);
    } else {
        _imp->_horizLayout->addWidget(page->tab);
    }
}

void
DockablePanel::removePageFromContainer(const KnobPageGuiPtr& page)
{
    if (_imp->_tabWidget) {
        int index = _imp->_tabWidget->indexOf(page->tab);
        if (index != -1) {
            _imp->_tabWidget->removeTab(index);
        }
    }
}

void
DockablePanel::setPagesOrder(const std::list<KnobPageGuiPtr>& orderedPages,
                             const KnobPageGuiPtr& curPage,
                             bool restorePageIndex)
{
    _imp->_tabWidget->clear();


    int index = 0;
    int i = 0;
    for (std::list<KnobPageGuiPtr>::const_iterator it = orderedPages.begin(); it != orderedPages.end(); ++it, ++i) {
        QString tabName = QString::fromUtf8( (*it)->pageKnob.lock()->getLabel().c_str() );
        _imp->_tabWidget->addTab( (*it)->tab, tabName );
        if ( restorePageIndex && (*it == curPage) ) {
            index = i;
        }
    }

    if ( (index >= 0) && ( index < int( orderedPages.size() ) ) ) {
        _imp->_tabWidget->setCurrentIndex(index);
    }
}

void
DockablePanel::onKnobsRecreated()
{
    NodeSettingsPanel* isNodePanel = dynamic_cast<NodeSettingsPanel*>(this);

    // Refresh the curve editor with potential new animated knobs
    if (isNodePanel) {
        Gui* gui = getGui();
        if (gui) {
            NodeGuiPtr node = isNodePanel->getNode();
            gui->getCurveEditor()->removeNode( node.get() );
            gui->getCurveEditor()->addNode(node);

            gui->removeNodeGuiFromDopeSheetEditor(node);
            gui->addNodeGuiToDopeSheetEditor(node);
        }
    }
}

void
DockablePanel::onPageIndexChanged(int index)
{
    if (!_imp->_tabWidget) {
        return;
    }
    QWidget* curTab = _imp->_tabWidget->widget(index);
    const PagesMap& pages = getPages();

    for (PagesMap::const_iterator it = pages.begin(); it != pages.end(); ++it) {
        if (it->second->tab == curTab) {
            setCurrentPage(it->second);
            EffectInstance* isEffect = dynamic_cast<EffectInstance*>(_imp->_holder);
            if (isEffect) {
                NodePtr node = isEffect->getNode();
                if (node && node->hasOverlay() ) {
                    isEffect->getApp()->redrawAllViewers();
                }
            }
        }
    }
}

void
DockablePanel::refreshCurrentPage()
{
    onPageIndexChanged( _imp->_tabWidget ? _imp->_tabWidget->currentIndex() : 0);
}

void
DockablePanel::onPageActivated(const KnobPageGuiPtr& page)
{
    if (!page) {
        return;
    }
    for (int i = 0; i < _imp->_tabWidget->count(); ++i) {
        if (_imp->_tabWidget->widget(i) == page->tab) {
            _imp->_tabWidget->setCurrentIndex(i);
            break;
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
}

void
DockablePanel::setPluginIDAndVersion(const std::string& pluginLabel,
                                     const std::string& pluginID,
                                     const std::string& pluginDesc,
                                     unsigned int version)
{
    if (_imp->_iconLabel) {
        QString pluginLabelVersioned = tr("%1 version %2").arg( QString::fromUtf8( pluginLabel.c_str() ) ).arg(version);
        _imp->_iconLabel->setToolTip(pluginLabelVersioned);
    }
    if (_imp->_helpButton) {
        _imp->_helpToolTip = QString::fromUtf8( pluginDesc.c_str() );
        _imp->_helpButton->setToolTip( helpString() );
        EffectInstance* iseffect = dynamic_cast<EffectInstance*>(_imp->_holder);
        if (iseffect) {
            _imp->_pluginID = QString::fromUtf8( pluginID.c_str() );
            _imp->_pluginVersionMajor = version;
            _imp->_pluginVersionMinor = 0;
            _imp->_helpButton->setToolTip( helpString() );
        }
    }
}

void
DockablePanel::setPluginIcon(const QPixmap& pix)
{
    if (_imp->_iconLabel) {
        _imp->_iconLabel->setPixmap(pix);
        if ( !_imp->_iconLabel->isVisible() ) {
            _imp->_iconLabel->show();
        }
    }
}

void
DockablePanel::onNodeScriptChanged(const QString& label)
{
    if (_imp->_nameLineEdit) {
        _imp->_nameLineEdit->setToolTip( QString::fromUtf8("<p>%1<br /><b><font size=4>%2</b></font></p>").arg( tr("Script name:") ).arg(label) );
    } else if (_imp->_nameLabel) {
        _imp->_nameLabel->setToolTip( QString::fromUtf8("<p>%1<br /><b><font size=4>%2</b></font></p>").arg( tr("Script name:") ).arg(label) );
    }
}

void
DockablePanel::onGuiClosing()
{
    if (_imp->_holder) {
        _imp->_holder->discardPanelPointer();
    }
    if (_imp->_nameLineEdit) {
        QObject::disconnect( _imp->_nameLineEdit, SIGNAL(editingFinished()), this, SLOT(onLineEditNameEditingFinished()) );
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
    std::list<KnobIPtr> knobsList;
    MultiInstancePanelPtr multiPanel = getMultiInstancePanel();

    if (multiPanel) {
        const std::list<std::pair<NodeWPtr, bool> > & instances = multiPanel->getInstances();
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            const std::vector<KnobIPtr> & knobs = it->first.lock()->getKnobs();
            for (std::vector<KnobIPtr>::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
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
        const std::vector<KnobIPtr> & knobs = _imp->_holder->getKnobs();
        for (std::vector<KnobIPtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            KnobButton* isBtn = dynamic_cast<KnobButton*>( it->get() );
            KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( it->get() );
            KnobSeparator* isSeparator = dynamic_cast<KnobSeparator*>( it->get() );
            if ( !isBtn && !isPage && !isGroup && !isSeparator && ( (*it)->getName() != kUserLabelKnobName ) ) {
                knobsList.push_back(*it);
            }
        }
    }

    /*
       This is not a perfect solution because here we only reset knob values to their defaults, but the plug-in
       may not revert its state to the original as if after the createInstanceAction.
       We may not either kill this node and create a new one because otherwise the undo/redo stack will be wiped.
     */
    pushUndoCommand( new RestoreDefaultsCommand(true, knobsList, -1) );
}

void
DockablePanel::onLineEditNameEditingFinished()
{
    if ( _imp->_gui->getApp()->isClosing() ) {
        return;
    }

    NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>(this);
    NodeGuiPtr node;
    QString newName = _imp->_nameLineEdit->text();
    QString oldName;
    if (panel) {
        node = panel->getNode();
        assert(node);
        if (node) {
            oldName = QString::fromUtf8( node->getNode()->getLabel().c_str() );
        }
    }

    if (oldName == newName) {
        return;
    }

    assert(node);

    if (node) {
        pushUndoCommand( new RenameNodeUndoRedoCommand(node, oldName, newName) );
    }
}

void
DockablePanel::onKnobsInitialized()
{
    assert(_imp->_tabWidget);
    _imp->_rightContainerLayout->addWidget(_imp->_tabWidget);


    RotoPanel* roto = initializeRotoPanel();
    if (roto) {
        _imp->_rightContainerLayout->addWidget(roto);
    }


    assert(!_imp->_trackerPanel);
    _imp->_trackerPanel = initializeTrackerPanel();

    if (_imp->_trackerPanel) {
        if ( !_imp->_tabWidget->count() ) {
            // No page, add it to the bottom
            _imp->_rightContainerLayout->addWidget(_imp->_trackerPanel);
        } else {
            // There is a page, add it to the first page

            QGridLayout* layout = dynamic_cast<QGridLayout*>( _imp->_tabWidget->widget(0)->layout() );
            assert(layout);
            if (layout) {
                layout->addWidget(_imp->_trackerPanel, layout->rowCount(), 0, 1, 2);
            }
        }
    }

    initializeExtraGui(_imp->_rightContainerLayout);

    NodeSettingsPanel* isNodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (isNodePanel) {
        NodeGuiPtr nodeGui = isNodePanel->getNode();
        NodePtr node;
        assert(nodeGui);
        if (nodeGui) {
            node = nodeGui->getNode();
        }
        assert(node);
        if (node) {
            NodeCollectionPtr collec = node->getGroup();
            NodeGroup* isGroup = dynamic_cast<NodeGroup*>( collec.get() );
            if (isGroup) {
                if ( !isGroup->getNode()->hasPyPlugBeenEdited() ) {
                    isNodePanel->setPyPlugUIEnabled(false);
                }
            }
        }
    }
} // DockablePanel::initializeKnobsInternal

void
DockablePanel::setPyPlugUIEnabled(bool enabled)
{
    std::vector<QWidget*> btns = {
        _imp->_centerNodeButton,
        _imp->_undoButton,
        _imp->_redoButton,
        _imp->_colorButton,
        _imp->_overlayButton,
        _imp->_restoreDefaultsButton,
        _imp->_nameLineEdit,
    };
    for (QWidget* btn : btns)
        if (btn)
            btn->setEnabled(enabled);
    if (_imp->_tabWidget)
    {
        int nTabs = _imp->_tabWidget->count();
        for (int i = 0; i < nTabs; ++i)
        {
            _imp->_tabWidget->widget(i)->setEnabled(enabled);
        }
    }
} // DockablePanel::setPyPlugUIEnabled

TrackerPanel*
DockablePanel::getTrackerPanel() const
{
    return _imp->_trackerPanel;
}

void
DockablePanel::refreshTabWidgetMaxHeight()
{
    /*
       Make the tab widget have the same height across all tabs to avoid the
       layout being adjusted everytimes the user switches from tab to tab
     */

    //Disabled for now - it leads to bad behaviour if the Node tab is bigger than the main actual tab
#if 0
    if (_imp->_tabWidget && !_imp->_useScrollAreasForTabs) {
        //Compute the tab maximum height
        int maxHeight = -1;
        for (int i = 0; i < _imp->_tabWidget->count(); ++i) {
            QWidget* w = _imp->_tabWidget->widget(i);
            if (w) {
                maxHeight = std::max(w->sizeHint().height(), maxHeight);
            }
        }
        if (maxHeight > 0) {
            _imp->_tabWidget->setFixedHeight(maxHeight);
        }
    }
#endif
}

void
DockablePanel::refreshUndoRedoButtonsEnabledNess(bool canUndo,
                                                 bool canRedo)
{
    if (_imp->_undoButton && _imp->_redoButton) {
        _imp->_undoButton->setEnabled(canUndo);
        _imp->_redoButton->setEnabled(canRedo);
    }
}

void
DockablePanel::onUndoClicked()
{
    QUndoStackPtr stack = getUndoStack();

    stack->undo();
    if (_imp->_undoButton && _imp->_redoButton) {
        _imp->_undoButton->setEnabled( stack->canUndo() );
        _imp->_redoButton->setEnabled( stack->canRedo() );
    }
    Q_EMIT undoneChange();
}

void
DockablePanel::onRedoPressed()
{
    QUndoStackPtr stack = getUndoStack();

    stack->redo();
    if (_imp->_undoButton && _imp->_redoButton) {
        _imp->_undoButton->setEnabled( stack->canUndo() );
        _imp->_redoButton->setEnabled( stack->canRedo() );
    }
    Q_EMIT redoneChange();
}

QString
DockablePanel::helpString() const
{
    //Base help
    QString tt;
    bool isMarkdown = false;
    EffectInstance* iseffect = dynamic_cast<EffectInstance*>(_imp->_holder);

    if (iseffect) {
        isMarkdown = iseffect->isPluginDescriptionInMarkdown();
    }

    if (Qt::mightBeRichText(_imp->_helpToolTip) || isMarkdown) {
        tt = _imp->_helpToolTip;
    } else {
        tt = NATRON_NAMESPACE::convertFromPlainText(_imp->_helpToolTip, NATRON_NAMESPACE::WhiteSpaceNormal);
    }

    if (iseffect) {
        //Prepend the plugin ID
        if ( !_imp->_pluginID.isEmpty() ) {
            QString pluginLabelVersioned = tr("%1 version %2.%3")
                                            .arg(_imp->_pluginID)
                                            .arg(_imp->_pluginVersionMajor)
                                            .arg(_imp->_pluginVersionMinor);
            if ( !pluginLabelVersioned.isEmpty() ) {
                if (isMarkdown) {
                    tt.prepend(pluginLabelVersioned + QString::fromUtf8("\n=========\n\n"));
                } else {
                    QString toPrepend = QString::fromUtf8("<p><b>");
                    toPrepend.append(pluginLabelVersioned);
                    toPrepend.append( QString::fromUtf8("</b></p>") );
                    tt.prepend(toPrepend);
                }
            }
        }
    }

    if (isMarkdown) {
        tt = Markdown::convert2html(tt);
        // Shrink H1/H2 (Can't do it in qt stylesheet)
        tt.replace( QString::fromUtf8("<h1>"), QString::fromUtf8("<h1 style=\"font-size:large;\">") );
        tt.replace( QString::fromUtf8("<h2>"), QString::fromUtf8("<h2 style=\"font-size:large;\">") );
    }

    return tt;
}

void
DockablePanel::showHelp()
{
    EffectInstance* iseffect = dynamic_cast<EffectInstance*>(_imp->_holder);

    if (iseffect) {
        NodePtr node = iseffect->getNode();
        int serverPort = appPTR->getDocumentationServerPort();
        QString localUrl = QString::fromUtf8("http://localhost:") + QString::number(serverPort) + QString::fromUtf8("/_plugin.html?id=") + _imp->_pluginID;
#ifdef NATRON_DOCUMENTATION_ONLINE
        QString remoteUrl = QString::fromUtf8(NATRON_DOCUMENTATION_ONLINE) + QString::fromUtf8("/plugins/") + _imp->_pluginID + QString::fromUtf8(".html");
        int docSource = appPTR->getCurrentSettings()->getDocumentationSource();
        if ( (serverPort == 0) && (docSource == 0) ) {
            docSource = 1;
        }
        switch (docSource) {
            case 0:
                QDesktopServices::openUrl( QUrl(localUrl) );
                break;
            case 1:
                QDesktopServices::openUrl( QUrl(remoteUrl) );
                break;
            case 2:
                Dialogs::informationDialog(_imp->_pluginLabel.toStdString(), helpString().toStdString(), true);
                break;
        }
#else
        QDesktopServices::openUrl( QUrl(localUrl) );
#endif
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

    {
        QMutexLocker l(&_imp->_isClosedMutex);
        if (c == _imp->_isClosed) {
            return;
        }
        _imp->_isClosed = c;
    }


    if (_imp->_floating) {
        floatPanel();

        return;
    }

    if (!c) {
        _imp->_gui->addVisibleDockablePanel(this);
    } else {
        _imp->_gui->removeVisibleDockablePanel(this);
        _imp->_gui->buildTabFocusOrderPropertiesBin();
    }

    ///Remove any color picker active
    const KnobsGuiMapping& knobs = getKnobsMapping();
    for (KnobsGuiMapping::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobGuiColor* ck = dynamic_cast<KnobGuiColor*>( it->second.get() );
        if (ck) {
            ck->setPickingEnabled(false);
        }
    }

    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);
    if (nodePanel) {
        NodeGuiPtr nodeGui = nodePanel->getNode();
        NodePtr internalNode = nodeGui->getNode();

        Gui* gui = getGui();
        if (gui) {
            if (internalNode && !c) {
                // when a panel is open, refresh its knob values
                GuiAppInstancePtr app = gui->getApp();
                if (app) {
                    TimeLinePtr timeline = app->getTimeLine();
                    if (timeline) {
                        internalNode->getEffectInstance()->refreshAfterTimeChange( false, timeline->currentFrame() );
                    }
                }
            }
        }

        MultiInstancePanelPtr panel = getMultiInstancePanel();

        if (gui) {
            if (!c) {
                gui->addNodeGuiToCurveEditor(nodeGui);
                gui->addNodeGuiToDopeSheetEditor(nodeGui);

                NodesList children;
                internalNode->getChildrenMultiInstance(&children);
                for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
                    NodeGuiIPtr gui_i = (*it)->getNodeGui();
                    assert(gui_i);
                    NodeGuiPtr childGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                    assert(childGui);
                    gui->addNodeGuiToCurveEditor(childGui);
                    gui->addNodeGuiToDopeSheetEditor(childGui);
                }
            } else {
                gui->removeNodeGuiFromCurveEditor(nodeGui);
                gui->removeNodeGuiFromDopeSheetEditor(nodeGui);

                NodesList children;
                internalNode->getChildrenMultiInstance(&children);
                for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
                    NodeGuiIPtr gui_i = (*it)->getNodeGui();
                    assert(gui_i);
                    NodeGuiPtr childGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                    assert(childGui);
                    gui->removeNodeGuiFromCurveEditor(childGui);
                    gui->removeNodeGuiFromDopeSheetEditor(childGui);
                }
            }
        }

        if (panel) {
            ///show all selected instances
            const std::list<std::pair<NodeWPtr, bool> > & childrenInstances = panel->getInstances();
            std::list<std::pair<NodeWPtr, bool> >::const_iterator next = childrenInstances.begin();
            if ( next != childrenInstances.end() ) {
                ++next;
            }
            for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = childrenInstances.begin();
                 it != childrenInstances.end();
                 ++it) {
                if (c) {
                    it->first.lock()->hideKeyframesFromTimeline( next == childrenInstances.end() );
                } else if (!c && it->second) {
                    it->first.lock()->showKeyframesOnTimeline( next == childrenInstances.end() );
                }

                // increment for next iteration
                if ( next != childrenInstances.end() ) {
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
} // DockablePanel::setClosedInternal

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

    Gui* gui = getGui();
    if (gui) {
        const std::list<ViewerTab*>& viewers = gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->getViewer()->redraw();
        }
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
    _imp->_rightContainer->setVisible(!_imp->_minimized);
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
    Gui* gui = getGui();
    if (gui) {
        gui->buildTabFocusOrderPropertiesBin();
    }
    update();
}

FloatingWidget*
DockablePanel::getFloatingWindow() const
{
    return _imp->_floatingWidget;
}

FloatingWidget*
DockablePanel::floatPanel()
{
    _imp->_floating = !_imp->_floating;
    {
        QMutexLocker k(&_imp->_isClosedMutex);
        _imp->_isClosed = false;
    }
    if (_imp->_floating) {
        assert(!_imp->_floatingWidget);

        QSize curSize = sizeHint();


        _imp->_floatingWidget = new FloatingWidget(_imp->_gui, _imp->_gui);
        QObject::connect( _imp->_floatingWidget, SIGNAL(closed()), this, SLOT(closePanel()) );
        _imp->_container->removeWidget(this);
        _imp->_floatingWidget->setWidget(this);
        _imp->_floatingWidget->resize(curSize);
        _imp->_gui->registerFloatingWindow(_imp->_floatingWidget);
    } else {
        assert(_imp->_floatingWidget);
        _imp->_gui->unregisterFloatingWindow(_imp->_floatingWidget);
        _imp->_floatingWidget->removeEmbeddedWidget();
        //setParent( _imp->_container->parentWidget() );
        //_imp->_container->insertWidget(0, this);
        _imp->_gui->addVisibleDockablePanel(this);
        _imp->_floatingWidget->deleteLater();
        _imp->_floatingWidget = 0;
    }
    Gui* gui = getGui();
    if (gui) {
        gui->buildTabFocusOrderPropertiesBin();
    }
    return _imp->_floatingWidget;
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

QVBoxLayout*
DockablePanel::getContainer() const
{
    return _imp->_container;
}

bool
DockablePanel::isClosed() const
{
    QMutexLocker l(&_imp->_isClosedMutex);

    return _imp->_isClosed;
}

bool
DockablePanel::isFloating() const
{
    return _imp->_floating;
}

void
DockablePanel::onColorDialogColorChanged(const QColor & color)
{
    if ( (_imp->_mode != eHeaderModeReadOnlyName) && _imp->_colorButton ) {
        QPixmap p(15, 15);
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
    NodePtr node = nodePanel->getNode()->getNode();
    if (!node) {
        return;
    }


    if ( (_imp->_mode  != eHeaderModeReadOnlyName) && _imp->_overlayButton ) {

        // Replace the circle portion from the icon with the color picked by the user
        int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
        QPixmap pixOverlay;
        appPTR->getIcon(NATRON_PIXMAP_OVERLAY, iconSize, &pixOverlay);

        QImage img = pixOverlay.toImage();
        if (!img.isNull()) {
            int width = img.width();
            int height = img.height();

            for (int y = 0; y < height; ++y) {
                QRgb* pix = reinterpret_cast<QRgb*>(img.scanLine(y));
                for (int x = 0; x < width; ++x) {
                    int alpha = qAlpha(*pix);
                    int r = int((float)color.red() * alpha * (1.f / 255));
                    int g = int((float)color.green() * alpha * (1.f / 255));
                    int b = int((float)color.blue() * alpha * (1.f / 255));

                    *pix = qRgba(r,g,b, alpha);
                    ++pix;
                }
            }
        }
        //QPixmap p(15, 15);
        //p.fill(color);
        QPixmap p = QPixmap::fromImage(img);
        _imp->_overlayButton->setIcon( QIcon(p) );
        {
            QMutexLocker k(&_imp->_currentColorMutex);
            _imp->_overlayColor = color;
            _imp->_hasOverlayColor = true;
        }

        Gui* gui = getGui();
        if (gui) {
            NodesList overlayNodes;

            gui->getNodesEntitledForOverlays(overlayNodes);
            NodesList::iterator found = std::find(overlayNodes.begin(), overlayNodes.end(), node);
            if ( found != overlayNodes.end() ) {
                gui->getApp()->redrawAllViewers();
            }
        }
    }
}

void
DockablePanel::onColorButtonClicked()
{
    QColorDialog dialog(this);

    dialog.setOption(QColorDialog::DontUseNativeDialog);
    QColor oldColor;
    {
        oldColor = getCurrentColor();
        dialog.setCurrentColor(oldColor);
    }
    QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onColorDialogColorChanged(QColor)) );

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
    NodePtr node = nodePanel->getNode()->getNode();
    if (!node) {
        return;
    }
    QColorDialog dialog(this);
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    QColor oldColor;
    bool hadOverlayColor;
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        dialog.setCurrentColor(_imp->_overlayColor);
        oldColor = _imp->_overlayColor;
        hadOverlayColor = _imp->_hasOverlayColor;
    }
    QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onOverlayColorDialogColorChanged(QColor)) );

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
            appPTR->getIcon(NATRON_PIXMAP_OVERLAY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixOverlay);
            _imp->_overlayButton->setIcon( QIcon(pixOverlay) );
        }
    }
    Gui* gui = getGui();
    if (gui) {
        NodesList overlayNodes;

        gui->getNodesEntitledForOverlays(overlayNodes);
        NodesList::iterator found = std::find(overlayNodes.begin(), overlayNodes.end(), node);
        if ( found != overlayNodes.end() ) {
            gui->getApp()->redrawAllViewers();
        }
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
DockablePanel::resetHostOverlayColor()
{
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(this);

    if (!nodePanel) {
        return;
    }
    NodePtr node = nodePanel->getNode()->getNode();
    if (!node) {
        return;
    }
    {
        QMutexLocker locker(&_imp->_currentColorMutex);
        _imp->_hasOverlayColor = false;
    }
    QPixmap pixOverlay;
    appPTR->getIcon(NATRON_PIXMAP_OVERLAY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixOverlay);
    _imp->_overlayButton->setIcon( QIcon(pixOverlay) );

    Gui* gui = getGui();
    if (gui) {
        NodesList overlayNodes;

        gui->getNodesEntitledForOverlays(overlayNodes);
        NodesList::iterator found = std::find(overlayNodes.begin(), overlayNodes.end(), node);
        if ( found != overlayNodes.end() ) {
            gui->getApp()->redrawAllViewers();
        }
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

    getUndoStack()->setActive();
}

void
DockablePanel::onRightClickMenuRequested(const QPoint & pos)
{
    QWidget* emitter = qobject_cast<QWidget*>( sender() );

    assert(emitter);

    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(_imp->_holder);
    if (isEffect) {
        NodePtr master = isEffect->getNode()->getMasterNode();
        Menu menu(this);
        //menu.setFont( QFont(appFont,appFontSize) );
        QAction* userParams = new QAction(tr("Manage user parameters..."), &menu);
        menu.addAction(userParams);


        QAction* setKeys = new QAction(tr("Set key on all parameters"), &menu);
        menu.addAction(setKeys);

        QAction* removeAnimation = new QAction(tr("Remove animation on all parameters"), &menu);
        menu.addAction(removeAnimation);

        if ( master || !_imp->_holder || !_imp->_holder->getApp() || _imp->_holder->getApp()->isGuiFrozen() ) {
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
    KnobHolder* holder = getHolder();
    if (!holder) {
        return;
    }
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    if (!isEffect) {
        return;
    }
    NodePtr node = isEffect->getNode();
    if (!node) {
        return;
    }
    // If this is a pyplug, warn that the user is about to break it
    if (node->isPyPlug()) {
        StandardButtons rep = Dialogs::questionDialog(tr("PyPlug").toStdString(), tr("You are about to edit parameters of this node which will "
                                                                                     "automatically convert this node as a Group. Are you sure "
                                                                                     "you want to edit it?").toStdString(), false);
        if (rep != eStandardButtonYes) {
            return;
        }
        node->setPyPlugEdited(true);


    }
    ManageUserParamsDialog dialog(this, this);

    ignore_result( dialog.exec() );
}

void
DockablePanel::setKeyOnAllParameters()
{
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    double time = gui->getApp()->getTimeLine()->currentFrame();
    AddKeysCommand::KeysToAddList keys;
    const KnobsGuiMapping& knobsMap = getKnobsMapping();

    for (KnobsGuiMapping::const_iterator it = knobsMap.begin(); it != knobsMap.end(); ++it) {
        KnobIPtr knob = it->first.lock();
        if ( knob->isAnimationEnabled() ) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                std::list<CurveGuiPtr> curves = gui->getCurveEditor()->findCurve(it->second, i);
                for (std::list<CurveGuiPtr>::iterator it2 = curves.begin(); it2 != curves.end(); ++it2) {
                    AddKeysCommand::KeyToAdd k;
                    KeyFrame kf;
                    kf.setTime(time);
                    KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( knob.get() );
                    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( knob.get() );
                    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>( knob.get() );
                    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( knob.get() );

                    if (isInt) {
                        kf.setValue( isInt->getValueAtTime(time, i) );
                    } else if (isBool) {
                        kf.setValue( isBool->getValueAtTime(time, i) );
                    } else if (isDouble) {
                        kf.setValue( isDouble->getValueAtTime(time, i) );
                    } else if (isString) {
                        std::string v = isString->getValueAtTime(time, i);
                        double dv;
                        isString->stringToKeyFrameValue(time, ViewIdx(0), v, &dv);
                        kf.setValue(dv);
                    }

                    k.keyframes.push_back(kf);
                    k.curveUI = *it2;
                    KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>( it2->get() );
                    if (isKnobCurve) {
                        k.knobUI = isKnobCurve->getKnobGui();
                        k.dimension = isKnobCurve->getDimension();
                    } else {
                        k.dimension = 0;
                    }
                    keys.push_back(k);
                }
            }
        }
    }
    pushUndoCommand( new AddKeysCommand(gui->getCurveEditor()->getCurveWidget(), keys) );
}

void
DockablePanel::removeAnimationOnAllParameters()
{
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    std::map<CurveGuiPtr, std::vector<KeyFrame> > keysToRemove;
    const KnobsGuiMapping& knobsMap = getKnobsMapping();

    for (KnobsGuiMapping::const_iterator it = knobsMap.begin(); it != knobsMap.end(); ++it) {
        KnobIPtr knob = it->first.lock();
        if ( knob->isAnimationEnabled() ) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                std::list<CurveGuiPtr> curves = gui->getCurveEditor()->findCurve(it->second, i);

                for (std::list<CurveGuiPtr>::iterator it2 = curves.begin(); it2 != curves.end(); ++it2) {
                    KeyFrameSet keys = (*it2)->getInternalCurve()->getKeyFrames_mt_safe();
                    std::vector<KeyFrame > vect;
                    for (KeyFrameSet::const_iterator it3 = keys.begin(); it3 != keys.end(); ++it3) {
                        vect.push_back(*it3);
                    }
                    keysToRemove.insert( std::make_pair(*it2, vect) );
                }
            }
        }
    }
    pushUndoCommand( new RemoveKeysCommand(gui->getCurveEditor()->getCurveWidget(), keysToRemove) );
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
    if (!panel) {
        throw std::logic_error("");
    }
    NodeGuiPtr node = panel->getNode();
    assert(node);
    if (!node) {
        throw std::logic_error("");
    }
    EffectInstancePtr effect = node->getNode()->getEffectInstance();
    assert(effect);
    if (!effect) {
        throw std::logic_error("");
    }
    NodeGroup* group = dynamic_cast<NodeGroup*>( effect.get() );
    assert(group);
    if (!group) {
        throw std::logic_error("");
    }
    NodeGraphI* graph_i = group->getNodeGraph();
    assert(graph_i);
    if (!graph_i) {
        throw std::logic_error("");
    }
    NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    TabWidget* isParentTab = dynamic_cast<TabWidget*>( graph->parentWidget() );
    if (isParentTab) {
        isParentTab->setCurrentWidget(graph);
    } else {
        NodeGraph* lastSelectedGraph = _imp->_gui->getLastSelectedGraph();
        if (!lastSelectedGraph) {
            const std::list<TabWidget*>& panes = _imp->_gui->getPanes();
            assert(panes.size() >= 1);
            isParentTab = panes.front();
        } else {
            isParentTab = dynamic_cast<TabWidget*>( lastSelectedGraph->parentWidget() );
        }

        assert(isParentTab);
        if (!isParentTab) {
            throw std::logic_error("");
        }
        isParentTab->appendTab(graph, graph);
    }
    QTimer::singleShot( 25, graph, SLOT(centerOnAllNodes()) );
} // DockablePanel::onEnterInGroupClicked

void
DockablePanel::onHideUnmodifiedButtonClicked(bool checked)
{
    if (checked) {
        _imp->_knobsVisibilityBeforeHideModif.clear();
        const KnobsGuiMapping& knobsMap = getKnobsMapping();
        KnobsGuiMapping groups;
        std::set<KnobGuiPtr> toHideGui;
        std::set<KnobIPtr> toHide;
        //printf("hiding...\n");
        for (KnobsGuiMapping::const_iterator it = knobsMap.begin(); it != knobsMap.end(); ++it) {
            KnobIPtr knob = it->first.lock();
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knob.get() );
            KnobParametric* isParametric = dynamic_cast<KnobParametric*>( knob.get() );
            if (isGroup) {
                //printf("groups += %s\n",knob->getName().c_str());
                groups.push_back(std::make_pair(it->first, it->second));
            } else if (!isParametric && !knob->hasModifications() && knob->getName() != kNatronWriteParamStartRender) {
                //printf("toHide += %s\n",knob->getName().c_str());
                toHide.insert(knob);
                toHideGui.insert(it->second);
            }
        }
        // now check if each groups is empty, i.e. all its children are either not visible or going to be hidden
        for (KnobsGuiMapping::const_iterator it = groups.begin(); it != groups.end(); ++it) {
            KnobIPtr knob = it->first.lock();
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knob.get() );
            assert(isGroup);
            std::vector<KnobIPtr> children = isGroup->getChildren();
            bool hideMe = true;
            //printf("should we hide group %s?\n",knob->getName().c_str());
            for (std::vector<KnobIPtr>::const_iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                KnobGroup* isGroup2 = dynamic_cast<KnobGroup*>( (*it2).get() );
                if (!isGroup2 && toHide.find(*it2) == toHide.end() && !(*it2)->getIsSecret()) {
                    //printf("- child %s still visible: NO\n",(*it2)->getName().c_str());
                    hideMe = false;
                    break;
                }
            }
            if (hideMe) {
                toHideGui.insert(it->second);
            }
        }
        for (std::set<KnobGuiPtr>::const_iterator it = toHideGui.begin(); it != toHideGui.end(); ++it) {
            _imp->_knobsVisibilityBeforeHideModif.insert( std::make_pair( *it, (*it)->isSecretRecursive() ) );
            (*it)->hide();
        }
    } else {
        for (std::map<KnobGuiWPtr, bool>::iterator it = _imp->_knobsVisibilityBeforeHideModif.begin();
             it != _imp->_knobsVisibilityBeforeHideModif.end(); ++it) {
            KnobGuiPtr knobGui = it->first.lock();
            if (!it->second && knobGui) {
                knobGui->show();
            }
        }
    }
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

struct TreeItem
{
    QTreeWidgetItem* item;
    KnobIPtr knob;
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

    void initializeKnobs(const KnobsVec& knobs, QTreeWidgetItem* parent, std::list<KnobI*>& markedKnobs);

    void rebuildUserPages();
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


    NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_DockablePanel.cpp"
