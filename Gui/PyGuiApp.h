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

#include <QtCore/QEvent>
#include <QtCore/qnamespace.h> // Qt::Key, Qt::KeyboardModifiers

#include "Engine/PyAppInstance.h"
#include "Engine/PyParameter.h" // ColorTuple
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

    /*
     * Simulated input, for tests. Positions are in canonical coordinates: the coordinates
     * the OpenFX overlay interacts receive (pixel aspect ratio included, y pointing up).
     * The events are real Qt events sent to the viewer's OpenGL widget, so they go through
     * the viewer (pan, zoom, picking) and the overlays of the nodes, as user input would.
     * The window does not need to be focused or on top, and the mouse cursor does not move.
     * button is a Qt::MouseButton (1: left, 2: right, 4: middle), modifiers a
     * Qt::KeyboardModifiers. A pressure other than 1 also sends tablet events.
     * They return whether the event was accepted, and False when not called from the main
     * thread (e.g. from a render callback such as afterFrameRender).
     */
    bool penDown(double x, double y, int button = 1, Qt::KeyboardModifiers modifiers = Qt::NoModifier, double pressure = 1.);

    /// a motion with button held (0 for a motion without any button pressed)
    bool penMotion(double x, double y, int button = 1, Qt::KeyboardModifiers modifiers = Qt::NoModifier, double pressure = 1.);

    bool penUp(double x, double y, int button = 1, Qt::KeyboardModifiers modifiers = Qt::NoModifier, double pressure = 1.);

    /// sends the same events as a double-click: press, release, double-click, release
    bool penDoubleClick(double x, double y, int button = 1, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    bool keyDown(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    bool keyUp(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /// keyDown() followed by keyUp(), returns whether the key down was accepted
    bool keyPress(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /// penDown() at (x0, y0), steps penMotion() to (x1, y1), penUp() at (x1, y1)
    bool drag(double x0, double y0, double x1, double y1, int steps = 10, int button = 1, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /// canonical coordinates to the viewer's widget coordinates (in logical pixels, y down)
    Double2DTuple toWidgetCoordinates(double x, double y) const;

    /// the viewer's widget coordinates to canonical coordinates
    Double2DTuple toCanonicalCoordinates(double wx, double wy) const;

    /// the zoom factor (widget pixels per canonical unit)
    double getZoom() const;

    /// the canonical coordinates of the bottom-left corner of the viewer
    Double2DTuple getPan() const;

    /// fits the project format in the viewer, as the F key does
    void fitToFormat();

    /**
     * @brief Processes the GUI events until the viewer has no render running, so that the
     * overlays are redrawn and the renders they trigger are done. Returns false on timeout,
     * or when not called from the main thread.
     **/
    bool waitForRender(int timeoutMs = 10000);

private:

    ViewerGL* getViewerGL() const;
    bool sendPenEvent(QEvent::Type type, double x, double y, int button, Qt::KeyboardModifiers modifiers, double pressure);
    bool sendKeyEvent(QEvent::Type type, Qt::Key key, Qt::KeyboardModifiers modifiers);
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

    /**
     * @brief Processes the pending GUI events (redraws, queued signals), e.g. after sending
     * input events to a viewer. Does nothing when not called from the main thread.
     **/
    void processEvents();
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // GUIAPPWRAPPER_H
