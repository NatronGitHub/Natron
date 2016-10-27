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

#ifndef NATRON_GUI_ANIMATIONMODULEVIEWPRIVATEBASE_H
#define NATRON_GUI_ANIMATIONMODULEVIEWPRIVATEBASE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/GuiFwd.h"

#include <QCoreApplication>
#include "Gui/AnimationModuleViewBase.h"

#define KF_TEXTURES_COUNT 18
#define KF_PIXMAP_SIZE 14
#define KF_X_OFFSET 7

// in pixels
#define CLICK_DISTANCE_TOLERANCE 5
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8
#define BOUNDING_BOX_HANDLE_SIZE 4
#define XHAIR_SIZE 20

NATRON_NAMESPACE_ENTER;

class AnimationViewBase;
/**
 * @class An internal class used by both CurveWidget and DopeSheetView to share common implementations
 **/
class AnimationModuleViewPrivateBase
{

    Q_DECLARE_TR_FUNCTIONS(AnimationModuleViewPrivateBase)

public:

    enum KeyframeTexture
    {
        kfTextureNone = -2,
        kfTextureInterpConstant = 0,
        kfTextureInterpConstantSelected,

        kfTextureInterpLinear,
        kfTextureInterpLinearSelected,

        kfTextureInterpCurve,
        kfTextureInterpCurveSelected,

        kfTextureInterpBreak,
        kfTextureInterpBreakSelected,

        kfTextureInterpCurveC,
        kfTextureInterpCurveCSelected,

        kfTextureInterpCurveH,
        kfTextureInterpCurveHSelected,

        kfTextureInterpCurveR,
        kfTextureInterpCurveRSelected,

        kfTextureInterpCurveZ,
        kfTextureInterpCurveZSelected,

        kfTextureMaster,
        kfTextureMasterSelected,
    };
    


    AnimationModuleViewPrivateBase(Gui* gui, AnimationViewBase* widget, const AnimationModuleBasePtr& model);

    virtual ~AnimationModuleViewPrivateBase();

    void drawTimelineMarkers();

    void drawSelectionRectangle();

    void drawSelectedKeyFramesBbox();

    void drawTexturedKeyframe(AnimationModuleViewPrivateBase::KeyframeTexture textureType,
                             bool drawTime,
                             double time,
                             const QColor& textColor,
                             const RectD &rect) const;


    void drawSelectionRect() const;

    void renderText(double x, double y, const QString & text, const QColor & color, const QFont & font, int flags = 0) const;

    void generateKeyframeTextures();

    AnimationModuleViewPrivateBase::KeyframeTexture kfTextureFromKeyframeType(KeyframeTypeEnum kfType, bool selected) const;

    bool isNearbyTimelineTopPoly(const QPoint & pt) const;

    bool isNearbyTimelineBtmPoly(const QPoint & pt) const;


    bool isNearbySelectedKeyFramesCrossWidget(const QPoint & pt) const;
    bool isNearbyBboxTopLeft(const QPoint& pt) const;
    bool isNearbyBboxMidLeft(const QPoint& pt) const;
    bool isNearbyBboxBtmLeft(const QPoint& pt) const;
    bool isNearbyBboxMidBtm(const QPoint& pt) const;
    bool isNearbyBboxBtmRight(const QPoint& pt) const;
    bool isNearbyBboxMidRight(const QPoint& pt) const;
    bool isNearbyBboxTopRight(const QPoint& pt) const;
    bool isNearbyBboxMidTop(const QPoint& pt) const;


    std::vector<CurveGuiPtr> getSelectedCurves() const;


    void createMenu();

    // Called in the end of createMenu
    virtual void addMenuOptions();

public:

    // ptr to the widget
    AnimationViewBase* _publicInterface;

    Gui* _gui;

    // weak ref to the animation module
    AnimationModuleBaseWPtr _model;

    // protects zoomCtx for serialization thread
    mutable QMutex zoomCtxMutex;
    ZoomContext zoomCtx;
    bool zoomOrPannedSinceLastFit;

    TextRenderer textRenderer;

    // for keyframe selection
    // This is in zoom coordinates...
    RectD selectionRect;

    // keyframe selection rect
    // This is in zoom coordinates
    RectD selectedKeysBRect;

    // keframe type textures
    GLuint kfTexturesIDs[KF_TEXTURES_COUNT];

    // Right click menu
    Menu* _rightClickMenu;

    GLuint savedTexture;

    bool drawnOnce;



};

NATRON_NAMESPACE_EXIT;
#endif // NATRON_GUI_ANIMATIONMODULEVIEWPRIVATEBASE_H
