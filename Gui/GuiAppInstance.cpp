/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include <QtCore/QDir>
#include <QSettings>
#include <QtCore/QMutex>
#include <QtCore/QCoreApplication>

#include "Engine/CLArgs.h"
#include "Engine/Project.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Settings.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/KnobFile.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Global/QtCompat.h"
#include "Global/StrUtils.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/BackdropGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGuiFile.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/ProgressPanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/SplashScreen.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ViewerGL.h"

NATRON_NAMESPACE_ENTER

struct RotoPaintData
{
    NodePtr rotoPaintNode;
    RotoStrokeItemPtr stroke;
    bool isPainting;
    bool turboAlreadyActiveBeforePainting;

    ///The last mouse event tick bounding box, to render the least possible
    RectD lastStrokeMovementBbox;

    ///The index of the points/stroke we have rendered
    int lastStrokeIndex, multiStrokeIndex;

    ///The last points of the mouse event
    std::list<std::pair<Point, double> > lastStrokePoints;

    ///Used for the rendering algorithm to know where we stopped along the path
    double distToNextIn, distToNextOut;

    //The image used to render the currently drawn stroke mask
    ImagePtr strokeImage;

    RotoPaintData()
        : rotoPaintNode()
        , stroke()
        , isPainting(false)
        , turboAlreadyActiveBeforePainting(false)
        , lastStrokeMovementBbox()
        , lastStrokeIndex(-1)
        , multiStrokeIndex(0)
        , lastStrokePoints()
        , distToNextIn(0)
        , distToNextOut(0)
        , strokeImage()
    {
    }
};

struct KnobDnDData
{
    KnobIWPtr source;
    int sourceDimension;
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
    int overlayRedrawRequests;
    mutable QMutex rotoDataMutex;
    RotoPaintData rotoData;
    std::list<SequenceTime> timelineKeyframes, timelineUserKeys;
    KnobDnDData knobDnd;

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
        , overlayRedrawRequests(0)
        , rotoDataMutex()
        , rotoData()
        , timelineKeyframes()
        , timelineUserKeys()
        , knobDnd()
    {
        _previewProvider = boost::make_shared<FileDialogPreviewProvider>();
        rotoData.turboAlreadyActiveBeforePainting = false;
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
            _imp->_previewProvider->viewerNodeInternal->destroyNode(true, false);
            _imp->_previewProvider->viewerNodeInternal.reset();
        }

#ifndef NATRON_ENABLE_IO_META_NODES
        for (std::map<std::string, NodePtr>::iterator it =
                 _imp->_previewProvider->readerNodes.begin();
             it != _imp->_previewProvider->readerNodes.end(); ++it) {
            it->second->destroyNode(true, false);
        }
        _imp->_previewProvider->readerNodes.clear();
#else
        if (_imp->_previewProvider->readerNode) {
            _imp->_previewProvider->readerNode->destroyNode(true, false);
            _imp->_previewProvider->readerNode.reset();
        }
#endif

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

    GuiAppInstancePtr thisShared = boost::dynamic_pointer_cast<GuiAppInstance>( shared_from_this() );
    assert(thisShared);
    _imp->_gui = new Gui(thisShared);
    _imp->_gui->createGui();

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
    _imp->_gui->loaded = true;
    SettingsPtr nSettings = appPTR->getCurrentSettings();
    QObject::connect( getProject().get(), SIGNAL(formatChanged(Format)), this, SLOT(projectFormatChanged(Format)) );

    {
        QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
        if ( !settings.contains( QString::fromUtf8("checkForUpdates") ) ) {
            StandardButtonEnum reply = Dialogs::questionDialog(tr("Updates").toStdString(),
                                                               tr("Do you want %1 to check for updates "
                                                                  "on launch of the application?").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), false);
            bool checkForUpdates = reply == eStandardButtonYes;
            nSettings->setCheckUpdatesEnabled(checkForUpdates);
        }

        if ( nSettings->isCheckForUpdatesEnabled() ) {
            appPTR->setLoadingStatus( tr("Checking if updates are available...") );
            checkForNewVersion();
        }
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

        if ( !appPTR->isShorcutVersionUpToDate() ) {
            StandardButtonEnum reply = questionDialog(tr("Shortcuts").toStdString(),
                                                      tr("Default shortcuts for %1 have changed, "
                                                         "would you like to set them to their defaults?\n"
                                                         "Clicking no will keep the old shortcuts hence if a new shortcut has been "
                                                         "set to something else than an empty shortcut you will not benefit of it.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(),
                                                      false,
                                                      StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                                      eStandardButtonNo);
            if (reply == eStandardButtonYes) {
                appPTR->restoreDefaultShortcuts();
            }
        }
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
        if ( (suffixPos == -1) || entry.contains( QString::fromUtf8("RENDER_SAVE") ) ) {
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

void
GuiAppInstance::createNodeGui(const NodePtr &node,
                              const NodePtr& parentMultiInstance,
                              const CreateNodeArgs& args)
{
    NodeCollectionPtr group = node->getGroup();
    NodeGraph* graph;

    if (group) {
        NodeGraphI* graph_i = group->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
        assert(graph);
    } else {
        graph = _imp->_gui->getNodeGraph();
    }
    if (!graph) {
        throw std::logic_error("");
    }

    NodesGuiList selectedNodes = graph->getSelectedNodes();
    NodeGuiPtr nodegui = _imp->_gui->createNodeGUI(node, args);
    assert(nodegui);

    if (parentMultiInstance && nodegui) {
        nodegui->hideGui();


        NodeGuiIPtr parentNodeGui_i = parentMultiInstance->getNodeGui();
        assert(parentNodeGui_i);
        nodegui->setParentMultiInstance( boost::dynamic_pointer_cast<NodeGui>(parentNodeGui_i) );
    }

    bool isViewer = node->isEffectViewer() != 0;
    if (isViewer) {
        _imp->_gui->createViewerGui(node);
    }

    // Must be done after the viewer gui has been created
    _imp->_gui->createNodeViewerInterface(nodegui);


    NodeGroup* isGroup = node->isEffectGroup();
    if ( isGroup && isGroup->isSubGraphUserVisible() ) {
        _imp->_gui->createGroupGui(node, args);
    }

    ///Don't initialize inputs if it is a multi-instance child since it is not part of  the graph
    if (!parentMultiInstance) {
        nodegui->initializeInputs();
    }
    
    NodeSerializationPtr serialization = args.getProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    if ( !serialization && !isViewer ) {
        ///we make sure we can have a clean preview.
        node->computePreviewImage( getTimeLine()->currentFrame() );
        triggerAutoSave();
    }


    ///only move main instances
    if ( node->getParentMultiInstanceName().empty() && !serialization) {
        bool autoConnect = args.getProperty<bool>(kCreateNodeArgsPropAutoConnect);

        if ( selectedNodes.empty() || serialization) {
            autoConnect = false;
        }
        double xPosHint = serialization ? INT_MIN : args.getProperty<double>(kCreateNodeArgsPropNodeInitialPosition, 0);
        double yPosHint = serialization ? INT_MIN : args.getProperty<double>(kCreateNodeArgsPropNodeInitialPosition, 1);
        if ( (xPosHint != INT_MIN) && (yPosHint != INT_MIN) && (!autoConnect) ) {
            QPointF pos = nodegui->mapToParent( nodegui->mapFromScene( QPointF(xPosHint, yPosHint) ) );
            nodegui->refreshPosition( pos.x(), pos.y(), true );
        } else {
            BackdropGui* isBd = dynamic_cast<BackdropGui*>( nodegui.get() );
            if (!isBd) {
                NodeGuiPtr selectedNode;
                if ( !serialization && (selectedNodes.size() == 1) ) {
                    selectedNode = selectedNodes.front();
                    BackdropGui* isBackdropGui = dynamic_cast<BackdropGui*>( selectedNode.get() );
                    if (isBackdropGui) {
                        selectedNode.reset();
                    }
                }
                nodegui->getDagGui()->moveNodesForIdealPosition(nodegui, selectedNode, autoConnect);
            }
        }
    }
} // createNodeGui

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
GuiAppInstance::loadProjectGui(bool isAutosave, boost::archive::xml_iarchive & archive) const
{
    _imp->_gui->loadProjectGui(isAutosave, archive);
}

void
GuiAppInstance::saveProjectGui(boost::archive::xml_oarchive & archive)
{
    if (_imp->_gui) {
        _imp->_gui->saveProjectGui(archive);
    }
}

void
GuiAppInstance::setupViewersForViews(const std::vector<std::string>& viewNames)
{
    _imp->_gui->updateViewersViewsMenu(viewNames);
}

void
GuiAppInstance::setViewersCurrentView(ViewIdx view)
{
    _imp->_gui->setViewersCurrentView(view);
}

void
GuiAppInstance::notifyRenderStarted(const QString & sequenceName,
                                    int firstFrame,
                                    int lastFrame,
                                    int frameStep,
                                    bool canPause,
                                    OutputEffectInstance* writer,
                                    const ProcessHandlerPtr & process)
{
    _imp->_gui->onRenderStarted(sequenceName, firstFrame, lastFrame, frameStep, canPause, writer, process);
}

void
GuiAppInstance::notifyRenderRestarted( OutputEffectInstance* writer,
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

void
GuiAppInstance::connectViewersToViewerCache()
{
    if (_imp->_gui) {
        _imp->_gui->connectViewersToViewerCache();
    }
}

void
GuiAppInstance::disconnectViewersFromViewerCache()
{
    if (_imp->_gui) {
        _imp->_gui->disconnectViewersFromViewerCache();
    }
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
        _imp->_previewProvider->viewerUI->getInternalNode()->renderCurrentFrame(true);
    }
}

bool
GuiAppInstance::isGuiFrozen() const
{
    return _imp->_gui ? _imp->_gui->isGUIFrozen() : false;
}

void
GuiAppInstance::clearViewersLastRenderedTexture()
{
    std::list<ViewerTab*> tabs = _imp->_gui->getViewersList_mt_safe();

    for (std::list<ViewerTab*>::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
        (*it)->getViewer()->clearLastRenderedTexture();
    }
}

void
GuiAppInstance::redrawAllViewers()
{
    _imp->_gui->redrawAllViewers();
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
    if ( node->isEffectViewer() ) {
        QMutexLocker k(&_imp->lastTimelineViewerMutex);
        _imp->lastTimelineViewer = node;
    }
}

ViewerInstance*
GuiAppInstance::getLastViewerUsingTimeline() const
{
    QMutexLocker k(&_imp->lastTimelineViewerMutex);

    if (!_imp->lastTimelineViewer) {
        return 0;
    }

    return _imp->lastTimelineViewer->isEffectViewer();
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
GuiAppInstance::renderAllViewers(bool canAbort)
{
    _imp->_gui->renderAllViewers(canAbort);
}

void
GuiAppInstance::refreshAllPreviews()
{
    getProject()->refreshPreviews();
}

void
GuiAppInstance::abortAllViewers()
{
    _imp->_gui->abortAllViewers();
}

void
GuiAppInstance::reloadStylesheet()
{
    if (_imp->_gui) {
        _imp->_gui->reloadStylesheet();
    }
}

void
GuiAppInstance::queueRedrawForAllViewers()
{
    assert( QThread::currentThread() == qApp->thread() );
    ++_imp->overlayRedrawRequests;
}

int
GuiAppInstance::getOverlayRedrawRequestsCount() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->overlayRedrawRequests;
}

void
GuiAppInstance::clearOverlayRedrawRequests()
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->overlayRedrawRequests = 0;
}

void
GuiAppInstance::onGroupCreationFinished(const NodePtr& node,
                                        const NodeSerializationPtr& serialization, bool autoConnect)
{
    NodeGuiIPtr node_gui_i = node->getNodeGui();
    if (autoConnect && !serialization && node_gui_i) {
        NodeGraph* graph = 0;
        NodeCollectionPtr collection = node->getGroup();
        assert(collection);
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>( collection.get() );
        if (isGrp) {
            NodeGraphI* graph_i = isGrp->getNodeGraph();
            assert(graph_i);
            graph = dynamic_cast<NodeGraph*>(graph_i);
        } else {
            graph = _imp->_gui->getNodeGraph();
        }
        assert(graph);
        if (!graph) {
            throw std::logic_error("");
        }
        NodesGuiList selectedNodes = graph->getSelectedNodes();
        NodeGuiPtr selectedNode;
        if ( !selectedNodes.empty() ) {
            selectedNode = selectedNodes.front();
            if ( dynamic_cast<BackdropGui*>( selectedNode.get() ) ) {
                selectedNode.reset();
            }
        }
        NodeGuiPtr nodeGui = boost::dynamic_pointer_cast<NodeGui>(node_gui_i);
        graph->moveNodesForIdealPosition(nodeGui, selectedNode, true);
    }
 
    AppInstance::onGroupCreationFinished(node, serialization, autoConnect);

    /*std::list<ViewerInstance* > viewers;
       node->hasViewersConnected(&viewers);
       for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
        (*it2)->renderCurrentFrame(false);
       }*/
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

void
GuiAppInstance::setUserIsPainting(const NodePtr& rotopaintNode,
                                  const RotoStrokeItemPtr& stroke,
                                  bool isPainting)
{
    {
        QMutexLocker k(&_imp->rotoDataMutex);
        bool newStroke = stroke != _imp->rotoData.stroke;
        if ( isPainting && ( (rotopaintNode != _imp->rotoData.rotoPaintNode) || newStroke ) ) {
            _imp->rotoData.strokeImage.reset();
        }

        _imp->rotoData.isPainting = isPainting;
        if (isPainting) {
            _imp->rotoData.rotoPaintNode = rotopaintNode;
            _imp->rotoData.stroke = stroke;
        }

        //Reset the index if the stroke is different
        if (newStroke) {
            _imp->rotoData.lastStrokeIndex = -1;
            _imp->rotoData.multiStrokeIndex = 0;
        }

        if (rotopaintNode) {
            _imp->rotoData.turboAlreadyActiveBeforePainting = _imp->_gui->isGUIFrozen();
        }
    }
}

void
GuiAppInstance::getActiveRotoDrawingStroke(NodePtr* node,
                                           RotoStrokeItemPtr* stroke,
                                           bool *isPainting) const
{
    QMutexLocker k(&_imp->rotoDataMutex);

    *node = _imp->rotoData.rotoPaintNode;
    *stroke = _imp->rotoData.stroke;
    *isPainting = _imp->rotoData.isPainting;
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
GuiAppInstance::updateLastPaintStrokeData(int newAge,
                                          const std::list<std::pair<Point, double> >& points,
                                          const RectD& lastPointsBbox,
                                          int strokeIndex)
{
    {
        QMutexLocker k(&_imp->rotoDataMutex);
        _imp->rotoData.lastStrokePoints = points;
        _imp->rotoData.lastStrokeMovementBbox = lastPointsBbox;
        _imp->rotoData.lastStrokeIndex = newAge;
        _imp->rotoData.distToNextIn = _imp->rotoData.distToNextOut;
        _imp->rotoData.multiStrokeIndex = strokeIndex;
    }
}

void
GuiAppInstance::getLastPaintStrokePoints(std::list<std::list<std::pair<Point, double> > >* strokes,
                                         int* strokeIndex) const
{
    QMutexLocker k(&_imp->rotoDataMutex);

    strokes->push_back(_imp->rotoData.lastStrokePoints);
    *strokeIndex = _imp->rotoData.multiStrokeIndex;
}

int
GuiAppInstance::getStrokeLastIndex() const
{
    QMutexLocker k(&_imp->rotoDataMutex);

    return _imp->rotoData.lastStrokeIndex;
}

void
GuiAppInstance::getStrokeAndMultiStrokeIndex(RotoStrokeItemPtr* stroke,
                                             int* strokeIndex) const
{
    QMutexLocker k(&_imp->rotoDataMutex);

    *stroke = _imp->rotoData.stroke;
    *strokeIndex = _imp->rotoData.multiStrokeIndex;
}

void
GuiAppInstance::getRenderStrokeData(RectD* lastStrokeMovementBbox,
                                    std::list<std::pair<Point, double> >* lastStrokeMovementPoints,
                                    double *distNextIn,
                                    ImagePtr* strokeImage) const
{
    QMutexLocker k(&_imp->rotoDataMutex);

    *lastStrokeMovementBbox = _imp->rotoData.lastStrokeMovementBbox;
    *lastStrokeMovementPoints = _imp->rotoData.lastStrokePoints;
    *distNextIn = _imp->rotoData.distToNextIn;
    *strokeImage = _imp->rotoData.strokeImage;
}

void
GuiAppInstance::updateStrokeImage(const ImagePtr& image,
                                  double distNextOut,
                                  bool setDistNextOut)
{
    QMutexLocker k(&_imp->rotoDataMutex);

    _imp->rotoData.strokeImage = image;
    if (setDistNextOut) {
        _imp->rotoData.distToNextOut = distNextOut;
    }
}

RectD
GuiAppInstance::getLastPaintStrokeBbox() const
{
    QMutexLocker k(&_imp->rotoDataMutex);

    return _imp->rotoData.lastStrokeMovementBbox;
}

RectD
GuiAppInstance::getPaintStrokeWholeBbox() const
{
    QMutexLocker k(&_imp->rotoDataMutex);

    if (!_imp->rotoData.stroke) {
        return RectD();
    }

    return _imp->rotoData.stroke->getWholeStrokeRoDWhilePainting();
}

void
GuiAppInstance::removeAllKeyframesIndicators()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    bool wasEmpty = _imp->timelineKeyframes.empty();
    _imp->timelineKeyframes.clear();
    if (!wasEmpty) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::addKeyframeIndicator(SequenceTime time)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _imp->timelineKeyframes.push_back(time);
    Q_EMIT keyframeIndicatorsChanged();
}

void
GuiAppInstance::addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & keys,
                                                   bool emitSignal)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _imp->timelineKeyframes.insert( _imp->timelineKeyframes.begin(), keys.begin(), keys.end() );
    if (!keys.empty() && emitSignal) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::removeKeyFrameIndicator(SequenceTime time)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<SequenceTime>::iterator it = std::find(_imp->timelineKeyframes.begin(), _imp->timelineKeyframes.end(), time);
    if ( it != _imp->timelineKeyframes.end() ) {
        _imp->timelineKeyframes.erase(it);
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::removeMultipleKeyframeIndicator(const std::list<SequenceTime> & keys,
                                                bool emitSignal)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    for (std::list<SequenceTime>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        std::list<SequenceTime>::iterator it2 = std::find(_imp->timelineKeyframes.begin(), _imp->timelineKeyframes.end(), *it);
        if ( it2 != _imp->timelineKeyframes.end() ) {
            _imp->timelineKeyframes.erase(it2);
        }
    }
    if (!keys.empty() && emitSignal) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::removeAllUserKeyframesIndicators()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    bool wasEmpty = _imp->timelineUserKeys.empty();
    _imp->timelineUserKeys.clear();
    if (!wasEmpty) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::addUserKeyframeIndicator(SequenceTime time)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _imp->timelineUserKeys.push_back(time);
    Q_EMIT keyframeIndicatorsChanged();
}

void
GuiAppInstance::addUserMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & keys,
                                                       bool emitSignal)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _imp->timelineUserKeys.insert( _imp->timelineUserKeys.begin(), keys.begin(), keys.end() );
    if (!keys.empty() && emitSignal) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::removeUserKeyFrameIndicator(SequenceTime time)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<SequenceTime>::iterator it = std::find(_imp->timelineUserKeys.begin(), _imp->timelineUserKeys.end(), time);
    if ( it != _imp->timelineUserKeys.end() ) {
        _imp->timelineUserKeys.erase(it);
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::removeUserMultipleKeyframeIndicator(const std::list<SequenceTime> & keys,
                                                    bool emitSignal)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    for (std::list<SequenceTime>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        std::list<SequenceTime>::iterator it2 = std::find(_imp->timelineUserKeys.begin(), _imp->timelineUserKeys.end(), *it);
        if ( it2 != _imp->timelineUserKeys.end() ) {
            _imp->timelineUserKeys.erase(it2);
        }
    }
    if (!keys.empty() && emitSignal) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
GuiAppInstance::getUserKeyframes(std::list<SequenceTime>* keys) const
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    *keys = _imp->timelineUserKeys;
}

void
GuiAppInstance::addNodesKeyframesToTimeline(const std::list<Node*> & nodes)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<Node*>::const_iterator next = nodes.begin();
    if ( next != nodes.end() ) {
        ++next;
    }
    for (std::list<Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->showKeyframesOnTimeline( next == nodes.end() );

        // increment for next iteration
        if ( next != nodes.end() ) {
            ++next;
        }
    } // for()
}

void
GuiAppInstance::addNodeKeyframesToTimeline(Node* node)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    node->showKeyframesOnTimeline(true);
}

void
GuiAppInstance::removeNodesKeyframesFromTimeline(const std::list<Node*> & nodes)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<Node*>::const_iterator next = nodes.begin();
    if ( next != nodes.end() ) {
        ++next;
    }
    for (std::list<Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->hideKeyframesFromTimeline( next == nodes.end() );

        // increment for next iteration
        if ( next != nodes.end() ) {
            ++next;
        }
    } // for(it)
}

void
GuiAppInstance::removeNodeKeyframesFromTimeline(Node* node)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    node->hideKeyframesFromTimeline(true);
}

void
GuiAppInstance::getKeyframes(std::list<SequenceTime>* keys) const
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    *keys = _imp->timelineKeyframes;
}

void
GuiAppInstance::goToPreviousKeyframe()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _imp->timelineKeyframes.sort();
    TimeLinePtr timeline = getProject()->getTimeLine();
    SequenceTime currentFrame = timeline->currentFrame();
    std::list<SequenceTime>::iterator lowerBound = std::lower_bound(_imp->timelineKeyframes.begin(), _imp->timelineKeyframes.end(), currentFrame);
    if ( lowerBound != _imp->timelineKeyframes.begin() ) {
        --lowerBound;
        timeline->seekFrame(*lowerBound, true, NULL, eTimelineChangeReasonPlaybackSeek);
    }
}

void
GuiAppInstance::goToNextKeyframe()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _imp->timelineKeyframes.sort();
    TimeLinePtr timeline = getProject()->getTimeLine();
    SequenceTime currentFrame = timeline->currentFrame();
    std::list<SequenceTime>::iterator upperBound = std::upper_bound(_imp->timelineKeyframes.begin(), _imp->timelineKeyframes.end(), currentFrame);
    if ( upperBound != _imp->timelineKeyframes.end() ) {
        timeline->seekFrame(*upperBound, true, NULL, eTimelineChangeReasonPlaybackSeek);
    }
}

void
GuiAppInstance::setKnobDnDData(QDrag* drag,
                               const KnobIPtr& knob,
                               int dimension)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->knobDnd.source = knob;
    _imp->knobDnd.sourceDimension = dimension;
    _imp->knobDnd.drag = drag;
}

void
GuiAppInstance::getKnobDnDData(QDrag** drag,
                               KnobIPtr* knob,
                               int* dimension) const
{
    assert( QThread::currentThread() == qApp->thread() );
    *knob = _imp->knobDnd.source.lock();
    *dimension = _imp->knobDnd.sourceDimension;
    *drag = _imp->knobDnd.drag;
}

bool
GuiAppInstance::checkAllReadersModificationDate(bool errorAndWarn)
{
    NodesList allNodes;
    SequenceTime time = getProject()->getCurrentTime();

    getProject()->getNodes_recursive(allNodes, true);
    bool changed =  false;
    for (NodesList::iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        if ( (*it)->getEffectInstance()->isReader() ) {
            KnobIPtr fileKnobI = (*it)->getKnobByName(kOfxImageEffectFileParamName);
            if (!fileKnobI) {
                continue;
            }
            KnobGuiIPtr knobUi_i = fileKnobI->getKnobGuiPointer();
            KnobGuiFile* isFileKnob = dynamic_cast<KnobGuiFile*>( knobUi_i.get() );

            if (!isFileKnob) {
                continue;
            }
            changed |= isFileKnob->checkFileModificationAndWarn(time, errorAndWarn);
        }
    }

    return changed;
}

void
GuiAppInstance::reloadScriptEditorFonts()
{
    _imp->_gui->getScriptEditor()->reloadFont();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_GuiAppInstance.cpp"
