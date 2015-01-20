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

#ifndef GLOBAL_GUI_WRAPPER_H
#define GLOBAL_GUI_WRAPPER_H

#include "Engine/GlobalFunctionsWrapper.h"
#include "Gui/GuiAppWrapper.h"

class PyGuiApplication : public PyCoreApplication
{
public:
    
    PyGuiApplication()
    : PyCoreApplication()
    {
        
    }
    
    virtual ~PyGuiApplication()
    {
        
    }
    
    GuiApp* getGuiInstance(int idx) const
    {
        AppInstance* app = Natron::getInstance(idx);
        if (!app) {
            return 0;
        }
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(app);
        if (!guiApp) {
            return 0;
        }
        return new GuiApp(app);
    }
};



#endif // GLOBAL_GUI_WRAPPER_H
