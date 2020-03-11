/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef Gui_ViewerGLPrivate_h
#define Gui_ViewerGLPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QSize>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Gui/TextRenderer.h"
#include "Gui/ViewerGL.h"
#include "Gui/GuiGLContext.h"
#include "Gui/ZoomContext.h"
#include "Gui/GuiFwd.h"

#define MAX_MIP_MAP_LEVELS 20

NATRON_NAMESPACE_ENTER

/*This class is the the core of the viewer : what displays images, overlays, etc...
   Everything related to OpenGL will (almost always) be in this class */


/**
 *@enum MouseStateEnum
 *@brief basic state switching for mouse events
 **/
enum MouseStateEnum
{
    eMouseStateSelecting = 0,
    eMouseStateDraggingImage,
    eMouseStatePickingColor,
    eMouseStatePickingInputColor,
    eMouseStateBuildingPickerRectangle,
    eMouseStateZoomingImage,
    eMouseStateUndefined
};



enum PickerStateEnum
{
    ePickerStateInactive = 0,
    ePickerStatePoint,
    ePickerStateRectangle
};

struct TextureInfo
{
    TextureInfo()
    : texture()
    , colorPickerImage()
    , colorPickerInputImage()
    , mipMapLevel(0)
    , premult(eImagePremultiplicationOpaque)
    , time(0)
    , rod()
    , format()
    , pixelAspectRatio(1.)
    , isPartialImage(false)
    , isVisible(false)
    {
    }

    GLTexturePtr texture;

    ImagePtr colorPickerImage, colorPickerInputImage;

    unsigned int mipMapLevel;

    // These are meta-datas at the time the texture was uploaded
    ImagePremultiplicationEnum premult;
    TimeValue time;
    RectD rod;
    RectD format;
    double pixelAspectRatio;

    bool isPartialImage;

    // false if this input is disconnected for the viewer
    bool isVisible;
};

struct ViewerGL::Implementation
{
    Implementation(ViewerGL* this_,
                   ViewerTab* parent);

    ~Implementation();

    ViewerGL* _this; // link to parent, for access to public methods

    /////////////////////////////////////////////////////////
    // The following are only accessed from the main thread:
    std::vector<GLuint> pboIds; //!< PBO's id's used by the OpenGL context
    GLuint vboVerticesId; //!< VBO holding the vertices for the texture mapping.
    GLuint vboTexturesId; //!< VBO holding texture coordinates.
    GLuint iboTriangleStripId; /*!< IBOs holding vertices indexes for triangle strip sets*/

    // Protects displayTextures, partialUpdateTextures, currentViewerInfo_btmLeftBBOXoverlay currentViewerInfo_topRightBBOXoverlay currentViewerInfo_resolutionOverlay
    mutable QMutex displayDataMutex;

    TextureInfo displayTextures[2]; /*!< A pointer to the textures that would be used if A and B are displayed*/
    std::vector<TextureInfo> partialUpdateTextures; /*!< Pointer to the partial rectangle textures overlayed onto the displayed texture when tracking*/
    InfoViewerWidget* infoViewer[2]; /*!< Pointer to the info bar below the viewer holding pixel/mouse/format related info*/
    ViewerTab* const viewerTab; /*!< Pointer to the viewer tab GUI*/
    bool zoomOrPannedSinceLastFit; //< true if the user zoomed or panned the image since the last call to fitToRoD
    QPoint oldClick;
    ViewerColorSpaceEnum displayingImageLut;
    MouseStateEnum ms; /*!< Holds the mouse state*/
    const QColor textRenderingColor;
    const QColor displayWindowOverlayColor;
    const QColor rodOverlayColor;
    QFont textFont;
    bool updatingTexture;
    QColor clearColor;
    QStringList persistentMessages;
    int persistentMessageType;
    bool displayPersistentMessage;
    TextRenderer textRenderer;
    QPoint lastMousePosition; //< in widget coordinates
    QPointF lastDragStartPos; //< in zoom coordinates
    bool hasMovedSincePress;

    /////// currentViewerInfo
    QString currentViewerInfo_btmLeftBBOXoverlay[2]; /*!< The string holding the bottom left corner coordinates of the dataWindow*/
    QString currentViewerInfo_topRightBBOXoverlay[2]; /*!< The string holding the top right corner coordinates of the dataWindow*/
    QString currentViewerInfo_resolutionOverlay[2]; /*!< The string holding the resolution overlay, e.g: "1920x1080"*/

    ///////Picker info, used only by the main-thread
    PickerStateEnum pickerState;
    QPointF lastPickerPos;
    QRectF pickerRect;

    // projection info, only used by the main thread
    QPointF glShadow; //!< pixel size in projection coordinates - used to create shadow

    //////////////////////////////////////////////////////////
    // The following are accessed from various threads
    ZoomContext zoomCtx; /*!< All zoom related variables are packed into this object. */
    mutable QMutex zoomCtxMutex; /// protectx zoomCtx*

    QRectF selectionRectangle;
    GLuint checkerboardTextureID;
    int checkerboardTileSize; // to avoid a call to getValue() of the settings at each draw
    GLuint savedTexture; // @see saveOpenGLContext/restoreOpenGLContext
    GLuint prevBoundTexture;
    QSize sizeH;
    PenType pointerTypeOnPress;
    double pressureOnPress, pressureOnRelease;
    int wheelDeltaSeekFrame; // accumulated wheel delta for frame seeking (crtl+wheel)
    bool isUpdatingTexture;
    bool renderOnPenUp;
    int updateViewerPboIndex;  // always accessed in the main thread: initialized in the constructor, then always accessed and modified by updateViewer()

    // A map storing the hash of the viewerProcess A node accross time.
    // This is used to display the timeline cache bar.
    ViewerCachedImagesMap uploadedTexturesViewerHash;

    // Protects uploadedTexturesViewerHash
    mutable QMutex uploadedTexturesViewerHashMutex;

    // A wrapper around the OpenGL context managed by QGLWidget
    // so that we can use the same API for the internal background rendering functions
    // as with OSGLContext
    boost::shared_ptr<GuiGLContext> glContextWrapper;


public:

    /**
     *@brief Fill the rendering VAO with vertices and texture coordinates
     * that depends upon the currently displayed texture.
     **/
    void drawRenderingVAO(unsigned int mipMapLevel, int textureIndex, DrawPolygonModeEnum polygonMode, bool background);

    void initializeGL();


    enum WipePolygonEnum
    {
        eWipePolygonEmpty = 0,  // don't draw anything
        eWipePolygonFull,  // draw the whole texture as usual
        eWipePolygonPartial, // use the polygon returned to draw
    };

    WipePolygonEnum getWipePolygon(const RectD & texRectClipped, //!< in canonical coordinates
                                   bool rightPlane,
                                   QPolygonF * polygonPoints) const;

    static void getPolygonTextureCoordinates(const QPolygonF & polygonPoints,
                                             const RectD & texRect, //!< in canonical coordinates
                                             QPolygonF & texCoords);

    void refreshSelectionRectangle(const QPointF & pos);

    void drawSelectionRectangle();

    void initializeCheckerboardTexture(bool mustCreateTexture);

    void drawCheckerboardTexture(const RectD& rod);

    void drawCheckerboardTexture(const QPolygonF& polygon);


private:
    /**
     *@brief Checks extensions and init glew on windows. Called by  initializeGL()
     **/
    bool initAndCheckGlExtensions ();
};

NATRON_NAMESPACE_EXIT

#endif //_Gui_ViewerGLPrivate_h_
