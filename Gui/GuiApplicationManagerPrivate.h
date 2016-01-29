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

#ifndef Gui_ApplicationManagerPrivate_h
#define Gui_ApplicationManagerPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFutureWatcher>
#include <QtCore/QString>
#include <QtCore/QTimer>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Variant.h"
#include "Engine/CLArgs.h"
#include "Engine/EngineFwd.h"

#include "Gui/ActionShortcuts.h" // AppShortcuts
#include "Gui/GuiApplicationManager.h" // PythonUserCommand
#include "Gui/NodeClipBoard.h"
#include "Gui/GuiFwd.h"
#include "Gui/PreviewThread.h"

NATRON_NAMESPACE_ENTER;

struct KnobsClipBoard
{
    std::list<Variant> values; //< values
    std::list< boost::shared_ptr<Curve> > curves; //< animation
    std::list< boost::shared_ptr<Curve> > parametricCurves; //< for parametric knobs
    std::map<int,std::string> stringAnimation; //< for animating string knobs
    bool isEmpty; //< is the clipboard empty
    bool copyAnimation; //< should we copy all the animation or not
    
    std::string appID;
    std::string nodeFullyQualifiedName;
    std::string paramName;
};



struct GuiApplicationManagerPrivate
{
    GuiApplicationManager* _publicInterface;
    std::list<boost::shared_ptr<PluginGroupNode> > _topLevelToolButtons;
    boost::scoped_ptr<KnobsClipBoard> _knobsClipBoard;
    boost::scoped_ptr<KnobGuiFactory> _knobGuiFactory;
    QCursor* _colorPickerCursor;
    SplashScreen* _splashScreen;

    ///We store here the file open request that was made on startup but that
    ///we couldn't handle at that time
    QString _openFileRequest;
    AppShortcuts _actionShortcuts;

    bool _shortcutsChangedVersion;
    
    QString _fontFamily;
    int _fontSize;
    
    NodeClipBoard _nodeCB;
    
    std::list<PythonUserCommand> pythonCommands;
    
    ///Used temporarily to store startup args while we load fonts
    CLArgs startupArgs;
    boost::shared_ptr<QFutureWatcher<void> > fontconfigUpdateWatcher;
    QTimer updateSplashscreenTimer;
    int fontconfigMessageDots;
        
    PreviewThread previewRenderThread;
    
    int dpiX,dpiY;


    GuiApplicationManagerPrivate(GuiApplicationManager* publicInterface);

    void createColorPickerCursor();
    
    void removePluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& n,const QStringList& grouping);
    void removePluginToolButton(const QStringList& grouping);

    void addStandardKeybind(const QString & grouping,const QString & id,
                            const QString & description,QKeySequence::StandardKey key,
                            const Qt::KeyboardModifiers & fallbackmodifiers,Qt::Key fallbacksymbol);

    void addKeybind(const QString & grouping,const QString & id,
                    const QString & description,
                    const Qt::KeyboardModifiers & modifiers,Qt::Key symbol);
    
    void addKeybind(const QString & grouping,const QString & id,
                    const QString & description,
                    const Qt::KeyboardModifiers & modifiers,Qt::Key symbol,
                    const Qt::KeyboardModifiers & modifiersMask);
    
    void addKeybind(const QString & grouping,const QString & id,
                    const QString & description,
                    const Qt::KeyboardModifiers & modifiers1,Qt::Key symbol1,
                    const Qt::KeyboardModifiers & modifiers2,Qt::Key symbol2);
    
    void addKeybindInternal(const QString & grouping,const QString & id,
                            const QString & description,
                            const std::list<Qt::KeyboardModifiers>& modifiersList,
                            const std::list<Qt::Key>& symbolsList,
                            const Qt::KeyboardModifiers& modifiersMask);
    
    void removeKeybind(const QString& grouping,const QString& id);

    void addMouseShortcut(const QString & grouping,const QString & id,
                          const QString & description,
                          const Qt::KeyboardModifiers & modifiers,Qt::MouseButton button);
    
    boost::shared_ptr<PluginGroupNode>  findPluginToolButtonInternal(const std::list<boost::shared_ptr<PluginGroupNode> >& children,
                                                                     const boost::shared_ptr<PluginGroupNode>& parent,
                                                                     const QStringList & grouping,
                                                                     const QString & name,
                                                                     const QStringList & groupingIcon,
                                                                     const QString & iconPath,
                                                                     int major,
                                                                     int minor,
                                                                     bool isUserCreatable);
    
    void updateFontConfigCache();
};

NATRON_NAMESPACE_EXIT;

#endif
