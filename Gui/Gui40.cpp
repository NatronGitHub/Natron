/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>

#include "Global/Macros.h"

#include <QtCore/QSettings>
#include <QtCore/QFileInfo>

#include <QAction>
#include <QApplication> // qApp

#include <cairo/cairo.h>

#include <boost/version.hpp>

#include "Engine/CLArgs.h"
#include "Engine/Image.h"
#include "Engine/Lut.h" // floatToInt, LutManager
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AboutWindow.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/ProjectGui.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeSettingsPanel.h"


NATRON_NAMESPACE_USING


void
Gui::refreshAllPreviews()
{
    _imp->_appInstance->getProject()->refreshPreviews();
}

void
Gui::forceRefreshAllPreviews()
{
    _imp->_appInstance->getProject()->forceRefreshPreviews();
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
Gui::showShortcutEditor()
{
    _imp->shortcutEditor->show();
    _imp->shortcutEditor->raise();
    _imp->shortcutEditor->activateWindow();
}

void
Gui::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        QFileInfo f( action->data().toString() );
        QString path = f.path() + '/';
        QString filename = path + f.fileName();
        int openedProject = appPTR->isProjectAlreadyOpened( filename.toStdString() );
        if (openedProject != -1) {
            AppInstance* instance = appPTR->getAppInstance(openedProject);
            if (instance) {
                GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(instance);
                assert(guiApp);
                if (guiApp) {
                    guiApp->getGui()->activateWindow();

                    return;
                }
            }
        }

        ///if the current graph has no value, just load the project in the same window
        if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
            _imp->_appInstance->getProject()->loadProject( path, f.fileName() );
        } else {
            CLArgs cl;
            AppInstance* newApp = appPTR->newAppInstance(cl, false);
            newApp->getProject()->loadProject( path, f.fileName() );
        }
    }
}

void
Gui::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    int numRecentFiles = std::min(files.size(), (int)NATRON_MAX_RECENT_FILES);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg( QFileInfo(files[i]).fileName() );
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
    if (w->objectName() == "CurveEditor") {
        return QPixmap::grabWidget(w);
    }

    return QPixmap::grabWindow( w->winId() );
#else

    return QApplication::primaryScreen()->grabWindow( w->winId() );
#endif
}

void
Gui::onProjectNameChanged(const QString & filePath, bool modified)
{
    // handles window title and appearance formatting
    // http://doc.qt.io/qt-4.8/qwidget.html#windowModified-prop
    setWindowModified(modified);
    // http://doc.qt.io/qt-4.8/qwidget.html#windowFilePath-prop
    setWindowFilePath(filePath.isEmpty() ? NATRON_PROJECT_UNTITLED : filePath);
}

void
Gui::setColorPickersColor(double r,double g, double b,double a)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(r,g,b,a);
}

void
Gui::registerNewColorPicker(boost::shared_ptr<KnobColor> knob)
{
    assert(_imp->_projectGui);
    const std::list<ViewerTab*> &viewers = getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
        if (!(*it)->isPickerEnabled()) {
            (*it)->setPickerEnabled(true);
        }
    }
    _imp->_projectGui->registerNewColorPicker(knob);
}

void
Gui::removeColorPicker(boost::shared_ptr<KnobColor> knob)
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
Gui::setViewersCurrentView(int view)
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
        
        if (v && v == _imp->_masterSyncViewer) {
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

const std::list<boost::shared_ptr<NodeGui> > &
Gui::getVisibleNodes() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes();
}

std::list<boost::shared_ptr<NodeGui> >
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
Gui::onProcessHandlerStarted(const QString & sequenceName,
                             int firstFrame,
                             int lastFrame,
                             int frameStep,
                             const boost::shared_ptr<ProcessHandler> & process)
{
    ///make the dialog which will show the progress
    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame, frameStep, process, this);
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::onRenderProgressDialogFinished()
{
    RenderingProgressDialog* dialog = qobject_cast<RenderingProgressDialog*>(sender());
    if (!dialog) {
        return;
    }
    dialog->close();
    dialog->deleteLater();
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

GuiAppInstance*
Gui::getApp() const
{
    return _imp->_appInstance;
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
        name.append("pane");
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
            name = QString("pane%1").arg(baseNumber);
        } else {
            break;
        }
    }

    return name;
}

void
Gui::setDraftRenderEnabled(bool b)
{
    QMutexLocker k(&_imp->_isInDraftModeMutex);
    _imp->_isInDraftMode = b;
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

CurveEditor*
Gui::getCurveEditor() const
{
    return _imp->_curveEditor;
}

DopeSheetEditor *Gui::getDopeSheetEditor() const
{
    return _imp->_dopeSheetEditor;
}

ScriptEditor*
Gui::getScriptEditor() const
{
    return _imp->_scriptEditor;
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
Gui::debugImage(const Natron::Image* image,
                const RectI& roi,
                const QString & filename )
{
    if (image->getBitDepth() != Natron::eImageBitDepthFloat) {
        qDebug() << "Debug image only works on float images.";
        return;
    }
    RectI renderWindow;
    RectI bounds = image->getBounds();
    if (roi.isNull()) {
        renderWindow = bounds;
    } else {
        if (!roi.intersect(bounds,&renderWindow)) {
            qDebug() << "The RoI does not interesect the bounds of the image.";
            return;
        }
    }
    QImage output(renderWindow.width(), renderWindow.height(), QImage::Format_ARGB32);
    const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
    lut->validate();
    Natron::Image::ReadAccess acc = image->getReadRights();
    const float* from = (const float*)acc.pixelAt( renderWindow.left(), renderWindow.bottom() );
    assert(from);
    int srcNComps = (int)image->getComponentsCount();
    int srcRowElements = srcNComps * bounds.width();
    
    for (int y = renderWindow.height() - 1; y >= 0; --y,
         from += (srcRowElements - srcNComps * renderWindow.width())) {
        
        QRgb* dstPixels = (QRgb*)output.scanLine(y);
        assert(dstPixels);
        
        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;
        
        for (int x = 0; x < renderWindow.width(); ++x, from += srcNComps, ++dstPixels) {
            float r,g,b,a;
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
            *dstPixels = qRgba(U8(error_r >> 8),
                              U8(error_g >> 8),
                              U8(error_b >> 8),
                              U8(a * 255));
        }
    }
    
    U64 hashKey = image->getHashKey();
    QString hashKeyStr = QString::number(hashKey);
    QString realFileName = filename.isEmpty() ? QString(hashKeyStr + ".png") : filename;
#ifdef DEBUG
    qDebug() << "Writing image: " << realFileName;
    renderWindow.debug();
#endif
    output.save(realFileName);
}

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
Gui::onWriterRenderStarted(const QString & sequenceName,
                           int firstFrame,
                           int lastFrame,
                           int frameStep,
                           Natron::OutputEffectInstance* writer)
{
    assert( QThread::currentThread() == qApp->thread() );

    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame, frameStep,
                                                                  boost::shared_ptr<ProcessHandler>(), this);
    RenderEngine* engine = writer->getRenderEngine();
    QObject::connect( dialog, SIGNAL( canceled() ), engine, SLOT( abortRendering_Blocking() ) );
    QObject::connect( engine, SIGNAL( frameRendered(int) ), dialog, SLOT( onFrameRendered(int) ) );
    QObject::connect( engine, SIGNAL( frameRenderedWithTimer(int,double,double) ), dialog, SLOT( onFrameRenderedWithTimer(int,double,double) ) );
    QObject::connect( engine, SIGNAL( renderFinished(int) ), dialog, SLOT( onVideoEngineStopped(int) ) );
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::setGlewVersion(const QString & version)
{
    _imp->_glewVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

void
Gui::setOpenGLVersion(const QString & version)
{
    _imp->_openGLVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

QString
Gui::getGlewVersion() const
{
    return _imp->_glewVersion;
}

QString
Gui::getOpenGLVersion() const
{
    return _imp->_openGLVersion;
}

QString
Gui::getBoostVersion() const
{
    return QString(BOOST_LIB_VERSION);
}

QString
Gui::getQtVersion() const
{
    return QString(QT_VERSION_STR) + " / " + qVersion();
}

QString
Gui::getCairoVersion() const
{
    return QString(CAIRO_VERSION_STRING) + " / " + QString( cairo_version_string() );
}

void
Gui::onNodeNameChanged(const QString & /*name*/)
{
    Natron::Node* node = qobject_cast<Natron::Node*>( sender() );

    if (!node) {
        return;
    }
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( node->getLiveInstance() );
    if (isViewer) {
        Q_EMIT viewersChanged();
    }
}

void
Gui::renderAllWriters()
{
    _imp->_appInstance->startWritersRendering(areRenderStatsEnabled(), false, std::list<AppInstance::RenderRequest>() );
}

void
Gui::renderSelectedNode()
{
    NodeGraph* graph = getLastSelectedGraph();
    if (!graph) {
        return;
    }
    
    const std::list<boost::shared_ptr<NodeGui> > & selectedNodes = graph->getSelectedNodes();

    if ( selectedNodes.empty() ) {
        Natron::warningDialog( tr("Render").toStdString(), tr("You must select a node to render first!").toStdString() );
    } else {
        std::list<AppInstance::RenderWork> workList;

        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = selectedNodes.begin();
             it != selectedNodes.end(); ++it) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            if (effect->isWriter()) {
                if (!effect->areKnobsFrozen()) {
                    //if ((*it)->getNode()->is)
                    ///if the node is a writer, just use it to render!
                    AppInstance::RenderWork w;
                    w.writer = dynamic_cast<Natron::OutputEffectInstance*>(effect);
                    assert(w.writer);
                    w.firstFrame = INT_MIN;
                    w.lastFrame = INT_MAX;
                    w.frameStep = INT_MIN;
                    workList.push_back(w);
                }
            } else {
                if (selectedNodes.size() == 1) {
                    ///create a node and connect it to the node and use it to render
                    boost::shared_ptr<Natron::Node> writer = createWriter();
                    if (writer) {
                        AppInstance::RenderWork w;
                        w.writer = dynamic_cast<Natron::OutputEffectInstance*>( writer->getLiveInstance() );
                        assert(w.writer);
                        w.firstFrame = INT_MIN;
                        w.lastFrame = INT_MAX;
                        w.frameStep = INT_MIN;
                        workList.push_back(w);
                    }
                }
            }
        }
        _imp->_appInstance->startWritersRendering(areRenderStatsEnabled(), false, workList);
    }
}

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
    assert(QThread::currentThread() == qApp->thread());
    
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
    assert(QThread::currentThread() == qApp->thread());
    _imp->wasLaskUserSeekDuringPlayback = false;
    const std::list<ViewerTab*>& viewers = getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        RenderEngine* engine = (*it)->getInternalNode()->getRenderEngine();
        _imp->wasLaskUserSeekDuringPlayback |= engine->abortRendering(true,true);
    }
}

void
Gui::onTimeChanged(SequenceTime time,
                         int reason)
{
    assert(QThread::currentThread() == qApp->thread());
    
    boost::shared_ptr<Project> project = getApp()->getProject();
    
    ///Refresh all visible knobs at the current time
    if (!getApp()->isGuiFrozen()) {
        for (std::list<DockablePanel*>::const_iterator it = _imp->openedPanels.begin(); it!=_imp->openedPanels.end(); ++it) {
            NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(*it);
            if (nodePanel) {
                nodePanel->getNode()->getNode()->getLiveInstance()->refreshAfterTimeChange(time);
            }
        }
    }

    
    ViewerInstance* leadViewer = getApp()->getLastViewerUsingTimeline();
    
    bool isPlayback = reason == eTimelineChangeReasonPlaybackSeek;
    
    

    const std::list<ViewerTab*>& viewers = getViewersList();
    ///Syncrhronize viewers
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end();++it) {
        if ((*it)->getInternalNode() == leadViewer && isPlayback) {
            continue;
        }
         (*it)->getInternalNode()->renderCurrentFrame(!isPlayback);
    }
}

