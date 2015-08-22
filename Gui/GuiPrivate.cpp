//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "GuiPrivate.h"

#include <cassert>
#include <fstream>
#include <algorithm> // min, max

#include <QtCore/QTextStream>
#include <QWaitCondition>
#include <QMutex>
#include <QCoreApplication>
#include <QAction>
#include <QSettings>
#include <QDebug>
#include <QThread>
#include <QCheckBox>
#include <QTimer>
#include <QTextEdit>


#if QT_VERSION >= 0x050000
#include <QScreen>
#endif
#include <QUndoGroup>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QCloseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QApplication>
#include <QMenuBar>
#include <QDesktopWidget>
#include <QToolBar>
#include <QKeySequence>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QMessageBox>
#include <QImage>
#include <QProgressDialog>

#include <cairo/cairo.h>

#include <boost/version.hpp>
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
GCC_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>

#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AboutWindow.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/FloatingWidget.h"
#include "Gui/FromQtEnums.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Histogram.h"
#include "Gui/Menu.h"
#include "Gui/MessageBox.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/PythonPanels.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ToolButton.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "SequenceParsing.h"

#define NAMED_PLUGIN_GROUP_NO 15

static std::string namedGroupsOrdered[NAMED_PLUGIN_GROUP_NO] = {
    PLUGIN_GROUP_IMAGE,
    PLUGIN_GROUP_COLOR,
    PLUGIN_GROUP_CHANNEL,
    PLUGIN_GROUP_MERGE,
    PLUGIN_GROUP_FILTER,
    PLUGIN_GROUP_TRANSFORM,
    PLUGIN_GROUP_TIME,
    PLUGIN_GROUP_PAINT,
    PLUGIN_GROUP_KEYER,
    PLUGIN_GROUP_MULTIVIEW,
    PLUGIN_GROUP_DEEP,
    PLUGIN_GROUP_3D,
    PLUGIN_GROUP_TOOLSETS,
    PLUGIN_GROUP_OTHER,
    PLUGIN_GROUP_DEFAULT
};

#define PLUGIN_GROUP_DEFAULT_ICON_PATH NATRON_IMAGES_PATH "GroupingIcons/Set" NATRON_ICON_SET_NUMBER "/other_grouping_" NATRON_ICON_SET_NUMBER ".png"


using namespace Natron;





GuiPrivate::GuiPrivate(GuiAppInstance* app,
               Gui* gui)
        : _gui(gui)
        , _isInDraftModeMutex()
        , _isInDraftMode(false)
        , _appInstance(app)
        , _uiUsingMainThreadCond()
        , _uiUsingMainThread(false)
        , _uiUsingMainThreadMutex()
        , _lastQuestionDialogAnswer(Natron::eStandardButtonNo)
        , _lastStopAskingAnswer(false)
        , _currentUndoAction(0)
        , _currentRedoAction(0)
        , _undoStacksGroup(0)
        , _undoStacksActions()
        , _splittersMutex()
        , _splitters()
        , _pyPanelsMutex()
        , _userPanels()
        , _isTripleSyncEnabled(false)
        , actionNew_project(0)
        , actionOpen_project(0)
        , actionClose_project(0)
        , actionSave_project(0)
        , actionSaveAs_project(0)
        , actionExportAsGroup(0)
        , actionSaveAndIncrementVersion(0)
        , actionPreferences(0)
        , actionExit(0)
        , actionProject_settings(0)
        , actionShowOfxLog(0)
        , actionShortcutEditor(0)
        , actionNewViewer(0)
        , actionFullScreen(0)
        , actionClearDiskCache(0)
        , actionClearPlayBackCache(0)
        , actionClearNodeCache(0)
        , actionClearPluginsLoadingCache(0)
        , actionClearAllCaches(0)
        , actionShowAboutWindow(0)
        , actionsOpenRecentFile()
        , renderAllWriters(0)
        , renderSelectedNode(0)
        , actionConnectInput1(0)
        , actionConnectInput2(0)
        , actionConnectInput3(0)
        , actionConnectInput4(0)
        , actionConnectInput5(0)
        , actionConnectInput6(0)
        , actionConnectInput7(0)
        , actionConnectInput8(0)
        , actionConnectInput9(0)
        , actionConnectInput10(0)
        , actionImportLayout(0)
        , actionExportLayout(0)
        , actionRestoreDefaultLayout(0)
        , actionNextTab(0)
        , actionPrevTab(0)
        , actionCloseTab(0)
        , _centralWidget(0)
        , _mainLayout(0)
        , _lastLoadSequenceOpenedDir()
        , _lastLoadProjectOpenedDir()
        , _lastSaveSequenceOpenedDir()
        , _lastSaveProjectOpenedDir()
        , _lastPluginDir()
        , _nextViewerTabPlace(0)
        , _leftRightSplitter(0)
        , _viewerTabsMutex()
        , _viewerTabs()
        , _masterSyncViewer(0)
        , _histogramsMutex()
        , _histograms()
        , _nextHistogramIndex(1)
        , _nodeGraphArea(0)
        , _lastFocusedGraph(0)
        , _groups()
        , _curveEditor(0)
        , _dopeSheetEditor(0)
        , _toolBox(0)
        , _propertiesBin(0)
        , _propertiesScrollArea(0)
        , _propertiesContainer(0)
        , _layoutPropertiesBin(0)
        , _clearAllPanelsButton(0)
        , _minimizeAllPanelsButtons(0)
        , _maxPanelsOpenedSpinBox(0)
        , _isGUIFrozenMutex()
        , _isGUIFrozen(false)
        , menubar(0)
        , menuFile(0)
        , menuRecentFiles(0)
        , menuEdit(0)
        , menuLayout(0)
        , menuDisplay(0)
        , menuOptions(0)
        , menuRender(0)
        , viewersMenu(0)
        , viewerInputsMenu(0)
        , viewersViewMenu(0)
        , cacheMenu(0)
        , _panesMutex()
        , _panes()
        , _floatingWindowMutex()
        , _floatingWindows()
        , _settingsGui(0)
        , _projectGui(0)
        , _currentlyDraggedPanel(0)
        , _aboutWindow(0)
        , _progressBars()
        , openedPanels()
        , _openGLVersion()
        , _glewVersion()
        , _toolButtonMenuOpened(NULL)
        , aboutToCloseMutex()
        , _aboutToClose(false)
        , shortcutEditor(0)
        , leftToolBarDisplayedOnHoverOnly(false)
        , _scriptEditor(0)
        , _lastEnteredTabWidget(0)
        , pythonCommands()
    {
    }

void
GuiPrivate::refreshLeftToolBarVisibility(const QPoint & p)
{
    int toolbarW = _toolBox->sizeHint().width();

    if (p.x() <= toolbarW) {
        _toolBox->show();
    } else {
        _toolBox->hide();
    }
}

void
GuiPrivate::notifyGuiClosing()
{
    ///This is to workaround an issue that when destroying a widget it calls the focusOut() handler hence can
    ///cause bad pointer dereference to the Gui object since we're destroying it.
    {
        QMutexLocker k(&_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _viewerTabs.begin(); it != _viewerTabs.end(); ++it) {
            (*it)->notifyAppClosing();
        }
    }

    const std::list<boost::shared_ptr<NodeGui> > allNodes = _nodeGraphArea->getAllActiveNodes();

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        DockablePanel* panel = (*it)->getSettingPanel();
        if (panel) {
            panel->onGuiClosing();
        }
    }
    _lastFocusedGraph = 0;
    _nodeGraphArea->discardGuiPointer();
    for (std::list<NodeGraph*>::iterator it = _groups.begin(); it != _groups.end(); ++it) {
        (*it)->discardGuiPointer();
    }

    {
        QMutexLocker k(&_panesMutex);
        for (std::list<TabWidget*>::iterator it = _panes.begin(); it != _panes.end(); ++it) {
            (*it)->discardGuiPointer();
        }
    }
}

void
GuiPrivate::createPropertiesBinGui()
{
    _propertiesBin = new PropertiesBinWrapper(_gui);
    _propertiesBin->setScriptName(kPropertiesBinName);
    _propertiesBin->setLabel( QObject::tr("Properties").toStdString() );

    QVBoxLayout* mainPropertiesLayout = new QVBoxLayout(_propertiesBin);
    mainPropertiesLayout->setContentsMargins(0, 0, 0, 0);
    mainPropertiesLayout->setSpacing(0);

    _propertiesScrollArea = new QScrollArea(_propertiesBin);
    QObject::connect( _propertiesScrollArea->verticalScrollBar(), SIGNAL( valueChanged(int) ), _gui, SLOT( onPropertiesScrolled() ) );
    _propertiesScrollArea->setObjectName("Properties");
    assert(_nodeGraphArea);

    _propertiesContainer = new QWidget(_propertiesScrollArea);
    _propertiesContainer->setObjectName("_propertiesContainer");
    _layoutPropertiesBin = new QVBoxLayout(_propertiesContainer);
    _layoutPropertiesBin->setSpacing(0);
    _layoutPropertiesBin->setContentsMargins(0, 0, 0, 0);
    _propertiesContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _propertiesScrollArea->setWidget(_propertiesContainer);
    _propertiesScrollArea->setWidgetResizable(true);

    QWidget* propertiesAreaButtonsContainer = new QWidget(_propertiesBin);
    QHBoxLayout* propertiesAreaButtonsLayout = new QHBoxLayout(propertiesAreaButtonsContainer);
    propertiesAreaButtonsLayout->setContentsMargins(0, 0, 0, 0);
    propertiesAreaButtonsLayout->setSpacing(5);
    QPixmap closePanelPix;
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_PANEL, &closePanelPix);
    _clearAllPanelsButton = new Button(QIcon(closePanelPix), "", propertiesAreaButtonsContainer);
    _clearAllPanelsButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _clearAllPanelsButton->setToolTip( Natron::convertFromPlainText(_gui->tr("Clears all the panels in the properties bin pane."),
                                                                Qt::WhiteSpaceNormal) );
    _clearAllPanelsButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _clearAllPanelsButton, SIGNAL( clicked(bool) ), _gui, SLOT( clearAllVisiblePanels() ) );
    QPixmap minimizePix, maximizePix;
    appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET, &minimizePix);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, &maximizePix);
    QIcon mIc;
    mIc.addPixmap(minimizePix, QIcon::Normal, QIcon::On);
    mIc.addPixmap(maximizePix, QIcon::Normal, QIcon::Off);
    _minimizeAllPanelsButtons = new Button(mIc, "", propertiesAreaButtonsContainer);
    _minimizeAllPanelsButtons->setCheckable(true);
    _minimizeAllPanelsButtons->setChecked(false);
    _minimizeAllPanelsButtons->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _minimizeAllPanelsButtons->setToolTip( Natron::convertFromPlainText(_gui->tr("Minimize / Maximize all panels."), Qt::WhiteSpaceNormal) );
    _minimizeAllPanelsButtons->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _minimizeAllPanelsButtons, SIGNAL( clicked(bool) ), _gui, SLOT( minimizeMaximizeAllPanels(bool) ) );

    _maxPanelsOpenedSpinBox = new SpinBox(propertiesAreaButtonsContainer);
    _maxPanelsOpenedSpinBox->setMaximumSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _maxPanelsOpenedSpinBox->setMinimum(1);
    _maxPanelsOpenedSpinBox->setMaximum(100);
    _maxPanelsOpenedSpinBox->setToolTip( Natron::convertFromPlainText(_gui->tr("Set the maximum of panels that can be opened at the same time "
                                                                           "in the properties bin pane. The special value of 0 indicates "
                                                                           "that an unlimited number of panels can be opened."),
                                                                  Qt::WhiteSpaceNormal) );
    _maxPanelsOpenedSpinBox->setValue( appPTR->getCurrentSettings()->getMaxPanelsOpened() );
    QObject::connect( _maxPanelsOpenedSpinBox, SIGNAL( valueChanged(double) ), _gui, SLOT( onMaxPanelsSpinBoxValueChanged(double) ) );

    propertiesAreaButtonsLayout->addWidget(_maxPanelsOpenedSpinBox);
    propertiesAreaButtonsLayout->addWidget(_clearAllPanelsButton);
    propertiesAreaButtonsLayout->addWidget(_minimizeAllPanelsButtons);
    propertiesAreaButtonsLayout->addStretch();

    mainPropertiesLayout->addWidget(propertiesAreaButtonsContainer);
    mainPropertiesLayout->addWidget(_propertiesScrollArea);

    _gui->registerTab(_propertiesBin, _propertiesBin);
} // createPropertiesBinGui

void
GuiPrivate::createNodeGraphGui()
{
    QGraphicsScene* scene = new QGraphicsScene(_gui);

    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(_gui, _appInstance->getProject(), scene, _gui);
    _nodeGraphArea->setScriptName(kNodeGraphObjectName);
    _nodeGraphArea->setLabel( QObject::tr("Node Graph").toStdString() );
    _gui->registerTab(_nodeGraphArea, _nodeGraphArea);
}

void
GuiPrivate::createCurveEditorGui()
{
    _curveEditor = new CurveEditor(_gui, _appInstance->getTimeLine(), _gui);
    _curveEditor->setScriptName(kCurveEditorObjectName);
    _curveEditor->setLabel( QObject::tr("Curve Editor").toStdString() );
    _gui->registerTab(_curveEditor, _curveEditor);
}

void
GuiPrivate::createDopeSheetGui()
{
    _dopeSheetEditor = new DopeSheetEditor(_gui,_appInstance->getTimeLine(), _gui);
    _dopeSheetEditor->setScriptName(kDopeSheetEditorObjectName);
    _dopeSheetEditor->setLabel(QObject::tr("Dope Sheet").toStdString());
    _gui->registerTab(_dopeSheetEditor, _dopeSheetEditor);
}

void
GuiPrivate::createScriptEditorGui()
{
    _scriptEditor = new ScriptEditor(_gui);
    _scriptEditor->setScriptName("scriptEditor");
    _scriptEditor->setLabel( QObject::tr("Script Editor").toStdString() );
    _scriptEditor->hide();
    _gui->registerTab(_scriptEditor, _scriptEditor);
}


TabWidget*
GuiPrivate::getOnly1NonFloatingPane(int & count) const
{
    assert( !_panesMutex.tryLock() );
    count = 0;
    if ( _panes.empty() ) {
        return NULL;
    }
    TabWidget* firstNonFloating = 0;
    for (std::list<TabWidget*>::const_iterator it = _panes.begin(); it != _panes.end(); ++it) {
        if ( !(*it)->isFloatingWindowChild() ) {
            if (!firstNonFloating) {
                firstNonFloating = *it;
            }
            ++count;
        }
    }
    ///there should always be at least 1 non floating window
    assert(firstNonFloating);

    return firstNonFloating;
}

namespace {
class AutoRaiseToolButton
    : public QToolButton
{
    Gui* _gui;
    bool _menuOpened;

public:

    AutoRaiseToolButton(Gui* gui,
                        QWidget* parent)
        : QToolButton(parent)
        , _gui(gui)
        , _menuOpened(false)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
    }

private:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        _menuOpened = !_menuOpened;
        if (_menuOpened) {
            setFocus();
            _gui->setToolButtonMenuOpened(this);
        } else {
            _gui->setToolButtonMenuOpened(NULL);
        }
        QToolButton::mousePressEvent(e);
    }

    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        _gui->setToolButtonMenuOpened(NULL);
        QToolButton::mouseReleaseEvent(e);
    }

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL
    {
        if (e->key() == Qt::Key_Right) {
            QMenu* m = menu();
            if (m) {
                QList<QAction*> actions = m->actions();
                if ( !actions.isEmpty() ) {
                    m->setActiveAction(actions[0]);
                }
            }
            showMenu();
        } else if (e->key() == Qt::Key_Left) {
            //This code won't work because the menu is active and modal
            //But at least it deactivate the focus tabbing when pressing the left key
            QMenu* m = menu();
            if ( m && m->isVisible() ) {
                m->hide();
            }
        } else {
            QToolButton::keyPressEvent(e);
        }
    }

    virtual void enterEvent(QEvent* e) OVERRIDE FINAL
    {
        AutoRaiseToolButton* btn = dynamic_cast<AutoRaiseToolButton*>( _gui->getToolButtonMenuOpened() );

        if ( btn && (btn != this) && btn->menu()->isActiveWindow() ) {
            btn->menu()->close();
            btn->_menuOpened = false;
            setFocus();
            _gui->setToolButtonMenuOpened(this);
            _menuOpened = true;
            showMenu();
        }
        QToolButton::enterEvent(e);
    }
};
} // anonymous namespace

void
GuiPrivate::addToolButton(ToolButton* tool)
{
    QToolButton* button = new AutoRaiseToolButton(_gui, _toolBox);

    button->setIcon( tool->getIcon() );
    button->setMenu( tool->getMenu() );
    button->setPopupMode(QToolButton::InstantPopup);
    button->setToolTip( Natron::convertFromPlainText(tool->getLabel().trimmed(), Qt::WhiteSpaceNormal) );
    _toolBox->addWidget(button);
}

void
GuiPrivate::setUndoRedoActions(QAction* undoAction,
                               QAction* redoAction)
{
    if (_currentUndoAction) {
        menuEdit->removeAction(_currentUndoAction);
    }
    if (_currentRedoAction) {
        menuEdit->removeAction(_currentRedoAction);
    }
    _currentUndoAction = undoAction;
    _currentRedoAction = redoAction;
    if (undoAction) {
        menuEdit->addAction(undoAction);
    }
    if (redoAction) {
        menuEdit->addAction(redoAction);
    }
}


bool
GuiPrivate::checkProjectLockAndWarn(const QString& projectPath,const QString& projectName)
{
    boost::shared_ptr<Natron::Project> project= _appInstance->getProject();
    QString author,lockCreationDate;
    qint64 lockPID;
    if (project->getLockFileInfos(projectPath,projectName,&author, &lockCreationDate, &lockPID)) {
        if (lockPID != QCoreApplication::applicationPid()) {
            Natron::StandardButtonEnum rep = Natron::questionDialog(QObject::tr("Project").toStdString(),
                                                                    QObject::tr("This project is already opened in another instance of Natron by ").toStdString() +
                                                                    author.toStdString() + QObject::tr(" and was opened on ").toStdString() + lockCreationDate.toStdString()
                                                                    + QObject::tr(" by a Natron process ID of ").toStdString() + QString::number(lockPID).toStdString() + QObject::tr(".\nContinue anyway?").toStdString(), false, Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo));
            if (rep == Natron::eStandardButtonYes) {
                return true;
            } else {
                return false;
            }
        }
    }
    return true;
    
}

void
GuiPrivate::restoreGuiGeometry()
{
    QSettings settings(NATRON_ORGANIZATION_NAME, NATRON_APPLICATION_NAME);

    settings.beginGroup("MainWindow");

    if ( settings.contains("pos") ) {
        QPoint pos = settings.value("pos").toPoint();
        _gui->move(pos);
    }
    if ( settings.contains("size") ) {
        QSize size = settings.value("size").toSize();
        _gui->resize(size);
    } else {
        ///No window size serialized, give some appriopriate default value according to the screen size
        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        _gui->resize( (int)( 0.93 * screen.width() ), (int)( 0.93 * screen.height() ) ); // leave some space
    }
    if ( settings.contains("fullScreen") ) {
        bool fs = settings.value("fullScreen").toBool();
        if (fs) {
            _gui->toggleFullScreen();
        }
    }

    if ( settings.contains("ToolbarHidden") ) {
        leftToolBarDisplayedOnHoverOnly = settings.value("ToolbarHidden").toBool();
    }

    settings.endGroup();

    if ( settings.contains("LastOpenProjectDialogPath") ) {
        _lastLoadProjectOpenedDir = settings.value("LastOpenProjectDialogPath").toString();
        QDir d(_lastLoadProjectOpenedDir);
        if ( !d.exists() ) {
            _lastLoadProjectOpenedDir.clear();
        }
    }
    if ( settings.contains("LastSaveProjectDialogPath") ) {
        _lastSaveProjectOpenedDir = settings.value("LastSaveProjectDialogPath").toString();
        QDir d(_lastSaveProjectOpenedDir);
        if ( !d.exists() ) {
            _lastSaveProjectOpenedDir.clear();
        }
    }
    if ( settings.contains("LastLoadSequenceDialogPath") ) {
        _lastLoadSequenceOpenedDir = settings.value("LastLoadSequenceDialogPath").toString();
        QDir d(_lastLoadSequenceOpenedDir);
        if ( !d.exists() ) {
            _lastLoadSequenceOpenedDir.clear();
        }
    }
    if ( settings.contains("LastSaveSequenceDialogPath") ) {
        _lastSaveSequenceOpenedDir = settings.value("LastSaveSequenceDialogPath").toString();
        QDir d(_lastSaveSequenceOpenedDir);
        if ( !d.exists() ) {
            _lastSaveSequenceOpenedDir.clear();
        }
    }
    if ( settings.contains("LastPluginDir") ) {
        _lastPluginDir = settings.value("LastPluginDir").toString();
    }
} // GuiPrivate::restoreGuiGeometry

void
GuiPrivate::saveGuiGeometry()
{
    QSettings settings(NATRON_ORGANIZATION_NAME, NATRON_APPLICATION_NAME);

    settings.beginGroup("MainWindow");
    settings.setValue( "pos", _gui->pos() );
    settings.setValue( "size", _gui->size() );
    settings.setValue( "fullScreen", _gui->isFullScreen() );
    settings.setValue( "ToolbarHidden", leftToolBarDisplayedOnHoverOnly);
    settings.endGroup();

    settings.setValue("LastOpenProjectDialogPath", _lastLoadProjectOpenedDir);
    settings.setValue("LastSaveProjectDialogPath", _lastSaveProjectOpenedDir);
    settings.setValue("LastLoadSequenceDialogPath", _lastLoadSequenceOpenedDir);
    settings.setValue("LastSaveSequenceDialogPath", _lastSaveSequenceOpenedDir);
    settings.setValue("LastPluginDir", _lastPluginDir);
}


QAction*
GuiPrivate::findActionRecursive(int i,
                                QWidget* widget,
                                const QStringList & grouping)
{
    assert( i < grouping.size() );
    QList<QAction*> actions = widget->actions();
    for (QList<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
        if ( (*it)->text() == grouping[i] ) {
            if (i == grouping.size() - 1) {
                return *it;
            } else {
                QMenu* menu = (*it)->menu();
                if (menu) {
                    return findActionRecursive(i + 1, menu, grouping);
                } else {
                    ///Error: specified the name of an already existing action
                    return 0;
                }
            }
        }
    }
    ///Create the entry
    if (i < grouping.size() - 1) {
        Menu* menu = new Menu(widget);
        menu->setTitle(grouping[i]);
        QMenu* isMenu = dynamic_cast<QMenu*>(widget);
        QMenuBar* isMenuBar = dynamic_cast<QMenuBar*>(widget);
        if (isMenu) {
            isMenu->addAction( menu->menuAction() );
        } else if (isMenuBar) {
            isMenuBar->addAction( menu->menuAction() );
        }

        return findActionRecursive(i + 1, menu, grouping);
    } else {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupGlobal, grouping[i], grouping[i], widget);
        QObject::connect( action, SIGNAL( triggered() ), _gui, SLOT( onUserCommandTriggered() ) );
        QMenu* isMenu = dynamic_cast<QMenu*>(widget);
        if (isMenu) {
            isMenu->addAction(action);
        }

        return action;
    }

    return 0;
}
