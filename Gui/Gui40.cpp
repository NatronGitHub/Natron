/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
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
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

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
                GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>( instance.get() );
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                dirParts[i] = QDir::toNativeSeparators(dirs.at(i)).split(QDir::separator(), Qt::SkipEmptyParts);
#else
                dirParts[i] = QDir::toNativeSeparators(dirs.at(i)).split(QDir::separator(), QString::SkipEmptyParts);
#endif
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                        dirNames[i] = QStringList(QDir::toNativeSeparators(dirNames.at(i)).split(QDir::separator(), Qt::SkipEmptyParts).mid(commonComps)).join(QDir::separator());
#else
                        dirNames[i] = QStringList(QDir::toNativeSeparators(dirNames.at(i)).split(QDir::separator(), QString::SkipEmptyParts).mid(commonComps)).join(QDir::separator());
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
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
Gui::setColorPickersColor(double r,
                          double g,
                          double b,
                          double a)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(r, g, b, a);
}

void
Gui::registerNewColorPicker(KnobColorPtr knob)
{
    assert(_imp->_projectGui);
    const std::list<ViewerTab*> &viewers = getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        if ( !(*it)->isPickerEnabled() ) {
            (*it)->setPickerEnabled(true);
        }
    }
    _imp->_projectGui->registerNewColorPicker(knob);
}

void
Gui::removeColorPicker(KnobColorPtr knob)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->removeColorPicker(knob);
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
Gui::updateViewersViewsMenu(const std::vector<std::string>& viewNames)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->updateViewsMenu(viewNames);
    }
}

void
Gui::setViewersCurrentView(ViewIdx view)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setCurrentView(view);
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
Gui::setMasterSyncViewer(ViewerTab* master)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    _imp->_masterSyncViewer = master;
}

ViewerTab*
Gui::getMasterSyncViewer() const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    return _imp->_masterSyncViewer;
}

void
Gui::activateViewerTab(ViewerInstance* viewer)
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
Gui::deactivateViewerTab(ViewerInstance* viewer)
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

        if ( v && (v == _imp->_masterSyncViewer) ) {
            _imp->_masterSyncViewer = 0;
        }
    }

    if (v) {
        removeViewerTab(v, true, false);
    }
}

ViewerTab*
Gui::getViewerTabForInstance(ViewerInstance* node) const
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

const std::list<TabWidget*> &
Gui::getPanes() const
{
    return _imp->_panes;
}

std::list<TabWidget*>
Gui::getPanes_mt_safe() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return _imp->_panes;
}

int
Gui::getPanesCount() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return (int)_imp->_panes.size();
}

QString
Gui::getAvailablePaneName(const QString & baseName) const
{
    QString name = baseName;
    QMutexLocker l(&_imp->_panesMutex);
    int baseNumber = _imp->_panes.size();

    if ( name.isEmpty() ) {
        name.append( QString::fromUtf8("pane") );
        name.append( QString::number(baseNumber) );
    }

    for (;; ) {
        bool foundName = false;
        for (std::list<TabWidget*>::const_iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            if ( (*it)->objectName_mt_safe() == name ) {
                foundName = true;
                break;
            }
        }
        if (foundName) {
            ++baseNumber;
            name = QString::fromUtf8("pane%1").arg(baseNumber);
        } else {
            break;
        }
    }

    return name;
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
    if (!_imp) {
        return nullptr;
    }
    return _imp->_nodeGraphArea;
}

CurveEditor*
Gui::getCurveEditor() const
{
    return _imp->_curveEditor;
}

DopeSheetEditor *
Gui::getDopeSheetEditor() const
{
    return _imp->_dopeSheetEditor;
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

void
Gui::debugImage(const Image* image,
                const RectI& roi,
                const QString & filename )
{
    if (image->getBitDepth() != eImageBitDepthFloat) {
        qDebug() << "Debug image only works on float images.";

        return;
    }
    RectI renderWindow;
    RectI bounds = image->getBounds();
    if ( roi.isNull() ) {
        renderWindow = bounds;
    } else {
        if ( !roi.intersect(bounds, &renderWindow) ) {
            qDebug() << "The RoI does not intersect the bounds of the image.";

            return;
        }
    }
    QImage output(renderWindow.width(), renderWindow.height(), QImage::Format_ARGB32);
    const Color::Lut* lut = Color::LutManager::sRGBLut();
    lut->validate();
    Image::ReadAccess acc = image->getReadRights();
    const float* from = (const float*)acc.pixelAt( renderWindow.left(), renderWindow.bottom() );
    assert(from);
    int srcNComps = (int)image->getComponentsCount();
    int srcRowElements = srcNComps * bounds.width();

    for ( int y = renderWindow.height() - 1; y >= 0; --y,
          from += ( srcRowElements - srcNComps * renderWindow.width() ) ) {
        QRgb* dstPixels = (QRgb*)output.scanLine(y);
        assert(dstPixels);

        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;

        for (int x = 0; x < renderWindow.width(); ++x, from += srcNComps, ++dstPixels) {
            float r, g, b, a;
            switch (srcNComps) {
            case 1:
                r = g = b = *from;
                a = 1;
                break;
            case 2:
                r = *from;
                g = *(from + 1);
                b = 0;
                a = 1;
                break;
            case 3:
                r = *from;
                g = *(from + 1);
                b = *(from + 2);
                a = 1;
                break;
            case 4:
                r = *from;
                g = *(from + 1);
                b = *(from + 2);
                a = *(from + 3);
                break;
            default:
                assert(false);

                return;
            }
            error_r = (error_r & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(r);
            error_g = (error_g & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(g);
            error_b = (error_b & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(b);
            assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
            *dstPixels = qRgba( U8(error_r >> 8),
                                U8(error_g >> 8),
                                U8(error_b >> 8),
                                U8(a * 255) );
        }
    }

    U64 hashKey = image->getHashKey();
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
                     int firstFrame,
                     int lastFrame,
                     int frameStep,
                     bool canPause,
                     OutputEffectInstance* writer,
                     const ProcessHandlerPtr & process)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->_progressPanel->startTask(writer->getNode(), firstFrame, lastFrame, frameStep, canPause, true, sequenceName, process);
}

void
Gui::onRenderRestarted(OutputEffectInstance* writer,
                       const ProcessHandlerPtr & process)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->_progressPanel->onTaskRestarted(writer->getNode(), process);
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
            std::list<TabWidget*> tabs;
            {
                QMutexLocker k(&_imp->_panesMutex);
                tabs = _imp->_panes;
            }
            if ( tabs.empty() ) {
                return;
            }
            pane = tabs.front();
        }
        assert(pane);
        pane->moveScriptEditorHere();
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
            std::list<TabWidget*> tabs;
            {
                QMutexLocker k(&_imp->_panesMutex);
                tabs = _imp->_panes;
            }
            if ( tabs.empty() ) {
                return 0;
            }
            pane = tabs.front();
        }
        assert(pane);
        PanelWidget* ret = pane->currentWidget();
        pane->moveProgressPanelHere();
        return ret;
    }
}

void
Gui::onNodeNameChanged(const QString & /*name*/)
{
    Node* node = qobject_cast<Node*>( sender() );

    if (!node) {
        return;
    }
    ViewerInstance* isViewer = node->isEffectViewer();
    if (isViewer) {
        Q_EMIT viewersChanged();
    }
}

void
Gui::renderAllWriters()
{
    try {
        getApp()->startWritersRenderingFromNames( areRenderStatsEnabled(), false, std::list<std::string>(), std::list<std::pair<int, std::pair<int, int> > >() );
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
    std::list<AppInstance::RenderWork> workList;
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
            if ( !effect->areKnobsFrozen() ) {
                //if ((*it)->getNode()->is)
                ///if the node is a writer, just use it to render!
                AppInstance::RenderWork w;
                w.writer = dynamic_cast<OutputEffectInstance*>( effect.get() );
                assert(w.writer);
                w.firstFrame = INT_MIN;
                w.lastFrame = INT_MAX;
                w.frameStep = INT_MIN;
                w.useRenderStats = useStats;
                workList.push_back(w);
            }
        } else {
            if (selectedNodes.size() == 1) {
                ///create a node and connect it to the node and use it to render
#ifndef NATRON_ENABLE_IO_META_NODES
                NodePtr writer = createWriter();
#else
                NodeGraph* graph = selectedNodes.front()->getDagGui();
                CreateNodeArgs args(PLUGINID_NATRON_WRITE, graph->getGroup());
                args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
                args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                NodePtr writer = getApp()->createWriter( std::string(), args );
#endif
                if (writer) {
                    AppInstance::RenderWork w;
                    w.writer = dynamic_cast<OutputEffectInstance*>( writer->getEffectInstance().get() );
                    assert(w.writer);
                    w.firstFrame = INT_MIN;
                    w.lastFrame = INT_MAX;
                    w.frameStep = INT_MIN;
                    w.useRenderStats = useStats;
                    workList.push_back(w);
                }
            }
        }
    }
    getApp()->startWritersRendering(false, workList);
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
        RenderEnginePtr engine = (*it)->getInternalNode()->getRenderEngine();
        engine->abortRenderingAutoRestart();
    }
}

void
Gui::renderViewersAndRefreshKnobsAfterTimelineTimeChange(SequenceTime time,
                                                         int reason)
{
    TimeLine* timeline = qobject_cast<TimeLine*>( sender() );

    if ( timeline != getApp()->getTimeLine().get() ) {
        return;
    }

    assert( QThread::currentThread() == qApp->thread() );
    if ( (reason == eTimelineChangeReasonUserSeek) ||
         ( reason == eTimelineChangeReasonDopeSheetEditorSeek) ||
         ( reason == eTimelineChangeReasonCurveEditorSeek) ) {
        if ( getApp()->checkAllReadersModificationDate(true) ) {
            return;
        }
    }

    ProjectPtr project = getApp()->getProject();
    bool isPlayback = reason == eTimelineChangeReasonPlaybackSeek;

    ///Refresh all visible knobs at the current time
    if ( !getApp()->isGuiFrozen() ) {
        for (std::list<DockablePanel*>::const_iterator it = _imp->openedPanels.begin(); it != _imp->openedPanels.end(); ++it) {
            NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(*it);
            if (nodePanel) {
                NodePtr node = nodePanel->getNode()->getNode();
                node->getEffectInstance()->refreshAfterTimeChange(isPlayback, time);

                NodesList children;
                node->getChildrenMultiInstance(&children);
                for (NodesList::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                    (*it2)->getEffectInstance()->refreshAfterTimeChange(isPlayback, time);
                }
            }
        }
    }


    ViewerInstance* leadViewer = getApp()->getLastViewerUsingTimeline();
    const std::list<ViewerTab*>& viewers = getViewersList();
    ///Syncrhronize viewers
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        if ( ( (*it)->getInternalNode() == leadViewer ) && isPlayback ) {
            continue;
        }
        if ( (*it)->getInternalNode()->isDoingPartialUpdates() ) {
            //When tracking, we handle rendering separately
            continue;
        }
        (*it)->getInternalNode()->renderCurrentFrame(!isPlayback);
    }
} // Gui::renderViewersAndRefreshKnobsAfterTimelineTimeChange

NATRON_NAMESPACE_EXIT
