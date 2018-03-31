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

#include "AnimationModuleViewPrivate.h"

#include <algorithm> // min, max
#include <limits>
#include <stdexcept>

// Qt includes
#include <QApplication>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtCore/QThread>
#include <QToolButton>

// Natron includes
#include "Engine/Curve.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewIdx.h"

#include "Global/Enums.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/DockablePanel.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleViewPrivate.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/CurveGui.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeAnim.h"
#include "Gui/TableItemAnim.h"
#include "Gui/TextRenderer.h"
#include "Gui/ticks.h"
#include "Gui/TabWidget.h"

#define NATRON_DOPESHEET_MIN_RANGE_FIT 10

#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER


// Constants
static const int DISTANCE_ACCEPTANCE_FROM_KEYFRAME = 5;
static const int DISTANCE_ACCEPTANCE_FROM_READER_EDGE = 14;
static const int DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM = 8;


////////////////////////// Helpers //////////////////////////


NATRON_NAMESPACE_ANONYMOUS_EXIT



bool
AnimationModuleViewPrivate::setDopeSheetCursor(const QPoint& eventPos)
{
    if (!treeView) {
        return false;
    }

    if ( isNearbySelectedKeyFramesCrossWidget(dopeSheetZoomContext, dopeSheetSelectedKeysBRect, eventPos) ) {
        _publicInterface->setCursor(Qt::OpenHandCursor);
        return true;
    } else if ( isNearbyBboxMidRight(dopeSheetZoomContext,  dopeSheetSelectedKeysBRect, eventPos) ) {
        _publicInterface->setCursor(Qt::SplitHCursor);
        return true;
    } else if ( isNearbyBboxMidLeft(dopeSheetZoomContext, dopeSheetSelectedKeysBRect, eventPos) ) {
        _publicInterface->setCursor(Qt::SplitHCursor);
        return true;
    } else if ( isNearbyTimelineBtmPoly(eventPos) || isNearbyTimelineTopPoly(eventPos)) {
        _publicInterface->setCursor(Qt::SplitHCursor);
        return true;
    }

    QPointF canonicalPos = dopeSheetZoomContext.toZoomCoordinates( eventPos.x(), eventPos.y() );


    // Look for a range node
    AnimationModuleBasePtr animModel = _model.lock();
    const std::list<NodeAnimPtr>& selectedNodes = animModel->getSelectionModel()->getCurrentNodesSelection();
    for (std::list<NodeAnimPtr>::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        if ( (*it)->isRangeDrawingEnabled() ) {


            RangeD range = (*it)->getFrameRange();
            QRectF treeItemRect = treeView->visualItemRect((*it)->getTreeItem());
            RectD nodeClipRect;
            nodeClipRect.x1 = range.min;
            nodeClipRect.x2 = range.max;
            nodeClipRect.y2 = dopeSheetZoomContext.toZoomCoordinates( 0, treeItemRect.top() ).y();
            nodeClipRect.y1 = dopeSheetZoomContext.toZoomCoordinates( 0, treeItemRect.bottom() ).y();

            AnimatedItemTypeEnum nodeType = (*it)->getItemType();
            if ( nodeClipRect.contains( canonicalPos.x(), canonicalPos.y() ) ) {
                _publicInterface->setCursor(Qt::OpenHandCursor);
                return true;
            }
            if (nodeType == eAnimatedItemTypeReader) {
                if ( isNearByClipRectLeft(canonicalPos, nodeClipRect) ) {
                    _publicInterface->setCursor(Qt::SplitHCursor);
                    return true;
                } else if ( isNearByClipRectRight(canonicalPos, nodeClipRect) ) {
                    _publicInterface->setCursor(Qt::SplitHCursor);
                    return true;
                } else if ( animModel->canSlipReader(*it) && isNearByClipRectBottom(canonicalPos, nodeClipRect) ) {
                    _publicInterface->setCursor(Qt::SizeHorCursor);
                    return true;
                }
            }
        }
    } // for (AnimTreeItemNodeMap::iterator it = nodes.begin(); it!=nodes.end(); ++it) {

    QTreeWidgetItem *treeItem = treeView->itemAt( 0, eventPos.y() );
    //Did not find a range node, look for keyframes
    if (treeItem) {

        AnimatedItemTypeEnum itemType;
        KnobAnimPtr isKnob;
        NodeAnimPtr isNode;
        TableItemAnimPtr isTableItem;
        ViewSetSpec view;
        DimSpec dimension;
        if (animModel->findItem(treeItem, &itemType, &isKnob, &isTableItem, &isNode, &view, &dimension)) {
            AnimItemBasePtr itemBase;
            if (isKnob) {
                itemBase = isKnob;
            }
            if (itemBase) {
                KeyFrameSet keysUnderMouse = isNearByKeyframe(itemBase, dimension, view, eventPos);

                if ( !keysUnderMouse.empty() ) {
                    _publicInterface->setCursor(Qt::CrossCursor);
                    return true;
                }
            }
        }
    } // if (treeItem) {

    return false;
} // setDopeSheetCursor


bool
AnimationModuleViewPrivate::isNearByClipRectLeft(const QPointF& zoomCoordPos,
                                           const RectD &clipRect) const
{
    QPointF widgetPos = dopeSheetZoomContext.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = dopeSheetZoomContext.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = dopeSheetZoomContext.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( (widgetPos.x() >= rectx1y1.x() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             ( widgetPos.x() <= rectx1y1.x() ) &&
             (widgetPos.y() <= rectx1y1.y() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() >= rectx2y2.y() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) );
}

bool
AnimationModuleViewPrivate::isNearByClipRectRight(const QPointF& zoomCoordPos,
                                            const RectD &clipRect) const
{
    QPointF widgetPos = dopeSheetZoomContext.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = dopeSheetZoomContext.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = dopeSheetZoomContext.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( ( widgetPos.x() >= rectx2y2.x() ) &&
             (widgetPos.x() <= rectx2y2.x() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() <= rectx1y1.y() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() >= rectx2y2.y() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) );
}

bool
AnimationModuleViewPrivate::isNearByClipRectBottom(const QPointF& zoomCoordPos,
                                             const RectD &clipRect) const
{
    QPointF widgetPos = dopeSheetZoomContext.toWidgetCoordinates( zoomCoordPos.x(), zoomCoordPos.y() );
    QPointF rectx1y1 = dopeSheetZoomContext.toWidgetCoordinates( clipRect.left(), clipRect.bottom() );
    QPointF rectx2y2 = dopeSheetZoomContext.toWidgetCoordinates( clipRect.right(), clipRect.top() );

    return ( (widgetPos.x() >= rectx1y1.x() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM)) &&
             (widgetPos.x() <= rectx2y2.x() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_EDGE)) &&
             (widgetPos.y() <= rectx1y1.y() + TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM)) &&
             (widgetPos.y() >= rectx1y1.y() - TO_DPIX(DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM)) );
}

KeyFrameSet AnimationModuleViewPrivate::isNearByKeyframe(const AnimItemBasePtr &item,
                                                                            DimSpec dimension, ViewSetSpec view,
                                                                            const QPointF &widgetCoords) const
{
    KeyFrameSet ret;
    KeyFrameSet keys;
    item->getKeyframes(dimension, view, AnimItemBase::eGetKeyframesTypeMerged, &keys);

    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {

        double keyframeXWidget = dopeSheetZoomContext.toWidgetCoordinates(it->getTime(), 0).x();

        if (std::abs( widgetCoords.x() - keyframeXWidget ) < TO_DPIX(DISTANCE_ACCEPTANCE_FROM_KEYFRAME)) {
            ret.insert(*it);
        }
    }

    return ret;
}

void
AnimationModuleViewPrivate::drawDopeSheetScale() const
{
    QPointF bottomLeft = dopeSheetZoomContext.toZoomCoordinates(0, _publicInterface->height() - 1);
    QPointF topRight = dopeSheetZoomContext.toZoomCoordinates(_publicInterface->width() - 1, 0);

    // Don't attempt to draw a scale on a widget with an invalid height
    if (_publicInterface->height() <= 1) {
        return;
    }

    QFontMetrics fontM(_publicInterface->font());
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.

    // Retrieve the appropriate settings for drawing
    double gridR, gridG, gridB;
    SettingsPtr settings = appPTR->getCurrentSettings();
    settings->getAnimationModuleEditorGridColor(&gridR, &gridG, &gridB);

    double scaleR, scaleG, scaleB;
    settings->getAnimationModuleEditorScaleColor(&scaleR, &scaleG, &scaleB);

    QColor scaleColor;
    scaleColor.setRgbF( Image::clamp(scaleR, 0., 1.),
                        Image::clamp(scaleG, 0., 1.),
                        Image::clamp(scaleB, 0., 1.) );

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const double rangePixel = _publicInterface->width();
        const double range_min = bottomLeft.x();
        const double range_max = topRight.x();
        const double range = range_max - range_min;
        double smallTickSize;
        bool half_tick;

        ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);
        // nothing happens in the dopesheet at a scale smaller than 1 frame
        if (smallTickSize < 1.) {
            smallTickSize = 1;
            half_tick = false;
        }
        int m1, m2;
        const int ticks_max = 1000;
        double offset;

        ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
        std::vector<int> ticks;
        ticks_fill(half_tick, ticks_max, m1, m2, &ticks);

        const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        const double largestTickSize = range * largestTickSizePixel / rangePixel;
        const double minTickSizeTextPixel = fontM.width( QString::fromUtf8("00") );
        const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;

        glCheckError(GL_GPU);

        for (int i = m1; i <= m2; ++i) {
            double value = i * smallTickSize + offset;
            const double tickSize = ticks[i - m1] * smallTickSize;
            const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

            GL_GPU::Color4f(gridR, gridG, gridB, alpha);

            // Draw the vertical lines belonging to the grid
            GL_GPU::Begin(GL_LINES);
            GL_GPU::Vertex2f( value, bottomLeft.y() );
            GL_GPU::Vertex2f( value, topRight.y() );
            GL_GPU::End();

            glCheckErrorIgnoreOSXBug(GL_GPU);

            // Draw the time indicators
            if (tickSize > minTickSizeText) {
                const int tickSizePixel = rangePixel * tickSize / range;
                const QString s = QString::number(value);
                const int sSizePixel = fontM.width(s);

                if (tickSizePixel > sSizePixel) {
                    const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                    double alphaText = 1.0; //alpha;

                    if (tickSizePixel < sSizeFullPixel) {
                        // when the text size is between sSizePixel and sSizeFullPixel,
                        // draw it with a lower alpha
                        alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                    }

                    QColor c = scaleColor;
                    c.setAlpha(255 * alphaText);

                    renderText(dopeSheetZoomContext, value, bottomLeft.y(), s, c, _publicInterface->font(), Qt::AlignHCenter);

                    // Uncomment the line below to draw the indicator on top too
                    // parent->renderText(value, topRight.y() - 20, s, c, *font);
                }
            }
        }
    }
} // drawDopeSheetScale


void
AnimationModuleViewPrivate::drawDopeSheetTreeItemRecursive(QTreeWidgetItem* item,  std::list<NodeAnimPtr>* nodesRowsOrdered) const
{
    assert(item);
    if ( item->isHidden() ) {
        return;
    }

    QTreeWidgetItem* parentItem = item->parent();
    if (parentItem && !parentItem->isExpanded()) {
        return;
    }

    AnimatedItemTypeEnum type = (AnimatedItemTypeEnum)item->data(0, QT_ROLE_CONTEXT_TYPE).toInt();
    void* ptr = item->data(0, QT_ROLE_CONTEXT_ITEM_POINTER).value<void*>();
    assert(ptr);
    if (!ptr) {
        return;
    }
    DimSpec dimension = DimSpec(item->data(0, QT_ROLE_CONTEXT_DIM).toInt());
    ViewSetSpec view = ViewSetSpec(item->data(0, QT_ROLE_CONTEXT_VIEW).toInt());

    NodeAnimPtr isNodeAnim;
    TableItemAnimPtr isTableItemAnim;
    KnobAnimPtr isKnobAnim;
    switch (type) {
        case eAnimatedItemTypeCommon:
        case eAnimatedItemTypeFrameRange:
        case eAnimatedItemTypeGroup:
        case eAnimatedItemTypeReader:
        case eAnimatedItemTypeRetime:
        case eAnimatedItemTypeTimeOffset: {
            isNodeAnim = ((NodeAnim*)ptr)->shared_from_this();
        }   break;
        case eAnimatedItemTypeTableItemRoot: {
            isTableItemAnim = ((TableItemAnim*)ptr)->shared_from_this();
        }   break;
        case eAnimatedItemTypeKnobDim:
        case eAnimatedItemTypeKnobRoot:
        case eAnimatedItemTypeKnobView: {
            isKnobAnim = toKnobAnim(((AnimItemBase*)ptr)->shared_from_this());
        }   break;
    }

    if (isNodeAnim) {
        drawDopeSheetNodeRow(item, isNodeAnim);
        nodesRowsOrdered->push_back(isNodeAnim);
    } else if (isKnobAnim) {
        drawDopeSheetKnobRow(item, isKnobAnim, dimension, view);
    } else if (isTableItemAnim) {
        drawDopeSheetTableItemRow(item, isTableItemAnim, type, dimension, view);
    }

    int nChildren = item->childCount();
    for (int i = 0; i < nChildren; ++i) {
        QTreeWidgetItem* child = item->child(i);
        drawDopeSheetTreeItemRecursive(child, nodesRowsOrdered);
    }

} // drawDopeSheetTreeItemRecursive

void
AnimationModuleViewPrivate::drawDopeSheetRows() const
{

    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::list<NodeAnimPtr> nodesAnimOrdered;
        int nTopLevelTreeItems = treeView->topLevelItemCount();
        for (int i = 0; i < nTopLevelTreeItems; ++i) {
            QTreeWidgetItem* topLevelItem = treeView->topLevelItem(i);
            if (!topLevelItem) {
                continue;
            }
            drawDopeSheetTreeItemRecursive(topLevelItem, &nodesAnimOrdered);
            
        }

        // Draw node rows separations
        for (std::list<NodeAnimPtr>::const_iterator it = nodesAnimOrdered.begin(); it != nodesAnimOrdered.end(); ++it) {
            bool isTreeViewTopItem = !treeView->itemAbove( (*it)->getTreeItem() );
            if (!isTreeViewTopItem) {
                drawDopeSheetNodeRowSeparation(*it);
            }
        }

    } // GlProtectAttrib
} // drawDopeSheetRows

QRectF
AnimationModuleViewPrivate::nameItemRectToRowRect(const QRectF &rect) const
{
    QPointF topLeft = rect.topLeft();

    topLeft = dopeSheetZoomContext.toZoomCoordinates( topLeft.x(), topLeft.y() );
    QPointF bottomRight = rect.bottomRight();
    bottomRight = dopeSheetZoomContext.toZoomCoordinates( bottomRight.x(), bottomRight.y() );

    return QRectF( QPointF( dopeSheetZoomContext.left(), topLeft.y() ),
                  QPointF( dopeSheetZoomContext.right(), bottomRight.y() ) );
}

void
AnimationModuleViewPrivate::drawDopeSheetNodeRow(QTreeWidgetItem* /*treeItem*/, const NodeAnimPtr& item) const
{
    QRectF nameItemRect = treeView->visualItemRect( item->getTreeItem() );
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);
    SettingsPtr settings = appPTR->getCurrentSettings();
    double rootR, rootG, rootB, rootA;

    settings->getAnimationModuleEditorRootRowBackgroundColor(&rootR, &rootG, &rootB, &rootA);

    GL_GPU::Color4f(rootR, rootG, rootB, rootA);

    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::Vertex2f( rowRect.left(), rowRect.top() );
    GL_GPU::Vertex2f( rowRect.left(), rowRect.bottom() );
    GL_GPU::Vertex2f( rowRect.right(), rowRect.bottom() );
    GL_GPU::Vertex2f( rowRect.right(), rowRect.top() );
    GL_GPU::End();

    {
        AnimationModulePtr animModel = toAnimationModule(_model.lock());
        if (animModel) {
            NodeAnimPtr group = animModel->getGroupNodeAnim(item);
            if (group) {
                drawDopeSheetGroupOverlay(item, group);
            }
        }
    }

    if ( item->isRangeDrawingEnabled() ) {
        drawDopeSheetRange(item);
    }

}

void
AnimationModuleViewPrivate::drawDopeSheetTableItemRow(QTreeWidgetItem* /*treeItem*/, const TableItemAnimPtr& /*item*/, AnimatedItemTypeEnum type, DimSpec /*dimension*/, ViewSetSpec /*view*/) const
{
    if (type == eAnimatedItemTypeTableItemRoot) {
#pragma message WARN("Todo when enabling lifetime table items")
    }
}

void
AnimationModuleViewPrivate::drawDopeSheetKnobRow(QTreeWidgetItem* treeItem, const KnobAnimPtr& item, DimSpec dimension, ViewSetSpec view) const
{
    drawDopeSheetAnimItemRow(treeItem, item, dimension, view);
}

void
AnimationModuleViewPrivate::drawDopeSheetAnimItemRow(QTreeWidgetItem* treeItem, const AnimItemBasePtr& item, DimSpec dimension, ViewSetSpec view) const
{

    AnimationModuleBasePtr model = _model.lock();
    bool drawdimed = !model->isCurveVisible(item, dimension, view);
    drawDopeSheetKeyframes(treeItem, item, dimension, view, drawdimed);
}


void
AnimationModuleViewPrivate::drawDopeSheetNodeRowSeparation(const NodeAnimPtr item) const
{
    QRectF nameItemRect = treeView->visualItemRect( item->getTreeItem() );
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    GL_GPU::LineWidth(TO_DPIY(NATRON_ANIMATION_TREE_VIEW_NODE_SEPARATOR_PX));
    GL_GPU::Color4f(0.f, 0.f, 0.f, 1.f);

    GL_GPU::Begin(GL_LINES);
    GL_GPU::Vertex2f( rowRect.left(), rowRect.top() );
    GL_GPU::Vertex2f( rowRect.right(), rowRect.top() );
    GL_GPU::End();
    GL_GPU::LineWidth(1.);
}

void
AnimationModuleViewPrivate::drawDopeSheetRange(const NodeAnimPtr &item) const
{
    // Draw the clip
    if (!item->isRangeDrawingEnabled()) {
        return;
    }
    {
        RangeD range = item->getFrameRange();

        QRectF treeItemRect = treeView->visualItemRect( item->getTreeItem() );
        QPointF treeRectTopLeft = treeItemRect.topLeft();
        treeRectTopLeft = dopeSheetZoomContext.toZoomCoordinates( treeRectTopLeft.x(), treeRectTopLeft.y() );
        QPointF treeRectBtmRight = treeItemRect.bottomRight();
        treeRectBtmRight = dopeSheetZoomContext.toZoomCoordinates( treeRectBtmRight.x(), treeRectBtmRight.y() );

        RectD clipRectZoomCoords;
        clipRectZoomCoords.x1 = range.min;
        clipRectZoomCoords.x2 = range.max;
        clipRectZoomCoords.y2 = treeRectTopLeft.y();
        clipRectZoomCoords.y1 = treeRectBtmRight.y();
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT);
        QColor fillColor = item->getNodeGui()->getCurrentColor();
        fillColor = QColor::fromHsl( fillColor.hslHue(), 50, fillColor.lightness() );

        bool isSelected = _model.lock()->getSelectionModel()->isNodeSelected(item);


        // If necessary, draw the original frame range line
        float clipRectCenterY = 0.;
        if ( isSelected && (item->getItemType() == eAnimatedItemTypeReader) ) {
            NodePtr node = item->getInternalNode();

            KnobIntBase *firstFrameKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameFirstFrame).get() );
            assert(firstFrameKnob);

            double speedValue = 1.0f;

            KnobIntBase *originalFrameRangeKnob = dynamic_cast<KnobIntBase *>( node->getKnobByName(kReaderParamNameOriginalFrameRange).get() );
            assert(originalFrameRangeKnob);

            int originalFirstFrame = originalFrameRangeKnob->getValue(DimIdx(0));
            int originalLastFrame = originalFrameRangeKnob->getValue(DimIdx(1));
            int firstFrame = firstFrameKnob->getValue();
            int lineBegin = range.min - (firstFrame - originalFirstFrame);
            int frameCount = originalLastFrame - originalFirstFrame + 1;
            int lineEnd = lineBegin + (frameCount / speedValue);

            clipRectCenterY = (clipRectZoomCoords.y1 + clipRectZoomCoords.y2) / 2.;

            GL_GPU::LineWidth(2);

            GL_GPU::Color4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);

            GL_GPU::Begin(GL_LINES);

            //horizontal line
            GL_GPU::Vertex2f(lineBegin, clipRectCenterY);
            GL_GPU::Vertex2f(lineEnd, clipRectCenterY);

            //left end
            GL_GPU::Vertex2d(lineBegin, clipRectZoomCoords.y1);
            GL_GPU::Vertex2d(lineBegin, clipRectZoomCoords.y2);


            //right end
            GL_GPU::Vertex2d(lineEnd, clipRectZoomCoords.y1);
            GL_GPU::Vertex2d(lineEnd, clipRectZoomCoords.y2);

            GL_GPU::End();
            GL_GPU::LineWidth(1);
        }

        QColor fadedColor;
        fadedColor.setRgbF(fillColor.redF() * 0.5, fillColor.greenF() * 0.5, fillColor.blueF() * 0.5);
        // Fill the range rect


        GL_GPU::Color4f(fadedColor.redF(), fadedColor.greenF(), fadedColor.blueF(), 1.f);

        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.top() );
        GL_GPU::Vertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.bottom() );
        GL_GPU::Vertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.bottom() );
        GL_GPU::Vertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.top() );
        GL_GPU::End();

        if (isSelected) {
            GL_GPU::Color4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);
            GL_GPU::Begin(GL_LINE_LOOP);
            GL_GPU::Vertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.top() );
            GL_GPU::Vertex2f( clipRectZoomCoords.left(), clipRectZoomCoords.bottom() );
            GL_GPU::Vertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.bottom() );
            GL_GPU::Vertex2f( clipRectZoomCoords.right(), clipRectZoomCoords.top() );
            GL_GPU::End();
        }

        if ( isSelected && (item->getItemType() == eAnimatedItemTypeReader) ) {
            SettingsPtr settings = appPTR->getCurrentSettings();
            double selectionColorRGB[3];
            settings->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
            QColor selectionColor;
            selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);

            QFontMetrics fm(_publicInterface->font());
            int fontHeigt = fm.height();
            QString leftText = QString::number(range.min);
            QString rightText = QString::number(range.max - 1);
            int rightTextW = fm.width(rightText);
            QPointF textLeftPos( dopeSheetZoomContext.toZoomCoordinates(dopeSheetZoomContext.toWidgetCoordinates(range.min, 0).x() + 3, 0).x(),
                                 dopeSheetZoomContext.toZoomCoordinates(0, dopeSheetZoomContext.toWidgetCoordinates(0, clipRectCenterY).y() + fontHeigt / 2.).y() );

            renderText(dopeSheetZoomContext, textLeftPos.x(), textLeftPos.y(), leftText, selectionColor, _publicInterface->font());

            QPointF textRightPos( dopeSheetZoomContext.toZoomCoordinates(dopeSheetZoomContext.toWidgetCoordinates(range.max, 0).x() - rightTextW - 3, 0).x(),
                                  dopeSheetZoomContext.toZoomCoordinates(0, dopeSheetZoomContext.toWidgetCoordinates(0, clipRectCenterY).y() + fontHeigt / 2.).y() );

            renderText(dopeSheetZoomContext, textRightPos.x(), textRightPos.y(), rightText, selectionColor, _publicInterface->font());
        }
    }
} // drawDopeSheetRange


void
AnimationModuleViewPrivate::drawDopeSheetKeyframes(QTreeWidgetItem* treeItem, const AnimItemBasePtr &item, DimSpec dimension, ViewSetSpec view, bool drawdimed) const
{


    double selectionColorRGB[3];
    QColor selectionColor;
    appPTR->getCurrentSettings()->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
    selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);


    double rowCenterYCanonical = dopeSheetZoomContext.toZoomCoordinates(0, treeView->visualItemRect(treeItem).center().y()).y();


    AnimationModulePtr animModel = toAnimationModule(_model.lock());
    AnimationModuleSelectionModelPtr selectModel = animModel->getSelectionModel();

    double singleSelectedTime;
    bool hasSingleKfTimeSelected = selectModel->hasSingleKeyFrameTimeSelected(&singleSelectedTime);

    KeyFrameSet dimViewKeys;
    item->getKeyframes(dimension, view, AnimItemBase::eGetKeyframesTypeMerged, &dimViewKeys);

    QRectF nameItemRect = treeView->visualItemRect(treeItem);
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    {
        double bkR, bkG, bkB, bkA;

        // If the item has children that means we are either drawing a root or view row.
        // Also if the item does not have any keyframe, do use the same background.
        if ( treeItem->childCount() > 0 || dimViewKeys.empty()) {
            appPTR->getCurrentSettings()->getAnimationModuleEditorRootRowBackgroundColor(&bkR, &bkG, &bkB, &bkA);
        } else {
            appPTR->getCurrentSettings()->getAnimationModuleEditorKnobRowBackgroundColor(&bkR, &bkG, &bkB, &bkA);
        }

        GL_GPU::Color4f(bkR, bkG, bkB, bkA);

        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( rowRect.left(), rowRect.top() );
        GL_GPU::Vertex2f( rowRect.left(), rowRect.bottom() );
        GL_GPU::Vertex2f( rowRect.right(), rowRect.bottom() );
        GL_GPU::Vertex2f( rowRect.right(), rowRect.top() );
        GL_GPU::End();
    }


    for (KeyFrameSet::const_iterator it = dimViewKeys.begin(); it != dimViewKeys.end(); ++it) {

        const TimeValue keyTime = it->getTime();
        RectD zoomKfRect = getKeyFrameBoundingRectCanonical(dopeSheetZoomContext, keyTime, rowCenterYCanonical);

        bool isKeyFrameSelected = selectModel->isKeyframeSelected(item, dimension, view, TimeValue(keyTime));
        bool drawSelected = isKeyFrameSelected;
        if (!drawSelected) {
            // If not selected but within the selection rectangle, draw also selected
            if (!dimension.isAll() && !view.isAll()) {
                drawSelected = !eventTriggeredFromCurveEditor && !selectionRect.isNull() && selectionRect.intersects(zoomKfRect);
            }
        }

        AnimationModuleViewPrivate::KeyframeTexture texType;
        bool drawMaster;
        if (view.isAll()) {
            drawMaster = true;
        } else if (dimension.isAll()) {
            drawMaster = item->getAllDimensionsVisible(ViewIdx(view));
        } else {
            drawMaster = false;
        }
        if (drawMaster) {
            texType = drawSelected ? AnimationModuleViewPrivate::kfTextureMasterSelected : AnimationModuleViewPrivate::kfTextureMaster;
        } else {
            texType = AnimationModuleViewPrivate::kfTextureFromKeyframeType( it->getInterpolation(), drawSelected);
        }

        if (texType != AnimationModuleViewPrivate::kfTextureNone) {
            drawTexturedKeyframe(texType, zoomKfRect, drawdimed);
            if (hasSingleKfTimeSelected && isKeyFrameSelected) {
                // Also draw time if single key selected
                drawKeyFrameTime(dopeSheetZoomContext, keyTime, selectionColor, zoomKfRect);
            }
        }

    } // for all keyframes

    // Draw selection highlight
    bool itemSelected = false;
    QList<QTreeWidgetItem*> selectedItems = animModel->getEditor()->getTreeView()->selectedItems();
    for (QList<QTreeWidgetItem*>::const_iterator it = selectedItems.begin(); it!=selectedItems.end(); ++it) {
        if (*it == treeItem) {
            itemSelected = true;
            break;
        }
    }
    if (itemSelected) {
        GL_GPU::Color4f(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2], 0.15);

        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( rowRect.left(), rowRect.top() );
        GL_GPU::Vertex2f( rowRect.left(), rowRect.bottom() );
        GL_GPU::Vertex2f( rowRect.right(), rowRect.bottom() );
        GL_GPU::Vertex2f( rowRect.right(), rowRect.top() );
        GL_GPU::End();
    }

} // drawDopeSheetKeyframes



void
AnimationModuleViewPrivate::drawDopeSheetGroupOverlay(const NodeAnimPtr &item,
                                       const NodeAnimPtr &group) const
{
    // Get the overlay color
    double r, g, b;

    item->getNodeGui()->getColor(&r, &g, &b);

    // Compute the area to fill
    int height = treeView->getHeightForItemAndChildren( item->getTreeItem() );
    QRectF nameItemRect = treeView->visualItemRect( item->getTreeItem() );

    RangeD groupRange = group->getFrameRange();
    RectD overlayRect;
    overlayRect.x1 = groupRange.min;
    overlayRect.x2 = groupRange.max;

    overlayRect.y1 = dopeSheetZoomContext.toZoomCoordinates(0, nameItemRect.top() + height).y();
    overlayRect.y2 = dopeSheetZoomContext.toZoomCoordinates( 0, nameItemRect.top() ).y();

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::Color4f(r, g, b, 0.30f);

        GL_GPU::Begin(GL_QUADS);
        GL_GPU::Vertex2f( overlayRect.left(), overlayRect.top() );
        GL_GPU::Vertex2f( overlayRect.left(), overlayRect.bottom() );
        GL_GPU::Vertex2f( overlayRect.right(), overlayRect.bottom() );
        GL_GPU::Vertex2f( overlayRect.right(), overlayRect.top() );
        GL_GPU::End();
    }
}


void
AnimationModuleViewPrivate::refreshDopeSheetSelectedKeysBRect()
{
    if (!treeView) {
        return;
    }
    if (dopeSheetZoomContext.screenHeight() == 0 || dopeSheetZoomContext.screenWidth() == 0) {
        return;
    }
    AnimationModuleBasePtr model = _model.lock();
    const AnimItemDimViewKeyFramesMap& selectedKeyframes = model->getSelectionModel()->getCurrentKeyFramesSelection();
    const std::list<NodeAnimPtr>& selectedNodes = model->getSelectionModel()->getCurrentNodesSelection();
    const std::list<TableItemAnimPtr>& selectedTableItems = model->getSelectionModel()->getCurrentTableItemsSelection();

    dopeSheetSelectedKeysBRect.clear();

    bool bboxSet = false;
    int nKeysInSelection = 0;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeyframes.begin(); it != selectedKeyframes.end(); ++it) {
        QTreeWidgetItem* treeItem = it->first.item->getTreeItem(it->first.dim, it->first.view);
        if (!treeItem) {
            continue;
        }
        if (!treeView->isItemVisibleRecursive(treeItem)) {
            continue;
        }
        double visualRectCenterYCanonical = dopeSheetZoomContext.toZoomCoordinates(0, treeView->visualItemRect(treeItem).center().y()).y();

        for (KeyFrameSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            ++nKeysInSelection;
            double x = it2->getTime();

            RectD zoomKfRect = getKeyFrameBoundingRectCanonical(dopeSheetZoomContext, x, visualRectCenterYCanonical);

            if (!bboxSet) {
                bboxSet = true;
                dopeSheetSelectedKeysBRect.x1 = zoomKfRect.x1;
                dopeSheetSelectedKeysBRect.x2 = zoomKfRect.x2;
                dopeSheetSelectedKeysBRect.y1 = zoomKfRect.y1;
                dopeSheetSelectedKeysBRect.y2 = zoomKfRect.y2;
            } else {
                dopeSheetSelectedKeysBRect.x1 = std::min(zoomKfRect.x1, dopeSheetSelectedKeysBRect.x1);
                dopeSheetSelectedKeysBRect.x2 = std::max(zoomKfRect.x2, dopeSheetSelectedKeysBRect.x2);
                dopeSheetSelectedKeysBRect.y1 = std::min(zoomKfRect.y1, dopeSheetSelectedKeysBRect.y1);
                dopeSheetSelectedKeysBRect.y2 = std::max(zoomKfRect.y2, dopeSheetSelectedKeysBRect.y2);
            }

        }
        
    }

    for (std::list<NodeAnimPtr>::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {

        if (!(*it)->isRangeDrawingEnabled()) {
            continue;
        }
        RangeD range = (*it)->getFrameRange();
        QTreeWidgetItem* nodeItem = (*it)->getTreeItem();

        QRect itemRect = treeView->visualItemRect(nodeItem);
        if (itemRect.isEmpty()) {
            continue;
        }
        double x1 = range.min;
        double x2 = range.max;
        double y = dopeSheetZoomContext.toZoomCoordinates(0, itemRect.center().y()).y();

        //x1,x2 are in zoom coords
        if (bboxSet) {
            dopeSheetSelectedKeysBRect.x1 = std::min(x1, dopeSheetSelectedKeysBRect.x1);
            dopeSheetSelectedKeysBRect.x2 = std::max(x2, dopeSheetSelectedKeysBRect.x2);
            dopeSheetSelectedKeysBRect.y1 = std::min(y, dopeSheetSelectedKeysBRect.y1);
            dopeSheetSelectedKeysBRect.y2 = std::max(y, dopeSheetSelectedKeysBRect.y2);
        } else {
            bboxSet = true;
            dopeSheetSelectedKeysBRect.x1 = x1;
            dopeSheetSelectedKeysBRect.x2 = x2;
            dopeSheetSelectedKeysBRect.y1 = dopeSheetSelectedKeysBRect.y2 = y;
        }

    }

    for (std::list<TableItemAnimPtr>::const_iterator it = selectedTableItems.begin(); it != selectedTableItems.end(); ++it) {

        if (!(*it)->isRangeDrawingEnabled()) {
            continue;
        }
        RangeD range = (*it)->getFrameRange();
        QTreeWidgetItem* nodeItem = (*it)->getRootItem();

        QRect itemRect = treeView->visualItemRect(nodeItem);
        if (itemRect.isEmpty()) {
            continue;
        }
        double x1 = range.min;
        double x2 = range.max;
        double y = dopeSheetZoomContext.toZoomCoordinates(0, itemRect.center().y()).y();

        //x1,x2 are in zoom coords
        if (bboxSet) {
            dopeSheetSelectedKeysBRect.x1 = std::min(x1, dopeSheetSelectedKeysBRect.x1);
            dopeSheetSelectedKeysBRect.x2 = std::max(x2, dopeSheetSelectedKeysBRect.x2);
            dopeSheetSelectedKeysBRect.y1 = std::min(y, dopeSheetSelectedKeysBRect.y1);
            dopeSheetSelectedKeysBRect.y2 = std::max(y, dopeSheetSelectedKeysBRect.y2);
        } else {
            bboxSet = true;
            dopeSheetSelectedKeysBRect.x1 = x1;
            dopeSheetSelectedKeysBRect.x2 = x2;
            dopeSheetSelectedKeysBRect.y1 = dopeSheetSelectedKeysBRect.y2 = y;
        }
        
    }


    if ( dopeSheetSelectedKeysBRect.isNull() ) {
        dopeSheetSelectedKeysBRect.clear();
        return;
    }

    // Adjust the bbox to match the top/bottom items edges
    {
        double y1Widget = dopeSheetZoomContext.toWidgetCoordinates(0, dopeSheetSelectedKeysBRect.y1).y();
        QTreeWidgetItem *bottomMostItem = treeView->itemAt(0, y1Widget);
        if (bottomMostItem) {
            double bottom = treeView->visualItemRect(bottomMostItem).bottom();
            bottom = dopeSheetZoomContext.toZoomCoordinates(0, bottom).y();
            dopeSheetSelectedKeysBRect.y1 = bottom;
        }
        double y2Widget = dopeSheetZoomContext.toWidgetCoordinates(0, dopeSheetSelectedKeysBRect.y2).y();
        QTreeWidgetItem *topMostItem = treeView->itemAt(0, y2Widget);
        if (topMostItem) {
            double top = treeView->visualItemRect(topMostItem).top();
            top = dopeSheetZoomContext.toZoomCoordinates(0, top).y();

            dopeSheetSelectedKeysBRect.y2 = top;
        }
    }
    
    if ( !dopeSheetSelectedKeysBRect.isNull() ) {
        /// Adjust the bounding rect of the size of a keyframe
        double leftWidget = dopeSheetZoomContext.toWidgetCoordinates(dopeSheetSelectedKeysBRect.x1, 0).x();
        double halfKeySize = TO_DPIX(getKeyframeTextureSize() / 2.);
        double leftAdjustedZoom = dopeSheetZoomContext.toZoomCoordinates(leftWidget - halfKeySize, 0).x();
        double xAdjustOffset = (dopeSheetSelectedKeysBRect.x1 - leftAdjustedZoom);

        dopeSheetSelectedKeysBRect.x1 -= xAdjustOffset;
        dopeSheetSelectedKeysBRect.x2 += xAdjustOffset;
    } else {
        dopeSheetSelectedKeysBRect.clear();
    }
} // computeSelectedKeysBRect

void
AnimationModuleViewPrivate::computeRangesBelow(const NodeAnimPtr& node)
{
    std::vector<NodeAnimPtr> allNodes;
    _model.lock()->getTopLevelNodes(true, &allNodes);

    double thisNodeY = treeView->visualItemRect( node->getTreeItem() ).y();
    for (std::vector<NodeAnimPtr>::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        if ( treeView->visualItemRect((*it)->getTreeItem()).y() >=  thisNodeY) {
            (*it)->refreshFrameRange();
        }
    }
}

void
AnimationModuleViewPrivate::checkAnimItemInRectInternal(const RectD& canonicalRect, QTreeWidgetItem* item, const AnimItemBasePtr& knob, ViewIdx view, DimIdx dimension, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* /*selectedNodes*/, std::vector<TableItemAnimPtr>* /*selectedItems*/)
{
    CurvePtr curve = knob->getCurve(dimension, view);
    if (!curve) {
        return;
    }

    KeyFrameSet set = curve->getKeyFrames_mt_safe();
    if ( set.empty() ) {
        return;
    }


    AnimItemDimViewIndexID id(knob, view, dimension);

    double visualRectCenterYCanonical = dopeSheetZoomContext.toZoomCoordinates(0, treeView->visualItemRect(item).center().y()).y();
    KeyFrameSet outKeys;
    for ( KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
        double x = it2->getTime();
        RectD zoomKfRect = getKeyFrameBoundingRectCanonical(dopeSheetZoomContext, x, visualRectCenterYCanonical);

        if ( canonicalRect.intersects(zoomKfRect) ) {
            outKeys.insert(*it2);
        }
    }
    if (!outKeys.empty()) {
        (*result)[id] = outKeys;
    }

} // checkAnimItemInRectInternal

void
AnimationModuleViewPrivate::checkAnimItemInRect(const RectD& rect, QTreeWidgetItem* item, const AnimItemBasePtr& knob, ViewSetSpec view, DimSpec dimension, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems)
{
    int nDims = knob->getNDimensions();
    if (view.isAll()) {
        std::list<ViewIdx> views = knob->getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            for (int i = 0; i < nDims; ++i) {
                if (!dimension.isAll() && dimension != i) {
                    continue;
                }
                checkAnimItemInRectInternal(rect, item, knob, *it, DimIdx(i), result, selectedNodes, selectedItems);
            }
        }
    } else {
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }
            checkAnimItemInRectInternal(rect, item, knob, ViewIdx(view), DimIdx(i), result, selectedNodes, selectedItems);
        }
    }
} // checkAnimItemInRect


void
AnimationModuleViewPrivate::checkTableItemAnimInRect(const RectD& /*rect*/,QTreeWidgetItem* /*item*/,  const TableItemAnimPtr& /*tableItem*/, AnimatedItemTypeEnum type, ViewSetSpec /*view*/, DimSpec /*dimension*/, AnimItemDimViewKeyFramesMap */*result*/, std::vector<NodeAnimPtr>* /*selectedNodes*/, std::vector<TableItemAnimPtr>* /*selectedItems*/)
{
    if (type == eAnimatedItemTypeTableItemRoot) {
#pragma message WARN("Todo when enabling lifetime for table items")
    }

}


void
AnimationModuleViewPrivate::checkNodeAnimInRect(const RectD& rect, QTreeWidgetItem* item, const NodeAnimPtr& node, AnimItemDimViewKeyFramesMap * /*result*/, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* /*selectedItems*/)
{


    if (!node->isRangeDrawingEnabled()) {
        return;
    }
    RangeD frameRange = node->getFrameRange();
    QPoint visualRectCenter = treeView->visualItemRect(item).center();
    QPointF center = dopeSheetZoomContext.toZoomCoordinates( visualRectCenter.x(), visualRectCenter.y() );

    if ( rect.contains( (frameRange.min + frameRange.max) / 2., center.y() ) ) {
        selectedNodes->push_back(node);
    }



}

void
AnimationModuleViewPrivate::createSelectionFromRectRecursive(const RectD &rect, QTreeWidgetItem* item, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems)
{
    assert(item);
    if ( item->isHidden() ) {
        return;
    }

    QTreeWidgetItem* parentItem = item->parent();
    if (parentItem && !parentItem->isExpanded()) {
        return;
    }

    AnimatedItemTypeEnum type = (AnimatedItemTypeEnum)item->data(0, QT_ROLE_CONTEXT_TYPE).toInt();
    void* ptr = item->data(0, QT_ROLE_CONTEXT_ITEM_POINTER).value<void*>();
    assert(ptr);
    if (!ptr) {
        return;
    }
    DimSpec dimension = DimSpec(item->data(0, QT_ROLE_CONTEXT_DIM).toInt());
    ViewSetSpec view = ViewSetSpec(item->data(0, QT_ROLE_CONTEXT_VIEW).toInt());

    NodeAnimPtr isNodeAnim;
    TableItemAnimPtr isTableItemAnim;
    KnobAnimPtr isKnobAnim;
    switch (type) {
        case eAnimatedItemTypeCommon:
        case eAnimatedItemTypeFrameRange:
        case eAnimatedItemTypeGroup:
        case eAnimatedItemTypeReader:
        case eAnimatedItemTypeRetime:
        case eAnimatedItemTypeTimeOffset: {
            isNodeAnim = ((NodeAnim*)ptr)->shared_from_this();
        }   break;
        case eAnimatedItemTypeTableItemRoot: {
            isTableItemAnim = ((TableItemAnim*)ptr)->shared_from_this();
        }   break;
        case eAnimatedItemTypeKnobDim:
        case eAnimatedItemTypeKnobRoot:
        case eAnimatedItemTypeKnobView: {
            isKnobAnim = toKnobAnim(((AnimItemBase*)ptr)->shared_from_this());
        }   break;
    }

    if (isNodeAnim) {
        checkNodeAnimInRect(rect, item, isNodeAnim, result, selectedNodes, selectedItems);
    } else if (isKnobAnim) {
        checkAnimItemInRect(rect, item, isKnobAnim, view, dimension, result, selectedNodes, selectedItems);
    } else if (isTableItemAnim) {
        checkTableItemAnimInRect(rect, item, isTableItemAnim, type, view, dimension, result, selectedNodes, selectedItems);
    }

    int nChildren = item->childCount();
    for (int i = 0; i < nChildren; ++i) {
        QTreeWidgetItem* child = item->child(i);
        createSelectionFromRectRecursive(rect, child, result, selectedNodes, selectedItems);
    }
} // createSelectionFromRectRecursive

void
AnimationModuleViewPrivate::makeSelectionFromDopeSheetSelectionRectangle(bool toggleSelection)
{
    AnimItemDimViewKeyFramesMap selectedKeys;
    std::vector<NodeAnimPtr> nodesSelection;
    std::vector<TableItemAnimPtr> tableItemSelection;
    makeSelectionFromDopeSheetSelectionRectangleInternal(selectionRect, &selectedKeys, &nodesSelection, &tableItemSelection);

    AnimationModuleSelectionModel::SelectionTypeFlags sFlags = ( toggleSelection )
    ? AnimationModuleSelectionModel::SelectionTypeToggle
    : AnimationModuleSelectionModel::SelectionTypeAdd;
    sFlags |= AnimationModuleSelectionModel::SelectionTypeRecurse;

    _model.lock()->getSelectionModel()->makeSelection(selectedKeys, tableItemSelection, nodesSelection, sFlags);

    _publicInterface->refreshSelectionBoundingBox();
}

void
AnimationModuleViewPrivate::makeSelectionFromDopeSheetSelectionRectangleInternal(const RectD &canonicalRect,
                                                                                 AnimItemDimViewKeyFramesMap *keys,
                                                                                 std::vector<NodeAnimPtr>* nodes,
                                                                                 std::vector<TableItemAnimPtr>* tableItems)
{
    if (canonicalRect.isNull()) {
        return;
    }
    int nTopLevelTreeItems = treeView->topLevelItemCount();
    for (int i = 0; i < nTopLevelTreeItems; ++i) {
        QTreeWidgetItem* topLevelItem = treeView->topLevelItem(i);
        if (!topLevelItem) {
            continue;
        }
        createSelectionFromRectRecursive(canonicalRect, topLevelItem, keys, nodes, tableItems);

    }

} // createSelectionFromRect


void
AnimationModuleView::onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem *item)
{
    // Compute the range rects of affected items
    AnimationModulePtr animModel = toAnimationModule(_imp->_model.lock());
    {
        NodeAnimPtr isNode;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        AnimatedItemTypeEnum type;
        ViewSetSpec view;
        DimSpec dimension;
        bool found = animModel->findItem(item, &type, &isKnob, &isTableItem, &isNode, &view, &dimension);
        if (!found) {
            return;
        }
        if (isTableItem) {
            isNode = isTableItem->getNode();
        } else if (isKnob) {
            TableItemAnimPtr knobContainer = toTableItemAnim(isKnob->getHolder());
            if (knobContainer) {
                isNode = knobContainer->getNode();
            }
            NodeAnimPtr nodeContainer = toNodeAnim(isKnob->getHolder());
            if (nodeContainer) {
                isNode = nodeContainer;
            }
        }

        if (isNode) {
            _imp->computeRangesBelow(isNode);
        }
    }

    refreshSelectionBboxAndRedraw();
}

void
AnimationModuleView::onAnimationTreeViewScrollbarMoved(int /*value*/)
{
    refreshSelectionBboxAndRedraw();
}


/**
 * @brief DopeSheetView::paintGL
 */
void
AnimationModuleViewPrivate::drawDopeSheetView()
{
    drawDopeSheetScale();
    glCheckError(GL_GPU);
    drawDopeSheetRows();
    glCheckError(GL_GPU);

    if (state == eEventStateSelectionRectDopeSheet) {
        drawSelectionRect();
    }

    if ( !dopeSheetSelectedKeysBRect.isNull()) {
        AnimationModuleBasePtr model = _model.lock();
        AnimationModuleSelectionModelPtr selectionModel = model->getSelectionModel();
        bool drawBbox = false;
        if (selectionModel->getSelectedKeyframesCount() > 1) {
            drawBbox = true;
        } else {
            drawBbox = !selectionModel->getCurrentTableItemsSelection().empty() || !selectionModel->getCurrentNodesSelection().empty();
        }
        if (drawBbox) {
            drawSelectedKeyFramesBbox(false);
        }
    }
}

bool
AnimationModuleViewPrivate::dopeSheetMousePressEvent(QMouseEvent *e)
{
    if (!treeView) {
        return false;
    }

    AnimationModulePtr animModule = toAnimationModule(_model.lock());
    assert(animModule);
    animModule->getEditor()->onInputEventCalled();


    QPointF clickZoomCoords = dopeSheetZoomContext.toZoomCoordinates( e->x(), e->y() );

    if ( buttonDownIsLeft(e) ) {

        if (modCASIsControl(e)) {
            // Left Click + CTRL = Double click for a tablet
            return dopeSheetDoubleClickEvent(e);
        }

        if ( !dopeSheetSelectedKeysBRect.isNull() && isNearbySelectedKeyFramesCrossWidget(dopeSheetZoomContext, dopeSheetSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingKeys;
            return true;
        } else if ( isNearbyTimelineBtmPoly(e->pos()) || isNearbyTimelineTopPoly(e->pos()) ) {
            state = eEventStateDraggingTimeline;
            return true;
        } else if ( !dopeSheetSelectedKeysBRect.isNull() && isNearbyBboxMidLeft(dopeSheetZoomContext, dopeSheetSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingMidLeftBbox;
            return true;
        } else if ( !dopeSheetSelectedKeysBRect.isNull() && isNearbyBboxMidRight(dopeSheetZoomContext, dopeSheetSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingMidRightBbox;
            return true;
        }

        AnimationModuleSelectionModel::SelectionTypeFlags sFlags = AnimationModuleSelectionModel::SelectionTypeAdd;
        if ( !modCASIsShift(e) ) {
            sFlags |= AnimationModuleSelectionModel::SelectionTypeClear;
        }

        // Look for a range node
        std::vector<NodeAnimPtr> topLevelNodes;
        animModule->getTopLevelNodes(true, &topLevelNodes);

        for (std::vector<NodeAnimPtr>::const_iterator it = topLevelNodes.begin(); it != topLevelNodes.end(); ++it) {
            if (!(*it)->isRangeDrawingEnabled()) {
                continue;
            }
            QRectF treeItemRect = treeView->visualItemRect((*it)->getTreeItem());
            RangeD range = (*it)->getFrameRange();
            RectD nodeClipRect;
            nodeClipRect.x1 = range.min;
            nodeClipRect.x2 = range.max;
            nodeClipRect.y2 = dopeSheetZoomContext.toZoomCoordinates( 0, treeItemRect.top() ).y();
            nodeClipRect.y1 = dopeSheetZoomContext.toZoomCoordinates( 0, treeItemRect.bottom() ).y();

            AnimatedItemTypeEnum nodeType = (*it)->getItemType();

            // If we click inside a range, start dragging
            if ( nodeClipRect.contains( clickZoomCoords.x(), clickZoomCoords.y() ) ) {
                AnimItemDimViewKeyFramesMap selectedKeys;
                std::vector<NodeAnimPtr> selectedNodes;
                selectedNodes.push_back(*it);
                std::vector<TableItemAnimPtr> selectedTableItems;
                animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);

                if ( (nodeType == eAnimatedItemTypeGroup) ||
                    ( nodeType == eAnimatedItemTypeReader) ||
                    ( nodeType == eAnimatedItemTypeTimeOffset) ||
                    ( nodeType == eAnimatedItemTypeFrameRange) ||
                    ( nodeType == eAnimatedItemTypeCommon)) {
                    state = eEventStateDraggingKeys;
                    return true;
                }
                break;
            }

            if (nodeType == eAnimatedItemTypeReader) {
                currentEditedReader = *it;
                if ( isNearByClipRectLeft(clickZoomCoords, nodeClipRect) ) {
                    AnimItemDimViewKeyFramesMap selectedKeys;
                    std::vector<NodeAnimPtr> selectedNodes;
                    selectedNodes.push_back(*it);
                    std::vector<TableItemAnimPtr> selectedTableItems;
                    animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);

                    state = eEventStateReaderLeftTrim;
                    return true;
                    break;
                } else if ( isNearByClipRectRight(clickZoomCoords, nodeClipRect) ) {
                    AnimItemDimViewKeyFramesMap selectedKeys;
                    std::vector<NodeAnimPtr> selectedNodes;
                    selectedNodes.push_back(*it);
                    std::vector<TableItemAnimPtr> selectedTableItems;
                    animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);
                    state = eEventStateReaderRightTrim;
                    return true;
                    break;
                } else if ( animModule->canSlipReader(*it) && isNearByClipRectBottom(clickZoomCoords, nodeClipRect) ) {
                    AnimItemDimViewKeyFramesMap selectedKeys;
                    std::vector<NodeAnimPtr> selectedNodes;
                    selectedNodes.push_back(*it);
                    std::vector<TableItemAnimPtr> selectedTableItems;
                    animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);
                    state = eEventStateReaderSlip;
                    return true;
                    break;
                }
            }
        } // for all range nodes


        QTreeWidgetItem *treeItem = treeView->itemAt( 0, e->y() );
        // Did not find a range node, look for keyframes
        if (treeItem) {

            AnimatedItemTypeEnum itemType;
            KnobAnimPtr isKnob;
            NodeAnimPtr isNode;
            TableItemAnimPtr isTableItem;
            ViewSetSpec view;
            DimSpec dimension;
            if (animModule->findItem(treeItem, &itemType, &isKnob, &isTableItem, &isNode, &view, &dimension)) {
                AnimItemBasePtr animItem;
                if (isKnob) {
                    animItem = isKnob;
                }
                if (animItem) {
                    KeyFrameSet keysUnderMouse = isNearByKeyframe(animItem, dimension, view, e->pos());

                    if (!keysUnderMouse.empty()) {
                        sFlags |= AnimationModuleSelectionModel::SelectionTypeRecurse;

                        AnimItemDimViewKeyFramesMap selectedKeys;
                        if (dimension.isAll()) {
                            int nDims = animItem->getNDimensions();
                            if (view.isAll()) {
                                std::list<ViewIdx> views = animItem->getViewsList();
                                for (int i = 0; i < nDims; ++i) {
                                    for (std::list<ViewIdx>::iterator it = views.begin(); it != views.end(); ++it) {
                                        AnimItemDimViewIndexID id(animItem, ViewIdx(*it), DimIdx(i));
                                        selectedKeys[id] = keysUnderMouse;
                                    }
                                }
                            } else {
                                for (int i = 0; i < nDims; ++i) {
                                    AnimItemDimViewIndexID id(animItem, ViewIdx(view), DimIdx(i));
                                    selectedKeys[id] = keysUnderMouse;
                                }
                            }
                        } else {
                            if (view.isAll()) {
                                std::list<ViewIdx> views = animItem->getViewsList();
                                for (std::list<ViewIdx>::iterator it = views.begin(); it != views.end(); ++it) {
                                    AnimItemDimViewIndexID id(animItem, ViewIdx(*it), DimIdx(dimension));
                                    selectedKeys[id] = keysUnderMouse;
                                }
                            } else {
                                AnimItemDimViewIndexID id(animItem, ViewIdx(view), DimIdx(dimension));
                                selectedKeys[id] = keysUnderMouse;
                            }
                        }

                        std::vector<NodeAnimPtr> selectedNodes;
                        std::vector<TableItemAnimPtr> selectedTableItems;
                        animModule->getSelectionModel()->makeSelection(selectedKeys, selectedTableItems, selectedNodes, sFlags);
                        
                        state = eEventStateDraggingKeys;
                        return true;
                    }
                    
                }
            }
        } // if (treeItem) {
        
    } // isLeft
    return false;
}

bool
AnimationModuleViewPrivate::dopeSheetAddKeyFrame(const QPoint& p)
{
    if (!treeView) {
        return false;
    }
    AnimationModuleBasePtr model = _model.lock();

    QTreeWidgetItem *itemUnderPoint = treeView->itemAt(TO_DPIX(5), p.y());
    if (!itemUnderPoint) {
        return false;
    }
    AnimatedItemTypeEnum foundType;
    KnobAnimPtr isKnob;
    TableItemAnimPtr isTableItem;
    NodeAnimPtr isNodeItem;
    ViewSetSpec view;
    DimSpec dim;
    bool found = model->findItem(itemUnderPoint, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
    (void)found;

    AnimItemBasePtr isAnim;
    if (isKnob) {
        isAnim = isKnob;
    } 

    if (!isAnim) {
        return false;
    }

    KnobStringBasePtr isStringKnob;
    if (isKnob) {
        isStringKnob = toKnobStringBase(isKnob->getInternalKnob());
    }
    if (isStringKnob) {
        // Cannot add keys on a string knob from the dopesheet
        return true;
    }
    KeyFrameSet underMouse = isNearByKeyframe(isAnim, dim, view, p);
    if (!underMouse.empty()) {
        // Already  a key
        return false;
    }

    double keyframeTime = std::floor(dopeSheetZoomContext.toZoomCoordinates(p.x(), 0).x() + 0.5);
    moveCurrentFrameIndicator(keyframeTime);

    AnimItemDimViewKeyFramesMap keysToAdd;
    if (dim.isAll()) {
        if (view.isAll()) {
            std::list<ViewIdx> views = isAnim->getViewsList();
            int nDims = isAnim->getNDimensions();
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                for (int i = 0; i < nDims; ++i) {
                    AnimItemDimViewIndexID itemKey(isAnim, *it, DimIdx(i));
                    KeyFrameSet& keys = keysToAdd[itemKey];
                    KeyFrame k;
                    double yCurve = isAnim->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(i), *it);
                    k = KeyFrame(keyframeTime, yCurve);
                    keys.insert(k);
                }
            }
        } else {
            int nDims = isAnim->getNDimensions();
            for (int i = 0; i < nDims; ++i) {
                AnimItemDimViewIndexID itemKey(isAnim, ViewIdx(view), DimIdx(i));
                KeyFrameSet& keys = keysToAdd[itemKey];
                KeyFrame k;
                double yCurve = isAnim->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(i), ViewIdx(view));
                k = KeyFrame(keyframeTime, yCurve);
                keys.insert(k);
            }
        }
    } else {
        if (view.isAll()) {
            std::list<ViewIdx> views = isAnim->getViewsList();
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                AnimItemDimViewIndexID itemKey(isAnim, *it, DimIdx(dim));
                KeyFrameSet& keys = keysToAdd[itemKey];
                KeyFrame k;
                double yCurve = isAnim->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(dim), *it);
                k = KeyFrame(keyframeTime, yCurve);
                keys.insert(k);

            }
        } else {
            AnimItemDimViewIndexID itemKey(isAnim, ViewIdx(view), DimIdx(dim));
            KeyFrameSet& keys = keysToAdd[itemKey];
            KeyFrame k;
            double yCurve = isAnim->evaluateCurve(false /*useExpr*/, keyframeTime, DimIdx(dim), ViewIdx(view));
            k = KeyFrame(keyframeTime, yCurve);
            keys.insert(k);
        }

    }

    model->pushUndoCommand( new AddKeysCommand(keysToAdd, model, false /*clearAndAdd*/) );
    return true;


} // dopeSheetAddKeyFrame

bool
AnimationModuleViewPrivate::dopeSheetDoubleClickEvent(QMouseEvent *e)
{
    return dopeSheetAddKeyFrame(e->pos());
} // mouseDoubleClickEvent

NATRON_NAMESPACE_EXIT
