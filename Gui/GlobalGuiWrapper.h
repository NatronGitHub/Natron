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

#ifndef GLOBAL_GUI_WRAPPER_H
#define GLOBAL_GUI_WRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QKeyEvent>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#endif

#include "Engine/GlobalFunctionsWrapper.h"

#include "Gui/GuiAppWrapper.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiFwd.h"

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
    
    QPixmap getIcon(Natron::PixmapEnum val) const
    {
        QPixmap ret;
        appPTR->getIcon(val,&ret);
        return ret;
    }
    
    GuiApp* getGuiInstance(int idx) const
    {
        AppInstance* app = appPTR->getAppInstance(idx);
        if (!app) {
            return 0;
        }
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(app);
        if (!guiApp) {
            return 0;
        }
        return new GuiApp(app);
    }
    
    void informationDialog(const std::string& title,const std::string& message)
    {
        Natron::informationDialog(title, message);
    }
    
    void warningDialog(const std::string& title,const std::string& message)
    {
        Natron::warningDialog(title,message);
    }
    
    void errorDialog(const std::string& title,const std::string& message)
    {
        Natron::errorDialog(title,message);
    }
    
    Natron::StandardButtonEnum questionDialog(const std::string& title,const std::string& message)
    {
        return Natron::questionDialog(title, message, false);
    }
    
    void addMenuCommand(const std::string& grouping,const std::string& pythonFunctionName)
    {
        appPTR->addCommand(grouping.c_str(), pythonFunctionName, (Qt::Key)0, Qt::NoModifier);
    }
    
    
    void addMenuCommand(const std::string& grouping,const std::string& pythonFunctionName,
                        Qt::Key key, const Qt::KeyboardModifiers& modifiers)
    {
        appPTR->addCommand(grouping.c_str(), pythonFunctionName, key, modifiers);
    }
    
};



#endif // GLOBAL_GUI_WRAPPER_H
