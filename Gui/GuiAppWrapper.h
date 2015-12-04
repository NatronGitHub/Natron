/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/AppInstanceWrapper.h"
#include "Engine/ParameterWrapper.h" // ColorTuple
#include "Engine/EngineFwd.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiFwd.h"


class PyViewer
{
    
    boost::shared_ptr<Natron::Node> _node;
    ViewerTab* _viewer;
    
public:
    
    PyViewer(const boost::shared_ptr<Natron::Node>& node);
    
    ~PyViewer();
    
    void seek(int frame);
    
    int getCurrentFrame();
    
    void startForward();
    
    void startBackward();
    
    void pause();
    
    void redraw();
    
    void renderCurrentFrame(bool useCache = true);
    
    void setFrameRange(int firstFrame,int lastFrame);
    
    void getFrameRange(int* firstFrame,int* lastFrame) const;
    
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
    
    void setCurrentView(int index);
    
    int getCurrentView() const;
    
};

class GuiApp : public App
{
    GuiAppInstance* _app;
    
public:
    
    GuiApp(AppInstance* app);
    
    virtual ~GuiApp();
    
    Gui* getGui() const;
    
    PyModalDialog* createModalDialog();
        
    PyTabWidget* getTabWidget(const std::string& name) const;
    
    bool moveTab(const std::string& scriptName,PyTabWidget* pane);
    
    void registerPythonPanel(PyPanel* panel,const std::string& pythonFunction);
    void unregisterPythonPanel(PyPanel* panel);
    
    std::string getFilenameDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;
    
    std::string getSequenceDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;
    
    std::string getDirectoryDialog(const std::string& location = std::string()) const;
    
    std::string saveFilenameDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;
    
    std::string saveSequenceDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;

    ColorTuple getRGBColorDialog() const;
    
    std::list<Effect*> getSelectedNodes(Group* group = 0) const;
    
    void selectNode(Effect* effect, bool clearPreviousSelection);
    void setSelection(const std::list<Effect*>& nodes);
    void selectAllNodes(Group* group = 0);
    void deselectNode(Effect* effect);
    void clearSelection(Group* group = 0);
    
    PyViewer* getViewer(const std::string& scriptName) const;
    
    PyPanel* getUserPanel(const std::string& scriptName) const;
    
    void renderBlocking(Effect* writeNode,int firstFrame, int lastFrame,int frameStep = 1);
    void renderBlocking(const std::list<Effect*>& effects,const std::list<int>& firstFrames,const std::list<int>& lastFrames,const std::list<int>& frameSteps);

};

#endif // GUIAPPWRAPPER_H
