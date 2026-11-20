/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef GUIAPPWRAPPER_H
#define GUIAPPWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#include "Engine/PyAppInstance.h"
#include "Engine/PyParameter.h" // ColorTuple
#include "Engine/RectD.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class PyViewer
{
    NodeWPtr _node;
    ViewerTab* _viewer;

public:

    PyViewer(const NodePtr& node);

    ~PyViewer();


    NodePtr getInternalNode() const
    {
        return _node.lock();
    }

    void seek(int frame);

    int getCurrentFrame();

    void startForward();

    void startBackward();

    void pause();

    void redraw();

    void renderCurrentFrame(bool useCache = true);

    void setFrameRange(int firstFrame, int lastFrame);

    void getFrameRange(int* firstFrame, int* lastFrame) const;

    void setPlaybackMode(Natron::PlaybackModeEnum mode);

    Natron::PlaybackModeEnum getPlaybackMode() const;

    Natron::ViewerCompositingOperatorEnum getCompositingOperator() const;

    void setCompositingOperator(Natron::ViewerCompositingOperatorEnum op);

    int getAInput() const;

    void setAInput(int index);

    int getBInput() const;

    void setBInput(int index);

    void setChannels(Natron::DisplayChannelsEnum channels);

    Natron::DisplayChannelsEnum getChannels() const;

    void setProxyModeEnabled(bool enabled);

    bool isProxyModeEnabled() const;

    void setProxyIndex(int index);

    int getProxyIndex() const;

    /* Python API: do not use ViewIdx */
    void setCurrentView(int index);

    /* Python API: do not use ViewIdx */
    int getCurrentView() const;

    /// fits the image to the viewer, as the F key does
    void fit();

    /// the zoom factor: 1 shows one image pixel per screen pixel (100%)
    double getZoom() const;

    /// sets the zoom factor, keeping the canvas point at the centre of the viewport fixed
    void setZoom(double zoom);

    /// the canvas coordinates of the centre of the viewport
    Double2DTuple getCenter() const;

    /// pans so that the canvas point (x, y) is at the centre of the viewport
    void setCenter(double x, double y);

    /// canvas coordinates to the coordinates of the viewer widget (Qt logical pixels, origin top-left)
    Double2DTuple canvasToWidget(double x, double y) const;

    /// the coordinates of the viewer widget to canvas coordinates
    Double2DTuple widgetToCanvas(double x, double y) const;

    /**
     * @brief The linear RGBA value displayed at the canvas point (x, y) for the input A (0) or B (1),
     * before the display LUT. Raises a Python exception if there is no image there.
     **/
    ColorTuple getPixel(double x, double y, int input = 0) const;

    /**
     * @brief The linear RGBA image displayed for the input A (0) or B (1), before the display LUT, at the
     * mipmap level of the last render, within region (canvas coordinates; the whole image if NULL).
     * Python: getImage(input=0, region=None) -> (RectD bounds, bytes of float32 RGBA, row-major from the
     * bottom row). Call App.waitForRenders() first: during a render, it is the previous image.
     **/
    bool getImage(RectD* bounds, std::vector<float>* pixels, int input = 0, const RectD* region = 0) const;

private:

    // the viewer widget, or NULL if the viewer node is deactivated
    ViewerGL* getViewerGL() const;
};

class GuiApp
    : public App
{
    Q_DECLARE_TR_FUNCTIONS(GuiApp)

private:
    GuiAppInstanceWPtr _app;

public:

    GuiApp(const GuiAppInstancePtr& app);

    virtual ~GuiApp();

    GuiAppInstancePtr getInternalGuiApp() const
    {
        return _app.lock();
    }

    Gui* getGui() const;
    PyModalDialog* createModalDialog();
    PyTabWidget* getTabWidget(const QString& name) const;
    PyTabWidget* getActiveTabWidget() const;

    bool moveTab(const QString& scriptName, PyTabWidget* pane);

    void registerPythonPanel(PyPanel* panel, const QString& pythonFunction);
    void unregisterPythonPanel(PyPanel* panel);

    QString getFilenameDialog( const QStringList& filters, const QString& location = QString() ) const;

    QString getSequenceDialog( const QStringList& filters, const QString& location = QString() ) const;

    QString getDirectoryDialog( const QString& location = QString() ) const;

    QString saveFilenameDialog( const QStringList& filters, const QString& location = QString() ) const;

    QString saveSequenceDialog( const QStringList& filters, const QString& location = QString() ) const;

    ColorTuple getRGBColorDialog() const;

    std::list<Effect*> getSelectedNodes(Group* group = 0) const;

    void selectNode(Effect* effect, bool clearPreviousSelection);
    void setSelection(const std::list<Effect*>& nodes);
    void selectAllNodes(Group* group = 0);
    void copySelectedNodes(Group* group = 0);
    void pasteNodes(Group* group = 0);
    void deselectNode(Effect* effect);
    void clearSelection(Group* group = 0);

    PyViewer* getViewer(const QString& scriptName) const;
    PyViewer* getActiveViewer() const;
    PyPanel* getUserPanel(const QString& scriptName) const;

    void renderBlocking(Effect* writeNode, int firstFrame, int lastFrame, int frameStep = 1);
    void renderBlocking(const std::list<Effect*>& effects, const std::list<int>& firstFrames, const std::list<int>& lastFrames, const std::list<int>& frameSteps);

    /**
     * @brief Saves an image of the main window, or of the tab or pane with the given
     * script name (e.g. "Viewer1", "nodeGraph", "properties", "pane1"), to filePath.
     * Only Natron's own widgets are captured (QWidget::grab()), never the screen.
     * The content of OpenGL widgets (viewers, curve editor, dope sheet...) is read from
     * their framebuffer. The file format follows the extension of filePath.
     * Returns true on success, false when not called from the main thread.
     **/
    bool saveScreenshot( const QString& filePath, const QString& scriptName = QString() ) const;
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // GUIAPPWRAPPER_H
