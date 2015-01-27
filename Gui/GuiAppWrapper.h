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
#include "Gui/GuiAppInstance.h"

class PyModalDialog;
class PyPanel;
class PyTabWidget;
class QWidget;

class GuiApp : public App
{
    GuiAppInstance* _app;
    
public:
    
    GuiApp(AppInstance* app);
    
    virtual ~GuiApp();
    
    Gui* getGui() const;
    
    PyModalDialog* createModalDialog();
        
    PyTabWidget* getTabWidget(const std::string& name) const;
    
    bool moveTab(QWidget* tab,PyTabWidget* pane);
    
    void registerPythonPanel(PyPanel* panel,const std::string& pythonFunction);
    void unregisterPythonPanel(PyPanel* panel);
    
    std::string getFilenameDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;
    
    std::string getSequenceDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;
    
    std::string getDirectoryDialog(const std::string& location = std::string()) const;
    
    std::string saveFilenameDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;
    
    std::string saveSequenceDialog(const std::vector<std::string>& filters,const std::string& location = std::string()) const;

    ColorTuple getRGBColorDialog() const;
    
};

#endif // GUIAPPWRAPPER_H
