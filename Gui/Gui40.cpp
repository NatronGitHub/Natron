/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "Gui.h"

#include <cassert>
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>

#if QT_VERSION >= 0x050000
#include <QtGui/QScreen>
#endif

#include <QAction>
#include <QApplication> // qApp


#include "Engine/CLArgs.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Image.h"
#include "Engine/Lut.h" // floatToInt, LutManager
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ProcessHandler.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/RenderQueue.h"
#include "Engine/RenderEngine.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/AboutWindow.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/ProjectGui.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/ProgressPanel.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeSettingsPanel.h"


NATRON_NAMESPACE_ENTER


void
Gui::refreshAllPreviews()
{
    getApp()->getProject()->refreshPreviews();
}

void
Gui::forceRefreshAllPreviews()
{
    getApp()->getProject()->forceRefreshPreviews();
}

void
Gui::startDragPanel(PanelWidget* panel)
{
    assert(!_imp->_currentlyDraggedPanel);
    _imp->_currentlyDraggedPanel = panel;
    if (panel) {
        _imp->_currentlyDraggedPanelInitialSize = panel->getWidget()->size();
    }
}

PanelWidget*
Gui::stopDragPanel(QSize* initialSize)
{
    assert(_imp->_currentlyDraggedPanel);
    PanelWidget* ret = _imp->_currentlyDraggedPanel;
    _imp->_currentlyDraggedPanel = 0;
    *initialSize = _imp->_currentlyDraggedPanelInitialSize;

    return ret;
}

void
Gui::showAbout()
{
    _imp->_aboutWindow->show();
    _imp->_aboutWindow->raise();
    _imp->_aboutWindow->activateWindow();
    ignore_result( _imp->_aboutWindow->exec() );
}

void
Gui::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        QFileInfo f( action->data().toString() );
        QString path = f.path() + QLatin1Char('/');
        QString filename = path + f.fileName();
        int openedProject = appPTR->isProjectAlreadyOpened( filename.toStdString() );
        if (openedProject != -1) {
            AppInstancePtr instance = appPTR->getAppInstance(openedProject);
            if (instance) {
                GuiAppInstancePtr guiApp = toGuiAppInstance(instance);
                assert(guiApp);
                if (guiApp) {
                    guiApp->getGui()->activateWindow();

                    return;
                }
            }
        }

        ///if the current graph has no value, just load the project in the same window
        if ( getApp()->getProject()->isGraphWorthLess() ) {
            getApp()->getProject()->loadProject( path, f.fileName() );
        } else {
            CLArgs cl;
            AppInstancePtr newApp = appPTR->newAppInstance(cl, false);
            newApp->getProject()->loadProject( path, f.fileName() );
        }
    }
}

void
Gui::updateRecentFileActions()
{
    // if there are two files with the same filename, give the dirname too
    QStringList files;
    QStringList fileNames;
    QStringList dirNames;
    std::map<QString,QStringList> allDirNames;

    {
        QSettings settings;
        QStringList allfiles = settings.value( QString::fromUtf8("recentFileList") ).toStringList();
        int numFiles = allfiles.size();
        int iTotal = 0;

        for (int i = 0; i < numFiles && iTotal < NATRON_MAX_RECENT_FILES; ++i) {
            QFileInfo fi(allfiles[i]);
            if ( fi.exists() ) {
                files.push_back(allfiles[i]);
                fileNames.push_back(fi.fileName());
                QString dirName = fi.dir().canonicalPath();
                dirNames.push_back(dirName);
                allDirNames[fi.fileName()] << dirName;
                ++iTotal;
            }
        }
    }

    assert(files.size() <= (int)NATRON_MAX_RECENT_FILES);
    assert(files.size() == fileNames.size());
    assert(dirNames.size() == fileNames.size());
    int numRecentFiles = std::min(fileNames.size(), (int)NATRON_MAX_RECENT_FILES);

    // the dirname can be the same too. for each fileName with count > 1, collect the indices of the identical filenames. if dirname and directory up-level is the same for at least two files, raise the directory level up until the two dirnames are different
    for (std::map<QString,QStringList>::const_iterator it = allDirNames.begin(); it != allDirNames.end(); ++it) {
        // dirs contains the list of dirs for that dirname
        const QStringList& dirs = it->second;
        if (dirs.size() > 1) {
            // split each dir, and find the common part of all dirs.
            std::vector<QStringList> dirParts(dirs.size());
            for (int i = 0; i < dirs.size(); ++i) {
                dirParts[i] = QDir::toNativeSeparators(dirs.at(i)).split(QDir::separator(), QString::SkipEmptyParts);
            }
            int minComps = dirParts[0].size();
            for (int i = 1; i < dirs.size(); ++i) {
                minComps = std::min(minComps, dirParts[i].size());
            }
            // count the number of elements to remove
            int commonComps = 0;
            for (int i = 0; i < minComps; ++i) {
                bool compIsCommon = true;
                for (int j = 1; j < dirs.size(); ++j) {
                    if (dirParts[j].at(i) != dirParts[0].at(i)) {
                        compIsCommon = false;
                        break;
                    }
                }
                if (compIsCommon) {
                    commonComps = i + 1;
                } else {
                    break;
                }
            }
            if (commonComps > 0) {
            // remove the n first element to each dirName corresponding to this filename, and recompose
                for (int i = 0; i < numRecentFiles; ++i) {
                    if (fileNames[i] == it->first) {
                        dirNames[i] = QStringList(QDir::toNativeSeparators(dirNames.at(i)).split(QDir::separator(), QString::SkipEmptyParts).mid(commonComps)).join(QDir::separator());
                    }
                }
            }
        }
    }

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text;
        if (allDirNames[fileNames[i]].size() > 1) {
            text = QString::fromUtf8("%1 - %2").arg(fileNames[i]).arg(dirNames[i]);
        } else {
            text = fileNames[i];
        }
        _imp->actionsOpenRecentFile[i]->setText(text);
        _imp->actionsOpenRecentFile[i]->setData(files[i]);
        _imp->actionsOpenRecentFile[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < NATRON_MAX_RECENT_FILES; ++j) {
        _imp->actionsOpenRecentFile[j]->setVisible(false);
    }
}

QPixmap
Gui::screenShot(QWidget* w)
{
#if QT_VERSION < 0x050000
    if ( w->objectName() == QString::fromUtf8("CurveEditor") ) {
        return QPixmap::grabWidget(w);
    }

    return QPixmap::grabWindow( w->winId() );
#else

    return QApplication::primaryScreen()->grabWindow( w->winId() );
#endif
}

void
Gui::onProjectNameChanged(const QString & filePath,
                          bool modified)
{
    // handles window title and appearance formatting
    // http://doc.qt.io/qt-4.8/qwidget.html#windowModified-prop
    setWindowModified(modified);
    // http://doc.qt.io/qt-4.8/qwidget.html#windowFilePath-prop
    setWindowFilePath(filePath.isEmpty() ? QString::fromUtf8(NATRON_PROJECT_UNTITLED) : filePath);
}

void
Gui::setColorPickersColor(ViewIdx view,
                          double r,
                          double g,
                          double b,
                          double a)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(view, r, g, b, a);
}

void
Gui::registerNewColorPicker(KnobColorPtr knob, ViewIdx view)
{
    assert(_imp->_projectGui);
    const std::list<ViewerTab*> &viewers = getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        ViewerNodePtr internalViewerNode = (*it)->getInternalNode();
        if (!internalViewerNode) {
            continue;
        }
        internalViewerNode->setPickerEnabled(true);
    }
    _imp->_projectGui->registerNewColorPicker(knob, view);
}

void
Gui::removeColorPicker(KnobColorPtr knob, ViewIdx view)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->removeColorPicker(knob, view);
}

void
Gui::clearColorPickers()
{
    assert(_imp->_projectGui);
    _imp->_projectGui->clearColorPickers();
}

bool
Gui::hasPickers() const
{
    assert(_imp->_projectGui);

    return _imp->_projectGui->hasPickers();
}

void
Gui::setViewersCurrentView(ViewIdx view)
{
    std::list<ViewerTab*> viewers;
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        viewers = _imp->_viewerTabs;
    }

    for (std::list<ViewerTab*>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        ViewerNodePtr internalViewerNode = (*it)->getInternalNode();
        if (!internalViewerNode) {
            continue;
        }
        internalViewerNode->setCurrentView(view);
    }
}

const std::list<ViewerTab*> &
Gui::getViewersList() const
{
    return _imp->_viewerTabs;
}

std::list<ViewerTab*>
Gui::getViewersList_mt_safe() const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    return _imp->_viewerTabs;
}


void
Gui::activateViewerTab(const ViewerNodePtr& viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                TabWidget* viewerAnchor = getAnchor();
                assert(viewerAnchor);
                viewerAnchor->appendTab(*it, *it);
                (*it)->show();
            }
        }
    }
    Q_EMIT viewersChanged();
}

void
Gui::deactivateViewerTab(const ViewerNodePtr& viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();
    ViewerTab* v = 0;
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                v = *it;
                break;
            }
        }

        if ( v && (viewer->getNode() == getApp()->getMasterSyncViewer()) ) {
            getApp()->setMasterSyncViewer(NodePtr());
        }
    }

    if (v) {
        removeViewerTab(v, true, false);
    }
}

ViewerTab*
Gui::getViewerTabForInstance(const ViewerNodePtr& node) const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->getInternalNode() == node ) {
            return *it;
        }
    }

    return NULL;
}

const NodesGuiList &
Gui::getVisibleNodes() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes();
}

NodesGuiList
Gui::getVisibleNodes_mt_safe() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes_mt_safe();
}

void
Gui::deselectAllNodes() const
{
    _imp->_nodeGraphArea->deselect();
}

void
Gui::setNextViewerAnchor(TabWidget* where)
{
    _imp->_nextViewerTabPlace = where;
}

const std::vector<ToolButton*> &
Gui::getToolButtons() const
{
    return _imp->_toolButtons;
}

GuiAppInstancePtr
Gui::getApp() const
{
    return _imp->_appInstance.lock();
}


void
Gui::setDraftRenderEnabled(bool b)
{
    {
        QMutexLocker k(&_imp->_isInDraftModeMutex);

        _imp->_isInDraftMode = b;
    }
    if (!b) {
        refreshAllTimeEvaluationParams(false);
    }
}

bool
Gui::isDraftRenderEnabled() const
{
    QMutexLocker k(&_imp->_isInDraftModeMutex);

    return _imp->_isInDraftMode;
}

bool
Gui::isDraggingPanel() const
{
    return _imp->_currentlyDraggedPanel != NULL;
}

NodeGraph*
Gui::getNodeGraph() const
{
    return _imp->_nodeGraphArea;
}


AnimationModuleEditor *
Gui::getAnimationModuleEditor() const
{
    return _imp->_animationModule;
}

ScriptEditor*
Gui::getScriptEditor() const
{
    return _imp->_scriptEditor;
}

ProgressPanel*
Gui::getProgressPanel() const
{
    return _imp->_progressPanel;
}

PropertiesBinWrapper*
Gui::getPropertiesBin() const
{
    return _imp->_propertiesBin;
}

QVBoxLayout*
Gui::getPropertiesLayout() const
{
    return _imp->_layoutPropertiesBin;
}

void
Gui::appendTabToDefaultViewerPane(PanelWidget* tab,
                                  ScriptObject* obj)
{
    TabWidget* viewerAnchor = getAnchor();

    assert(viewerAnchor);
    viewerAnchor->appendTab(tab, obj);
}

QWidget*
Gui::getCentralWidget() const
{
    std::list<QWidget*> children;

    _imp->_leftRightSplitter->getChildren_mt_safe(children);
    if (children.size() != 2) {
        ///something is wrong
        return NULL;
    }
    for (std::list<QWidget*>::iterator it = children.begin(); it != children.end(); ++it) {
        if (*it == _imp->_toolBox) {
            continue;
        }

        return *it;
    }

    return NULL;
}

const RegisteredTabs &
Gui::getRegisteredTabs() const
{
    return _imp->_registeredTabs;
}

template <typename PIX>
void debugImageInternal(const RectI& renderWindow,
                        const Image::CPUData &imageData,
                        QImage& output)
{
    const Color::Lut* lut = Color::LutManager::sRGBLut();
    lut->validate();


    int dstY = 0;
    for ( int y = renderWindow.y2 - 1; y >= renderWindow.y1; --y , ++dstY) {

        const PIX* src_pixels[4] = {NULL, NULL, NULL, NULL};
        int pixelStride;
        Image::getChannelPointers<PIX>((const PIX**)imageData.ptrs, renderWindow.x1, y, imageData.bounds, imageData.nComps, (PIX**)src_pixels, &pixelStride);

        QRgb* dstPixels = (QRgb*)output.scanLine(dstY);
        assert(dstPixels);

        unsigned error[3] = {0x80, 0x80, 0x80};

        for (int x = renderWindow.x1; x < renderWindow.x2; ++x, ++dstPixels) {
            float tmpPix[4] = {0, 0, 0, 1};
            switch (imageData.nComps) {
                case 1: {
                    tmpPix[0] = tmpPix[1] = tmpPix[2] = Image::convertPixelDepth<PIX, float>(*src_pixels[0]);
                    src_pixels[0] += pixelStride;

                }   break;
                case 2:
                case 3:
                case 4: {
                    for (int i = 0; i < imageData.nComps; ++i) {
                        tmpPix[i] = Image::convertPixelDepth<PIX, float>(*src_pixels[i]);
                        src_pixels[i] += pixelStride;
                    }
                }   break;

                default:
                    assert(false);

                    return;
            }

            for (int i = 0; i < 3; ++i) {
                error[i] = (error[i] & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(tmpPix[i]);
                assert(error[i] < 0x10000);
            }
            unsigned char alpha = Image::convertPixelDepth<float, unsigned char>(tmpPix[3]);
            *dstPixels = qRgba( U8(error[0] >> 8),
                               U8(error[1] >> 8),
                               U8(error[2] >> 8),
                               U8(alpha) );


        }
    }

} // debugImageInternal

void
Gui::debugImage(const ImagePtr& image,
                const RectI& roi,
                const QString & filename )
{

    RectI renderWindow;
    RectI bounds = image->getBounds();
    if ( roi.isNull() ) {
        renderWindow = bounds;
    } else {
        if ( !roi.intersect(bounds, &renderWindow) ) {
            qDebug() << "The RoI does not interesect the bounds of the image.";

            return;
        }
    }

    ImagePtr imageToWrite = image;

    if (image->getStorageMode() != eStorageModeRAM) {
        Image::InitStorageArgs initArgs;
        {

            initArgs.bounds = image->getBounds();
            initArgs.plane = image->getLayer();
            initArgs.bitdepth = image->getBitDepth();
            initArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
            initArgs.storage = eStorageModeRAM;
        }
        ImagePtr tmpImage = Image::create(initArgs);
        if (!tmpImage) {
            qDebug() << "debugImage: failed to create temporary image";
            return;
        }
        Image::CopyPixelsArgs cpyArgs;
        cpyArgs.roi = initArgs.bounds;
        ActionRetCodeEnum stat = tmpImage->copyPixels(*image, cpyArgs);
        if (isFailureRetCode(stat)) {
            qDebug() << "debugImage: failed to copy pixels on temporary image";
            return;
        }
        imageToWrite = tmpImage;
    }

    Image::CPUData imageData;
    imageToWrite->getCPUData(&imageData);
 

    QImage output(renderWindow.width(), renderWindow.height(), QImage::Format_ARGB32);
    switch (imageToWrite->getBitDepth()) {
        case eImageBitDepthByte:
            debugImageInternal<unsigned char>(roi, imageData, output);
            break;
        case eImageBitDepthFloat:
            debugImageInternal<float>(roi, imageData, output);
            break;
        case eImageBitDepthShort:
            debugImageInternal<unsigned short>(roi, imageData, output);
            break;
        default:
            return;
    }
    U64 hashKey = rand();
    QString hashKeyStr = QString::number(hashKey);
    QString realFileName = filename.isEmpty() ? QString( hashKeyStr + QString::fromUtf8(".png") ) : filename;
#ifdef DEBUG
    qDebug() << "Writing image: " << realFileName;
    renderWindow.debug();
#endif
    output.save(realFileName);
} // Gui::debugImage

void
Gui::updateLastSequenceOpenedPath(const QString & path)
{
    _imp->_lastLoadSequenceOpenedDir = path;
}

void
Gui::updateLastSequenceSavedPath(const QString & path)
{
    _imp->_lastSaveSequenceOpenedDir = path;
}

void
Gui::updateLastSavedProjectPath(const QString & project)
{
    _imp->_lastSaveProjectOpenedDir = project;
}

void
Gui::updateLastOpenedProjectPath(const QString & project)
{
    _imp->_lastLoadProjectOpenedDir = project;
}

void
Gui::onRenderStarted(const QString & sequenceName,
                     TimeValue firstFrame,
                     TimeValue lastFrame,
                     TimeValue frameStep,
                     bool canPause,
                     const NodePtr& writer,
                     const ProcessHandlerPtr & process)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->_progressPanel->startTask(writer, firstFrame, lastFrame, frameStep, canPause, true, sequenceName, process);
}

void
Gui::onRenderRestarted(const NodePtr& writer,
                       const ProcessHandlerPtr & process)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->_progressPanel->onTaskRestarted(writer, process);
}

void
Gui::ensureScriptEditorVisible()
{
    // Ensure that the script editor is visible
    TabWidget* pane = _imp->_scriptEditor->getParentPane();

    if (pane != 0) {
        pane->setCurrentWidget(_imp->_scriptEditor);
    } else {
        pane = _imp->_nodeGraphArea->getParentPane();
        if (!pane) {
            std::list<TabWidgetI*> tabs = getApp()->getTabWidgetsSerialization();
            if ( tabs.empty() ) {
                return;
            }
            pane = dynamic_cast<TabWidget*>(tabs.front());
        }
        assert(pane);
        if (pane) {
            pane->moveScriptEditorHere();
        }
    }
}

PanelWidget*
Gui::ensureProgressPanelVisible()
{
    TabWidget* pane = _imp->_progressPanel->getParentPane();

    if (pane != 0) {
        PanelWidget* ret = pane->currentWidget();
        pane->setCurrentWidget(_imp->_progressPanel);
        return ret;
    } else {
        pane = _imp->_nodeGraphArea->getParentPane();
        if (!pane) {
            std::list<TabWidgetI*> tabs = getApp()->getTabWidgetsSerialization();
            if ( tabs.empty() ) {
                return 0;
            }
            pane = dynamic_cast<TabWidget*>(tabs.front());
        }
        assert(pane);
        if (!pane) {
            return NULL;
        }
        PanelWidget* ret = pane->currentWidget();
        pane->moveProgressPanelHere();
        return ret;
    }
}

void
Gui::onNodeNameChanged(const QString& /*oldLabel*/, const QString & /*newLabel*/)
{
    Node* node = qobject_cast<Node*>( sender() );

    if (!node) {
        return;
    }
    ViewerInstancePtr isViewer = node->isEffectViewerInstance();
    if (isViewer) {
        Q_EMIT viewersChanged();
    }
}

void
Gui::renderAllWriters()
{
    try {
        std::list<RenderQueue::RenderWork> requests;
        getApp()->getRenderQueue()->createRenderRequestsFromCommandLineArgs(areRenderStatsEnabled(), std::list<std::string>(), std::list<std::pair<int, std::pair<int, int> > >(), requests);
        getApp()->getRenderQueue()->renderNonBlocking(requests);
    } catch (const std::exception& e) {
        Dialogs::warningDialog( tr("Render").toStdString(), e.what() );
    }
}

void
Gui::renderSelectedNode()
{
    NodeGraph* graph = getLastSelectedGraph();

    if (!graph) {
        return;
    }

    NodesGuiList selectedNodes = graph->getSelectedNodes();

    if ( selectedNodes.empty() ) {
        Dialogs::warningDialog( tr("Render").toStdString(), tr("You must select a node to render first!").toStdString() );

        return;
    }
    std::list<RenderQueue::RenderWork> workList;
    bool useStats = getApp()->isRenderStatsActionChecked();
    for (NodesGuiList::const_iterator it = selectedNodes.begin();
         it != selectedNodes.end(); ++it) {
        NodePtr internalNode = (*it)->getNode();
        if (!internalNode) {
            continue;
        }
        EffectInstancePtr effect = internalNode->getEffectInstance();
        if (!effect) {
            continue;
        }
        if ( effect->isWriter() ) {
            if ( !internalNode->isDoingSequentialRender() ) {
                //if ((*it)->getNode()->is)
                ///if the node is a writer, just use it to render!
                RenderQueue::RenderWork w;
                w.treeRoot = internalNode;
                w.useRenderStats = useStats;
                workList.push_back(w);
            }
        } else {
            if (selectedNodes.size() == 1) {
                ///create a node and connect it to the node and use it to render

                NodeGraph* graph = selectedNodes.front()->getDagGui();
                CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_WRITE, graph->getGroup()));
                args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
                args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                NodePtr writer = getApp()->createWriter( std::string(), args );
                if (writer) {
                    RenderQueue::RenderWork w;
                    w.treeRoot = writer;
                    w.useRenderStats = useStats;
                    workList.push_back(w);
                }
            }
        }
    }
    getApp()->getRenderQueue()->renderNonBlocking(workList);
} // Gui::renderSelectedNode

void
Gui::setRenderStatsEnabled(bool enabled)
{
    {
        QMutexLocker k(&_imp->areRenderStatsEnabledMutex);
        _imp->areRenderStatsEnabled = enabled;
    }
    _imp->enableRenderStats->setChecked(enabled);
}

bool
Gui::areRenderStatsEnabled() const
{
    QMutexLocker k(&_imp->areRenderStatsEnabledMutex);

    return _imp->areRenderStatsEnabled;
}

RenderStatsDialog*
Gui::getRenderStatsDialog() const
{
    return _imp->statsDialog;
}

RenderStatsDialog*
Gui::getOrCreateRenderStatsDialog()
{
    if (_imp->statsDialog) {
        return _imp->statsDialog;
    }
    _imp->statsDialog = new RenderStatsDialog(this);

    return _imp->statsDialog;
}

void
Gui::onEnableRenderStatsActionTriggered()
{
    assert( QThread::currentThread() == qApp->thread() );

    bool checked = _imp->enableRenderStats->isChecked();
    setRenderStatsEnabled(checked);
    if (!checked) {
        if (_imp->statsDialog) {
            _imp->statsDialog->hide();
        }
    } else {
        if (!_imp->statsDialog) {
            _imp->statsDialog = new RenderStatsDialog(this);
        }
        _imp->statsDialog->show();
    }
}

void
Gui::onTimelineTimeAboutToChange()
{
    assert( QThread::currentThread() == qApp->thread() );
    const std::list<ViewerTab*>& viewers = getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        RenderEnginePtr engine = (*it)->getInternalNode()->getNode()->getRenderEngine();
        engine->abortRenderingAutoRestart();
    }
}

void
Gui::onMustRefreshViewersAndKnobsLaterReceived()
{
    if (!_imp->nKnobsRefreshAfterTimeChangeRequests) {
        return;
    }
    _imp->nKnobsRefreshAfterTimeChangeRequests = 0;

    TimeLinePtr timeline = getApp()->getTimeLine();

    TimelineChangeReasonEnum reason = timeline->getLastSeekReason();
    TimeValue frame(timeline->currentFrame());

    assert( QThread::currentThread() == qApp->thread() );
    if ( (reason == eTimelineChangeReasonUserSeek) ||
        ( reason == eTimelineChangeReasonAnimationModuleSeek) ) {
        if ( getApp()->checkAllReadersModificationDate(true) ) {
            return;
        }
    }

    ProjectPtr project = getApp()->getProject();
    bool isPlayback = reason == eTimelineChangeReasonPlaybackSeek;

    ///Refresh all visible knobs at the current time
    if ( !getApp()->isGuiFrozen() ) {
        std::list<DockablePanelI*> openedPanels = getApp()->getOpenedSettingsPanels();
        for (std::list<DockablePanelI*>::const_iterator it = openedPanels.begin(); it != openedPanels.end(); ++it) {
            NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(*it);
            if (nodePanel) {
                NodePtr node = nodePanel->getNodeGui()->getNode();
                node->getEffectInstance()->refreshAfterTimeChange(isPlayback, frame);
            }
        }
    }


    ViewerNodePtr leadViewer = getApp()->getLastViewerUsingTimeline();
    const std::list<ViewerTab*>& viewers = getViewersList();
    ///Syncrhronize viewers
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        ViewerNodePtr internalNode =  (*it)->getInternalNode() ;
        if ( (internalNode == leadViewer ) && isPlayback ) {
            continue;
        }
        if ( internalNode->isDoingPartialUpdates() ) {
            //When tracking, we handle rendering separatly
            continue;
        }
        internalNode->getNode()->getRenderEngine()->renderCurrentFrame();
    }
} // onMustRefreshViewersAndKnobsLaterReceived

void
Gui::onTimelineTimeChanged(SequenceTime /*time*/,int /*reason*/)
{
    ++_imp->nKnobsRefreshAfterTimeChangeRequests;
    Q_EMIT mustRefreshViewersAndKnobsLater();
}

void
Gui::refreshTimelineGuiKeyframesLater()
{
    // Concatenate refresh requests
    ++_imp->nKeysRefreshRequests;
    Q_EMIT mustRefreshTimelineGuiKeyframesLater();
}

void
Gui::onMustRefreshTimelineGuiKeyframesLaterReceived()
{
    if (_imp->nKeysRefreshRequests == 0) {
        return;
    }
    _imp->nKeysRefreshRequests = 0;
    refreshTimelineGuiKeyframesNow();
}

void
Gui::refreshTimelineGuiKeyframesNow()
{
    _imp->keyframesVisibleOnTimeline.clear();

    std::list<DockablePanelI*> openedPanels = getApp()->getOpenedSettingsPanels();
    for (std::list<DockablePanelI*>::const_iterator it = openedPanels.begin(); it!=openedPanels.end(); ++it) {
        NodeSettingsPanel* isNodePanel = dynamic_cast<NodeSettingsPanel*>(*it);
        if (!isNodePanel) {
            continue;
        }
        NodeGuiPtr node = isNodePanel->getNodeGui();
        if (!node) {
            continue;
        }
        
        node->getAllVisibleKnobsKeyframes(&_imp->keyframesVisibleOnTimeline);
    }

    // Now redraw timelines
    const std::list<ViewerTab*>& viewers = getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
        (*it)->redrawTimeline();
    }

}

const TimeLineKeysSet&
Gui::getTimelineGuiKeyframes() const
{
    return _imp->keyframesVisibleOnTimeline;
}

void
Gui::setEditExpressionDialogLanguage(ExpressionLanguageEnum lang)
{
    _imp->lastExpressionDialogLanguage = lang;
}

void
Gui::setEditExpressionDialogLanguageValid(bool valid)
{
    _imp->isLastExpressionDialogLanguageValid = valid;
}

ExpressionLanguageEnum
Gui::getEditExpressionDialogLanguage() const
{
    if (_imp->isLastExpressionDialogLanguageValid) {
        return _imp->lastExpressionDialogLanguage;
    } else {
        // By default, return Python so that if dropped on the Script Editor the expression is in Python
        return eExpressionLanguagePython;
    }
}

NATRON_NAMESPACE_EXIT
