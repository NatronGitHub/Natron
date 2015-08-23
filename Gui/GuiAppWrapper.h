//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef GUIAPPWRAPPER_H
#define GUIAPPWRAPPER_H

#include "Engine/AppInstanceWrapper.h"
#include "Engine/ParameterWrapper.h" // ColorTuple

#include "Gui/GuiAppInstance.h"

class PyModalDialog;
class PyPanel;
class PyTabWidget;
class QWidget;
class Effect;
class Group;
class ViewerTab;

namespace Natron {
class Node;
}

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
    
    PyViewer* getViewer(const std::string& scriptName) const;
    
    PyPanel* getUserPanel(const std::string& scriptName) const;

};

#endif // GUIAPPWRAPPER_H
