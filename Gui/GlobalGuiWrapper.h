/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include "Engine/GlobalFunctionsWrapper.h"

#include <QKeyEvent>

#include "Gui/GuiAppWrapper.h"
#include "Gui/GuiApplicationManager.h"

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
