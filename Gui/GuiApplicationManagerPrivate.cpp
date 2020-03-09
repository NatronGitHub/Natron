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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GuiApplicationManagerPrivate.h"

#include <stdexcept>

#include <fontconfig/fontconfig.h>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/Gui.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/DocumentationManager.h"


NATRON_NAMESPACE_ENTER


GuiApplicationManagerPrivate::GuiApplicationManagerPrivate(GuiApplicationManager* publicInterface)
    :   _publicInterface(publicInterface)
    , _topLevelToolButtons()
    , _knobsClipBoard(new KnobsClipBoard)
    , _knobGuiFactory( new KnobGuiFactory() )
    , _colorPickerCursor()
    , _linkToCursor()
    , _splashScreen(NULL)
    , _openFileRequest()
    , _actionShortcuts()
    , _shortcutsChangedVersion(false)
    , _fontFamily()
    , _fontSize(0)
    , _nodeCB()
    , pythonCommands()
    , startupArgs()
    , fontconfigUpdateWatcher()
    , updateSplashscreenTimer()
    , fontconfigMessageDots(3)
    , previewRenderThread()
    , dpiX(96)
    , dpiY(96)
{
}

void
GuiApplicationManagerPrivate::removePluginToolButtonInternal(const PluginGroupNodePtr& n,
                                                             const QStringList& grouping)
{
    assert(grouping.size() > 0);

    const std::list<PluginGroupNodePtr>& children = n->getChildren();
    for (std::list<PluginGroupNodePtr>::const_iterator it = children.begin();
         it != children.end(); ++it) {
        if ( (*it)->getID() == grouping[0] ) {
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ( (*it)->getChildren().empty() ) {
                    n->tryRemoveChild( it->get() );
                }
            } else {
                n->tryRemoveChild( it->get() );
            }
            break;
        }
    }
}

void
GuiApplicationManagerPrivate::removePluginToolButton(const QStringList& grouping)
{
    assert(grouping.size() > 0);

    for (std::list<PluginGroupNodePtr>::iterator it = _topLevelToolButtons.begin();
         it != _topLevelToolButtons.end(); ++it) {
        if ( (*it)->getID() == grouping[0] ) {
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ( (*it)->getChildren().empty() ) {
                    _topLevelToolButtons.erase(it);
                }
            } else {
                _topLevelToolButtons.erase(it);
            }
            break;
        }
    }
}

PluginGroupNodePtr
GuiApplicationManagerPrivate::findPluginToolButtonInternal(const std::list<PluginGroupNodePtr>& children,
                                                           const PluginGroupNodePtr& parent,
                                                           const QStringList & grouping,
                                                           const QString & name,
                                                           const QStringList & groupingIcon,
                                                           const QString & iconPath,
                                                           int major,
                                                           int minor,
                                                           bool isUserCreatable)
{
    assert(grouping.size() > 0);
    assert( groupingIcon.size() == grouping.size() - 1 || groupingIcon.isEmpty() );

    for (std::list<PluginGroupNodePtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
        if ( (*it)->getID() == grouping[0] ) {
            if (grouping.size() > 1) {
                QStringList newGrouping, newIconsGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                for (int i = 1; i < groupingIcon.size(); ++i) {
                    newIconsGrouping.push_back(groupingIcon[i]);
                }

                return findPluginToolButtonInternal( (*it)->getChildren(), *it, newGrouping, name, newIconsGrouping, iconPath, major, minor, isUserCreatable );
            }
            if ( major == (*it)->getMajorVersion() ) {
                return *it;
            } else {
                (*it)->setNotHighestMajorVersion(true);
            }
        }
    }

    QString iconFilePath;
    if (grouping.size() > 1) {
        iconFilePath = groupingIcon.isEmpty() ? QString() : groupingIcon[0];
    } else {
        iconFilePath = iconPath;
    }
    PluginGroupNodePtr ret( new PluginGroupNode(grouping[0], grouping.size() == 1 ? name : grouping[0], iconFilePath, major, minor, isUserCreatable) );
    if (parent) {
        parent->tryAddChild(ret);
        ret->setParent(parent);
    } else {
        _topLevelToolButtons.push_back(ret);
    }

    if (grouping.size() > 1) {
        QStringList newGrouping, newIconsGrouping;
        for (int i = 1; i < grouping.size(); ++i) {
            newGrouping.push_back(grouping[i]);
        }
        for (int i = 1; i < groupingIcon.size(); ++i) {
            newIconsGrouping.push_back(groupingIcon[i]);
        }

        return findPluginToolButtonInternal(ret->getChildren(), ret, newGrouping, name, newIconsGrouping, iconPath, major, minor, isUserCreatable);
    }

    return ret;
} // GuiApplicationManagerPrivate::findPluginToolButtonInternal

void
GuiApplicationManagerPrivate::createColorPickerCursor()
{
    QImage originalImage;

    originalImage.load( QString::fromUtf8(NATRON_IMAGES_PATH "color_picker.png") );
    originalImage = originalImage.scaled(16, 16);
    QImage dstImage(32, 32, QImage::Format_ARGB32);
    dstImage.fill( QColor(0, 0, 0, 0) );

    int oW = originalImage.width();
    int oH = originalImage.height();
    for (int y = 0; y < oH; ++y) {
        for (int x = 0; x < oW; ++x) {
            dstImage.setPixel( x + oW, y, originalImage.pixel(x, y) );
        }
    }
    QPixmap pix = QPixmap::fromImage(dstImage);
    _colorPickerCursor = QCursor(pix);
}

void
GuiApplicationManagerPrivate::createLinkToCursor()
{
    QPixmap p;

    appPTR->getIcon(NATRON_PIXMAP_LINK_CURSOR, &p);
    _linkToCursor = QCursor(p);
}

void
GuiApplicationManagerPrivate::createLinkMultCursor()
{
    QPixmap p;

    appPTR->getIcon(NATRON_PIXMAP_LINK_MULT_CURSOR, &p);
    _linkMultCursor = QCursor(p);
}

void
GuiApplicationManagerPrivate::addKeybindInternal(const QString & grouping,
                                                 const QString & id,
                                                 const QString & description,
                                                 const std::list<Qt::KeyboardModifiers>& modifiersList,
                                                 const std::list<Qt::Key>& symbolsList,
                                                 const Qt::KeyboardModifiers& modifiersMask)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);

    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    KeyBoundAction* kA = new KeyBoundAction;

    kA->grouping = grouping;
    kA->description = description;

    assert( modifiersList.size() == symbolsList.size() );
    std::list<Qt::KeyboardModifiers>::const_iterator mit = modifiersList.begin();
    for (std::list<Qt::Key>::const_iterator it = symbolsList.begin(); it != symbolsList.end(); ++it, ++mit) {
        if ( (*it) != (Qt::Key)0 ) {
            kA->defaultModifiers.push_back(*mit);
            kA->modifiers.push_back(*mit);
            kA->defaultShortcut.push_back(*it);
            kA->currentShortcut.push_back(*it);
        }
    }

    kA->ignoreMask = modifiersMask;

    kA->actionID = id;
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, kA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }

    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>( _publicInterface->getTopLevelInstance().get() );
    if (app) {
        app->getGui()->addShortcut(kA);
    }
}

void
GuiApplicationManagerPrivate::addKeybind(const std::string & grouping,
                                         const std::string & id,
                                         const std::string & description,
                                         const Qt::KeyboardModifiers & modifiers1,
                                         Qt::Key symbol1,
                                         const Qt::KeyboardModifiers & modifiers2,
                                         Qt::Key symbol2)
{
    std::list<Qt::KeyboardModifiers> m;

    m.push_back(modifiers1);
    m.push_back(modifiers2);
    std::list<Qt::Key> symbols;
    symbols.push_back(symbol1);
    symbols.push_back(symbol2);
    addKeybindInternal(QString::fromUtf8( grouping.c_str() ), QString::fromUtf8( id.c_str() ), QString::fromUtf8( description.c_str() ), m, symbols, Qt::NoModifier);
}

void
GuiApplicationManagerPrivate::addKeybind(const std::string & grouping,
                                         const std::string & id,
                                         const std::string & description,
                                         const Qt::KeyboardModifiers & modifiers,
                                         Qt::Key symbol,
                                         const Qt::KeyboardModifiers & modifiersMask)
{
    std::list<Qt::KeyboardModifiers> m;

    m.push_back(modifiers);
    std::list<Qt::Key> symbols;
    symbols.push_back(symbol);
    addKeybindInternal(QString::fromUtf8( grouping.c_str() ), QString::fromUtf8( id.c_str() ), QString::fromUtf8( description.c_str() ), m, symbols, modifiersMask);
}

void
GuiApplicationManagerPrivate::addKeybind(const std::string & grouping,
                                         const std::string & id,
                                         const std::string & description,
                                         const Qt::KeyboardModifiers & modifiers,
                                         Qt::Key symbol)
{
    std::list<Qt::KeyboardModifiers> m;

    m.push_back(modifiers);
    std::list<Qt::Key> symbols;
    symbols.push_back(symbol);
    addKeybindInternal(QString::fromUtf8( grouping.c_str() ), QString::fromUtf8( id.c_str() ), QString::fromUtf8( description.c_str() ), m, symbols, Qt::NoModifier);
}

void
GuiApplicationManagerPrivate::removeKeybind(const QString& grouping,
                                            const QString& id)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);

    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            foundGroup->second.erase(foundAction);
        }
        if ( foundGroup->second.empty() ) {
            _actionShortcuts.erase(foundGroup);
        }
    }
}

void
GuiApplicationManagerPrivate::addMouseShortcut(const std::string & grouping,
                                               const std::string & id,
                                               const std::string & description,
                                               const Qt::KeyboardModifiers & modifiers,
                                               Qt::MouseButton button)
{
    QString groupingStr = QString::fromUtf8( grouping.c_str() );
    QString idStr = QString::fromUtf8( id.c_str() );
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(groupingStr);

    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(idStr);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    MouseAction* mA = new MouseAction;

    mA->grouping = groupingStr;
    mA->description = QString::fromUtf8( description.c_str() );
    mA->defaultModifiers.push_back(modifiers);
    mA->actionID = idStr;
    if ( modifiers & (Qt::AltModifier | Qt::MetaModifier) ) {
        qDebug() << "Warning: mouse shortcut " << groupingStr << '/' << description.c_str() << '(' << idStr << ')' << " uses the Alt or Meta modifier, which is reserved for three-button mouse emulation. Fix this ASAP.";
    }
    mA->modifiers.push_back(modifiers);
    mA->button = button;

    ///Mouse shortcuts are not editable.
    mA->editable = false;

    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(idStr, mA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(idStr, mA) );
        _actionShortcuts.insert( std::make_pair(groupingStr, group) );
    }

    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>( _publicInterface->getTopLevelInstance().get() );
    if (app) {
        app->getGui()->addShortcut(mA);
    }
}

void
GuiApplicationManagerPrivate::addStandardKeybind(const std::string & grouping,
                                                 const std::string & id,
                                                 const std::string & description,
                                                 QKeySequence::StandardKey key,
                                                 const Qt::KeyboardModifiers & fallbackmodifiers,
                                                 Qt::Key fallbacksymbol)
{
    QString groupingStr = QString::fromUtf8( grouping.c_str() );
    QString idStr = QString::fromUtf8( id.c_str() );
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(groupingStr);

    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(idStr);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }

    Qt::KeyboardModifiers modifiers;
    Qt::Key symbol;

    extractKeySequence(QKeySequence(key), modifiers, symbol);

    if (symbol == (Qt::Key)0) {
        symbol = fallbacksymbol;
        modifiers = fallbackmodifiers;
    }

    KeyBoundAction* kA = new KeyBoundAction;
    kA->grouping = groupingStr;
    kA->description = QString::fromUtf8( description.c_str() );
    kA->defaultModifiers.push_back(modifiers);
    kA->modifiers.push_back(modifiers);
    kA->defaultShortcut.push_back(symbol);
    kA->currentShortcut.push_back(symbol);
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(idStr, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(idStr, kA) );
        _actionShortcuts.insert( std::make_pair(groupingStr, group) );
    }

    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>( _publicInterface->getTopLevelInstance().get() );
    if (app) {
        app->getGui()->addShortcut(kA);
    }
}

void
GuiApplicationManagerPrivate::updateFontConfigCache()
{
    qDebug() << "Building Fontconfig fonts...";
    FcConfig *fcConfig = FcInitLoadConfig();
    FcConfigBuildFonts(fcConfig);
    qDebug() << "Fontconfig fonts built";
}

NATRON_NAMESPACE_EXIT
