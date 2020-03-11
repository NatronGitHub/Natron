/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <QCursor>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Variant.h"
#include "Engine/CLArgs.h"
#include "Engine/EngineFwd.h"
#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"

#include "Gui/GuiApplicationManager.h" // PythonUserCommand
#include "Serialization/NodeClipBoard.h"
#include "Gui/GuiFwd.h"
#include "Gui/PreviewThread.h"


NATRON_NAMESPACE_ENTER


struct KnobsClipBoard
{
    KnobClipBoardType type;
    DimSpec dimension;
    ViewSetSpec view;
    KnobIPtr serialization;
};

struct GuiApplicationManagerPrivate
{
    GuiApplicationManager* _publicInterface; // can not be a smart ptr
    std::list<PluginGroupNodePtr> _topLevelToolButtons;
    boost::scoped_ptr<KnobsClipBoard> _knobsClipBoard;
    boost::scoped_ptr<KnobGuiFactory> _knobGuiFactory;
    QCursor _colorPickerCursor, _linkToCursor, _linkMultCursor;
    SplashScreen* _splashScreen;

    ///We store here the file open request that was made on startup but that
    ///we couldn't handle at that time
    QString _openFileRequest;
    QString _fontFamily;
    int _fontSize;

    ///Used temporarily to store startup args while we load fonts
    CLArgs startupArgs;
    boost::shared_ptr<QFutureWatcher<void> > fontconfigUpdateWatcher;
    QTimer updateSplashscreenTimer;
    int fontconfigMessageDots;
    PreviewThread previewRenderThread;
    int dpiX, dpiY;
    boost::scoped_ptr<DocumentationManager> documentation;


    GuiApplicationManagerPrivate(GuiApplicationManager* publicInterface);

    void createColorPickerCursor();

    void createLinkToCursor();

    void createLinkMultCursor();

    void removePluginToolButtonInternal(const PluginGroupNodePtr& n, const std::vector<std::string>& grouping);
    void removePluginToolButton(const std::vector<std::string>& grouping);


    /**
     * @brief Finds and existing (or create) PluginGroupNode that matches the given grouping, e.g: "Color/Math" and plugin
     **/
    PluginGroupNodePtr  findPluginToolButtonOrCreateInternal(const std::list<PluginGroupNodePtr>& children,
                                                             const PluginGroupNodePtr& parent,
                                                             const PluginPtr& plugin,
                                                             const QStringList& grouping,
                                                             const QStringList& groupingIcon);

    void updateFontConfigCache();
};

NATRON_NAMESPACE_EXIT

#endif // ifndef Gui_ApplicationManagerPrivate_h
