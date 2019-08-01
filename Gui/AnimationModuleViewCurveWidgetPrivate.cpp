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

#include "CurveWidgetPrivate.h"

#include <cmath> // floor
#include <algorithm> // min, max
#include <stdexcept>

#include <QtCore/QThread>
#include <QApplication>
#include <QtCore/QDebug>

#include "Engine/Bezier.h"
#include "Engine/TimeLine.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h" // Image::clamp
#include "Engine/OSGLFunctions.h"
#include "Engine/Utils.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleViewPrivate.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Menu.h"
#include "Gui/ticks.h"


#define AXIS_MAX 100000.
#define AXIS_MIN -100000.

NATRON_NAMESPACE_ENTER


void
AnimationModuleViewPrivate::drawCurves()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    //now draw each curve
    std::vector<CurveGuiPtr> curves = getVisibleCurves();
    int i = 0;
    int nCurves = (int)curves.size();
    for (std::vector<CurveGuiPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it, ++i) {
        (*it)->drawCurve(i, nCurves);
    }
}

void
AnimationModuleViewPrivate::drawCurveEditorScale()
{
    glCheckError(GL_GPU);
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF btmLeft = curveEditorZoomContext.toZoomCoordinates(0, _publicInterface->height() - 1);
    QPointF topRight = curveEditorZoomContext.toZoomCoordinates(_publicInterface->width() - 1, 0);

    ///don't attempt to draw a scale on a widget with an invalid height/width
    if ( (_publicInterface->height() <= 1) || (_publicInterface->width() <= 1) ) {
        return;
    }

    QFontMetrics fontM = _publicInterface->fontMetrics();
    const double smallestTickSizePixel = 10.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 500.; // tick size (in pixels) for alpha = 1.
    double gridR, gridG, gridB;
    SettingsPtr sett = appPTR->getCurrentSettings();
    sett->getAnimationModuleEditorGridColor(&gridR, &gridG, &gridB);


    double scaleR, scaleG, scaleB;
    sett->getAnimationModuleEditorScaleColor(&scaleR, &scaleG, &scaleB);

    QColor scaleColor;
    scaleColor.setRgbF( Image::clamp(scaleR, 0., 1.),
                        Image::clamp(scaleG, 0., 1.),
                        Image::clamp(scaleB, 0., 1.) );


    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (int axis = 0; axis < 2; ++axis) {
            const double rangePixel = (axis == 0) ? _publicInterface->width() : _publicInterface->height(); // AXIS-SPECIFIC
            const double range_min = (axis == 0) ? btmLeft.x() : btmLeft.y(); // AXIS-SPECIFIC
            const double range_max = (axis == 0) ? topRight.x() : topRight.y(); // AXIS-SPECIFIC
            const double range = range_max - range_min;
            double smallTickSize;
            bool half_tick;
            ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);
            int m1, m2;
            const int ticks_max = 1000;
            double offset;
            ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
            std::vector<int> ticks;
            ticks_fill(half_tick, ticks_max, m1, m2, &ticks);
            const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
            const double largestTickSize = range * largestTickSizePixel / rangePixel;
            const double minTickSizeTextPixel = (axis == 0) ? fontM.width( QLatin1String("00") ) : fontM.height(); // AXIS-SPECIFIC
            const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;
            for (int i = m1; i <= m2; ++i) {
                double value = i * smallTickSize + offset;
                const double tickSize = ticks[i - m1] * smallTickSize;
                const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

                glCheckError(GL_GPU);
                GL_GPU::Color4f(gridR, gridG, gridB, alpha);

                GL_GPU::Begin(GL_LINES);
                if (axis == 0) {
                    GL_GPU::Vertex2f( value, btmLeft.y() ); // AXIS-SPECIFIC
                    GL_GPU::Vertex2f( value, topRight.y() ); // AXIS-SPECIFIC
                } else {
                    GL_GPU::Vertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                    GL_GPU::Vertex2f(topRight.x(), value); // AXIS-SPECIFIC
                }
                GL_GPU::End();
                glCheckErrorIgnoreOSXBug(GL_GPU);

                if (tickSize > minTickSizeText) {
                    const int tickSizePixel = rangePixel * tickSize / range;
                    const QString s = QString::number(value);
                    const int sSizePixel = (axis == 0) ? fontM.width(s) : fontM.height(); // AXIS-SPECIFIC
                    if (tickSizePixel > sSizePixel) {
                        const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                        double alphaText = 1.0; //alpha;
                        if (tickSizePixel < sSizeFullPixel) {
                            // when the text size is between sSizePixel and sSizeFullPixel,
                            // draw it with a lower alpha
                            alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                        }
                        alphaText = std::min(alphaText, alpha); // don't draw more opaque than tcks
                        QColor c = scaleColor;
                        c.setAlpha(255 * alphaText);
                        if (axis == 0) {
                            _publicInterface->renderText(value, btmLeft.y(), s.toStdString(), c.redF(), c.greenF(), c.blueF(), c.alphaF(), Qt::AlignHCenter);
                        } else {
                            _publicInterface->renderText(btmLeft.x(), value, s.toStdString(), c.redF(), c.greenF(), c.blueF(), c.alphaF(), Qt::AlignVCenter);
                        }
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);


    glCheckError(GL_GPU);
    GL_GPU::Color4f(gridR, gridG, gridB, 1.);
    GL_GPU::Begin(GL_LINES);
    GL_GPU::Vertex2f(AXIS_MIN, 0);
    GL_GPU::Vertex2f(AXIS_MAX, 0);
    GL_GPU::Vertex2f(0, AXIS_MIN);
    GL_GPU::Vertex2f(0, AXIS_MAX);
    GL_GPU::End();


    glCheckErrorIgnoreOSXBug(GL_GPU);
} // drawCurveEditorScale


CurveGuiPtr
AnimationModuleViewPrivate::isNearbyCurve(const QPoint &pt, double* x, double *y) const
{
    QPointF curvePos = curveEditorZoomContext.toZoomCoordinates( pt.x(), pt.y() );

    std::vector<CurveGuiPtr> curves = getVisibleCurves();

    for (std::vector<CurveGuiPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it) {

        AnimItemBasePtr item = (*it)->getItem();
        double yCurve = 0;
        
        try {
            yCurve = item->evaluateCurve( true /*useExpression*/, curvePos.x(), (*it)->getDimension(), (*it)->getView());
        } catch (...) {
        }
        
        double yWidget = curveEditorZoomContext.toWidgetCoordinates(0, yCurve).y();
        if ( (pt.y() < yWidget + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) && (pt.y() > yWidget - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) {
            if (x != NULL) {
                *x = curvePos.x();
            }
            if (y != NULL) {
                *y = yCurve;
            }
            
            return *it;
        }
    }

    return CurveGuiPtr();
} // isNearbyCurve

AnimItemDimViewKeyFrame
AnimationModuleViewPrivate::isNearbyKeyFrame(const ZoomContext& ctx, const QPoint & pt) const
{
    AnimItemDimViewKeyFrame ret;
    
    std::vector<CurveGuiPtr> curves = getVisibleCurves();

    for (std::vector<CurveGuiPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it) {

        KeyFrameSet set = (*it)->getKeyFrames();

        for (KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            QPointF keyFramewidgetPos = ctx.toWidgetCoordinates( it2->getTime(), it2->getValue() );
            if ( (std::abs( pt.y() - keyFramewidgetPos.y() ) < TO_DPIX(CLICK_DISTANCE_TOLERANCE)) &&
                (std::abs( pt.x() - keyFramewidgetPos.x() ) < TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) {
                
                ret.key = *it2;
       
                ret.id = (*it)->getCurveID();
                
                return ret;
            }
           
        }

    } // for all curves
    return ret;
} // CurveWidgetPrivate::isNearbyKeyFrame

AnimItemDimViewKeyFrame
AnimationModuleViewPrivate::isNearbyKeyFrameText(const QPoint& pt) const
{
    AnimItemDimViewKeyFrame ret;
    
    QFontMetrics fm( _publicInterface->font() );
    int yOffset = TO_DPIY(4);
    AnimationModuleBasePtr model = _model.lock();
    const AnimItemDimViewKeyFramesMap& selectedKeys = model->getSelectionModel()->getCurrentKeyFramesSelection();
    if (selectedKeys.empty() || selectedKeys.size() > 1) {
        return ret;
    }

    AnimItemDimViewKeyFramesMap::const_iterator curveIT = selectedKeys.begin();
    const KeyFrameSet& keys = curveIT->second;
    if (keys.empty() || keys.size() > 1) {
        return ret;
    }

    const KeyFrame& key = *keys.begin();


    QPointF topLeftWidget = curveEditorZoomContext.toWidgetCoordinates( key.getTime(), key.getValue() );
    topLeftWidget.ry() += yOffset;

    QString coordStr =  QString::fromUtf8("x: %1, y: %2").arg( key.getTime() ).arg( key.getValue() );
    QPointF btmRightWidget( topLeftWidget.x() + fm.width(coordStr), topLeftWidget.y() + fm.height() );

    if ( (pt.x() >= topLeftWidget.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) && (pt.x() <= btmRightWidget.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) &&
        ( pt.y() >= topLeftWidget.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) && ( pt.y() <= btmRightWidget.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) {
        ret.key = key;

        ret.id = curveIT->first;

        return ret;
    }


    return ret;
}

std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame >
AnimationModuleViewPrivate::isNearbyTangent(const QPoint & pt) const
{
    
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > ret;

    std::vector<CurveGuiPtr> curves = getVisibleCurves();

    for (std::vector<CurveGuiPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it) {


        KeyFrameSet set = (*it)->getKeyFrames();
        
        for (KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            
            QPointF leftTanPos, rightTanPos;
            _publicInterface->getKeyTangentPoints(it2, set, &leftTanPos, &rightTanPos);
            QPointF leftTanPt = curveEditorZoomContext.toWidgetCoordinates(leftTanPos.x(), leftTanPos.y());
            QPointF rightTanPt = curveEditorZoomContext.toWidgetCoordinates( rightTanPos.x(), rightTanPos.y());
            if ( ( pt.x() >= (leftTanPt.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
                ( pt.x() <= (leftTanPt.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
                ( pt.y() <= (leftTanPt.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
                ( pt.y() >= (leftTanPt.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
                
                ret.second.key = *it2;

                ret.second.id = (*it)->getCurveID();
                ret.first = MoveTangentCommand::eSelectedTangentLeft;
                return ret;
            } else if ( ( pt.x() >= (rightTanPt.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
                       ( pt.x() <= (rightTanPt.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
                       ( pt.y() <= (rightTanPt.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
                       ( pt.y() >= (rightTanPt.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
                ret.second.key = *it2;

                ret.second.id = (*it)->getCurveID();
                ret.first = MoveTangentCommand::eSelectedTangentRight;
                return ret;
            }
        }
        
    } // for all curves
    return ret;
    
}

std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame >
AnimationModuleViewPrivate::isNearbySelectedTangentText(const QPoint & pt) const
{
    QFontMetrics fm( _publicInterface->font() );
    int yOffset = TO_DPIY(4);
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > ret;
    
    AnimationModuleBasePtr model = _model.lock();
    const AnimItemDimViewKeyFramesMap& selectedKeys = model->getSelectionModel()->getCurrentKeyFramesSelection();
    if (selectedKeys.empty() || selectedKeys.size() > 1) {
        return ret;
    }

    AnimItemDimViewKeyFramesMap::const_iterator curveIT = selectedKeys.begin();
    const KeyFrameSet& keys = curveIT->second;
    if (keys.empty() || keys.size() > 1) {
        return ret;
    }

    const KeyFrame& key = *keys.begin();
    CurvePtr curve = curveIT->first.item->getCurve(curveIT->first.dim, curveIT->first.view);
    if (!curve) {
        return ret;
    }
    KeyFrameSet curveKeys = curve->getKeyFrames_mt_safe();

    KeyFrameSet::iterator foundKey = Curve::findWithTime(curveKeys, curveKeys.begin(), key.getTime());
    assert(foundKey != curveKeys.end());
    if (foundKey == curveKeys.end()) {
        return ret;
    }
    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {


        QPointF leftTanPos, rightTanPos;
        _publicInterface->getKeyTangentPoints(foundKey, curveKeys, &leftTanPos, &rightTanPos);

        double rounding = ipow(10, CURVEWIDGET_DERIVATIVE_ROUND_PRECISION);
        QPointF topLeft_LeftTanWidget = curveEditorZoomContext.toWidgetCoordinates( leftTanPos.x(), leftTanPos.y() );
        QPointF topLeft_RightTanWidget = curveEditorZoomContext.toWidgetCoordinates( rightTanPos.x(), rightTanPos.y() );
        topLeft_LeftTanWidget.ry() += yOffset;
        topLeft_RightTanWidget.ry() += yOffset;

        QString leftCoordStr =  QString( tr("l: %1") ).arg(std::floor( ( key.getLeftDerivative() * rounding ) + 0.5 ) / rounding);
        QString rightCoordStr =  QString( tr("r: %1") ).arg(std::floor( ( key.getRightDerivative() * rounding ) + 0.5 ) / rounding);
        QPointF btmRight_LeftTanWidget( topLeft_LeftTanWidget.x() + fm.width(leftCoordStr), topLeft_LeftTanWidget.y() + fm.height() );
        QPointF btmRight_RightTanWidget( topLeft_RightTanWidget.x() + fm.width(rightCoordStr), topLeft_RightTanWidget.y() + fm.height() );

        if ( (pt.x() >= topLeft_LeftTanWidget.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) && (pt.x() <= btmRight_LeftTanWidget.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) &&
            ( pt.y() >= topLeft_LeftTanWidget.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) && ( pt.y() <= btmRight_LeftTanWidget.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) {
            ret.second.key = key;

            ret.second.id = curveIT->first;
            ret.first = MoveTangentCommand::eSelectedTangentLeft;
        } else if ( (pt.x() >= topLeft_RightTanWidget.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) && (pt.x() <= btmRight_RightTanWidget.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) &&
                   ( pt.y() >= topLeft_RightTanWidget.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) && ( pt.y() <= btmRight_RightTanWidget.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) {
            ret.second.key = key;

            ret.second.id = curveIT->first;
            ret.first = MoveTangentCommand::eSelectedTangentRight;
        }



    } // for all curves
    return ret;

} // isNearbySelectedTangentText


void
AnimationModuleViewPrivate::keyFramesWithinRect(const RectD& canonicalRect, AnimItemDimViewKeyFramesMap* keys) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (canonicalRect.isNull()) {
        return;
    }
    
    std::vector<CurveGuiPtr> curves = getVisibleCurves();

    for (std::vector<CurveGuiPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it) {

        KeyFrameSet set = (*it)->getKeyFrames();
        if ( set.empty() ) {
            continue;
        }

        AnimItemDimViewIndexID curveID = (*it)->getCurveID();

        KeyFrameSet outKeys;
        
        for ( KeyFrameSet::const_iterator it2 = set.begin(); it2 != set.end(); ++it2) {
            double y = it2->getValue();
            double x = it2->getTime();
            if ( (x <= canonicalRect.x2) && (x >= canonicalRect.x1) && (y <= canonicalRect.y2) && (y >= canonicalRect.y1) ) {
                outKeys.insert(*it2);
            }
        }
        if (!outKeys.empty()) {
            (*keys)[curveID] = outKeys;
        }
        
    }
} // CurveWidgetPrivate::keyFramesWithinRect



void
AnimationModuleViewPrivate::moveSelectedTangent(const QPointF & pos)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );



    AnimItemDimViewKeyFrame& key = selectedDerivative.second;
    double dx,dy;

    /*
     CurvePtr curve = key.id.item->getCurve(key.id.dim, key.id.view);
     if (!curve) {
     return;
     }
     KeyFrameSet keysSet = curve->getKeyFrames_mt_safe();
     KeyFrameSet::iterator foundKey = keysSet.find(key.key.key);
     assert(foundKey != keysSet.end());
     if (foundKey == keysSet.end()) {
     return;
     }

     QPointF leftTanPos, rightTanPos;
     _publicInterface->getKeyTangentPoints(foundKey, keysSet, &leftTanPos, &rightTanPos);
     if (selectedDerivative.first == MoveTangentCommand::eSelectedTangentLeft) {
        dx = pos.x() - leftTanPos.x();
        dy = pos.y() - leftTanPos.y();
    } else {
        dx = pos.x() - rightTanPos.x();
        dy = pos.y() - rightTanPos.y();
    }*/

    dy = key.key.getValue() - pos.y();
    dx = key.key.getTime() - pos.x();


    AnimationModuleBasePtr model = _model.lock();
    model->pushUndoCommand(new MoveTangentCommand(model, selectedDerivative.first, key, dx, dy));
} // moveSelectedTangent

void
AnimationModuleViewPrivate::makeSelectionFromCurveEditorSelectionRectangle(bool toggleSelection)
{

    AnimationModuleBasePtr model = _model.lock();
    AnimationModuleSelectionModelPtr selectModel = model->getSelectionModel();

    AnimItemDimViewKeyFramesMap selectedKeys;
    keyFramesWithinRect(selectionRect, &selectedKeys);

    AnimationModuleSelectionModel::SelectionTypeFlags sFlags = ( toggleSelection )
    ? AnimationModuleSelectionModel::SelectionTypeToggle
    : AnimationModuleSelectionModel::SelectionTypeAdd;
    sFlags |= AnimationModuleSelectionModel::SelectionTypeRecurse;

    selectModel->makeSelection( selectedKeys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>(), sFlags );
    _publicInterface->refreshSelectionBoundingBox();
}

void
AnimationModuleViewPrivate::refreshSelectionRectangle(const ZoomContext& ctx, double x, double y)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF dragStartPointCanonical = ctx.toZoomCoordinates(dragStartPoint.x(),dragStartPoint.y());

    // x and y are in canonical coordinates as well

    selectionRect.x1 = std::min(dragStartPointCanonical.x(), x);
    selectionRect.x2 = std::max(dragStartPointCanonical.x(), x);
    selectionRect.y1 = std::min(dragStartPointCanonical.y(), y);
    selectionRect.y2 = std::max(dragStartPointCanonical.y(), y);

}


void
AnimationModuleViewPrivate::addMenuCurveEditorMenuOptions(Menu* menu)
{
    Menu* fileMenu = new Menu(menu);
    //fileMenu->setFont( QFont(appFont,appFontSize) );
    fileMenu->setTitle( tr("File") );
    menu->addAction( fileMenu->menuAction() );

    AnimationModulePtr isAnimModule = toAnimationModule(_model.lock());

    Menu* predefMenu  = 0;
    if (isAnimModule) {
        predefMenu = new Menu(menu);
        predefMenu->setTitle( tr("Predefined") );
        menu->addAction( predefMenu->menuAction() );
    }

    


    QAction* exportCurveToAsciiAction = new QAction(tr("Export curve to Ascii file..."), fileMenu);
    QObject::connect( exportCurveToAsciiAction, SIGNAL(triggered()), _publicInterface, SLOT(onExportCurveToAsciiActionTriggered()) );
    fileMenu->addAction(exportCurveToAsciiAction);

    QAction* importCurveFromAsciiAction = new QAction(tr("Import curve from Ascii file..."), fileMenu);
    QObject::connect( importCurveFromAsciiAction, SIGNAL(triggered()), _publicInterface, SLOT(onImportCurveFromAsciiActionTriggered()) );
    fileMenu->addAction(importCurveFromAsciiAction);


    if (predefMenu) {
        QAction* loop = new QAction(tr("Loop"), menu);
        QObject::connect( loop, SIGNAL(triggered()), _publicInterface, SLOT(onApplyLoopExpressionOnSelectedCurveActionTriggered()) );
        predefMenu->addAction(loop);

        QAction* reverse = new QAction(tr("Reverse"), menu);
        QObject::connect( reverse, SIGNAL(triggered()), _publicInterface, SLOT(onApplyReverseExpressionOnSelectedCurveActionTriggered()) );
        predefMenu->addAction(reverse);


        QAction* negate = new QAction(tr("Negate"), menu);
        QObject::connect( negate, SIGNAL(triggered()), _publicInterface, SLOT(onApplyNegateExpressionOnSelectedCurveActionTriggered()) );
        predefMenu->addAction(negate);
    }
} // addMenuCurveEditorMenuOptions

bool
AnimationModuleViewPrivate::setCurveEditorCursor(const QPoint& eventPos)
{
    if (!curveEditorSelectedKeysBRect.isNull()) {
        if ( isNearbySelectedKeyFramesCrossWidget(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeAllCursor) );
            return true;
        } else if ( isNearbyBboxMidBtm(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeVerCursor) );
            return true;
        } else if ( isNearbyBboxMidTop(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeVerCursor) );
            return true;
        } else if ( isNearbyBboxMidLeft(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeHorCursor) );
            return true;
        } else if ( isNearbyBboxMidRight(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeHorCursor) );
            return true;
        } else if ( isNearbyBboxTopLeft(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeFDiagCursor) );
            return true;
        } else if ( isNearbyBboxBtmRight(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeFDiagCursor) );
            return true;
        } else if ( isNearbyBboxTopRight(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeBDiagCursor) );
            return true;
        } else if ( isNearbyBboxBtmLeft(curveEditorZoomContext, curveEditorSelectedKeysBRect, eventPos) ) {
            _publicInterface->setCursor( QCursor(Qt::SizeBDiagCursor) );
            return true;
        }
    }

    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > selectedTan = isNearbyTangent(eventPos);
    AnimItemDimViewKeyFrame nearbyKey = isNearbyKeyFrame(curveEditorZoomContext, eventPos);

    // if there's a keyframe or derivative handle nearby set the cursor to cross
    if (nearbyKey.id.item || selectedTan.second.id.item) {
        _publicInterface->setCursor( QCursor(Qt::CrossCursor) );
        return true;
    }

    AnimItemDimViewKeyFrame nearbyKeyText = isNearbyKeyFrameText(eventPos);
    if (nearbyKeyText.id.item) {
        _publicInterface->setCursor( QCursor(Qt::IBeamCursor) );
        return true;
    }
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame> tangentText = isNearbySelectedTangentText(eventPos);
    if (tangentText.second.id.item) {
        _publicInterface->setCursor( QCursor(Qt::IBeamCursor) );
        return true;
    }
    // if we're nearby a timeline polygon, set cursor to horizontal displacement
    if ( isNearbyTimelineBtmPoly(eventPos) || isNearbyTimelineTopPoly(eventPos) ) {
        _publicInterface->setCursor( QCursor(Qt::SizeHorCursor) );
        return true;
    }
    // default case
    //_publicInterface->unsetCursor();

    return false;

} // setCursor


NATRON_NAMESPACE_EXIT

