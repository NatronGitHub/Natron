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

#include "GuiAppInstance.h"

#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QSettings>
#include <QtCore/QMutex>
#include <QApplication>
#ifdef Q_OS_MAC
#include <QMacStyle>
#endif

#include "Engine/CLArgs.h"
#include "Engine/Project.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Plugin.h"
#include "Engine/ProcessHandler.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Settings.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/KnobFile.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RenderEngine.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Global/QtCompat.h"
#include "Global/StrUtils.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/BackdropGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGuiFile.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/Histogram.h"
#include "Gui/ProgressPanel.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/ViewerTab.h"
#include "Gui/SplashScreen.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ViewerGL.h"

#include "Serialization/WorkspaceSerialization.h"


NATRON_NAMESPACE_ENTER


struct KnobDnDData
{
    KnobIWPtr source;
    DimSpec sourceDimension;
    ViewSetSpec sourceView;
    QDrag* drag;
};

struct GuiAppInstancePrivate
{
    Gui* _gui; //< ptr to the Gui interface
    bool _isClosing;

    //////////////
    ////This one is a little tricky:
    ////If the GUI is showing a dialog, then when the exec loop of the dialog returns, it will
    ////immediately process events that were triggered meanwhile. That means that if the dialog
    ////is popped after a request made inside a plugin action, then it may trigger another action
    ////like, say, overlay focus gain on the viewer. If this boolean is true we then don't do any
    ////check on the recursion level of actions. This is a limitation of using the main "qt" thread
    ////as the main "action" thread.
    int _showingDialog;
    mutable QMutex _showingDialogMutex;
    FileDialogPreviewProviderPtr _previewProvider;
    mutable QMutex lastTimelineViewerMutex;
    NodePtr lastTimelineViewer;
    LoadProjectSplashScreen* loadProjectSplash;
    std::string declareAppAndParamsString;
    mutable QMutex rotoDataMutex;

    // When drawing a stroke, this is a pointer to the stroke being painted.
    RotoStrokeItemPtr strokeBeingPainted;

    // We remember what was the last stroke drawn
    RotoStrokeItemWPtr lastStrokeBeingPainted;
    KnobDnDData knobDnd;

    // The viewer that's mastering others when all viewports are in sync
    NodeWPtr masterSyncViewer;

    GuiAppInstancePrivate()
        : _gui(NULL)
        , _isClosing(false)
        , _showingDialog(0)
        , _showingDialogMutex()
        , _previewProvider()
        , lastTimelineViewerMutex()
        , lastTimelineViewer()
        , loadProjectSplash(0)
        , declareAppAndParamsString()
        , rotoDataMutex()
        , strokeBeingPainted()
        , knobDnd()
        , masterSyncViewer()
    {
    }

    void findOrCreateToolButtonRecursive(const PluginGroupNodePtr& n);
};

GuiAppInstance::GuiAppInstance(int appID)
    : AppInstance(appID)
    , _imp(new GuiAppInstancePrivate)

{
#ifdef DEBUG
    qDebug() << "GuiAppInstance()" << (void*)(this);
#endif
}

void
GuiAppInstance::resetPreviewProvider()
{
    deletePreviewProvider();
    _imp->_previewProvider = boost::make_shared<FileDialogPreviewProvider>();
}

void
GuiAppInstance::deletePreviewProvider()
{
    /**
       Kill the nodes used to make the previews in the file dialogs
     **/
    if (_imp->_previewProvider) {
        if (_imp->_previewProvider->viewerNode) {
            //_imp->_gui->removeViewerTab(_imp->_previewProvider->viewerUI, true, true);
            _imp->_previewProvider->viewerNodeInternal->destroyNode();
            _imp->_previewProvider->viewerNodeInternal.reset();
        }


        if (_imp->_previewProvider->readerNode) {
            _imp->_previewProvider->readerNode->destroyNode();
            _imp->_previewProvider->readerNode.reset();
        }

        _imp->_previewProvider.reset();
    }
}

void
GuiAppInstance::aboutToQuit()
{
#ifdef DEBUG
    qDebug() << "GuiAppInstance::aboutToQuit()" << (void*)(this);
#endif
    deletePreviewProvider();

    if (_imp->_gui) {
        ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
        _imp->_gui->setGuiAboutToClose(true);

        _imp->_gui->notifyGuiClosing();

        AppInstance::aboutToQuit();

        _imp->_isClosing = true;
        _imp->_gui->close();
        //delete _imp->_gui;
        _imp->_gui->deleteLater();



        // Make sure all events are processed
        qApp->processEvents();
        // Make sure all deleteLater calls are reached
        qApp->sendPostedEvents(0, QEvent::DeferredDelete);

        _imp->_gui = 0;
    }
}

GuiAppInstance::~GuiAppInstance()
{
#ifdef DEBUG
    qDebug() << "~GuiAppInstance()" << (void*)(this);
#endif
}

bool
GuiAppInstance::isClosing() const
{
    return !_imp || _imp->_isClosing;
}

void
GuiAppInstancePrivate::findOrCreateToolButtonRecursive(const PluginGroupNodePtr& n)
{
    _gui->findOrCreateToolButton(n);
    const std::list<PluginGroupNodePtr>& children = n->getChildren();
    for (std::list<PluginGroupNodePtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
        findOrCreateToolButtonRecursive(*it);
    }
}

void
GuiAppInstance::createMainWindow()
{
    boost::shared_ptr<GuiAppInstance> thisShared = toGuiAppInstance( shared_from_this() );
    assert(thisShared);
    _imp->_gui = new Gui(thisShared);
    _imp->_gui->createGui();
    setMainWindowPointer(_imp->_gui);
}

#ifdef Q_OS_MAC
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class Qt4RetinaFixStyle : public QMacStyle
{
public:

    Qt4RetinaFixStyle()
    : QMacStyle()
    {

    }

    virtual ~Qt4RetinaFixStyle()
    {

    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption *opt, const QWidget *widget) const OVERRIDE FINAL
    {
        if (metric == PM_SmallIconSize || metric == PM_ToolBarIconSize) {
            int ret = QMacStyle::pixelMetric(metric, opt, widget);
            Gui* gui = getGui();
            if (gui) {
                double scale = gui->getHighDPIScaleFactor();
                if (scale > 1) {
                    ret /= scale;
                }
            }
            return ret;
        } else {
            return QMacStyle::pixelMetric(metric, opt, widget);
        }
    }

private:

    Gui* getGui() const
    {
        const AppInstanceVec& apps = appPTR->getAppInstances();
        for (AppInstanceVec::const_iterator it = apps.begin(); it != apps.end(); ++it) {
            GuiAppInstancePtr a = toGuiAppInstance(*it);
            if (a) {
                return a->getGui();
            }
        }
        return 0;
    }
};
#endif
#endif

void
GuiAppInstance::loadInternal(const CLArgs& cl,
                             bool makeEmptyInstance)
{
    if (getAppID() == 0) {
        appPTR->setLoadingStatus( tr("Creating user interface...") );
    }

    try {
        declareCurrentAppVariable_Python();
    } catch (const std::exception& e) {
        throw std::runtime_error( e.what() );
    }

    createMainWindow();

#ifdef Q_OS_MAC
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    // On Qt4 for Mac, Retina is not well supported, see:
    // https://bugreports.qt.io/browse/QTBUG-23870
    // Note that setting the application style will override any
    // style set on the command line with the -style option.
    if (getAppID() == 0) {
        QApplication::setStyle(new Qt4RetinaFixStyle);
    }
#endif
#endif

    printAutoDeclaredVariable(_imp->declareAppAndParamsString);

    ///if the app is interactive, build the plugins toolbuttons from the groups we extracted off the plugins.
    const std::list<PluginGroupNodePtr> & _toolButtons = appPTR->getTopLevelPluginsToolButtons();
    for (std::list<PluginGroupNodePtr  >::const_iterator it = _toolButtons.begin(); it != _toolButtons.end(); ++it) {
        _imp->findOrCreateToolButtonRecursive(*it);
    }
    _imp->_gui->sortAllPluginsToolButtons();

    Q_EMIT pluginsPopulated();

    ///show the gui
    _imp->_gui->show();


    SettingsPtr nSettings = appPTR->getCurrentSettings();
    QObject::connect( getProject().get(), SIGNAL(formatChanged(Format)), this, SLOT(projectFormatChanged(Format)) );


    if ( nSettings->isCheckForUpdatesEnabled() ) {
        appPTR->setLoadingStatus( tr("Checking if updates are available...") );
        checkForNewVersion();
    }


    if ( nSettings->isDefaultAppearanceOutdated() ) {
        StandardButtonEnum reply = Dialogs::questionDialog(tr("Appearance").toStdString(),
                                                           tr("The default appearance of %1 changed since last version.\n"
                                                              "Would you like to use the new default appearance?").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), false);
        if (reply == eStandardButtonYes) {
            nSettings->restoreDefaultAppearance();
        }
    }

    /// Create auto-save dir if it does not exists
    QDir dir = Project::autoSavesDir();
    dir.mkpath( QString::fromUtf8(".") );


    if (getAppID() == 0) {
        QString missingOpenGLError;
        if ( !appPTR->hasOpenGLForRequirements(eOpenGLRequirementsTypeViewer, &missingOpenGLError) ) {
            throw std::runtime_error( missingOpenGLError.toStdString() );
        }

        appPTR->getCurrentSettings()->doOCIOStartupCheckIfNeeded();
    }

    if (makeEmptyInstance) {
        return;
    }

    executeCommandLinePythonCommands(cl);

    /// If this is the first instance of the software, try to load an autosave
    if ( (getAppID() == 0) && cl.getScriptFilename().isEmpty() ) {
        if ( findAndTryLoadUntitledAutoSave() ) {
            ///if we successfully loaded an autosave ignore the specified project in the launch args.
            return;
        }
    }


    QFileInfo info( cl.getScriptFilename() );

    if ( cl.getScriptFilename().isEmpty() || !info.exists() ) {
        getProject()->createViewer();
        execOnProjectCreatedCallback();

        const QString& imageFile = cl.getImageFilename();
        if ( !imageFile.isEmpty() ) {
            handleFileOpenEvent( imageFile.toStdString() );
        }
    } else {
        if ( info.suffix() == QString::fromUtf8("py") ) {
            appPTR->setLoadingStatus( tr("Loading script: ") + cl.getScriptFilename() );

            ///If this is a Python script, execute it
            loadPythonScript(info);
            execOnProjectCreatedCallback();
        } else if ( info.suffix() == QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ) {
            ///Otherwise just load the project specified.
            QString name = info.fileName();
            QString path = info.path();
            StrUtils::ensureLastPathSeparator(path);
            appPTR->setLoadingStatus(tr("Loading project: ") + path + name);
            getProject()->loadProject(path, name);
            ///remove any file open event that might have occurred
            appPTR->setFileToOpen( QString() );
        } else {
            Dialogs::errorDialog( tr("Invalid file").toStdString(),
                                  tr("%1 only accepts python scripts or .ntp project files").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
            execOnProjectCreatedCallback();
        }
    }

    const QString& extraOnProjectCreatedScript = cl.getDefaultOnProjectLoadedScript();
    if ( !extraOnProjectCreatedScript.isEmpty() ) {
        QFileInfo cbInfo(extraOnProjectCreatedScript);
        if ( cbInfo.exists() ) {
            loadPythonScript(cbInfo);
        }
    }
} // load

bool
GuiAppInstance::findAndTryLoadUntitledAutoSave()
{
    if ( !appPTR->getCurrentSettings()->isAutoSaveEnabledForUnsavedProjects() ) {
        return false;
    }

    QDir savesDir( Project::autoSavesDir() );
    QStringList entries = savesDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QStringList foundAutosaves;
    for (int i = 0; i < entries.size(); ++i) {
        const QString & entry = entries.at(i);
        QString searchStr( QLatin1Char('.') );
        searchStr.append( QString::fromUtf8(NATRON_PROJECT_FILE_EXT) );
        searchStr.append( QString::fromUtf8(".autosave") );
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos == -1) {
            continue;
        }

        foundAutosaves << entry;
    }
    if ( foundAutosaves.empty() ) {
        return false;
    }

    QString text = tr("An auto-saved project was found with no associated project file.\n"
                      "Would you like to restore it?\n"
                      "Clicking No will remove this auto-save.");

    appPTR->hideSplashScreen();

    StandardButtonEnum ret = Dialogs::questionDialog(tr("Auto-save").toStdString(),
                                                     text.toStdString(), false, StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                                     eStandardButtonYes);
    if ( (ret == eStandardButtonNo) || (ret == eStandardButtonEscape) ) {
        Project::clearAutoSavesDir();

        return false;
    }

    for (int i = 0; i < foundAutosaves.size(); ++i) {
        const QString& autoSaveFileName = foundAutosaves[i];
        if (i == 0) {
            //Load the first one into the current instance of Natron, then open-up new instances
            if ( !getProject()->loadProject(savesDir.path() + QLatin1Char('/'), autoSaveFileName, true) ) {
                return false;
            }
        } else {
            CLArgs cl;
            AppInstancePtr newApp = appPTR->newAppInstance(cl, false);
            if ( !newApp->getProject()->loadProject(savesDir.path() + QLatin1Char('/'), autoSaveFileName, true) ) {
                return false;
            }
        }
    }

    return true;
} // findAndTryLoadAutoSave



std::string
GuiAppInstance::openImageFileDialog()
{
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    std::string ret = _imp->_gui->openImageSequenceDialog();
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }

    return ret;
}

std::string
GuiAppInstance::saveImageFileDialog()
{
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    std::string ret =  _imp->_gui->saveImageSequenceDialog();
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }

    return ret;
}

Gui*
GuiAppInstance::getGui() const
{
    return _imp->_gui;
}

bool
GuiAppInstance::shouldRefreshPreview() const
{
    return !_imp->_gui->isDraftRenderEnabled();
}

void
GuiAppInstance::errorDialog(const std::string & title,
                            const std::string & message,
                            bool useHtml) const
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return;
    }
    _imp->_gui->errorDialog(title, message, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }
}

void
GuiAppInstance::errorDialog(const std::string & title,
                            const std::string & message,
                            bool* stopAsking,
                            bool useHtml) const
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return;
    }
    _imp->_gui->errorDialog(title, message, stopAsking, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }
}

void
GuiAppInstance::warningDialog(const std::string & title,
                              const std::string & message,
                              bool useHtml) const
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return;
    }
    _imp->_gui->warningDialog(title, message, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }
}

void
GuiAppInstance::warningDialog(const std::string & title,
                              const std::string & message,
                              bool* stopAsking,
                              bool useHtml) const
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return;
    }
    _imp->_gui->warningDialog(title, message, stopAsking, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }
}

void
GuiAppInstance::informationDialog(const std::string & title,
                                  const std::string & message,
                                  bool useHtml) const
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return;
    }
    _imp->_gui->informationDialog(title, message, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }
}

void
GuiAppInstance::informationDialog(const std::string & title,
                                  const std::string & message,
                                  bool* stopAsking,
                                  bool useHtml) const
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return;
    }
    _imp->_gui->informationDialog(title, message, stopAsking, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }
}

StandardButtonEnum
GuiAppInstance::questionDialog(const std::string & title,
                               const std::string & message,
                               bool useHtml,
                               StandardButtons buttons,
                               StandardButtonEnum defaultButton) const
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return eStandardButtonIgnore;
    }
    StandardButtonEnum ret =  _imp->_gui->questionDialog(title, message, useHtml, buttons, defaultButton);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }

    return ret;
}

StandardButtonEnum
GuiAppInstance::questionDialog(const std::string & title,
                               const std::string & message,
                               bool useHtml,
                               StandardButtons buttons,
                               StandardButtonEnum defaultButton,
                               bool* stopAsking)
{
    if ( appPTR->isSplashcreenVisible() ) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        ++_imp->_showingDialog;
    }
    if (!_imp->_gui) {
        return eStandardButtonIgnore;
    }
    StandardButtonEnum ret =  _imp->_gui->questionDialog(title, message, useHtml, buttons, defaultButton, stopAsking);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        --_imp->_showingDialog;
    }

    return ret;
}

bool
GuiAppInstance::isShowingDialog() const
{
    QMutexLocker l(&_imp->_showingDialogMutex);

    return _imp->_showingDialog > 0;
}

void
GuiAppInstance::loadProjectGui(bool isAutosave, const SERIALIZATION_NAMESPACE::ProjectSerializationPtr& serialization) const
{
    _imp->_gui->loadProjectGui(isAutosave, serialization);
}


void
GuiAppInstance::setupViewersForViews(const std::vector<std::string>& viewNames)
{
    _imp->_gui->updateViewsActions(viewNames.size());
}

void
GuiAppInstance::setViewersCurrentView(ViewIdx view)
{
    _imp->_gui->setViewersCurrentView(view);
}

void
GuiAppInstance::notifyRenderStarted(const QString & sequenceName,
                                    TimeValue firstFrame,
                                    TimeValue lastFrame,
                                    TimeValue frameStep,
                                    bool canPause,
                                    const NodePtr& writer,
                                    const ProcessHandlerPtr & process)
{
    _imp->_gui->onRenderStarted(sequenceName, firstFrame, lastFrame, frameStep, canPause, writer, process);
}

void
GuiAppInstance::notifyRenderRestarted( const NodePtr& writer,
                                       const ProcessHandlerPtr & process)
{
    _imp->_gui->onRenderRestarted(writer, process);
}

void
GuiAppInstance::setUndoRedoStackLimit(int limit)
{
    _imp->_gui->setUndoRedoStackLimit(limit);
}

void
GuiAppInstance::progressStart(const NodePtr& node,
                              const std::string &message,
                              const std::string &messageid,
                              bool canCancel)
{
    _imp->_gui->progressStart(node, message, messageid, canCancel);
}

void
GuiAppInstance::progressEnd(const NodePtr& node)
{
    _imp->_gui->progressEnd(node);
}

bool
GuiAppInstance::progressUpdate(const NodePtr& node,
                               double t)
{
    bool ret =  _imp->_gui->progressUpdate(node, t);

    return ret;
}

void
GuiAppInstance::onMaxPanelsOpenedChanged(int maxPanels)
{
    _imp->_gui->onMaxVisibleDockablePanelChanged(maxPanels);
}

void
GuiAppInstance::onRenderQueuingChanged(bool queueingEnabled)
{
    _imp->_gui->getProgressPanel()->onRenderQueuingSettingChanged(queueingEnabled);
}


FileDialogPreviewProviderPtr
GuiAppInstance::getPreviewProvider() const
{
    return _imp->_previewProvider;
}

void
GuiAppInstance::projectFormatChanged(const Format& /*f*/)
{
    if (_imp->_previewProvider && _imp->_previewProvider->viewerNode && _imp->_previewProvider->viewerUI) {
        _imp->_previewProvider->viewerUI->getInternalNode()->getNode()->getRenderEngine()->renderCurrentFrame();
    }
}

bool
GuiAppInstance::isGuiFrozen() const
{
    return _imp->_gui ? _imp->_gui->isGUIFrozen() : false;
}


void
GuiAppInstance::redrawAllViewers()
{
    _imp->_gui->redrawAllViewers();
}

void
GuiAppInstance::redrawAllTimelines()
{
    _imp->_gui->redrawAllTimelines();
}

void
GuiAppInstance::toggleAutoHideGraphInputs()
{
    _imp->_gui->toggleAutoHideGraphInputs();
}

void
GuiAppInstance::appendToScriptEditor(const std::string& str)
{
    _imp->_gui->appendToScriptEditor(str);
}

void
GuiAppInstance::printAutoDeclaredVariable(const std::string& str)
{
    if (_imp->_gui) {
        _imp->_gui->printAutoDeclaredVariable(str);
    }
}

void
GuiAppInstance::setLastViewerUsingTimeline(const NodePtr& node)
{
    if (!node) {
        QMutexLocker k(&_imp->lastTimelineViewerMutex);
        _imp->lastTimelineViewer.reset();

        return;
    }
    if ( node->isEffectViewerNode() ) {
        QMutexLocker k(&_imp->lastTimelineViewerMutex);
        _imp->lastTimelineViewer = node;
    }
}

ViewerNodePtr
GuiAppInstance::getLastViewerUsingTimeline() const
{
    QMutexLocker k(&_imp->lastTimelineViewerMutex);

    if (!_imp->lastTimelineViewer) {
        return ViewerNodePtr();
    }

    return _imp->lastTimelineViewer->isEffectViewerNode();
}

void
GuiAppInstance::discardLastViewerUsingTimeline()
{
    QMutexLocker k(&_imp->lastTimelineViewerMutex);

    _imp->lastTimelineViewer.reset();
}

void
GuiAppInstance::declareCurrentAppVariable_Python()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    std::string appIDStr = getAppIDString();
    /// define the app variable
    std::stringstream ss;

    ss << appIDStr << " = " << NATRON_GUI_PYTHON_MODULE_NAME << ".natron.getGuiInstance(" << getAppID() << ") \n";
    const KnobsVec& knobs = getProject()->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss << appIDStr << "." << (*it)->getName() << " = "  << appIDStr  << ".getProjectParam('" <<
        (*it)->getName() << "')\n";
    }

    std::string script = ss.str();
    std::string err;
    _imp->declareAppAndParamsString = script;
    assert(!PyErr_Occurred());
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    if (!ok) {
        throw std::runtime_error("GuiAppInstance::declareCurrentAppVariable_Python() failed!");
    }
}

void
GuiAppInstance::createLoadProjectSplashScreen(const QString& projectFile)
{
    if (_imp->loadProjectSplash) {
        return;
    }
    _imp->loadProjectSplash = new LoadProjectSplashScreen(projectFile);
    _imp->loadProjectSplash->setAttribute(Qt::WA_DeleteOnClose, 0);
}

void
GuiAppInstance::updateProjectLoadStatus(const QString& str)
{
    if (!_imp->loadProjectSplash) {
        return;
    }
    _imp->loadProjectSplash->updateText(str);
}

void
GuiAppInstance::closeLoadPRojectSplashScreen()
{
    if (_imp->loadProjectSplash) {
        _imp->loadProjectSplash->close();
        delete _imp->loadProjectSplash;
        _imp->loadProjectSplash = 0;
    }
}

void
GuiAppInstance::getAllViewers(std::list<ViewerNodePtr>* viewers) const
{
    std::list<ViewerTab*> viewerTabs = _imp->_gui->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
        viewers->push_back((*it)->getInternalNode());
    }
}

void
GuiAppInstance::renderAllViewers()
{
    _imp->_gui->renderAllViewers();
}

void
GuiAppInstance::refreshAllPreviews()
{
    getProject()->refreshPreviews();
}

void
GuiAppInstance::abortAllViewers(bool autoRestartPlayback)
{
    _imp->_gui->abortAllViewers(autoRestartPlayback);
}

void
GuiAppInstance::getViewersOpenGLContextFormat(int* bitdepthPerComponent, bool *hasAlpha) const
{
    ViewerTab* viewer = _imp->_gui->getActiveViewer();
    if (viewer) {
        return viewer->getViewer()->getOpenGLContextFormat(bitdepthPerComponent, hasAlpha);
    }
}

void
GuiAppInstance::reloadStylesheet()
{
    if (_imp->_gui) {
        _imp->_gui->reloadStylesheet();
    }
}

void
GuiAppInstance::createGroupGui(const NodePtr & group, const CreateNodeArgs& args)
{
    _imp->_gui->createGroupGui(group, args);
}


bool
GuiAppInstance::isDraftRenderEnabled() const
{
    return _imp->_gui ? _imp->_gui->isDraftRenderEnabled() : false;
}

void
GuiAppInstance::setDraftRenderEnabled(bool b)
{
    if (_imp->_gui) {
        _imp->_gui->setDraftRenderEnabled(b);
    }
}



bool
GuiAppInstance::isRenderStatsActionChecked() const
{
    return _imp->_gui->areRenderStatsEnabled();
}

bool
GuiAppInstance::save(const std::string& filename)
{
    if ( filename.empty() ) {
        ProjectPtr project = getProject();
        if ( project->hasProjectBeenSavedByUser() ) {
            return _imp->_gui->saveProject();
        } else {
            return _imp->_gui->saveProjectAs();
        }
    } else {
        return _imp->_gui->saveProjectAs(filename);
    }
}

bool
GuiAppInstance::saveAs(const std::string& filename)
{
    return _imp->_gui->saveProjectAs(filename);
}

AppInstancePtr
GuiAppInstance::loadProject(const std::string& filename)
{
    return _imp->_gui->openProject(filename);
}

///Close the current project but keep the window
bool
GuiAppInstance::resetProject()
{
    return _imp->_gui->abortProject(false, true, true);
}

///Reset + close window, quit if last window
bool
GuiAppInstance::closeProject()
{
    return _imp->_gui->abortProject(true, true, true);
}

///Opens a new window
AppInstancePtr
GuiAppInstance::newProject()
{
    return _imp->_gui->createNewProject();
}

void
GuiAppInstance::handleFileOpenEvent(const std::string &filename)
{
    QString fileCopy( QString::fromUtf8( filename.c_str() ) );

    fileCopy.replace( QLatin1Char('\\'), QLatin1Char('/') );
    QString ext = QtCompat::removeFileExtension(fileCopy);
    if ( ext == QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ) {
        AppInstancePtr app = getGui()->openProject(filename);
        if (!app) {
            Dialogs::errorDialog(tr("Project").toStdString(), tr("Failed to open project").toStdString() + ' ' + filename);
        }
    } else {
        appPTR->handleImageFileOpenRequest(filename);
    }
}

void*
GuiAppInstance::getOfxHostOSHandle() const
{
    if (!_imp->_gui) {
        return (void*)0;
    }
    WId ret = _imp->_gui->winId();

    return (void*)ret;
}

void
GuiAppInstance::setUserIsPainting(const RotoStrokeItemPtr& stroke)
{
    RotoStrokeItemPtr lastStroke;
    {
        QMutexLocker k(&_imp->rotoDataMutex);
        _imp->strokeBeingPainted = stroke;
        lastStroke = _imp->lastStrokeBeingPainted.lock();
        if (stroke) {
            _imp->lastStrokeBeingPainted = stroke;
        }

    }
    if (stroke && lastStroke != stroke) {
        // We are starting a new stroke, we need to clear any preview data that is stored on effects in the tree.
        clearAllLastRenderedImages();
    }
}

RotoStrokeItemPtr
GuiAppInstance::getActiveRotoDrawingStroke() const
{
    QMutexLocker k(&_imp->rotoDataMutex);
    return _imp->strokeBeingPainted;
}


void
GuiAppInstance::goToPreviousKeyframe()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    TimeLinePtr timeline = getProject()->getTimeLine();
    int currentFrame = timeline->currentFrame();
    const TimeLineKeysSet& keys = _imp->_gui->getTimelineGuiKeyframes();
    if (keys.size() == 0) {
        return;
    }
    for (TimeLineKeysSet::const_reverse_iterator it = keys.rbegin(); it != keys.rend(); ++it) {
        if (it->frame < currentFrame) {
            timeline->seekFrame(it->frame, true, EffectInstancePtr(), eTimelineChangeReasonPlaybackSeek);
            break;
        }
    }
}

void
GuiAppInstance::goToNextKeyframe()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    TimeLinePtr timeline = getProject()->getTimeLine();
    int currentFrame = timeline->currentFrame();
    const TimeLineKeysSet& keys = _imp->_gui->getTimelineGuiKeyframes();
    if (keys.size() == 0) {
        return;
    }
    for (TimeLineKeysSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        if (it->frame > currentFrame) {
            timeline->seekFrame(it->frame, true, EffectInstancePtr(), eTimelineChangeReasonPlaybackSeek);
            break;
        }
    }
}

void
GuiAppInstance::setKnobDnDData(QDrag* drag,
                               const KnobIPtr& knob,
                               DimSpec dimension,
                               ViewSetSpec view)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->knobDnd.source = knob;
    _imp->knobDnd.sourceDimension = dimension;
    _imp->knobDnd.sourceView = view;
    _imp->knobDnd.drag = drag;
}

void
GuiAppInstance::getKnobDnDData(QDrag** drag,
                               KnobIPtr* knob,
                               DimSpec* dimension,
                               ViewSetSpec* view) const
{
    assert( QThread::currentThread() == qApp->thread() );
    *knob = _imp->knobDnd.source.lock();
    *dimension = _imp->knobDnd.sourceDimension;
    *view = _imp->knobDnd.sourceView;
    *drag = _imp->knobDnd.drag;
}

bool
GuiAppInstance::checkAllReadersModificationDate(bool errorAndWarn)
{
    NodesList allNodes;
    TimeValue time = getProject()->getTimelineCurrentTime();

    getProject()->getNodes_recursive(allNodes);
    bool changed =  false;
    for (NodesList::iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        if ( !(*it)->getEffectInstance()->isReader() ) {
            continue;
        }
        KnobIPtr fileKnobI = (*it)->getKnobByName(kOfxImageEffectFileParamName);
        if (!fileKnobI) {
            continue;
        }

        NodeGuiPtr nodeUI = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());
        if (!nodeUI) {
            continue;
        }
        NodeSettingsPanel* panel = nodeUI->getSettingPanel();
        if (!panel) {
            continue;
        }
        KnobGuiPtr knobUi_i = panel->getKnobGui(fileKnobI);

        if (!knobUi_i) {
            continue;
        }
        boost::shared_ptr<KnobGuiFile> isFileKnob = boost::dynamic_pointer_cast<KnobGuiFile>(knobUi_i->getWidgetsForView(ViewIdx(0)));

        if (!isFileKnob) {
            continue;
        }
        changed |= isFileKnob->checkFileModificationAndWarn(time, errorAndWarn);

    }

    return changed;
}

void
GuiAppInstance::refreshAllTimeEvaluationParams(bool onlyTimeEvaluationKnobs)
{
    _imp->_gui->refreshAllTimeEvaluationParams(onlyTimeEvaluationKnobs);
}

void
GuiAppInstance::reloadScriptEditorFonts()
{
    _imp->_gui->getScriptEditor()->reloadFont();
}

void
GuiAppInstance::setMasterSyncViewer(const NodePtr& viewerNode)
{
    _imp->masterSyncViewer = viewerNode;
}

NodePtr
GuiAppInstance::getMasterSyncViewer() const
{
    return _imp->masterSyncViewer.lock();
}

void
GuiAppInstance::showRenderStatsWindow()
{
    getGui()->getOrCreateRenderStatsDialog()->show();
}

void
GuiAppInstance::setGuiFrozen(bool frozen)
{
    getGui()->onFreezeUIButtonClicked(frozen);
}

void
GuiAppInstance::onTabWidgetRegistered(TabWidgetI* tabWidget)
{
    getGui()->onPaneRegistered(tabWidget);
}

void
GuiAppInstance::onTabWidgetUnregistered(TabWidgetI* tabWidget)
{
    getGui()->onPaneUnRegistered(tabWidget);
}

void
GuiAppInstance::getHistogramScriptNames(std::list<std::string>* histograms) const
{
    const std::list<Histogram*>& histos = getGui()->getHistograms();
    for (std::list<Histogram*>::const_iterator it = histos.begin(); it!=histos.end(); ++it) {
        histograms->push_back((*it)->getScriptName());
    }
}

void
GuiAppInstance::getViewportsProjection(std::map<std::string,SERIALIZATION_NAMESPACE::ViewportData>* projections) const
{
    RegisteredTabs tabs = getGui()->getRegisteredTabs();
    for (RegisteredTabs::const_iterator it = tabs.begin(); it!=tabs.end(); ++it) {
        SERIALIZATION_NAMESPACE::ViewportData data;
        if (it->second.first->saveProjection(&data)) {
            (*projections)[it->first] = data;
        }
    }
}

void
GuiAppInstance::onNodeAboutToBeCreated(const NodePtr& node, const CreateNodeArgsPtr& args)
{
    NodeGraph* nodegraph = dynamic_cast<NodeGraph*>(node->getGroup()->getNodeGraph());
    if (nodegraph) {
        nodegraph->onNodeAboutToBeCreated(node, args);
    }
}

void
GuiAppInstance::onNodeCreated(const NodePtr& node, const CreateNodeArgsPtr& args)
{
    NodeGraph* nodegraph = dynamic_cast<NodeGraph*>(node->getGroup()->getNodeGraph());
    if (nodegraph) {
        nodegraph->onNodeCreated(node, args);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_GuiAppInstance.cpp"
