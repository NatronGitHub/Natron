/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h

#include <QtCore/QSize>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/OpenGLViewerI.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER

/**
 *@class ViewerGL
 *@brief The main viewport. This class is part of the ViewerTab GUI and handles all
 * OpenGL related code as well as user events.
 **/
class ViewerGL
    : public QGLWidget, public OpenGLViewerI
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


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
    const RectD& getRoD(int textureIndex) const;


    /**
     *@returns Returns the displayWindow of the currentFrame(Resolution)
     **/
    RectD getCanonicalFormat(int texIndex) const;

    double getPAR(int texIndex) const;

    void setClipToFormat(bool b);

    virtual bool isClippingImageToFormat() const OVERRIDE FINAL;
    virtual ImageBitDepthEnum getBitDepth() const OVERRIDE FINAL;

    /**
     *@brief Hack to allow the resizeEvent to be publicly used elsewhere.
     * It calls QGLWidget::resizeEvent(QResizeEvent*).
     **/
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;

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
    virtual RectI getImageRectangleDisplayed(const RectI & imageRoD, const double par, unsigned int mipMapLevel) OVERRIDE FINAL;
    virtual RectI getExactImageRectangleDisplayed(int texIndex, const RectD & rod, const double par, unsigned int mipMapLevel) OVERRIDE FINAL;
    virtual RectI getImageRectangleDisplayedRoundedToTileSize(int texIndex, const RectD & rod, const double par, unsigned int mipMapLevel, std::vector<RectI>* tiles, std::vector<RectI>* tilesRounded, int *tileSize, RectI* roiNotRounded) OVERRIDE FINAL WARN_UNUSED_RETURN;
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

    /**
     *@brief Turns on the overlays on the viewer.
     **/
    void turnOnOverlay();

    /**
     *@brief Turns off the overlays on the viewer.
     **/
    void turnOffOverlay();

    virtual void clearPartialUpdateTextures() OVERRIDE FINAL;
    virtual bool isViewerUIVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     *@brief Copies the data stored in the  RAM buffer into the currently
     * used texture.
     * It does:
     * 1) glMapBuffer
     * 2) memcpy to copy data from RAM to GPU
     * 3) glUnmapBuffer
     * 4) glTexSubImage2D or glTexImage2D depending whether we resize the texture or not.
     **/
    virtual void transferBufferFromRAMtoGPU(const unsigned char* ramBuffer,
                                            size_t bytesCount,
                                            const RectI &roiRoundedToTileSize,
                                            const RectI& roi,
                                            const TextureRect & tileRect,
                                            int textureIndex,
                                            bool isPartialRect,
                                            bool isFirstTile,
                                            TexturePtr* texture) OVERRIDE FINAL;
    virtual void endTransferBufferFromRAMToGPU(int textureIndex,
                                               const TexturePtr& texture,
                                               const ImagePtr& image,
                                               int time,
                                               const RectD& rod,
                                               double par,
                                               ImageBitDepthEnum depth,
                                               unsigned int mipMapLevel,
                                               ImagePremultiplicationEnum premult,
                                               double gain,
                                               double gamma,
                                               double offset,
                                               int lut,
                                               bool recenterViewer,
                                               const Point& viewportCenter,
                                               bool isPartialRect) OVERRIDE FINAL;
    virtual void clearLastRenderedImage() OVERRIDE FINAL;
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


    /**
     *@brief Slot used by the GUI to zoom at the current center the viewport.
     *@param v[in] an integer holding the desired zoom factor (in percent).
     **/
    void zoomSlot(int v);

    /**
     *@brief Convenience function. See ViewerGL::zoomSlot(int)
     **/
    void zoomSlot(double v);


    void setRegionOfDefinition(const RectD & rod, double par, int textureIndex);
    void setFormat(const std::string& formatName, const RectD& format, double par, int textureIndex);

    virtual void updateColorPicker(int textureIndex, int x = INT_MAX, int y = INT_MAX) OVERRIDE FINAL;

    void clearColorBuffer(double r = 0., double g = 0., double b = 0., double a = 1.);

    void toggleOverlays();

    void toggleWipe();

    void centerWipe();

    void onCheckerboardSettingsChanged();


    /**
     * @brief Reset the wipe position so it is in the center of the B input.
     * If B input is disconnected it goes in the middle of the A input.
     * Otherwise it goes in the middle of the project window
     **/
    void resetWipeControls();

    void clearLastRenderedTexture();


public:


    virtual void makeOpenGLcontextCurrent() OVERRIDE FINAL;
    virtual void removeGUI() OVERRIDE FINAL;
    virtual ViewIdx getCurrentView() const OVERRIDE FINAL;
    virtual TimeLinePtr getTimeline() const OVERRIDE FINAL;

public:

    virtual bool renderText(double x,
                            double y,
                            const std::string &string,
                            double r,
                            double g,
                            double b,
                            int flags = 0) OVERRIDE FINAL; //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10


    void renderText(double x,
                    double y,
                    const QString &string,
                    const QColor & color,
                    const QFont & font,
                    int flags = 0); //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10

    void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const;

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);

    void setUserRoIEnabled(bool b);

    void setBuildNewUserRoI(bool b);

    virtual bool isUserRegionOfInterestEnabled() const OVERRIDE FINAL;
    virtual RectD getUserRegionOfInterest() const OVERRIDE FINAL;

    void setUserRoI(const RectD & r);

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
    ViewerInstance* getInternalNode() const;
    ViewerTab* getViewerTab() const;

    /**
     * @brief Returns the viewport visible portion in canonical coordinates
     **/
    virtual RectD getViewportRect() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns the cursor position in canonical coordinates
     **/
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;
    virtual ViewerInstance* getInternalViewerNode() const OVERRIDE FINAL;

    /**
     * @brief can only be called on the main-thread
     **/
    void setGain(double d);

    void setGamma(double g);

    void setLut(int lut);

    bool isWipeHandleVisible() const;

    void setZoomOrPannedSinceLastFit(bool enabled);

    bool getZoomOrPannedSinceLastFit() const;

    virtual ViewerCompositingOperatorEnum getCompositingOperator() const OVERRIDE FINAL;
    virtual void setCompositingOperator(ViewerCompositingOperatorEnum op) OVERRIDE FINAL;

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
     * @brief Called by the Histogram when it wants to refresh. It returns a pointer to the last
     * rendered image by the viewer. It doesn't re-render the image if it is not present.
     **/
    ImagePtr getLastRenderedImage(int textureIndex) const;

    ImagePtr getLastRenderedImageByMipMapLevel(int textureIndex, unsigned int mipMapLevel) const;

    /**
     * @brief Get the color of the currently displayed image at position x,y.
     * @param forceLinear If true, then it will not use the viewer current colorspace
     * to get r,g and b values, otherwise the color returned will be in the same color-space
     * than the one chosen by the user on the gui.
     * X and Y are in CANONICAL COORDINATES
     * @return true if the point is inside the image and colors were set
     **/
    bool getColorAt(double x, double y, bool forceLinear, int textureIndex, float* r,
                    float* g, float* b, float* a, unsigned int* mipMapLevel) WARN_UNUSED_RETURN;

    // same as getColor, but computes the mean over a given rectangle
    bool getColorAtRect(const RectD &rect, // rectangle in canonical coordinates
                        bool forceLinear, int textureIndex, float* r, float* g, float* b, float* a, unsigned int* mipMapLevel);


    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL;

    ///same as getMipMapLevel but with the zoomFactor taken into account
    int getMipMapLevelCombinedToZoomFactor() const WARN_UNUSED_RETURN;

    virtual int getCurrentlyDisplayedTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    QPointF toZoomCoordinates(const QPointF& position) const;

    QPointF toWidgetCoordinates(const QPointF& position) const;

    void getTopLeftAndBottomRightInZoomCoords(QPointF* topLeft, QPointF* bottomRight) const;

    void checkIfViewPortRoIValidOrRender();

    void s_selectionCleared()
    {
        Q_EMIT selectionCleared();
    }

Q_SIGNALS:

    /**
     *@brief Signal emitted when the current zoom factor changed.
     **/
    void zoomChanged(int v);

    /**
     * @brief Emitted when the image texture changes.
     **/
    void imageChanged(int texIndex, bool hasImageBackEnd);

    /**
     * @brief Emitted when the selection rectangle has changed.
     * @param onRelease When true, this signal is emitted on the mouse release event
     * which means this is the last selection desired by the user.
     * Receivers can either update the selection always or just on mouse release.
     **/
    void selectionRectangleChanged(bool onRelease);

    void selectionCleared();

private:
    /**
     *@brief The paint function. That's where all the drawing is done.
     **/
    virtual void paintGL() OVERRIDE FINAL;
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
     *@brief initializes OpenGL context related stuff. This is called once after widget creation.
     **/
    virtual void initializeGL() OVERRIDE FINAL;

    /**
     *@brief Handles the resizing of the viewer
     **/
    virtual void resizeGL(int width, int height) OVERRIDE FINAL;

private:

    void setParametricParamsPickerColor(const OfxRGBAColourD& color, bool setColor, bool hasColor);

    bool checkIfViewPortRoIValidOrRenderForInput(int texIndex);

    bool penMotionInternal(int x, int y, double pressure, double timestamp, QInputEvent* event);

    /**
     * @brief Returns the OpenGL handle of the PBO at the given index.
     * If PBO at the given index doesn't exist, this function will create it.
     **/
    GLuint getPboID(int index);


    void populateMenu();

    /**
     *@brief Prints a message if the current frame buffer is incomplete.
     * where will be printed to indicate a function. If silent is off,
     * this function will report even when the frame buffer is complete.
     **/
    void checkFrameBufferCompleteness(const char where[], bool silent = true);

    /**
     *@brief Initialises shaders. If they were initialized ,returns instantly.
     **/
    void initShaderGLSL(); // init shaders


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
    void drawOverlay(unsigned int mipMapLevel);

    /**
     * @brief Called by drawOverlay to draw the user region of interest.
     **/
    void drawUserRoI();

    void drawPickerRectangle();

    void drawPickerPixel();

    void drawWipeControl();

    /**
     *@brief Draws the persistent message if it is on.
     **/
    void drawPersistentMessage();

    bool isNearByUserRoITopEdge(const RectD & roi,
                                const QPointF & zoomPos,
                                double zoomScreenPixelWidth,
                                double zoomScreenPixelHeight);

    bool isNearByUserRoIRightEdge(const RectD & roi,
                                  const QPointF & zoomPos,
                                  double zoomScreenPixelWidth,
                                  double zoomScreenPixelHeight);

    bool isNearByUserRoILeftEdge(const RectD & roi,
                                 const QPointF & zoomPos,
                                 double zoomScreenPixelWidth,
                                 double zoomScreenPixelHeight);


    bool isNearByUserRoIBottomEdge(const RectD & roi,
                                   const QPointF & zoomPos,
                                   double zoomScreenPixelWidth,
                                   double zoomScreenPixelHeight);

    bool isNearByUserRoI(double x,
                         double y,
                         const QPointF & zoomPos,
                         double zoomScreenPixelWidth,
                         double zoomScreenPixelHeight);

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
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
};

NATRON_NAMESPACE_EXIT

#endif // GLVIEWER_H
