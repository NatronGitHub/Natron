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

#ifndef GUIAPPWRAPPER_H
#define GUIAPPWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

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

    void setPlaybackMode(NATRON_NAMESPACE::PlaybackModeEnum mode);

    NATRON_NAMESPACE::PlaybackModeEnum getPlaybackMode() const;

    NATRON_NAMESPACE::ViewerCompositingOperatorEnum getCompositingOperator() const;

    void setCompositingOperator(NATRON_NAMESPACE::ViewerCompositingOperatorEnum op);

    int getAInput() const;

    void setAInput(int index);

    int getBInput() const;

    void setBInput(int index);

    void setChannels(NATRON_NAMESPACE::DisplayChannelsEnum channels);

    NATRON_NAMESPACE::DisplayChannelsEnum getChannels() const;

    void setProxyModeEnabled(bool enabled);

    bool isProxyModeEnabled() const;

    void setProxyIndex(int index);

    int getProxyIndex() const;

    /* Python API: do not use ViewIdx */
    void setCurrentView(int index);

    /* Python API: do not use ViewIdx */
    int getCurrentView() const;
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
    void deselectNode(Effect* effect);
    void clearSelection(Group* group = 0);

    PyViewer* getViewer(const QString& scriptName) const;
    PyViewer* getActiveViewer() const;
    PyPanel* getUserPanel(const QString& scriptName) const;

    void renderBlocking(Effect* writeNode, int firstFrame, int lastFrame, int frameStep = 1);
    void renderBlocking(const std::list<Effect*>& effects, const std::list<int>& firstFrames, const std::list<int>& lastFrames, const std::list<int>& frameSteps);
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // GUIAPPWRAPPER_H
