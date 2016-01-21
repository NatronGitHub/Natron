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

#include "DotGui.h"

#include <cassert>
#include <algorithm> // min, max

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QLayout>
#include <QAction>
#include <QtConcurrentRun>
#include <QFontMetrics>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QGridLayout>
#include <QFile>
#include <QDialogButtonBox>
#include <QApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <ofxNatron.h>

#include "Engine/Backdrop.h"
#include "Engine/Image.h"
#include "Engine/Knob.h"
#include "Engine/MergingEnum.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/BackdropGui.h"
#include "Gui/Button.h"
#include "Gui/CurveEditor.h"
#include "Gui/HostOverlay.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiString.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/NodeGraphTextItem.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#define NATRON_STATE_INDICATOR_OFFSET 5

#define NATRON_EDGE_DROP_TOLERANCE 15

#define NATRON_MAGNETIC_GRID_GRIP_TOLERANCE 20

#define NATRON_MAGNETIC_GRID_RELEASE_DISTANCE 30

#define NATRON_ELLIPSE_WARN_DIAMETER 10

#define DOT_GUI_DIAMETER 15

#define NATRON_PLUGIN_ICON_SIZE 20
#define PLUGIN_ICON_OFFSET 2

NATRON_NAMESPACE_ENTER;

using std::make_pair;

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

//////////Dot node gui
DotGui::DotGui(QGraphicsItem* parent)
: NodeGui(parent)
, diskShape(NULL)
, ellipseIndicator(NULL)
{
}

void
DotGui::createGui()
{
    double depth = getBaseDepth();
    setZValue(depth);

    diskShape = new QGraphicsEllipseItem(this);
    diskShape->setZValue(depth);
    QPointF topLeft = mapFromParent( pos() );
    diskShape->setRect( QRectF(topLeft.x(),topLeft.y(),DOT_GUI_DIAMETER,DOT_GUI_DIAMETER) );

    ellipseIndicator = new QGraphicsEllipseItem(this);
    ellipseIndicator->setRect(QRectF(topLeft.x() - NATRON_STATE_INDICATOR_OFFSET,
                                     topLeft.y() - NATRON_STATE_INDICATOR_OFFSET,
                                     DOT_GUI_DIAMETER + NATRON_STATE_INDICATOR_OFFSET * 2,
                                     DOT_GUI_DIAMETER + NATRON_STATE_INDICATOR_OFFSET * 2));
    ellipseIndicator->hide();
}

void
DotGui::refreshStateIndicator()
{
    bool showIndicator = true;
    if (getIsSelected()) {
        ellipseIndicator->setBrush(QColor(255,255,255,128));
    } else {
        showIndicator = false;
    }

    if (showIndicator && !ellipseIndicator->isVisible()) {
        ellipseIndicator->show();
    } else if (!showIndicator && ellipseIndicator->isVisible()) {
        ellipseIndicator->hide();
    } else {
        update();
    }

}

void
DotGui::applyBrush(const QBrush & brush)
{
    diskShape->setBrush(brush);
}

NodeSettingsPanel*
DotGui::createPanel(QVBoxLayout* container,
                    const boost::shared_ptr<NodeGui> & thisAsShared)
{
    NodeSettingsPanel* panel = new NodeSettingsPanel( boost::shared_ptr<MultiInstancePanel>(),
                                                      getDagGui()->getGui(),
                                                      thisAsShared,
                                                      container,container->parentWidget() );

    ///Always close the panel by default for Dots
    panel->setClosed(true);

    return panel;
}

QRectF
DotGui::boundingRect() const
{
    QTransform t;
    QRectF bbox = diskShape->boundingRect();
    QPointF center = bbox.center();

    t.translate( center.x(), center.y() );
    t.scale( scale(), scale() );
    t.translate( -center.x(), -center.y() );

    return t.mapRect(bbox);
}

QPainterPath
DotGui::shape() const
{
    return diskShape->shape();
}

NATRON_NAMESPACE_EXIT;

//NATRON_NAMESPACE_USING;
//#include "moc_DotGui.cpp"
