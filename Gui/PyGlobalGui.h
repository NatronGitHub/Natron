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

#include "Engine/PyGlobalFunctions.h"

#include "Gui/PyGuiApp.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class PyGuiApplication
    : public PyCoreApplication
{
public:

    PyGuiApplication()
        : PyCoreApplication()
    {
    }

    virtual ~PyGuiApplication()
    {
    }

    QPixmap getIcon(NATRON_NAMESPACE::PixmapEnum val) const
    {
        QPixmap ret;

        appPTR->getIcon(val, &ret);

        return ret;
    }

    GuiApp* getGuiInstance(int idx) const
    {
        AppInstancePtr app = appPTR->getAppInstance(idx);

        if (!app) {
            return 0;
        }
        GuiAppInstancePtr guiApp = boost::dynamic_pointer_cast<GuiAppInstance>(app);
        if (!guiApp) {
            return 0;
        }

        return new GuiApp(guiApp);
    }

    void informationDialog(const QString& title,
                           const QString& message)
    {
        Dialogs::informationDialog( title.toStdString(), message.toStdString() );
    }

    void warningDialog(const QString& title,
                       const QString& message)
    {
        Dialogs::warningDialog( title.toStdString(), message.toStdString() );
    }

    void errorDialog(const QString& title,
                     const QString& message)
    {
        Dialogs::errorDialog( title.toStdString(), message.toStdString() );
    }

    NATRON_NAMESPACE::StandardButtonEnum questionDialog(const QString& title,
                                                        const QString& message)
    {
        return Dialogs::questionDialog(title.toStdString(), message.toStdString(), false);
    }

    void addMenuCommand(const QString& grouping,
                        const QString& pythonFunctionName)
    {
        appPTR->addCommand(grouping, pythonFunctionName.toStdString(), (Qt::Key)0, Qt::NoModifier);
    }

    void addMenuCommand(const QString& grouping,
                        const QString& pythonFunctionName,
                        Qt::Key key,
                        const Qt::KeyboardModifiers& modifiers)
    {
        appPTR->addCommand(grouping, pythonFunctionName.toStdString(), key, modifiers);
    }
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // GLOBAL_GUI_WRAPPER_H
