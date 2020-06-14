/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "AnimationModuleView.h"

#include <cmath> // floor
#include <stdexcept>
#include <cfloat>
#include <sstream> // stringstream

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
#include "Engine/OverlayInteractBase.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/OSGLFunctions.h"

#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/ActionShortcuts.h"
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

NATRON_NAMESPACE_ENTER

std::pair<double,double>
AnimationModuleView::getCurvesKeyframeTimesBbox(const std::vector<CurveGuiPtr> & curves)
{
    double xmin = 0, xmax = 0;
    double rangeSet = false;
    for (std::size_t i = 0; i < curves.size(); ++i) {
        const CurveGuiPtr& c = curves[i];
        KeyFrameSet keys = c->getKeyFrames();

        if ( keys.empty() ) {
            continue;
        }
        double cmin = keys.begin()->getTime();
        double cmax = keys.rbegin()->getTime();
        if (!rangeSet) {
            rangeSet = true;
            xmin = cmin;
            xmax = cmax;
        } else {
            xmin = std::min(cmin, xmin);
            xmax = std::max(cmax, xmax);
        }
    }
    return std::make_pair(xmin, xmax);
}

RectD
AnimationModuleView::getCurvesKeyframeBbox(const std::vector<CurveGuiPtr> & curves)
{
    RectD ret;
    bool retSet = false;
    for (std::size_t i = 0; i < curves.size(); ++i) {
        const CurveGuiPtr& c = curves[i];
        KeyFrameSet keys = c->getKeyFrames();

        if ( keys.empty() ) {
            continue;
        }
        double xmin = keys.begin()->getTime();
        double xmax = keys.rbegin()->getTime();
        double ymin = INT_MAX;
        double ymax = INT_MIN;
        //find out ymin,ymax
        for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            double value = it2->getValue();
            ymin = std::min(value, ymin);
            ymax = std::max(value, ymax);
        }
        if (!retSet) {
            retSet = true;
            ret.x1 = xmin;
            ret.x2 = xmax;
            ret.y1 = ymin;
            ret.y2 = ymax;
        } else {
            ret.merge(xmin, ymin, xmax, ymax);
        }
    }
    return ret;
}

RectD
AnimationModuleView::getCurvesDisplayRangesBbox(const std::vector<CurveGuiPtr> & curves)
{
    RectD ret;
    bool retSet = false;
    for (std::size_t i = 0; i < curves.size(); ++i) {
        CurvePtr curve = curves[i]->getInternalCurve();
        if (!curve) {
            continue;
        }
        Curve::YRange thisCurveRange = curve->getCurveDisplayYRange();
        std::pair<double, double> thisXRange = curve->getXRange();

        // Ignore unset ranges
        if (thisCurveRange.min == -std::numeric_limits<double>::infinity() ||
            thisCurveRange.min == INT_MIN ||
            thisCurveRange.min == -DBL_MAX ||
            thisCurveRange.max == std::numeric_limits<double>::infinity() ||
            thisCurveRange.max == INT_MAX ||
            thisCurveRange.max == DBL_MAX ||
            thisXRange.first == -std::numeric_limits<double>::infinity() ||
            thisXRange.first == INT_MIN ||
            thisXRange.first == -DBL_MAX ||
            thisXRange.second == std::numeric_limits<double>::infinity() ||
            thisXRange.second == INT_MAX ||
            thisXRange.second == DBL_MAX) {
            continue;
        }


        if (!retSet) {
            ret.x1 = thisXRange.first;
            ret.x2 = thisXRange.second;
            ret.y1 = thisCurveRange.min;
            ret.y2 = thisCurveRange.max;
            retSet = true;
        } else {
            ret.merge(thisXRange.first, thisCurveRange.min, thisXRange.second, thisCurveRange.max);
        }
    }
    return ret;
}

void
AnimationModuleView::centerOnCurves(const std::vector<CurveGuiPtr> & curves, bool useDisplayRange)
{

    // If curves is empty, use currently selected curves from the tree
    std::vector<CurveGuiPtr> curvesToFrame;
    if (!curves.empty()) {
        curvesToFrame = curves;
    } else {
        // Frame all curves selected in the tree view
        curvesToFrame = _imp->getVisibleCurves();
    }

    if (curvesToFrame.empty()) {
        return;
    }

    // First try to center curves given their display range if useDisplayRange is true

    if (useDisplayRange) {
        RectD bbox = getCurvesDisplayRangesBbox(curvesToFrame);

        if (!bbox.isNull()) {
            //bbox.addPaddingPercentage(0.2, 0.2);
            AnimationModuleView::centerOn(bbox.x1, bbox.x2, bbox.y1, bbox.y2);
            return;
        }
    } // useDisplayRange
    
    // If no range or useDisplayRange is false, center them using the bounding box of keyframes
    RectD bbox = getCurvesKeyframeBbox(curvesToFrame);
    if (!bbox.isNull()) {
        bbox.addPaddingPercentage(0.1, 0.1);
    }
    if ( !bbox.isNull() ) {
        AnimationModuleView::centerOn(bbox.x1, bbox.x2, bbox.y1, bbox.y2);
    }
} // centerOnCurves



void
AnimationModuleViewPrivate::drawCurveEditorView()
{

    OverlayInteractBasePtr interact = customInteract.lock();
    if (interact) {
        // Don't protect GL_COLOR_BUFFER_BIT, because it seems to hit an OpenGL bug on
        // some macOS configurations (10.10-10.12), where garbage is displayed in the viewport.
        // see https://github.com/MrKepzie/Natron/issues/1460
        //GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
        GLProtectAttrib<GL_GPU> a(GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        RenderScale scale(1.);
        interact-> drawOverlay_public(_publicInterface, TimeValue(0.), scale, ViewIdx(0));
        glCheckErrorIgnoreOSXBug(GL_GPU);
    }

    drawCurveEditorScale();



    if (_model.lock()->getSelectionModel()->getSelectedKeyframesCount() > 1) {
        drawSelectedKeyFramesBbox(true);
    }

    drawCurves();
    glCheckError(GL_GPU);

    if ( state == eEventStateSelectionRectCurveEditor ) {
        drawSelectionRectangle();
    }

} // drawCurveEditorView



bool
AnimationModuleViewPrivate::curveEditorDoubleClickEvent(QMouseEvent* e)
{
    // If nearby a curve, add a keyframe on a double click

    AnimItemDimViewKeyFrame selectedKey = isNearbyKeyFrame(curveEditorZoomContext, e->pos());
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > selectedTan = isNearbyTangent( e->pos() );

    if (selectedKey.id.item || selectedTan.second.id.item) {
        // We are nearby a keyframe or its tangents, do not do anything
        return false;
    }



    EditKeyFrameDialog::EditModeEnum mode = EditKeyFrameDialog::eEditModeKeyframePosition;

    // Check if we're nearby a selected keyframe's text
    AnimItemDimViewKeyFrame keyText = isNearbyKeyFrameText( e->pos() );
    if (!keyText.id.item) {

        // Check if we're nearby a selected keyframe's tangent text
        std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame> tangentText = isNearbySelectedTangentText( e->pos() );
        if (tangentText.second.id.item) {
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
        EditKeyFrameDialog* dialog = new EditKeyFrameDialog(mode, _publicInterface, keyText, _publicInterface);
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
        QObject::connect( dialog, SIGNAL(dialogFinished(bool)), _publicInterface, SLOT(onEditKeyFrameDialogFinished(bool)) );
        dialog->show();

        e->accept();

        return true;
    }

    // is the click nearby a curve?
    double xCurve, yCurve;
    CurveGuiPtr foundCurveNearby = isNearbyCurve( e->pos(), &xCurve, &yCurve );
    if (foundCurveNearby) {

        // Add a keyframe
        addKey(foundCurveNearby, xCurve, yCurve);

        dragStartPoint = e->pos();
        lastMousePos = e->pos();
        e->accept();

        return true;
    }
    return false;
} // CurveWidget::mouseDoubleClickEvent

void
AnimationModuleView::onEditKeyFrameDialogFinished(bool /*accepted*/)
{
    EditKeyFrameDialog* dialog = qobject_cast<EditKeyFrameDialog*>( sender() );

    if (dialog) {
        dialog->close();
    }
}


bool
AnimationModuleViewPrivate::curveEditorMousePressEvent(QMouseEvent* e)
{

    if ( modCASIsControlAlt(e) ) { // Ctrl+Alt (Cmd+Alt on Mac) = insert keyframe
        ////
        // is the click near a curve?
        double xCurve, yCurve;
        CurveGuiPtr foundCurveNearby = isNearbyCurve( e->pos(), &xCurve, &yCurve );
        if (foundCurveNearby) {
            addKey(foundCurveNearby, xCurve, yCurve);
        }
        return true;
    }


    AnimationModuleBasePtr model = _model.lock();
    // is the click near the multiple-keyframes selection box center?
    if (model->getSelectionModel()->getSelectedKeyframesCount() > 1) {
        bool caughtBbox = true;
        if ( isNearbySelectedKeyFramesCrossWidget(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingKeys;
        } else if ( isNearbyBboxBtmLeft(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingBtmLeftBbox;
        } else if ( isNearbyBboxMidLeft(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingMidLeftBbox;
        } else if ( isNearbyBboxTopLeft(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingTopLeftBbox;
        } else if ( isNearbyBboxMidTop(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingMidTopBbox;
        } else if ( isNearbyBboxTopRight(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingTopRightBbox;
        } else if ( isNearbyBboxMidRight(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingMidRightBbox;
        } else if ( isNearbyBboxBtmRight(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingBtmRightBbox;
        } else if ( isNearbyBboxMidBtm(curveEditorZoomContext, curveEditorSelectedKeysBRect, e->pos() ) ) {
            state = eEventStateDraggingMidBtmBbox;
        } else {
            caughtBbox = false;
        }
        if (caughtBbox) {
            return true;
        }
    }
    // is the click near a keyframe manipulator?
    AnimItemDimViewKeyFrame nearbyKeyframe = isNearbyKeyFrame(curveEditorZoomContext, e->pos());
    if (nearbyKeyframe.id.item) {

        AnimItemDimViewKeyFramesMap keysToAdd;
        KeyFrameSet& keys = keysToAdd[nearbyKeyframe.id];
        keys.insert(nearbyKeyframe.key);
        model->getSelectionModel()->makeSelection(keysToAdd, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>(), (AnimationModuleSelectionModel::SelectionTypeAdd |
                                                                                                                           AnimationModuleSelectionModel::SelectionTypeClear) );
        state = eEventStateDraggingKeys;
        _publicInterface->setCursor( QCursor(Qt::CrossCursor) );
        return true;
    }


    ////
    // is the click near a derivative manipulator?
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > selectedTan = isNearbyTangent( e->pos() );

    //select the derivative only if it is not a constant keyframe
    if ( selectedTan.second.id.item && (selectedTan.second.key.getInterpolation() != eKeyframeTypeConstant) ) {
        state = eEventStateDraggingTangent;
        selectedDerivative = selectedTan;
        return true;
    }

    AnimItemDimViewKeyFrame nearbyKeyText = isNearbyKeyFrameText( e->pos() );
    if (nearbyKeyText.id.item) {
        // do nothing, doubleclick edits the text
        return true;
    }

    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame> tangentText = isNearbySelectedTangentText( e->pos() );
    if (tangentText.second.id.item) {
        // do nothing, doubleclick edits the text
        return true;
    }

    return false;
} // mousePressEvent

void
AnimationModuleViewPrivate::refreshCurveEditorSelectedKeysBRect()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if ( (curveEditorZoomContext.screenWidth() < 1) || (curveEditorZoomContext.screenHeight() < 1) ) {
        return;
    }

    AnimationModuleBasePtr model = _model.lock();
    AnimationModuleSelectionModelPtr selectModel = model->getSelectionModel();

    const AnimItemDimViewKeyFramesMap& selectedKeys = selectModel->getCurrentKeyFramesSelection();

    RectD keyFramesBbox;
    bool bboxSet = false;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
        for (KeyFrameSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            double x = it2->getTime();
            double y = it2->getValue();
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

    curveEditorSelectedKeysBRect = keyFramesBbox;
} // refreshCurveEditorSelectedKeysBRect



void
AnimationModuleView::onApplyLoopExpressionOnSelectedCurveActionTriggered()
{
    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }

    std::vector<CurveGuiPtr> curves = isAnimModule->getEditor()->getTreeView()->getSelectedCurves();
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
        ss << "curve(((frame - " << first << ") % (" << last << " - " << first << " + 1)) + " << first << ", " << curve->getDimension() << ", \"" <<  viewName << "\")";
        std::string script = ss.str();
        try {
            isAnimModule->getEditor()->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ), eExpressionLanguagePython );
        } catch(const std::exception& e) {
            Dialogs::errorDialog(tr("Animation Module").toStdString(), e.what());
        }

    }
} // loopSelectedCurve

void
AnimationModuleView::onApplyNegateExpressionOnSelectedCurveActionTriggered()
{

    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }

    std::vector<CurveGuiPtr> curves = isAnimModule->getEditor()->getTreeView()->getSelectedCurves();
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
    ss << "-curve(frame, " << curve->getDimension() << ", \"" <<  viewName << "\")";
    std::string script = ss.str();
    try {
        isAnimModule->getEditor()->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ), eExpressionLanguagePython );
    } catch(const std::exception& e) {
        Dialogs::errorDialog(tr("Animation Module").toStdString(), e.what());
    }

} // negateSelectedCurve

void
AnimationModuleView::onApplyReverseExpressionOnSelectedCurveActionTriggered()
{
    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }

    std::vector<CurveGuiPtr> curves = isAnimModule->getEditor()->getTreeView()->getSelectedCurves();
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
    ss << "curve(-frame, " << curve->getDimension() << ", \"" <<  viewName << "\")";
    std::string script = ss.str();
    try {
        isAnimModule->getEditor()->setSelectedCurveExpression( QString::fromUtf8( script.c_str() ), eExpressionLanguagePython );
    } catch(const std::exception& e) {
        Dialogs::errorDialog(tr("Animation Module").toStdString(), e.what());
    }

} // reverseSelectedCurve

void
AnimationModuleView::onExportCurveToAsciiActionTriggered()
{
    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }

    std::vector<CurveGuiPtr> curves = isAnimModule->getEditor()->getTreeView()->getSelectedCurves();
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
        double xstart = dialog.getXStart();
        int count = dialog.getXCount();
        double incr = dialog.getXIncrement();
        std::map<int, CurveGuiPtr> columns;
        dialog.getCurveColumns(&columns);

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            //double end = xstart + (count-1) * incr;
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ((int)incr != incr) || ((int)xstart != xstart ) )) {
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

        for (int i = 0; i < count; ++i) {
            for (int c = 0; c < columnsCount; ++c) {
                std::map<int, CurveGuiPtr>::const_iterator foundCurve = columns.find(c);
                if ( foundCurve != columns.end() ) {
                    QString str = QString::number(foundCurve->second->evaluate(true, xstart + i * incr), 'f', 10);
                    ts << str;
                } else {
                    ts <<  0;
                }
                if (c < columnsCount - 1) {
                    ts << ' ';
                }
            }
            ts << '\n';
        }


        ///close the file
        file.close();
    }
} // exportCurveToAscii

void
AnimationModuleView::onImportCurveFromAsciiActionTriggered()
{
    AnimationModulePtr isAnimModule = toAnimationModule(_imp->_model.lock());
    if (!isAnimModule) {
        return;
    }


    std::vector<CurveGuiPtr> curves = isAnimModule->getEditor()->getTreeView()->getSelectedCurves();
    if ( curves.empty() ) {
        Dialogs::warningDialog( tr("Curve Editor").toStdString(), tr("You must select a curve first").toStdString() );
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

        double xstart = dialog.getXStart();
        double incr = dialog.getXIncrement();
        std::map<int, CurveGuiPtr> columns;
        dialog.getCurveColumns(&columns);
        assert( !columns.empty() );

        for (U32 i = 0; i < curves.size(); ++i) {
            ///if the curve only supports integers values for X steps, and values are not rounded warn the user that the settings are not good
            if ( curves[i]->areKeyFramesTimeClampedToIntegers() &&
                 ( ( (int)incr != incr) || ( (int)xstart != xstart ) ) ) {
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

            for (std::map<int, CurveGuiPtr>::const_iterator col = columns.begin(); col != columns.end(); ++col) {
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
            KeyFrameSet& keys = finalValues[id];

            const std::vector<double> & values = it->second;
            double xIndex = xstart;
            for (U32 i = 0; i < values.size(); ++i) {
                KeyFrame k(xIndex, values[i], 0., 0., eKeyframeTypeLinear);
                keys.insert(k);
                xIndex += incr;
            }
        }

        AnimationModuleBasePtr model = _imp->_model.lock();


        model->pushUndoCommand( new AddKeysCommand(finalValues, model, true /*clearExistingAnimation*/));

    }
} // importCurveFromAscii

void
AnimationModuleView::setCustomInteract(const OverlayInteractBasePtr & interactDesc)
{
    _imp->customInteract = interactDesc;
}

OverlayInteractBasePtr
AnimationModuleView::getCustomInteract() const
{
    return _imp->customInteract.lock();
}

void
AnimationModuleViewPrivate::addKey(const CurveGuiPtr& curve, double xCurve, double yCurve)
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
    KeyFrameSet& keys = keysToAdd[itemKey];

    AnimationModuleBasePtr model = _model.lock();
    KeyFrame k(xCurve, yCurve, 0, 0);
    keys.insert(k);
    model->pushUndoCommand( new AddKeysCommand(keysToAdd, model, false /*clearAndAdd*/) );

    state = eEventStateDraggingKeys;
    _publicInterface->setCursor( QCursor(Qt::CrossCursor) );

    model->getSelectionModel()->makeSelection(keysToAdd, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr>(), (AnimationModuleSelectionModel::SelectionTypeAdd | AnimationModuleSelectionModel::SelectionTypeClear));

} // addKey

void
AnimationModuleView::getKeyTangentPoints(KeyFrameSet::const_iterator it,
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
    QPointF keyWidgetCoord = _imp->curveEditorZoomContext.toWidgetCoordinates(x, y);
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
            double prevKeyXWidgetCoord = _imp->curveEditorZoomContext.toWidgetCoordinates(prevTime, 0).x();
            //set the left derivative X to be at 1/3 of the interval [prev,k], and clamp it to 1/8 of the widget width.
            leftTanXWidgetDiffMax = std::min(leftTanXWidgetDiffMax, (keyWidgetCoord.x() - prevKeyXWidgetCoord) * (1. / 3));
        }
        //clamp the left derivative Y to 1/8 of the widget height.
        double leftTanYWidgetDiffMax = std::min( h / 8., leftTanXWidgetDiffMax);
        assert(leftTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(leftTanYWidgetDiffMax >= 0.);
        
        QPointF tanMax = _imp->curveEditorZoomContext.toZoomCoordinates(keyWidgetCoord.x() + leftTanXWidgetDiffMax, keyWidgetCoord.y() - leftTanYWidgetDiffMax) - QPointF(x, y);
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
            double nextKeyXWidgetCoord = _imp->curveEditorZoomContext.toWidgetCoordinates(nextTime, 0).x();
            //set the right derivative X to be at 1/3 of the interval [k,next], and clamp it to 1/8 of the widget width.
            rightTanXWidgetDiffMax = std::min(rightTanXWidgetDiffMax, ( nextKeyXWidgetCoord - keyWidgetCoord.x() ) * (1. / 3));
        }
        //clamp the right derivative Y to 1/8 of the widget height.
        double rightTanYWidgetDiffMax = std::min( h / 8., rightTanXWidgetDiffMax);
        assert(rightTanXWidgetDiffMax >= 0.); // both bounds should be positive
        assert(rightTanYWidgetDiffMax >= 0.);
        
        QPointF tanMax = _imp->curveEditorZoomContext.toZoomCoordinates(keyWidgetCoord.x() + rightTanXWidgetDiffMax, keyWidgetCoord.y() - rightTanYWidgetDiffMax) - QPointF(x, y);
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


NATRON_NAMESPACE_EXIT

