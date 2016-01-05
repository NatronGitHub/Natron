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
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/ImageInfo.h"

#include "Gui/TextRenderer.h"
#include "Gui/ViewerGL.h"
#include "Gui/ZoomContext.h"
#include "Gui/GuiFwd.h"


#define WIPE_MIX_HANDLE_LENGTH 50.
#define WIPE_ROTATE_HANDLE_LENGTH 100.
#define WIPE_ROTATE_OFFSET 30


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
    eMouseStateDraggingRoiLeftEdge,
    eMouseStateDraggingRoiRightEdge,
    eMouseStateDraggingRoiTopEdge,
    eMouseStateDraggingRoiBottomEdge,
    eMouseStateDraggingRoiTopLeft,
    eMouseStateDraggingRoiTopRight,
    eMouseStateDraggingRoiBottomRight,
    eMouseStateDraggingRoiBottomLeft,
    eMouseStateDraggingRoiCross,
    eMouseStateBuildingUserRoI,
    eMouseStatePickingColor,
    eMouseStateBuildingPickerRectangle,
    eMouseStateDraggingWipeCenter,
    eMouseStateDraggingWipeMixHandle,
    eMouseStateRotatingWipeHandle,
    eMouseStateZoomingImage,
    eMouseStateUndefined
};

enum HoverStateEnum
{
    eHoverStateNothing = 0,
    eHoverStateWipeMix,
    eHoverStateWipeRotateHandle
};

enum PickerStateEnum
{
    ePickerStateInactive = 0,
    ePickerStatePoint,
    ePickerStateRectangle
};

struct ViewerGL::Implementation
{

    Implementation(ViewerGL* this_, ViewerTab* parent);

    ~Implementation();
    
    ViewerGL* _this; // link to parent, for access to public methods

    /////////////////////////////////////////////////////////
    // The following are only accessed from the main thread:

    std::vector<GLuint> pboIds; //!< PBO's id's used by the OpenGL context
    //   GLuint vaoId; //!< VAO holding the rendering VBOs for texture mapping.
    GLuint vboVerticesId; //!< VBO holding the vertices for the texture mapping.
    GLuint vboTexturesId; //!< VBO holding texture coordinates.
    GLuint iboTriangleStripId; /*!< IBOs holding vertices indexes for triangle strip sets*/
    Texture* activeTextures[2]; /*!< A pointer to the current textures used to display. One for A and B. May point to blackTex */
    Texture* displayTextures[2]; /*!< A pointer to the textures that would be used if A and B are displayed*/
    QGLShaderProgram* shaderRGB; /*!< The shader program used to render RGB data*/
    QGLShaderProgram* shaderBlack; /*!< The shader program used when the viewer is disconnected.*/
    bool shaderLoaded; /*!< Flag to check whether the shaders have already been loaded.*/
    InfoViewerWidget* infoViewer[2]; /*!< Pointer to the info bar below the viewer holding pixel/mouse/format related info*/
    ViewerTab* const viewerTab; /*!< Pointer to the viewer tab GUI*/
    bool zoomOrPannedSinceLastFit; //< true if the user zoomed or panned the image since the last call to fitToRoD
    QPoint oldClick;
    ImageInfo blankViewerInfo; /*!< Pointer to the info used when the viewer is disconnected.*/
    double displayingImageGain[2];
    double displayingImageGamma[2];
    double displayingImageOffset[2];
    unsigned int displayingImageMipMapLevel[2];
    Natron::ImagePremultiplicationEnum displayingImagePremult[2];
    int displayingImageTime[2];
    Natron::ViewerColorSpaceEnum displayingImageLut;
    MouseStateEnum ms; /*!< Holds the mouse state*/
    HoverStateEnum hs;
    const QColor textRenderingColor;
    const QColor displayWindowOverlayColor;
    const QColor rodOverlayColor;
    QFont* textFont;
    bool overlay; /*!< True if the user enabled overlay dispay*/

    // supportsGLSL is accessed from several threads, but is set only once at startup
    bool supportsGLSL; /*!< True if the user has a GLSL version supporting everything requested.*/
    bool updatingTexture;
    QColor clearColor;
    QMenu* menu;
    QStringList persistentMessages;
    int persistentMessageType;
    bool displayPersistentMessage;
    Natron::TextRenderer textRenderer;
    
    
    QPoint lastMousePosition; //< in widget coordinates
    QPointF lastDragStartPos; //< in zoom coordinates
    bool hasMovedSincePress;

    /////// currentViewerInfo
    ImageInfo currentViewerInfo[2]; /*!< Pointer to the ViewerInfo  used for rendering*/
    
    mutable QMutex projectFormatMutex;
    Format projectFormat;
    QString currentViewerInfo_btmLeftBBOXoverlay[2]; /*!< The string holding the bottom left corner coordinates of the dataWindow*/
    QString currentViewerInfo_topRightBBOXoverlay[2]; /*!< The string holding the top right corner coordinates of the dataWindow*/
    QString currentViewerInfo_resolutionOverlay; /*!< The string holding the resolution overlay, e.g: "1920x1080"*/

    ///////Picker info, used only by the main-thread
    PickerStateEnum pickerState;
    QPointF lastPickerPos;
    QRectF pickerRect;

    // projection info, only used by the main thread
    QPointF glShadow; //!< pixel size in projection coordinates - used to create shadow

    //////////////////////////////////////////////////////////
    // The following are accessed from various threads
    QMutex userRoIMutex;
    bool userRoIEnabled;
    RectD userRoI; //< in canonical coords
    bool buildUserRoIOnNextPress;
    RectD draggedUserRoI;
    
    ZoomContext zoomCtx; /*!< All zoom related variables are packed into this object. */
    mutable QMutex zoomCtxMutex; /// protectx zoomCtx*
    QMutex clipToDisplayWindowMutex;
    bool clipToDisplayWindow;
    mutable QMutex wipeControlsMutex;
    double mixAmount; /// the amount of the second input to blend, by default 1.
    double wipeAngle; /// the angle to the X axis
    QPointF wipeCenter; /// the center of the wipe control
    QRectF selectionRectangle;
    
    GLuint checkerboardTextureID;
    int checkerboardTileSize; // to avoid a call to getValue() of the settings at each draw

    GLuint savedTexture; // @see saveOpenGLContext/restoreOpenGLContext
    GLuint prevBoundTexture; // @see bindTextureAndActivateShader/unbindTextureAndReleaseShader

    mutable QMutex lastRenderedImageMutex; //protects lastRenderedImage & memoryHeldByLastRenderedImages
    std::vector<Natron::ImageList> lastRenderedTiles[2]; //<  last image passed to transferRAMBuffer
    U64 memoryHeldByLastRenderedImages[2];
    
    QSize sizeH;

    Natron::PenType pointerTypeOnPress;
    bool subsequentMousePressIsTablet;
    double pressureOnPress, pressureOnRelease;

    int wheelDeltaSeekFrame; // accumulated wheel delta for frame seeking (crtl+wheel)
    
    RectD lastTextureRoi;
    bool isUpdatingTexture;
    
    bool renderOnPenUp;

public:

    /**
     *@brief Fill the rendering VAO with vertices and texture coordinates
     * that depends upon the currently displayed texture.
     **/
    void drawRenderingVAO(unsigned int mipMapLevel,int textureIndex,DrawPolygonModeEnum polygonMode);

    void initializeGL();

    bool isNearbyWipeCenter(const QPointF & pos,double zoomScreenPixelWidth, double zoomScreenPixelHeight ) const;
    bool isNearbyWipeRotateBar(const QPointF & pos,double zoomScreenPixelWidth, double zoomScreenPixelHeight) const;
    bool isNearbyWipeMixHandle(const QPointF & pos,double zoomScreenPixelWidth, double zoomScreenPixelHeight) const;

    void drawArcOfCircle(const QPointF & center,double radiusX,double radiusY,double startAngle,double endAngle);

    void bindTextureAndActivateShader(int i,
                                      bool useShader);

    void unbindTextureAndReleaseShader(bool useShader);

    /**
     *@brief Starts using the RGB shader to display the frame
     **/
    void activateShaderRGB(int texIndex);

    enum WipePolygonEnum
    {
        eWipePolygonEmpty = 0,  // don't draw anything
        eWipePolygonFull,  // draw the whole texture as usual
        eWipePolygonPartial, // use the polygon returned to draw
    };

    WipePolygonEnum getWipePolygon(const RectD & texRectClipped, //!< in canonical coordinates
                                   QPolygonF & polygonPoints,
                                   bool rightPlane) const;

    static void getBaseTextureCoordinates(const RectI & texRect,int closestPo2,int texW,int texH,
                                          GLfloat & bottom,GLfloat & top,GLfloat & left,GLfloat & right);
    static void getPolygonTextureCoordinates(const QPolygonF & polygonPoints,
                                             const RectD & texRect, //!< in canonical coordinates
                                             QPolygonF & texCoords);

    void refreshSelectionRectangle(const QPointF & pos);

    void drawSelectionRectangle();
    
    void initializeCheckerboardTexture(bool mustCreateTexture);
    
    void drawCheckerboardTexture(const RectD& rod);
    
    void getProjectFormatCanonical(RectD& canonicalProjectFormat) const
    {
        QMutexLocker k(&projectFormatMutex);
        canonicalProjectFormat = projectFormat.toCanonicalFormat();
    }

private:
    /**
     *@brief Checks extensions and init glew on windows. Called by  initializeGL()
     **/
    void initAndCheckGlExtensions ();
};


#endif //_Gui_ViewerGLPrivate_h_