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

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <cmath> // ceil
//#include <cstdlib>
//#include <set>
//#include <map>
#include <list>
//#include <vector>
//#include <locale>
#include <algorithm> // min, max

//GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
//// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
//#include <QGraphicsProxyWidget>
//GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
//#include <QGraphicsTextItem>
//#include <QFileSystemModel>
//#include <QScrollBar>
//#include <QVBoxLayout>
//#include <QGraphicsLineItem>
//#include <QGraphicsPixmapItem>
//#include <QDialogButtonBox>
//#include <QUndoStack>
//#include <QToolButton>
//#include <QThread>
//#include <QDropEvent>
//#include <QApplication>
//#include <QCheckBox>
//#include <QMimeData>
//#include <QLineEdit>
//#include <QDebug>
//#include <QtCore/QRectF>
//#include <QRegExp>
//#include <QtCore/QTimer>
//#include <QAction>
//#include <QPainter>
//CLANG_DIAG_OFF(deprecated)
//CLANG_DIAG_OFF(uninitialized)
//#include <QMutex>
//CLANG_DIAG_ON(deprecated)
//CLANG_DIAG_ON(uninitialized)
//
//#include <SequenceParsing.h>
//
//#include "Engine/AppManager.h"
//#include "Engine/BackDrop.h"
//#include "Engine/Dot.h"
//#include "Engine/FrameEntry.h"
//#include "Engine/Hash64.h"
//#include "Engine/KnobFile.h"
//#include "Engine/Node.h"
//#include "Engine/NodeGroup.h"
//#include "Engine/NodeSerialization.h"
//#include "Engine/OfxEffectInstance.h"
//#include "Engine/OutputSchedulerThread.h"
//#include "Engine/Plugin.h"
//#include "Engine/Project.h"
#include "Engine/Settings.h"
//#include "Engine/ViewerInstance.h"
//
//#include "Gui/ActionShortcuts.h"
//#include "Gui/BackDropGui.h"
//#include "Gui/CurveEditor.h"
//#include "Gui/CurveWidget.h"
//#include "Gui/DockablePanel.h"
//#include "Gui/Edge.h"
//#include "Gui/FloatingWidget.h"
//#include "Gui/Gui.h"
//#include "Gui/Gui.h"
//#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
//#include "Gui/GuiMacros.h"
//#include "Gui/Histogram.h"
//#include "Gui/KnobGui.h"
//#include "Gui/Label.h"
//#include "Gui/Menu.h"
//#include "Gui/NodeBackDropSerialization.h"
//#include "Gui/NodeClipBoard.h"
//#include "Gui/NodeCreationDialog.h"
//#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGui.h"
//#include "Gui/NodeGuiSerialization.h"
//#include "Gui/NodeSettingsPanel.h"
//#include "Gui/SequenceFileDialog.h"
//#include "Gui/TabWidget.h"
//#include "Gui/TimeLineGui.h"
//#include "Gui/ToolButton.h"
//#include "Gui/ViewerGL.h"
//#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h"

using namespace Natron;
//using std::cout; using std::endl;



bool
NodeGraph::isNearbyNavigator(const QPoint& widgetPos,QPointF& scenePos) const
{
    if (!_imp->_navigator->isVisible()) {
        return false;
    }
    
    QRect visibleWidget = visibleWidgetRect();
    
    int navWidth = std::ceil(width() * NATRON_NAVIGATOR_BASE_WIDTH);
    int navHeight = std::ceil(height() * NATRON_NAVIGATOR_BASE_HEIGHT);

    QPoint btmRightWidget = visibleWidget.bottomRight();
    QPoint navTopLeftWidget = btmRightWidget - QPoint(navWidth,navHeight );
    
    if (widgetPos.x() >= navTopLeftWidget.x() && widgetPos.x() < btmRightWidget.x() &&
        widgetPos.y() >= navTopLeftWidget.y() && widgetPos.y() <= btmRightWidget.y()) {
        
        ///The bbox of all nodes in the nodegraph
        QRectF sceneR = _imp->calcNodesBoundingRect();
        
        ///The visible portion of the nodegraph
        QRectF viewRect = visibleSceneRect();
        sceneR = sceneR.united(viewRect);
        
        ///Make sceneR and viewRect keep the same aspect ratio as the navigator
        double xScale = navWidth / sceneR.width();
        double yScale =  navHeight / sceneR.height();
        double scaleFactor = std::max(0.001,std::min(xScale,yScale));

        ///Make the widgetPos relative to the navTopLeftWidget
        QPoint clickNavPos(widgetPos.x() - navTopLeftWidget.x(), widgetPos.y() - navTopLeftWidget.y());
        
        scenePos.rx() = clickNavPos.x() / scaleFactor;
        scenePos.ry() = clickNavPos.y() / scaleFactor;
        
        ///Now scenePos is in scene coordinates, but relative to the sceneR top left
        scenePos.rx() += sceneR.x();
        scenePos.ry() += sceneR.y();
        return true;
    }
    
    return false;
    
}

void
NodeGraph::pushUndoCommand(QUndoCommand* command)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(command);
}

bool
NodeGraph::areOptionalInputsAutoHidden() const
{
    return appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
}

void
NodeGraph::deselect()
{
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            (*it)->setUserSelected(false);
        }
    }
  
    _imp->_selection.clear();

    if (_imp->_magnifiedNode && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}
