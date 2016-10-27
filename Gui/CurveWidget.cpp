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

#include "CurveWidget.h"

#include <cmath> // floor
#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QApplication>
#include <QToolButton>
#include <QDesktopWidget>

#include "Engine/Bezier.h"
#include "Engine/PyParameter.h" // IntParam
#include "Engine/Project.h"
#include "Engine/Image.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"

#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidgetDialogs.h"
#include "Gui/CurveWidgetPrivate.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobAnim.h"

#include "Gui/KnobGui.h"
#include "Gui/PythonPanels.h" // PyModelDialog
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"

NATRON_NAMESPACE_ENTER;

CurveWidget::CurveWidget(QWidget* parent)
    : AnimationViewBase(parent)
{

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    if ( parent->objectName() == QString::fromUtf8("CurveEditorSplitter") ) {
        ///if this is the curve widget associated to the CurveEditor
        //        QDesktopWidget* desktop = QApplication::desktop();
        //        _imp->sizeH = desktop->screenGeometry().size();
        _imp->sizeH = QSize(10000, 10000);
    } else {
        ///a random parametric param curve editor
        _imp->sizeH =  QSize(400, 400);
    }
}

CurveWidget::~CurveWidget()
{
 
}

void
CurveWidget::initializeImplementation(Gui* gui, AnimationModuleBasePtr& model, AnimationViewBase* publicInterface)
{
    _imp.reset(new CurveWidgetPrivate(gui, model, publicInterface));
    _imp->_widget = this;
    setImplementation(_imp);
}

void
CurveWidget::centerOnCurves(const std::vector<CurveGuiPtr> & curves, bool useDisplayRange)
{

    // First try to center curves given their display range
    Curve::YRange displayRange(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    std::pair<double, double> xRange = std::make_pair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    bool rangeSet = false;

    std::vector<CurveGuiPtr> curvesToFrame;
    if (curves.empty()) {
        // Frame all curves selected in the tree view
        const AnimItemDimViewKeyFramesMap& selectedKeys = _imp->_model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();
        for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
            CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
            if (!guiCurve) {
                continue;
            }
            curvesToFrame.push_back(guiCurve);
        }
    } else {
        curvesToFrame = curves;
    }

    if (curvesToFrame.empty()) {
        return;
    }

    if (useDisplayRange) {
        for (std::vector<boost::shared_ptr<CurveGui> > ::const_iterator it = curvesToFrame.begin(); it != curvesToFrame.end(); ++it) {
            boost::shared_ptr<Curve> curve = (*it)->getInternalCurve();
            if (!curve) {
                continue;
            }
            Curve::YRange thisCurveRange = curve->getCurveDisplayYRange();
            std::pair<double, double> thisXRange = curve->getXRange();


            if (thisCurveRange.min == -std::numeric_limits<double>::infinity() ||
                thisCurveRange.min == INT_MIN ||
                thisCurveRange.max == std::numeric_limits<double>::infinity() ||
                thisCurveRange.max == INT_MAX ||
                thisXRange.first == -std::numeric_limits<double>::infinity() ||
                thisXRange.first == INT_MIN ||
                thisXRange.second == std::numeric_limits<double>::infinity() ||
                thisXRange.second == INT_MAX) {
                continue;
            }


            if (!rangeSet) {
                displayRange = thisCurveRange;
                xRange = thisXRange;
                rangeSet = true;
            } else {
                displayRange.min = std::min(displayRange.min, thisCurveRange.min);
                displayRange.max = std::min(displayRange.max, thisCurveRange.max);
                xRange.first = std::min(xRange.first, thisXRange.first);
                xRange.second = std::min(xRange.second, thisXRange.second);
            }
        } // for all curves

        if (rangeSet) {
            double paddingX = (xRange.second - xRange.first) / 20.;
            double paddingY = (displayRange.max - displayRange.min) / 20.;
            AnimationViewBase::centerOn(xRange.first - paddingX, xRange.second + paddingX, displayRange.min - paddingY, displayRange.max + paddingY);
            return;
        }
    } // useDisplayRange
    
    // If no range, center them using the bounding box of keyframes
    
    bool doCenter = false;
    RectD ret;
    for (U32 i = 0; i < curves.size(); ++i) {
        const CurveGuiPtr& c = curves[i];
        KeyFrameSet keys = c->getKeyFrames();

        if ( keys.empty() ) {
            continue;
        }
        doCenter = true;
        double xmin = keys.begin()->getTime();
        double xmax = keys.rbegin()->getTime();
        double ymin = INT_MAX;
        double ymax = INT_MIN;
        //find out ymin,ymax
        for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            double value = it2->getValue();
            if (value < ymin) {
                ymin = value;
            }
            if (value > ymax) {
                ymax = value;
            }
        }
        ret.merge(xmin, ymin, xmax, ymax);
    }
    ret.set_bottom(ret.bottom() - ret.height() / 10);
    ret.set_left(ret.left() - ret.width() / 10);
    ret.set_right(ret.right() + ret.width() / 10);
    ret.set_top(ret.top() + ret.height() / 10);
    if ( doCenter && !ret.isNull() ) {
        AnimationViewBase::centerOn( ret.left(), ret.right(), ret.bottom(), ret.top() );
    }
} // centerOn


void
CurveWidget::getBackgroundColour(double &r,
                                 double &g,
                                 double &b) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&r, &g, &b);
}

void
CurveWidget::drawView()
{

    OfxParamOverlayInteractPtr customInteract = getCustomInteract();
    if (customInteract) {
        // Don't protect GL_COLOR_BUFFER_BIT, because it seems to hit an OpenGL bug on
        // some macOS configurations (10.10-10.12), where garbage is displayed in the viewport.
        // see https://github.com/MrKepzie/Natron/issues/1460
        //GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
        GLProtectAttrib<GL_GPU> a(GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        RenderScale scale(1.);
        customInteract->setCallingViewport(this);
        customInteract->drawAction(0, scale, 0, customInteract->hasColorPicker() ? &customInteract->getLastColorPickerColor() : 0);
        glCheckErrorIgnoreOSXBug();
    }

    _imp->drawScale();


    _imp->drawTimelineMarkers();


    if (_imp->_model.lock()->getSelectionModel()->getSelectedKeyframesCount() > 1) {
        _imp->drawSelectedKeyFramesBbox();
    }

    _imp->drawCurves();

    if ( !_imp->selectionRect.isNull() ) {
        _imp->drawSelectionRectangle();
    }

} // CurveWidget::paintGL



void
CurveWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    // If nearby a curve, add a keyframe on a double click

    AnimItemDimViewKeyFrame selectedKey = _imp->isNearbyKeyFrame(e->pos());
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > selectedTan = _imp->isNearbyTangent( e->pos() );

    if (selectedKey.id.item || selectedTan.second.id.item) {
        // We are nearby a keyframe or its tangents, do not do anything
        return;
    }



    EditKeyFrameDialog::EditModeEnum mode = EditKeyFrameDialog::eEditModeKeyframePosition;

    // Check if we're nearby a selected keyframe's text
    AnimItemDimViewKeyFrame keyText = _imp->isNearbyKeyFrameText( e->pos() );
    if (!keyText.id.item) {

        // Check if we're nearby a selected keyframe's tangent text
        std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame> tangentText = _imp->isNearbySelectedTangentText( e->pos() );
        if (keyText.id.item) {
            if (tangentText.first == MoveTangentCommand::eSelectedTangentLeft) {
                mode = EditKeyFrameDialog::eEditModeLeftDerivative;
            } else {
                mode = EditKeyFrameDialog::eEditModeRightDerivative;
            }
            keyText = tangentText.second;
        }
    }


    // If nearby a text item, edit it
    if (keyText.id.item) {
        EditKeyFrameDialog* dialog = new EditKeyFrameDialog(mode, this, keyText, this);
        int dialogW = dialog->sizeHint().width();
        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        QPoint gP = e->globalPos();
        if ( gP.x() > (screen.width() - dialogW) ) {
            gP.rx() -= dialogW;
        }

        dialog->move(gP);

        ///This allows us to have a non-modal dialog: when the user clicks outside of the dialog,
        ///it closes it.
        QObject::connect( dialog, SIGNAL(dialogFinished(bool)), this, SLOT(onEditKeyFrameDialogFinished(bool)) );
        dialog->show();

        e->accept();

        return;
    }

    // is the click nearby a curve?
    double xCurve, yCurve;
    CurveGuiPtr foundCurveNearby = _imp->isNearbyCurve( e->pos(), &xCurve, &yCurve );
    if (foundCurveNearby) {

        // Add a keyframe
        addKey(foundCurveNearby, xCurve, yCurve);

        _imp->_dragStartPoint = e->pos();
        _imp->_lastMousePos = e->pos();
        e->accept();

        return;
    }
} // CurveWidget::mouseDoubleClickEvent

void
CurveWidget::onEditKeyFrameDialogFinished(bool /*accepted*/)
{
    EditKeyFrameDialog* dialog = qobject_cast<EditKeyFrameDialog*>( sender() );

    if (dialog) {
        //QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        dialog->deleteLater();
    }
}

//
// Decide what should be done in response to a mouse press.
// When the reason is found, process it and return.
// (this function has as many return points as there are reasons)
//
void
CurveWidget::mousePressEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    {
        AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
        if (isAnimModule) {
            isAnimModule->getEditor()->onInputEventCalled();
        }
    }

    setFocus();
    ////
    // right button: popup menu
    if ( buttonDownIsRight(e) ) {
        _imp->createMenu();
        _imp->_rightClickMenu->exec( mapToGlobal( e->pos() ) );
        _imp->_dragStartPoint = e->pos();
        // no need to set _imp->_lastMousePos
        // no need to set _imp->_dragStartPoint

        // no need to update()
        e->accept();

        return;
    }

    if ( modCASIsControlAlt(e) ) { // Ctrl+Alt (Cmd+Alt on Mac) = insert keyframe
        ////
        // is the click near a curve?
        double xCurve, yCurve;
        CurveGuiPtr foundCurveNearby = _imp->isNearbyCurve( e->pos(), &xCurve, &yCurve );
        if (foundCurveNearby) {
            addKey(foundCurveNearby, xCurve, yCurve);

            _imp->_dragStartPoint = e->pos();
            _imp->_lastMousePos = e->pos();
        }
        e->accept();

        return;
    }

    ////
    // middle button: scroll view
    if ( buttonDownIsMiddle(e) ) {
        _imp->_state = eEventStateDraggingView;
        _imp->_lastMousePos = e->pos();
        _imp->_dragStartPoint = e->pos();
        // no need to set _imp->_dragStartPoint

        // no need to update()
        e->accept();

        return;
    } else if ( ( (e->buttons() & Qt::MiddleButton) &&
                  ( ( buttonMetaAlt(e) == Qt::AltModifier) || (e->buttons() & Qt::LeftButton) ) ) ||
                ( (e->buttons() & Qt::LeftButton) &&
                  ( buttonMetaAlt(e) == (Qt::AltModifier | Qt::MetaModifier) ) ) ) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->_state = eEventStateZooming;
        _imp->_lastMousePos = e->pos();
        _imp->_dragStartPoint = e->pos();

        e->accept();

        return;
    }

    AnimationModuleBasePtr model = _imp->_model.lock();
    // is the click near the multiple-keyframes selection box center?
    if (model->getSelectionModel()->getSelectedKeyframesCount() > 1) {
        bool caughtBbox = true;
        if ( _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
            _imp->_state = eEventStateDraggingKeys;
        } else if ( _imp->isNearbyBboxBtmLeft( e->pos() ) ) {
            _imp->_state = eEventStateDraggingBtmLeftBbox;
        } else if ( _imp->isNearbyBboxMidLeft( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidLeftBbox;
        } else if ( _imp->isNearbyBboxTopLeft( e->pos() ) ) {
            _imp->_state = eEventStateDraggingTopLeftBbox;
        } else if ( _imp->isNearbyBboxMidTop( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidTopBbox;
        } else if ( _imp->isNearbyBboxTopRight( e->pos() ) ) {
            _imp->_state = eEventStateDraggingTopRightBbox;
        } else if ( _imp->isNearbyBboxMidRight( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidRightBbox;
        } else if ( _imp->isNearbyBboxBtmRight( e->pos() ) ) {
            _imp->_state = eEventStateDraggingBtmRightBbox;
        } else if ( _imp->isNearbyBboxMidBtm( e->pos() ) ) {
            _imp->_state = eEventStateDraggingMidBtmBbox;
        } else {
            caughtBbox = false;
        }
        if (caughtBbox) {
            _imp->_mustSetDragOrientation = true;

            _imp->_dragStartPoint = e->pos();
            _imp->_lastMousePos = e->pos();

            //no need to update()
            e->accept();

            return;
        }
    }
    // is the click near a keyframe manipulator?
    AnimItemDimViewKeyFrame nearbyKeyframe = _imp->isNearbyKeyFrame(e->pos());
    if (nearbyKeyframe.id.item) {

        AnimItemDimViewKeyFramesMap keysToAdd;
        KeyFrameWithStringSet& keys = keysToAdd[nearbyKeyframe.id];
        keys.insert(nearbyKeyframe.key);
        model->getSelectionModel()->makeSelection(keysToAdd, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>(), (AnimationModuleSelectionModel::SelectionTypeAdd |
                                                                                                                           AnimationModuleSelectionModel::SelectionTypeClear) );
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingKeys;
        setCursor( QCursor(Qt::CrossCursor) );

        _imp->_dragStartPoint = e->pos();
        _imp->_lastMousePos = e->pos();
        update(); // the keyframe changes color and the derivatives must be drawn
        e->accept();

        return;
    }


    ////
    // is the click near a derivative manipulator?
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > selectedTan = _imp->isNearbyTangent( e->pos() );

    //select the derivative only if it is not a constant keyframe
    if ( selectedTan.second.id.item && (selectedTan.second.key.key.getInterpolation() != eKeyframeTypeConstant) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingTangent;
        _imp->_selectedDerivative = selectedTan;
        _imp->_lastMousePos = e->pos();
        //no need to set _imp->_dragStartPoint
        update();
        e->accept();

        return;
    }

    AnimItemDimViewKeyFrame nearbyKeyText = _imp->isNearbyKeyFrameText( e->pos() );
    if (nearbyKeyText.id.item) {
        // do nothing, doubleclick edits the text
        e->accept();

        return;
    }

    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame> tangentText = _imp->isNearbySelectedTangentText( e->pos() );
    if (tangentText.second.id.item) {
        // do nothing, doubleclick edits the text
        e->accept();

        return;
    }


    ////
    // is the click near the vertical current time marker?
    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
        _imp->_mustSetDragOrientation = true;
        _imp->_state = eEventStateDraggingTimeline;
        _imp->_lastMousePos = e->pos();
        // no need to set _imp->_dragStartPoint

        // no need to update()
        e->accept();

        return;
    }


    ////
    // default behaviour: unselect selected keyframes, if any, and start a new selection
    if ( !modCASIsControl(e) ) {
        model->getSelectionModel()->clearSelection();
    }
    _imp->_state = eEventStateSelecting;
    _imp->_lastMousePos = e->pos();
    _imp->_dragStartPoint = e->pos();

    e->accept();
} // mousePressEvent

void
CurveWidget::mouseReleaseEvent(QMouseEvent*)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    EventStateEnum prevState = _imp->_state;
    _imp->_state = eEventStateNone;
    _imp->selectionRect.clear();

    // If we were dragging timeline and autoproxy is on, then on mouse release trigger a non proxy render
    if (prevState == eEventStateDraggingTimeline) {
        if ( _imp->_gui->isDraftRenderEnabled() ) {
            _imp->_gui->setDraftRenderEnabled(false);
            bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
            if (autoProxyEnabled) {
                _imp->_gui->renderAllViewers(true);
            }
        }
    }

    if (prevState == eEventStateSelecting) { // should other cases be considered?
        update();
    }
} // CurveWidget::mouseReleaseEvent

void
CurveWidget::mouseMoveEvent(QMouseEvent* e)
{

    // set cursor depending on the situation

    AnimationModuleBasePtr model = _imp->_model.lock();
    AnimationModuleSelectionModelPtr selectModel = model->getSelectionModel();

    // find out if there is a nearby  derivative handle
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > selectedTan = _imp->isNearbyTangent( e->pos() );

    // if the selected keyframes rectangle is drawn and we're nearby the cross
    int nbSelectedKeys = selectModel->getSelectedKeyframesCount();
    if ( nbSelectedKeys > 1 && _imp->isNearbySelectedKeyFramesCrossWidget( e->pos() ) ) {
        setCursor( QCursor(Qt::SizeAllCursor) );
    } else {

        // if there's a keyframe handle nearby
        AnimItemDimViewKeyFrame nearbyKey = _imp->isNearbyKeyFrame(e->pos());

        // if there's a keyframe or derivative handle nearby set the cursor to cross
        if (nearbyKey.id.item || selectedTan.second.id.item) {
            setCursor( QCursor(Qt::CrossCursor) );
        } else {
            AnimItemDimViewKeyFrame nearbyKeyText = _imp->isNearbyKeyFrameText( e->pos() );
            if (nearbyKeyText.id.item) {
                setCursor( QCursor(Qt::IBeamCursor) );
            } else {
                std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame> tangentText = _imp->isNearbySelectedTangentText( e->pos() );
                if (tangentText.second.id.item) {
                    setCursor( QCursor(Qt::IBeamCursor) );
                } else {
                    // if we're nearby a timeline polygon, set cursor to horizontal displacement
                    if ( _imp->isNearbyTimelineBtmPoly( e->pos() ) || _imp->isNearbyTimelineTopPoly( e->pos() ) ) {
                        setCursor( QCursor(Qt::SizeHorCursor) );
                    } else {
                        //default case
                        unsetCursor();
                    }
                }
            }
        }
    }

    if (_imp->_state == eEventStateNone) {

        // nothing else to do
        TabWidget* tab = 0;
        AnimationModulePtr isAnimModule = toAnimationModule(model);
        if (isAnimModule) {
            tab = isAnimModule->getEditor()->getParentPane();
        }
        if (tab) {
            // If the Viewer is in a tab, send the tab widget the event directly
            qApp->sendEvent(tab, e);
        } else {
            QGLWidget::mouseMoveEvent(e);
        }

        return;
    }

    // after this point , only mouse dragging situations are handled
    assert(_imp->_state != eEventStateNone);

    if (_imp->_mustSetDragOrientation) {
        QPointF diff(e->pos() - _imp->_dragStartPoint);
        double dist = diff.manhattanLength();
        if (dist > 5) {
            if ( std::abs( diff.x() ) > std::abs( diff.y() ) ) {
                _imp->_mouseDragOrientation.setX(1);
                _imp->_mouseDragOrientation.setY(0);
            } else {
                _imp->_mouseDragOrientation.setX(0);
                _imp->_mouseDragOrientation.setY(1);
            }
            _imp->_mustSetDragOrientation = false;
        }
    }

    QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->_lastMousePos.x(), _imp->_lastMousePos.y() );
    switch (_imp->_state) {
        case eEventStateDraggingView: {
            _imp->zoomOrPannedSinceLastFit = true;
            double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
            double dy = ( oldClick_opengl.y() - newClick_opengl.y() );


        {
            QMutexLocker k(&_imp->zoomCtxMutex);
            _imp->zoomCtx.translate(dx, dy);
        }

            // Synchronize the dope sheet editor and opened viewers
            if ( _imp->_gui->isTripleSyncEnabled() ) {
                _imp->updateDopeSheetViewFrameRange();
                _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
            }
        }   break;

        case eEventStateDraggingKeys: {
            if (!_imp->_mustSetDragOrientation) {
                if (nbSelectedKeys > 0) {
                    if (_imp->_gui) {
                        _imp->_gui->setDraftRenderEnabled(true);
                    }
                    double dx = ( newClick_opengl.x() - oldClick_opengl.x() );
                    double dy = ( newClick_opengl.y() - oldClick_opengl.y() );

                    model->moveSelectedKeysAndNodes(dx, dy);
                }
            }
        }   break;
        case eEventStateDraggingBtmLeftBbox:
        case eEventStateDraggingMidBtmBbox:
        case eEventStateDraggingBtmRightBbox:
        case eEventStateDraggingMidRightBbox:
        case eEventStateDraggingTopRightBbox:
        case eEventStateDraggingMidTopBbox:
        case eEventStateDraggingTopLeftBbox:
        case eEventStateDraggingMidLeftBbox: {
            if (nbSelectedKeys > 0) {
                if (_imp->_gui) {
                    _imp->_gui->setDraftRenderEnabled(true);
                }
                _imp->transformSelectedKeyFrames( oldClick_opengl, newClick_opengl, modCASIsShift(e) );
            }
        }  break;
        case eEventStateSelecting:
            _imp->refreshSelectionRectangle( (double)e->x(), (double)e->y() );
            break;

        case eEventStateDraggingTangent:
            if (_imp->_gui) {
                _imp->_gui->setDraftRenderEnabled(true);
            }
            _imp->moveSelectedTangent(newClick_opengl);
            break;
        case eEventStateDraggingTimeline: {
            TimeLinePtr timeline = model->getTimeline();
            if (timeline) {
                _imp->_gui->setDraftRenderEnabled(true);
                _imp->_gui->getApp()->setLastViewerUsingTimeline( NodePtr() );
                timeline->seekFrame( (int)newClick_opengl.x(), false, OutputEffectInstancePtr(),  eTimelineChangeReasonCurveEditorSeek );
            }
        }   break;
        case eEventStateZooming: {
            if ( (_imp->zoomCtx.screenWidth() > 0) && (_imp->zoomCtx.screenHeight() > 0) ) {
                _imp->zoomOrPannedSinceLastFit = true;

                int deltaX = 2 * ( e->x() - _imp->_lastMousePos.x() );
                int deltaY = -2 * ( e->y() - _imp->_lastMousePos.y() );
                // Wheel: zoom values and time, keep point under mouse
                const double zoomFactor_min = 0.0001;
                const double zoomFactor_max = 10000.;
                const double par_min = 0.0001;
                const double par_max = 10000.;
                double zoomFactor;
                double scaleFactorX = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaX);
                double scaleFactorY = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaY);
                QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( _imp->_dragStartPoint.x(), _imp->_dragStartPoint.y() );

                // Alt + Shift + Wheel: zoom values only, keep point under mouse
                zoomFactor = _imp->zoomCtx.factor() * scaleFactorY;

                if (zoomFactor <= zoomFactor_min) {
                    zoomFactor = zoomFactor_min;
                    scaleFactorY = zoomFactor / _imp->zoomCtx.factor();
                } else if (zoomFactor > zoomFactor_max) {
                    zoomFactor = zoomFactor_max;
                    scaleFactorY = zoomFactor / _imp->zoomCtx.factor();
                }

                double par = _imp->zoomCtx.aspectRatio() / scaleFactorY;
                if (par <= par_min) {
                    par = par_min;
                    scaleFactorY = par / _imp->zoomCtx.aspectRatio();
                } else if (par > par_max) {
                    par = par_max;
                    scaleFactorY = par / _imp->zoomCtx.factor();
                }

                {
                    QMutexLocker k(&_imp->zoomCtxMutex);
                    _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactorY);
                }

                // Alt + Wheel: zoom time only, keep point under mouse
                par = _imp->zoomCtx.aspectRatio() * scaleFactorX;
                if (par <= par_min) {
                    par = par_min;
                    scaleFactorX = par / _imp->zoomCtx.aspectRatio();
                } else if (par > par_max) {
                    par = par_max;
                    scaleFactorX = par / _imp->zoomCtx.factor();
                }

                {
                    QMutexLocker k(&_imp->zoomCtxMutex);
                    _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactorX);
                }

                if (nbSelectedKeys > 1) {
                    refreshSelectionBoundingBox();
                }
                
                // Synchronize the dope sheet editor and opened viewers
                if ( _imp->_gui->isTripleSyncEnabled() ) {
                    _imp->updateDopeSheetViewFrameRange();
                    _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
                }
            }
            break;
        }
        case eEventStateNone:
            assert(0);
            break;
    } // switch
    
    _imp->_lastMousePos = e->pos();
    
    update();

    QGLWidget::mouseMoveEvent(e);
} // mouseMoveEvent
void
CurveWidget::refreshSelectionBoundingBox()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( (_imp->zoomCtx.screenWidth() < 1) || (_imp->zoomCtx.screenHeight() < 1) ) {
        return;
    }

    AnimationModuleBasePtr model = _imp->_model.lock();
    AnimationModuleSelectionModelPtr selectModel = model->getSelectionModel();

    const AnimItemDimViewKeyFramesMap& selectedKeys = selectModel->getCurrentKeyFramesSelection();

    RectD keyFramesBbox;
    bool bboxSet = false;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
        for (KeyFrameWithStringSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            double x = it2->key.getTime();
            double y = it2->key.getValue();
            if (bboxSet) {
                if ( x < keyFramesBbox.left() ) {
                    keyFramesBbox.set_left(x);
                }
                if ( x > keyFramesBbox.right() ) {
                    keyFramesBbox.set_right(x);
                }
                if ( y > keyFramesBbox.top() ) {
                    keyFramesBbox.set_top(y);
                }
                if ( y < keyFramesBbox.bottom() ) {
                    keyFramesBbox.set_bottom(y);
                }
            } else {
                bboxSet = true;
                keyFramesBbox.set_left(x);
                keyFramesBbox.set_right(x);
                keyFramesBbox.set_top(y);
                keyFramesBbox.set_bottom(y);
            }

        }

    }

    _imp->selectedKeysBRect = keyFramesBbox;
} // CurveWidget::refreshSelectionBoundingBox

void
CurveWidget::wheelEvent(QWheelEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    const double zoomFactor_min = 0.0001;
    const double zoomFactor_max = 10000.;
    const double par_min = 0.0001;
    const double par_max = 10000.;
    double zoomFactor;
    double par;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    if ( modCASIsControlShift(e) ) {
        _imp->zoomOrPannedSinceLastFit = true;
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        par = _imp->zoomCtx.aspectRatio() / scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }

        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else if ( modCASIsControl(e) ) {
        _imp->zoomOrPannedSinceLastFit = true;
        // Alt + Wheel: zoom time only, keep point under mouse
        par = _imp->zoomCtx.aspectRatio() * scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }

        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else {
        _imp->zoomOrPannedSinceLastFit = true;
        // Wheel: zoom values and time, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }

        QMutexLocker k(&_imp->zoomCtxMutex);
        _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    }

    if (_imp->_model.lock()->getSelectionModel()->getSelectedKeyframesCount() > 1) {
        refreshSelectionBoundingBox();
    }


    // Synchronize the dope sheet editor and opened viewers
    if ( _imp->_gui->isTripleSyncEnabled() ) {
        _imp->updateDopeSheetViewFrameRange();
        _imp->_gui->centerOpenedViewersOn( _imp->zoomCtx.left(), _imp->zoomCtx.right() );
    }
    update();
} // wheelEvent

QSize
CurveWidget::sizeHint() const
{
    return _imp->sizeH;
}


void
CurveWidget::onApplyLoopExpressionOnSelectedCurveActionTriggered()
{
    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }

    std::vector<CurveGuiPtr> curves = _imp->getSelectedCurves();
    if (curves.empty() || curves.size() > 1) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select exactly one parmaeter curve to apply an expression").toStdString() );
        return;
    }
    const CurveGuiPtr& curve = curves.front();
    KnobAnimPtr isKnobAnim = toKnobAnim(curve->getItem());
    if (!isKnobAnim) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select exactly one parmaeter curve to apply an expression").toStdString() );
        return;
    }

    NATRON_PYTHON_NAMESPACE::PyModalDialog dialog(_imp->_gui);
    boost::shared_ptr<NATRON_PYTHON_NAMESPACE::IntParam> firstFrame( dialog.createIntParam( QString::fromUtf8("firstFrame"), QString::fromUtf8("First frame") ) );
    firstFrame->setAnimationEnabled(false);
    boost::shared_ptr<NATRON_PYTHON_NAMESPACE::IntParam> lastFrame( dialog.createIntParam( QString::fromUtf8("lastFrame"), QString::fromUtf8("Last frame") ) );
    lastFrame->setAnimationEnabled(false);
    dialog.refreshUserParamsGUI();
    if ( dialog.exec() ) {
        int first = firstFrame->getValue();
        int last = lastFrame->getValue();
        std::stringstream ss;
        std::string viewName = _imp->_gui->getApp()->getProject()->getViewName(curve->getView());
        assert(!viewName.empty());
        if (viewName.empty()) {
            viewName = "Main";
        }
        ss << "curve(((frame - " << first << ") % (" << last << " - " << first << " + 1)) + " << first << ", " << curve->getDimension() << ", " <<  viewName << ")";
        std::string script = ss.str();
        isAnimModule->getEditor()->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ) );
    }
} // loopSelectedCurve

void
CurveWidget::onApplyNegateExpressionOnSelectedCurveActionTriggered()
{

    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }

    std::vector<CurveGuiPtr> curves = _imp->getSelectedCurves();
    if (curves.empty() || curves.size() > 1) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select exactly one parmaeter curve to apply an expression").toStdString() );
        return;
    }
    const CurveGuiPtr& curve = curves.front();
    KnobAnimPtr isKnobAnim = toKnobAnim(curve->getItem());
    if (!isKnobAnim) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select exactly one parmaeter curve to apply an expression").toStdString() );
        return;
    }
    std::string viewName = _imp->_gui->getApp()->getProject()->getViewName(curve->getView());
    assert(!viewName.empty());
    if (viewName.empty()) {
        viewName = "Main";
    }
    std::stringstream ss;
    ss << "-curve(frame, " << curve->getDimension() << ", " <<  viewName << ")";
    std::string script = ss.str();
    isAnimModule->getEditor()->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ) );

} // negateSelectedCurve

void
CurveWidget::onApplyReverseExpressionOnSelectedCurveActionTriggered()
{
    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }

    std::vector<CurveGuiPtr> curves = _imp->getSelectedCurves();
    if (curves.empty() || curves.size() > 1) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select exactly one parmaeter curve to apply an expression").toStdString() );
        return;
    }
    const CurveGuiPtr& curve = curves.front();
    KnobAnimPtr isKnobAnim = toKnobAnim(curve->getItem());
    if (!isKnobAnim) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select exactly one parmaeter curve to apply an expression").toStdString() );
        return;
    }
    std::string viewName = _imp->_gui->getApp()->getProject()->getViewName(curve->getView());
    assert(!viewName.empty());
    if (viewName.empty()) {
        viewName = "Main";
    }
    std::stringstream ss;
    ss << "curve(-frame, " << curve->getDimension() << ", " <<  viewName << ")";
    std::string script = ss.str();
    isAnimModule->getEditor()->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ) );

} // reverseSelectedCurve

void
CurveWidget::onExportCurveToAsciiActionTriggered()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<CurveGuiPtr > curves = _imp->getSelectedCurves();
    if ( curves.empty() ) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select an item first").toStdString() );
        return;
    }
    for (std::size_t i = 0; i < curves.size(); ++i) {
        if (!curves[i]->isYComponentMovable()) {
            Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("String curves cannot be imported/exported.").toStdString() );
            return;
        }
    }


    ImportExportCurveDialog dialog(true, curves, _imp->_gui, this);
    if ( dialog.exec() ) {
        double x = dialog.getXStart();
        double end = dialog.getXEnd();
        double incr = dialog.getXIncrement();
        std::map<int, CurveGuiPtr > columns;
        dialog.getCurveColumns(&columns);

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            double incrInt = std::floor(incr);
            double xInt = std::floor(x);
            double endInt = std::floor(end);
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ( incrInt != incr) || ( xInt != x) || ( endInt != end) ) ) {
                Dialogs::warningDialog( tr("Curve Export").toStdString(), tr("%1 doesn't support X values that are not integers.").arg( curves[i]->getName() ).toStdString() );

                return;
            }
        }

        assert( !columns.empty() );
        int columnsCount = columns.rbegin()->first + 1;

        ///setup the file
        QString name = dialog.getFilePath();
        QFile file(name);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream ts(&file);

        for (double i = x; i <= end; i += incr) {
            for (int c = 0; c < columnsCount; ++c) {
                std::map<int, CurveGuiPtr >::const_iterator foundCurve = columns.find(c);
                if ( foundCurve != columns.end() ) {
                    QString str = QString::number(foundCurve->second->evaluate(true, i), 'f', 10);
                    ts << str;
                } else {
                    ts <<  0;
                }
                if (c < columnsCount - 1) {
                    ts << '_';
                }
            }
            ts << '\n';
        }


        ///close the file
        file.close();
    }
} // exportCurveToAscii

void
CurveWidget::onImportCurveFromAsciiActionTriggered()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<CurveGuiPtr > curves = _imp->getSelectedCurves();
    if ( curves.empty() ) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select an item first").toStdString() );
        return;
    }
    for (std::size_t i = 0; i < curves.size(); ++i) {
        if (!curves[i]->isYComponentMovable()) {
            Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("String curves cannot be imported/exported.").toStdString() );
            return;
        }
    }


    ImportExportCurveDialog dialog(false, curves, _imp->_gui, this);
    if ( dialog.exec() ) {
        QString filePath = dialog.getFilePath();
        if ( !QFile::exists(filePath) ) {
            Dialogs::warningDialog( tr("Curve Import").toStdString(), tr("File not found.").toStdString() );

            return;
        }

        double x = dialog.getXStart();
        double incr = dialog.getXIncrement();
        std::map<int, CurveGuiPtr > columns;
        dialog.getCurveColumns(&columns);
        assert( !columns.empty() );

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            double incrInt = std::floor(incr);
            double xInt = std::floor(x);
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ( incrInt != incr) || ( xInt != x) ) ) {
                Dialogs::warningDialog( tr("Curve Import").toStdString(), tr("%1 doesn't support X values that are not integers.").arg( curves[i]->getName() ).toStdString() );

                return;
            }
        }

        QFile file( dialog.getFilePath() );
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        std::map<CurveGuiPtr, std::vector<double> > curvesValues;
        ///scan the file to get the curve values
        while ( !ts.atEnd() ) {
            QString line = ts.readLine();
            if ( line.isEmpty() ) {
                continue;
            }
            int i = 0;
            std::vector<double> values;

            ///read the line to extract all values
            while ( i < line.size() ) {
                QString value;
                while ( i < line.size() && line.at(i) != QLatin1Char('_') ) {
                    value.push_back( line.at(i) );
                    ++i;
                }
                if ( i < line.size() ) {
                    if ( line.at(i) != QLatin1Char('_') ) {
                        Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("The file could not be read.").toStdString() );

                        return;
                    }
                    ++i;
                }
                bool ok;
                double v = value.toDouble(&ok);
                if (!ok) {
                    Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("The file could not be read.").toStdString() );

                    return;
                }
                values.push_back(v);
            }
            ///assert that the values count is greater than the number of curves provided by the user
            if ( values.size() < columns.size() ) {
                Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("The file contains less curves than what you selected.").toStdString() );

                return;
            }

            for (std::map<int, CurveGuiPtr >::const_iterator col = columns.begin(); col != columns.end(); ++col) {
                if ( col->first >= (int)values.size() ) {
                    Dialogs::errorDialog( tr("Curve Import").toStdString(), tr("One of the curve column index is not a valid index for the given file.").toStdString() );

                    return;
                }
                std::map<CurveGuiPtr, std::vector<double> >::iterator foundCurve = curvesValues.find(col->second);
                if ( foundCurve != curvesValues.end() ) {
                    foundCurve->second.push_back(values[col->first]);
                } else {
                    std::vector<double> curveValues(1);
                    curveValues[0] = values[col->first];
                    curvesValues.insert( std::make_pair(col->second, curveValues) );
                }
            }
        }
        ///now restore the curves since we know what we read is valid

        AnimItemDimViewKeyFramesMap finalValues;
        for (std::map<CurveGuiPtr, std::vector<double> >::const_iterator it = curvesValues.begin(); it != curvesValues.end(); ++it) {

            AnimItemDimViewIndexID id(it->first->getItem(), it->first->getView(), it->first->getDimension());
            KeyFrameWithStringSet& keys = finalValues[id];

            const std::vector<double> & values = it->second;
            double xIndex = x;
            for (U32 i = 0; i < values.size(); ++i) {
                KeyFrameWithString k;
                k.key = KeyFrame(xIndex, values[i], 0., 0., eKeyframeTypeLinear);
                keys.insert(k);
                xIndex += incr;
            }
        }

        AnimationModuleBasePtr model = _imp->_model.lock();


        model->pushUndoCommand( new AddKeysCommand(finalValues, model, true /*clearExistingAnimation*/));

    }
} // importCurveFromAscii

void
CurveWidget::setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc)
{
    _imp->_customInteract = interactDesc;
}

OfxParamOverlayInteractPtr
CurveWidget::getCustomInteract() const
{
    return _imp->_customInteract.lock();
}

void
CurveWidget::addKey(const CurveGuiPtr& curve, double xCurve, double yCurve)
{
    Curve::YRange yRange = curve->getCurveYRange();
    if ( (yCurve < yRange.min) || (yCurve > yRange.max) ) {
        QString err =  tr("Out of curve y range ") +
        QString::fromUtf8("[%1 - %2]").arg(yRange.min).arg(yRange.max);
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), err.toStdString() );

        return;
    }

    if (!curve->isYComponentMovable()) {
        QString err =  tr("You cannot add keyframes on the animation of a String parameter from the Curve Editor");
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), err.toStdString() );
        return;
    }

    AnimItemDimViewIndexID itemKey(curve->getItem(), curve->getView(), curve->getDimension());
    AnimItemDimViewKeyFramesMap keysToAdd;
    KeyFrameWithStringSet& keys = keysToAdd[itemKey];

    AnimationModuleBasePtr model = _imp->_model.lock();
    KeyFrameWithString k;
    k.key = KeyFrame(xCurve, yCurve, 0, 0);
    keys.insert(k);
    model->pushUndoCommand( new AddKeysCommand(keysToAdd, model, false /*clearAndAdd*/) );

    _imp->_mustSetDragOrientation = true;
    _imp->_state = eEventStateDraggingKeys;
    setCursor( QCursor(Qt::CrossCursor) );

    model->getSelectionModel()->makeSelection(keysToAdd, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>(), (AnimationModuleSelectionModel::SelectionTypeAdd | AnimationModuleSelectionModel::SelectionTypeClear));

} // addKey

void
CurveWidget::getKeyTangentPoints(KeyFrameSet::const_iterator it,
                                 const KeyFrameSet& keys,
                                 QPointF* leftTanPos,
                                 QPointF* rightTanPos)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    double w = (double)width();
    double h = (double)height();
    double x = it->getTime();
    double y = it->getValue();
    QPointF keyWidgetCoord = _imp->zoomCtx.toWidgetCoordinates(x, y);
    double leftTanX, leftTanY;
    {
        double prevTime = x - 1.;
        bool hasPrevious = false;
        if (it != keys.begin()) {
            KeyFrameSet::const_iterator prev = it;
            --prev;
            prevTime = prev->getTime();
            hasPrevious = true;
        }
        
        // (!key->hasPrevious) ? (x - 1.) : key->prevKey.getTime();
        double leftTan = it->getLeftDerivative();
        double leftTanXWidgetDiffMax = w / 8.;
        if (hasPrevious) {
            double prevKeyXWidgetCoord = _imp->zoomCtx.toWidgetCoordinates(prevTime, 0).x();
            //set the left derivative X to be at 1/3 of the interval [prev,k], and clamp it to 1/8 of the widget width.
            leftTanXWidgetDiffMax = std::min(leftTanXWidgetDiffMax, (keyWidgetCoord.x() - prevKeyXWidgetCoord) / 3.);
        }
        //clamp the left derivative Y to 1/8 of the widget height.
        double leftTanYWidgetDiffMax = std::min( h / 8., leftTanXWidgetDiffMax);
        assert(leftTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(leftTanYWidgetDiffMax >= 0.);
        
        QPointF tanMax = _imp->zoomCtx.toZoomCoordinates(keyWidgetCoord.x() + leftTanXWidgetDiffMax, keyWidgetCoord.y() - leftTanYWidgetDiffMax) - QPointF(x, y);
        assert(tanMax.x() >= 0.); // both should be positive
        assert(tanMax.y() >= 0.);
        
        if ( tanMax.x() * std::abs(leftTan) < tanMax.y() ) {
            leftTanX = x - tanMax.x();
            leftTanY = y - tanMax.x() * leftTan;
        } else {
            leftTanX = x - tanMax.y() / std::abs(leftTan);
            leftTanY = y - tanMax.y() * (leftTan > 0 ? 1 : -1);
        }
        assert(std::abs(leftTanX - x) <= tanMax.x() * 1.001); // check that they are effectively clamped (taking into account rounding errors)
        assert(std::abs(leftTanY - y) <= tanMax.y() * 1.001);
    }
    double rightTanX, rightTanY;
    {
        double nextTime = x + 1.;
        bool hasNext = false;
        {
            KeyFrameSet::const_iterator next = it;
            ++next;
            if (next != keys.end()) {
                nextTime = next->getTime();
                hasNext = true;
            }
        }
        double rightTan = it->getRightDerivative();
        double rightTanXWidgetDiffMax = w / 8.;
        if (hasNext) {
            double nextKeyXWidgetCoord = _imp->zoomCtx.toWidgetCoordinates(nextTime, 0).x();
            //set the right derivative X to be at 1/3 of the interval [k,next], and clamp it to 1/8 of the widget width.
            rightTanXWidgetDiffMax = std::min(rightTanXWidgetDiffMax, ( nextKeyXWidgetCoord - keyWidgetCoord.x() ) / 3.);
        }
        //clamp the right derivative Y to 1/8 of the widget height.
        double rightTanYWidgetDiffMax = std::min( h / 8., rightTanXWidgetDiffMax);
        assert(rightTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(rightTanYWidgetDiffMax >= 0.);
        
        QPointF tanMax = _imp->zoomCtx.toZoomCoordinates(keyWidgetCoord.x() + rightTanXWidgetDiffMax, keyWidgetCoord.y() - rightTanYWidgetDiffMax) - QPointF(x, y);
        assert(tanMax.x() >= 0.); // both bounds should be positive
        assert(tanMax.y() >= 0.);
        
        if ( tanMax.x() * std::abs(rightTan) < tanMax.y() ) {
            rightTanX = x + tanMax.x();
            rightTanY = y + tanMax.x() * rightTan;
        } else {
            rightTanX = x + tanMax.y() / std::abs(rightTan);
            rightTanY = y + tanMax.y() * (rightTan > 0 ? 1 : -1);
        }
        assert(std::abs(rightTanX - x) <= tanMax.x() * 1.001); // check that they are effectively clamped (taking into account rounding errors)
        assert(std::abs(rightTanY - y) <= tanMax.y() * 1.001);
    }
    leftTanPos->rx() = leftTanX;
    leftTanPos->ry() = leftTanY;
    rightTanPos->rx() = rightTanX;
    rightTanPos->ry() = rightTanY;
} // getKeyTangentPoints


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_CurveWidget.cpp"
