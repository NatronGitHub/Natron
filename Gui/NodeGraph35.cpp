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

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <cmath>
#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QAction>
#include <QFileSystemModel>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Cache.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/ToolButton.h"

#include "Serialization/KnobSerialization.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER
QImage
NodeGraph::getFullSceneScreenShot()
{
    _imp->isDoingPreviewRender = true;

    // The bbox of all nodes in the nodegraph
    QRectF sceneR = _imp->calcNodesBoundingRect();

    // The visible portion of the nodegraph
    QRectF viewRect = visibleSceneRect();

    // Make sure the visible rect is included in the scene rect
    sceneR = sceneR.united(viewRect);

    int navWidth = std::ceil(width() * NATRON_NAVIGATOR_BASE_WIDTH);
    int navHeight = std::ceil(height() * NATRON_NAVIGATOR_BASE_HEIGHT);

    // Make sceneR and viewRect keep the same aspect ratio as the navigator
    double xScale = navWidth / sceneR.width();
    double yScale =  navHeight / sceneR.height();
    double scaleFactor = std::max( 0.001, std::min(xScale, yScale) );
    int sceneW_navPixelCoord = std::floor(sceneR.width() * scaleFactor);
    int sceneH_navPixelCoord = std::floor(sceneR.height() * scaleFactor);

    // Render the scene in an image with the same aspect ratio  as the scene rect
    QImage renderImage(sceneW_navPixelCoord, sceneH_navPixelCoord, QImage::Format_ARGB32_Premultiplied);

    // Fill the background
    renderImage.fill( QColor(71, 71, 71, 255) );

    // Offset the visible rect corner as an offset relative to the scene rect corner
    viewRect.setX( viewRect.x() - sceneR.x() );
    viewRect.setY( viewRect.y() - sceneR.y() );
    viewRect.setWidth( viewRect.width() - sceneR.x() );
    viewRect.setHeight( viewRect.height() - sceneR.y() );

    QRectF viewRect_navCoordinates = viewRect;
    viewRect_navCoordinates.setLeft(viewRect.left() * scaleFactor);
    viewRect_navCoordinates.setBottom(viewRect.bottom() * scaleFactor);
    viewRect_navCoordinates.setRight(viewRect.right() * scaleFactor);
    viewRect_navCoordinates.setTop(viewRect.top() * scaleFactor);

    // Paint the visible portion with a highlight
    QPainter painter(&renderImage);

    // Remove the overlays from the scene before rendering it
    scene()->removeItem(_imp->_cacheSizeText);
    scene()->removeItem(_imp->_navigator);

    // Render into the QImage with downscaling
    scene()->render(&painter, renderImage.rect(), sceneR, Qt::KeepAspectRatio);

    // Add the overlays back
    scene()->addItem(_imp->_navigator);
    scene()->addItem(_imp->_cacheSizeText);

    // Fill the highlight with a semi transparent whitish grey
    painter.fillRect( viewRect_navCoordinates, QColor(200, 200, 200, 100) );

    // Draw a border surrounding the
    QPen p;
    p.setWidth(2);
    p.setBrush(Qt::yellow);
    painter.setPen(p);
    // Make sure the border is visible
    viewRect_navCoordinates.adjust(2, 2, -2, -2);
    painter.drawRect(viewRect_navCoordinates);

    // Now make an image of the requested size of the navigator and center the render image into it
    QImage img(navWidth, navHeight, QImage::Format_ARGB32_Premultiplied);
    img.fill( QColor(71, 71, 71, 255) );

    int xOffset = ( img.width() - renderImage.width() ) / 2;
    int yOffset = ( img.height() - renderImage.height() ) / 2;
    int dstY = yOffset;
    for (int srcY = 0; srcY < renderImage.height(); ++srcY, ++dstY) {
        if ( dstY < 0 || dstY >= img.height() ) {
            break;
        }
        QRgb* dst_pixels = (QRgb*)img.scanLine(dstY);
        assert(dst_pixels);

        const QRgb* src_pixels = (const QRgb*)renderImage.scanLine(srcY);
        assert(src_pixels);

        int dstX = xOffset;
        for (int srcX = 0; srcX < renderImage.width(); ++srcX, ++dstX) {
            if ( dstX < 0 || dstX >= img.width() ) {
                dst_pixels[dstX] = qRgba(0, 0, 0, 0);
            } else {
                dst_pixels[dstX] = src_pixels[srcX];
            }
        }
    }

    _imp->isDoingPreviewRender = false;

    return img;
} // getFullSceneScreenShot

const NodesGuiList &
NodeGraph::getAllActiveNodes() const
{
    return _imp->_nodes;
}

NodesGuiList
NodeGraph::getAllActiveNodes_mt_safe() const
{
    QMutexLocker l(&_imp->_nodesMutex);

    return _imp->_nodes;
}


// grabbed from QDirModelPrivate::size() in qtbase/src/widgets/itemviews/qdirmodel.cpp
static
QString
QDirModelPrivate_size(quint64 bytes)
{
    // According to the Si standard KB is 1000 bytes, KiB is 1024
    // but on windows sizes are calulated by dividing by 1024 so we do what they do.
    const quint64 kb = 1024;
    const quint64 mb = 1024 * kb;
    const quint64 gb = 1024 * mb;
    const quint64 tb = 1024 * gb;

    if (bytes >= tb) {
        return QFileSystemModel::tr("%1 TB").arg( QLocale().toString(qreal(bytes) / tb, 'f', 3) );
    }
    if (bytes >= gb) {
        return QFileSystemModel::tr("%1 GB").arg( QLocale().toString(qreal(bytes) / gb, 'f', 2) );
    }
    if (bytes >= mb) {
        return QFileSystemModel::tr("%1 MB").arg( QLocale().toString(qreal(bytes) / mb, 'f', 1) );
    }
    if (bytes >= kb) {
        return QFileSystemModel::tr("%1 KB").arg( QLocale().toString(bytes / kb) );
    }

    return QFileSystemModel::tr("%1 byte(s)").arg( QLocale().toString(bytes) );
}

void
NodeGraph::updateCacheSizeText()
{
    if ( !getGui() ) {
        return;
    }
    if ( getGui()->isGUIFrozen() ) {
        if ( _imp->_cacheSizeText->isVisible() ) {
            _imp->_cacheSizeText->hide();
        }

        return;
    } else {
        if ( !_imp->cacheSizeHidden && !_imp->_cacheSizeText->isVisible() ) {
            _imp->_cacheSizeText->show();
        } else if ( _imp->cacheSizeHidden && _imp->_cacheSizeText->isVisible() ) {
            _imp->_cacheSizeText->hide();

            return;
        }
    }

    QString oldText = _imp->_cacheSizeText->text();
    quint64 cacheSize = appPTR->getTileCache()->getCurrentSize();
    QString cacheSizeStr = QDirModelPrivate_size(cacheSize);
    QString newText = tr("Image Cache: %1").arg(cacheSizeStr);
    if (newText != oldText) {
        _imp->_cacheSizeText->setText(newText);
    }
}

void
NodeGraph::onRefreshNodesRenderStateTimerTimeout()
{
    for (NodesGuiList::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshRenderingIndicator();
    }
}

void
NodeGraph::toggleCacheInfo()
{
    _imp->cacheSizeHidden = !_imp->cacheSizeHidden;
    if (_imp->cacheSizeHidden) {
        _imp->_cacheSizeText->hide();
    } else {
        _imp->_cacheSizeText->show();
    }
}

void
NodeGraph::toggleKnobLinksVisible()
{
    _imp->_knobLinksVisible = !_imp->_knobLinksVisible;

    refreshNodeLinksLater();

  
}

void
NodeGraph::toggleAutoPreview()
{
    getGui()->getApp()->getProject()->toggleAutoPreview();
}

void
NodeGraph::forceRefreshAllPreviews()
{
    getGui()->forceRefreshAllPreviews();
}

void
NodeGraph::showMenu(const QPoint & pos)
{
    _imp->_menu->clear();

    QAction* findAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphFindNode,
                                                 kShortcutActionGraphFindNodeLabel, _imp->_menu);
    _imp->_menu->addAction(findAction);
    _imp->_menu->addSeparator();

    //QFont font(appFont,appFontSize);
    Menu* editMenu = new Menu(tr("Edit"), _imp->_menu);
    //editMenu->setFont(font);
    _imp->_menu->addAction( editMenu->menuAction() );

    QAction* copyAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphCopy,
                                                 kShortcutActionGraphCopyLabel, editMenu);
    QObject::connect( copyAction, SIGNAL(triggered()), this, SLOT(copySelectedNodes()) );
    editMenu->addAction(copyAction);

    QAction* cutAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphCut,
                                                kShortcutActionGraphCutLabel, editMenu);
    QObject::connect( cutAction, SIGNAL(triggered()), this, SLOT(cutSelectedNodes()) );
    editMenu->addAction(cutAction);


    QAction* pasteAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphPaste,
                                                  kShortcutActionGraphPasteLabel, editMenu);
    bool cbEnabled = false;
    {
        QClipboard* clipboard = QApplication::clipboard();
        const QMimeData* data = clipboard->mimeData();
        if (data && data->hasFormat(QLatin1String("text/plain"))) {
            cbEnabled = true;
        }
    }
    pasteAction->setEnabled(cbEnabled);
    editMenu->addAction(pasteAction);

    QAction* deleteAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphRemoveNodes,
                                                   kShortcutActionGraphRemoveNodesLabel, editMenu);
    QObject::connect( deleteAction, SIGNAL(triggered()), this, SLOT(deleteSelection()) );
    editMenu->addAction(deleteAction);

    QAction* renameAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphRenameNode,
                                                   kShortcutActionGraphRenameNodeLabel, editMenu);
    QObject::connect( renameAction, SIGNAL(triggered()), this, SLOT(renameNode()) );
    editMenu->addAction(renameAction);

    QAction* duplicateAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphDuplicate,
                                                      kShortcutActionGraphDuplicateLabel, editMenu);
    editMenu->addAction(duplicateAction);

    QAction* cloneAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphClone,
                                                  kShortcutActionGraphCloneLabel, editMenu);
    editMenu->addAction(cloneAction);

    QAction* decloneAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphDeclone,
                                                    kShortcutActionGraphDecloneLabel, editMenu);
    QObject::connect( decloneAction, SIGNAL(triggered()), this, SLOT(decloneSelectedNodes()) );
    editMenu->addAction(decloneAction);

    QAction* switchInputs = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphExtractNode,
                                                   kShortcutActionGraphExtractNodeLabel, editMenu);
    QObject::connect( switchInputs, SIGNAL(triggered()), this, SLOT(extractSelectedNode()) );
    editMenu->addAction(switchInputs);

    QAction* extractNode = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphSwitchInputs,
                                                  kShortcutActionGraphSwitchInputsLabel, editMenu);
    QObject::connect( extractNode, SIGNAL(triggered()), this, SLOT(switchInputs1and2ForSelectedNodes()) );
    editMenu->addAction(extractNode);

    QAction* disableNodes = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphDisableNodes,
                                                   kShortcutActionGraphDisableNodesLabel, editMenu);
    QObject::connect( disableNodes, SIGNAL(triggered()), this, SLOT(toggleSelectedNodesEnabled()) );
    editMenu->addAction(disableNodes);

    QAction* groupFromSel = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphMakeGroup,
                                                   kShortcutActionGraphMakeGroupLabel, editMenu);
    QObject::connect( groupFromSel, SIGNAL(triggered()), this, SLOT(createGroupFromSelection()) );
    editMenu->addAction(groupFromSel);

    QAction* expandGroup = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphExpandGroup,
                                                  kShortcutActionGraphExpandGroupLabel, editMenu);
    QObject::connect( expandGroup, SIGNAL(triggered()), this, SLOT(expandSelectedGroups()) );
    editMenu->addAction(expandGroup);

    QAction* displayCacheInfoAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphShowCacheSize,
                                                             kShortcutActionGraphShowCacheSizeLabel, _imp->_menu);
    displayCacheInfoAction->setCheckable(true);
    displayCacheInfoAction->setChecked( _imp->_cacheSizeText->isVisible() );
    QObject::connect( displayCacheInfoAction, SIGNAL(triggered()), this, SLOT(toggleCacheInfo()) );
    _imp->_menu->addAction(displayCacheInfoAction);

    NodesGuiList selectedNodes = getSelectedNodes();
    if ( !selectedNodes.empty() ) {
        QAction* turnOffPreviewAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphTogglePreview,
                                                               kShortcutActionGraphTogglePreviewLabel, _imp->_menu);
        turnOffPreviewAction->setCheckable(true);
        turnOffPreviewAction->setChecked(false);
        QObject::connect( turnOffPreviewAction, SIGNAL(triggered()), this, SLOT(togglePreviewsForSelectedNodes()) );
        _imp->_menu->addAction(turnOffPreviewAction);

        if (selectedNodes.size() == 1) {
            QAction* openNodePanelAction = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphOpenNodePanel,
                                                                  kShortcutActionGraphOpenNodePanelLabel, _imp->_menu);
            openNodePanelAction->setCheckable(true);
            openNodePanelAction->setChecked(false);
            QObject::connect( openNodePanelAction, SIGNAL(triggered()), this, SLOT(showSelectedNodeSettingsPanel()) );
            _imp->_menu->addAction(openNodePanelAction);
        }
    }

    QAction* autoHideInputs = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphAutoHideInputs,
                                                     kShortcutActionGraphAutoHideInputsLabel, _imp->_menu);
    autoHideInputs->setCheckable(true);
    autoHideInputs->setChecked( appPTR->getCurrentSettings()->areOptionalInputsAutoHidden() );
    QObject::connect( autoHideInputs, SIGNAL(triggered()), this, SLOT(toggleAutoHideInputs()) );
    _imp->_menu->addAction(autoHideInputs);

    QAction* hideInputs = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphHideInputs,
                                                 kShortcutActionGraphHideInputsLabel, _imp->_menu);
    hideInputs->setCheckable(true);
    bool hideInputsVal = false;
    if ( !selectedNodes.empty() ) {
        hideInputsVal = selectedNodes.front()->getNode()->getEffectInstance()->getHideInputsKnobValue();
    }
    hideInputs->setChecked(hideInputsVal);
    QObject::connect( hideInputs, SIGNAL(triggered()), this, SLOT(toggleHideInputs()) );
    _imp->_menu->addAction(hideInputs);


    QAction* knobLinks = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphShowExpressions,
                                                kShortcutActionGraphShowExpressionsLabel, _imp->_menu);
    knobLinks->setCheckable(true);
    knobLinks->setChecked( areKnobLinksVisible() );
    QObject::connect( knobLinks, SIGNAL(triggered()), this, SLOT(toggleKnobLinksVisible()) );
    _imp->_menu->addAction(knobLinks);

    QAction* autoPreview = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphToggleAutoPreview,
                                                  kShortcutActionGraphToggleAutoPreviewLabel, _imp->_menu);
    autoPreview->setCheckable(true);
    autoPreview->setChecked( getGui()->getApp()->getProject()->isAutoPreviewEnabled() );
    QObject::connect( autoPreview, SIGNAL(triggered()), this, SLOT(toggleAutoPreview()) );
    QObject::connect( getGui()->getApp()->getProject().get(), SIGNAL(autoPreviewChanged(bool)), autoPreview, SLOT(setChecked(bool)) );
    _imp->_menu->addAction(autoPreview);

    QAction* autoTurbo = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphToggleAutoTurbo,
                                                kShortcutActionGraphToggleAutoTurboLabel, _imp->_menu);
    autoTurbo->setCheckable(true);
    autoTurbo->setChecked( appPTR->getCurrentSettings()->isAutoTurboEnabled() );
    QObject::connect( autoTurbo, SIGNAL(triggered()), this, SLOT(toggleAutoTurbo()) );
    _imp->_menu->addAction(autoTurbo);


    QAction* forceRefreshPreviews = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphForcePreview,
                                                           kShortcutActionGraphForcePreviewLabel, _imp->_menu);
    QObject::connect( forceRefreshPreviews, SIGNAL(triggered()), this, SLOT(forceRefreshAllPreviews()) );
    _imp->_menu->addAction(forceRefreshPreviews);

    QAction* frameAllNodes = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutActionGraphFrameNodes,
                                                    kShortcutActionGraphFrameNodesLabel, _imp->_menu);
    QObject::connect( frameAllNodes, SIGNAL(triggered()), this, SLOT(centerOnAllNodes()) );
    _imp->_menu->addAction(frameAllNodes);

    _imp->_menu->addSeparator();

    std::list<ToolButton*> orederedToolButtons = getGui()->getToolButtonsOrdered();
    for (std::list<ToolButton*>::iterator it = orederedToolButtons.begin(); it != orederedToolButtons.end(); ++it) {
        (*it)->getMenu()->setIcon( (*it)->getMenuIcon() );
        _imp->_menu->addAction( (*it)->getMenu()->menuAction() );
    }

    QAction* ret = _imp->_menu->exec(pos);
    if (ret == findAction) {
        popFindDialog();
    } else if (ret == duplicateAction) {
        QRectF rect = visibleSceneRect();
        duplicateSelectedNodes( rect.center() );
    } else if (ret == cloneAction) {
        QRectF rect = visibleSceneRect();
        cloneSelectedNodes( rect.center() );
    } else if (ret == pasteAction) {
        QRectF rect = visibleSceneRect();
        pasteClipboard( rect.center() );
    }
} // NodeGraph::showMenu

NATRON_NAMESPACE_EXIT
