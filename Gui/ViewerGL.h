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

#ifndef NATRON_GUI_VIEWERGL_H
#define NATRON_GUI_VIEWERGL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <utility>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/OpenGLViewerI.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/ViewIdx.h"
#include "Engine/Color.h"
#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER

typedef std::map<FrameViewPair, ImageCacheKeyPtr, FrameView_compare_less> ViewerCachedImagesMap;

/**
 *@class ViewerGL
 *@brief The main viewport. This class is part of the ViewerTab GUI and handles all
 * OpenGL related code as well as user events.
 **/
class ViewerGL
    : public QGLWidget
    , public OpenGLViewerI
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    struct Implementation;



    /*3 different constructors, that all take a different parameter related to OpenGL or Qt widget parenting.
       When constructing a viewer for the 1st time in the app, you must pass a NULL shareWidget. Otherwise,you
       can pass a pointer to the 1st viewer you created. It allows the viewers to share the same OpenGL context.
     */
    explicit ViewerGL(ViewerTab* parent,
                      const QGLWidget* shareWidget = NULL);


    virtual ~ViewerGL() OVERRIDE;

    virtual QSize sizeHint() const OVERRIDE FINAL;
    const QFont & textFont() const;

    void setTextFont(const QFont & f);


    /**
     *@returns Returns true if the viewer is displaying something.
     **/
    bool displayingImage() const;


    /**
     *@returns Returns a const reference to the dataWindow of the currentFrame(BBOX) in canonical coordinates
     **/
    // MT-SAFE: used only by the main-thread, can return a const ref
    RectD getRoD(int textureIndex) const;


    /**
     *@returns Returns the displayWindow of the currentFrame(Resolution)
     **/
    RectD getCanonicalFormat(int texIndex) const;

    double getPAR(int texIndex) const;



    /**
     *@returns Returns the height of the frame with the scale factor applied to it.
     **/
    //double zoomedHeight(){return std::floor(getDisplayWindow().height()*_zoomCtx._zoomFactor);}

    /**
     *@returns Returns the width of the frame with the scale factor applied to it.
     **/
    //double zoomedWidth(){return std::floor(getDisplayWindow().width()*_zoomCtx._zoomFactor);}

    /**
     *@returns Returns the current zoom factor that is applied to the display.
     **/
    double getZoomFactor() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns the rectangle of the image displayed by the viewer
     **/
    virtual RectD getImageRectangleDisplayed() const OVERRIDE FINAL;

    /**
     *@brief Set the pointer to the InfoViewerWidget. This is called once after creation
     * of the ViewerGL.
     **/
    void setInfoViewer(InfoViewerWidget* i, int textureIndex);

    /**
     *@brief Handy function that zoom automatically the viewer so it fit
     * the displayWindow  entirely in the viewer
     **/
    virtual void fitImageToFormat() OVERRIDE FINAL;

    virtual void clearPartialUpdateTextures() OVERRIDE FINAL;
    virtual bool isViewerUIVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void refreshMetadata(int inputNb, const NodeMetadata& metadata) OVERRIDE FINAL;

    virtual void clearLastRenderedImage() OVERRIDE FINAL;
    
    /**
     *@brief Copies the data stored in the  RAM buffer into the currently
     * used texture.
     * It does:
     * 1) glMapBuffer
     * 2) memcpy to copy data from RAM to GPU
     * 3) glUnmapBuffer
     * 4) glTexSubImage2D or glTexImage2D depending whether we resize the texture or not.
     **/
    virtual void transferBufferFromRAMtoGPU(const TextureTransferArgs& args) OVERRIDE FINAL;


    virtual void disconnectInputTexture(int textureIndex, bool clearRoD) OVERRIDE FINAL;


    /**
     *@brief Disconnects the viewer.
     * Clears out the viewer and reset the viewer info. Note that calling this
     * function while the engine is processing will abort the engine.
     **/
    void disconnectViewer();

    void updatePersistentMessage();
    void updatePersistentMessageToWidth(int w);


    virtual void getViewerFrameRange(int* first, int* last) const OVERRIDE FINAL;

    bool operatorIsWipe(ViewerCompositingOperatorEnum compOperator)
    {
        return (compOperator != eViewerCompositingOperatorNone &&
                compOperator != eViewerCompositingOperatorStackUnder &&
                compOperator != eViewerCompositingOperatorStackOver &&
                compOperator != eViewerCompositingOperatorStackMinus &&
                compOperator != eViewerCompositingOperatorStackOnionSkin &&
                true);
    }

    bool operatorIsStack(ViewerCompositingOperatorEnum compOperator)
    {
        return (compOperator != eViewerCompositingOperatorNone &&
                compOperator != eViewerCompositingOperatorWipeUnder &&
                compOperator != eViewerCompositingOperatorWipeOver &&
                compOperator != eViewerCompositingOperatorWipeMinus &&
                compOperator != eViewerCompositingOperatorWipeOnionSkin &&
                true);
    }

public Q_SLOTS:


    void setRegionOfDefinition(const RectD & rod, double par, int textureIndex);
    void setFormat(const std::string& formatName, const RectD& format, double par, int textureIndex);

    virtual void updateColorPicker(int textureIndex, int x = INT_MAX, int y = INT_MAX) OVERRIDE FINAL;

    void clearColorBuffer(double r = 0., double g = 0., double b = 0., double a = 1.);

    void onCheckerboardSettingsChanged();


    /**
     * @brief Reset the wipe position so it is in the center of the B input.
     * If B input is disconnected it goes in the middle of the A input.
     * Otherwise it goes in the middle of the project window
     **/
    void resetWipeControls();


public:

    virtual OSGLContextPtr getOpenGLViewerContext() const OVERRIDE FINAL;
    virtual void removeGUI() OVERRIDE FINAL;
    virtual TimeLinePtr getTimeline() const OVERRIDE FINAL;

public:

    virtual bool renderText(double x,
                            double y,
                            const std::string &string,
                            double r,
                            double g,
                            double b,
                            double a,
                            int flags = 0) OVERRIDE FINAL; //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10


    void renderText(double x,
                    double y,
                    const QString &string,
                    const QColor & color,
                    const QFont & font,
                    int flags = 0); //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10

    virtual void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const OVERRIDE FINAL;

    virtual void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio) OVERRIDE FINAL;

    virtual void translateViewport(double dx, double dy) OVERRIDE FINAL;

    virtual void zoomViewport(double newZoomFactor) OVERRIDE FINAL;

    virtual void setInfoBarVisible(bool visible) OVERRIDE FINAL;

    virtual void setInfoBarVisible(int index, bool visible) OVERRIDE FINAL;

    virtual void setLeftToolBarVisible(bool visible) OVERRIDE FINAL;

    virtual void setTopToolBarVisible(bool visible) OVERRIDE FINAL;

    virtual void setPlayerVisible(bool visible) OVERRIDE FINAL;

    virtual void setTimelineVisible(bool visible) OVERRIDE FINAL;

    virtual void setTabHeaderVisible(bool visible) OVERRIDE FINAL;

    virtual void setTimelineBounds(double first, double last) OVERRIDE FINAL;

    virtual void setTripleSyncEnabled(bool toggled) OVERRIDE FINAL;

    /**
     * @brief Swap the OpenGL buffers.
     **/
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;

    /**
     * @brief update()
     **/
    virtual void redraw() OVERRIDE FINAL;

    /**
     * @brief updateGL();
     **/
    virtual void redrawNow() OVERRIDE FINAL;

    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE FINAL;
    
    /**
     * @brief Returns the width and height of the viewport in window coordinates.
     **/
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;

    /**
     * @brief Returns the pixel scale of the viewport.
     **/
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;

#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE FINAL;
#endif

    /**
     * @brief Returns the colour of the background (i.e: clear color) of the viewport.
     **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void getTextureColorAt(int x, int y, double* r, double *g, double *b, double *a) OVERRIDE FINAL;

    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x,
                                     double *y) const OVERRIDE FINAL
    {
        QPointF p = toWidgetCoordinates( QPointF(*x, *y) );

        *x = p.x();
        *y = p.y();
    }

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x,
                                        double *y) const OVERRIDE FINAL
    {
        QPointF p = toZoomCoordinates( QPointF(*x, *y) );

        *x = p.x();
        *y = p.y();
    }

    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    ViewerNodePtr getInternalNode() const;
    ViewerTab* getViewerTab() const;

    /**
     * @brief Returns the viewport visible portion in canonical coordinates
     **/
    virtual RectD getViewportRect() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns the cursor position in canonical coordinates
     **/
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;

    virtual RangeD getFrameRange() const OVERRIDE FINAL;

    virtual ViewerNodePtr getViewerNode() const OVERRIDE FINAL;

    void setZoomOrPannedSinceLastFit(bool enabled);

    bool getZoomOrPannedSinceLastFit() const;

    ///Not MT-Safe
    void getSelectionRectangle(double &left, double &right, double &bottom, double &top) const;

    /**
     * @brief Must save all relevant OpenGL bits so that they can be restored as-is after the draw action of a plugin.
     **/
    virtual void saveOpenGLContext() OVERRIDE FINAL;

    /**
     * @brief Must restore all OpenGL bits saved in saveOpenGLContext()
     **/
    virtual void restoreOpenGLContext() OVERRIDE FINAL;

    /**
     * @brief Get the color of the currently displayed image at position x,y.
     * @param forceLinear If true, then it will not use the viewer current colorspace
     * to get r,g and b values, otherwise the color returned will be in the same color-space
     * than the one chosen by the user on the gui.
     * X and Y are in CANONICAL COORDINATES
     * @return true if the point is inside the image and colors were set
     **/
    bool getColorAt(double x, double y, bool forceLinear, int textureIndex, bool pickInput, float* r,
                    float* g, float* b, float* a, unsigned int* mipMapLevel) WARN_UNUSED_RETURN;

    // same as getColor, but computes the mean over a given rectangle
    bool getColorAtRect(const RectD &rect, // rectangle in canonical coordinates
                        bool forceLinear, int textureIndex, bool pickInput, float* r, float* g, float* b, float* a, unsigned int* mipMapLevel);


    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL;

    ///same as getMipMapLevel but with the zoomFactor taken into account
    int getMipMapLevelCombinedToZoomFactor() const WARN_UNUSED_RETURN;

    virtual TimeValue getCurrentlyDisplayedTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    QPointF toZoomCoordinates(const QPointF& position) const;

    QPointF toWidgetCoordinates(const QPointF& position) const;

    void getTopLeftAndBottomRightInZoomCoords(QPointF* topLeft, QPointF* bottomRight) const;

    void checkIfViewPortRoIValidOrRender();

    /**
     * @brief Returns the viewer process hash for each frame that was uploaded on the viewer.
     * This is used to display the cache green line on the timeline
     **/
    void getViewerProcessHashStored(ViewerCachedImagesMap* hashes) const;

    /**
     * @brief Removes a hash stored in the viewer process frame/hash map
     **/
    void removeViewerProcessHashAtTime(TimeValue time, ViewIdx view);

    // Re-implemented
    QPixmap renderPixmap(int w = 0, int h = 0, bool useContext = false);
    QImage grabFrameBuffer(bool withAlpha = false);

Q_SIGNALS:

    /**
     *@brief Signal emitted when the current zoom factor changed.
     **/
    void zoomChanged(int v);

    /**
     * @brief Emitted when the image texture changes.
     **/
    void imageChanged(int texIndex);

    void mustCallUpdateOnMainThread();

    void mustCallUpdateGLOnMainThread();

private:
    /**
     *@brief The paint function. That's where all the drawing is done.
     **/
    virtual void paintGL() OVERRIDE FINAL;
    virtual void glDraw() OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void tabletEvent(QTabletEvent* e) OVERRIDE FINAL;

    /**
     *@brief initiliazes OpenGL context related stuff. This is called once after widget creation.
     **/
    virtual void initializeGL() OVERRIDE FINAL;

    /**
     *@brief Handles the resizing of the viewer
     **/
    virtual void resizeGL(int width, int height) OVERRIDE FINAL;

    /**
     *@brief Hack to allow the resizeEvent to be publicly used elsewhere.
     * It calls QGLWidget::resizeEvent(QResizeEvent*).
     **/
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;

private:

    void setParametricParamsPickerColor(const ColorRgba<double>& color, bool setColor, bool hasColor);

    int getMipMapLevelFromZoomFactor() const;

    bool checkIfViewPortRoIValidOrRenderForInput(int texIndex);

    bool penMotionInternal(int x, int y, double pressure, TimeValue timestamp, QInputEvent* event);

    /**
     * @brief Returns the OpenGL handle of the PBO at the given index.
     * If PBO at the given index doesn't exist, this function will create it.
     **/
    GLuint getPboID(int index);


    /**
     *@brief Prints a message if the current frame buffer is incomplete.
     * where will be printed to indicate a function. If silent is off,
     * this function will report even when the frame buffer is complete.
     **/
    void checkFrameBufferCompleteness(const char where[], bool silent = true);


    enum DrawPolygonModeEnum
    {
        eDrawPolygonModeWhole = 0,
        eDrawPolygonModeWipeLeft,
        eDrawPolygonModeWipeRight
    };

    /**
     *@brief Makes the viewer display black only.
     **/
    void clearViewer();

    /**
     *@brief Returns !=0 if the extension given by its name is supported by this OpenGL version.
     **/
    int isExtensionSupported(const char* extension);

    /**
     *@brief Called inside paintGL(). It will draw all the overlays.
     **/
    void drawOverlay(unsigned int mipMapLevel,
                     const NodePtr& aInput,
                     const NodePtr& bInput);

    void drawPickerRectangle();

    void drawPickerPixel();


    /**
     *@brief Draws the persistent message if it is on.
     **/
    void drawPersistentMessage();

    

    void updateInfoWidgetColorPicker(const QPointF & imgPos,
                                     const QPoint & widgetPos);

    void updateInfoWidgetColorPickerInternal(const QPointF & imgPos,
                                             const QPoint & widgetPos,
                                             int width,
                                             int height,
                                             const RectD & rod, // in canonical coordinates
                                             const RectD & dispW, // in canonical coordinates
                                             int texIndex);
    void updateRectangleColorPicker();
    void updateRectangleColorPickerInternal();


    /**
     * @brief X and Y are in widget coords!
     **/
    bool pickColor(double x, double y, bool pickInput);
    bool pickColorInternal(double x, double y, bool pickInput);


    static double currentTimeForEvent(QInputEvent* e);

private:
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
};

NATRON_NAMESPACE_EXIT

#endif // GLVIEWER_H
