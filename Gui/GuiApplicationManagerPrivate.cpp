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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GuiApplicationManagerPrivate.h"

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/Gui.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/GuiAppInstance.h"

using namespace Natron;


GuiApplicationManagerPrivate::GuiApplicationManagerPrivate(GuiApplicationManager* publicInterface)
:   _publicInterface(publicInterface)
, _topLevelToolButtons()
, _knobsClipBoard(new KnobsClipBoard)
, _knobGuiFactory( new KnobGuiFactory() )
, _colorPickerCursor(NULL)
, _splashScreen(NULL)
, _openFileRequest()
, _actionShortcuts()
, _shortcutsChangedVersion(false)
, _fontFamily()
, _fontSize(0)
, _nodeCB()
, pythonCommands()
{
}


void
GuiApplicationManagerPrivate::removePluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& n,const QStringList& grouping)
{
    assert(grouping.size() > 0);
    
    const std::list<boost::shared_ptr<PluginGroupNode> >& children = n->getChildren();
    for (std::list<boost::shared_ptr<PluginGroupNode> >::const_iterator it = children.begin();
         it != children.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ((*it)->getChildren().empty()) {
                    n->tryRemoveChild(it->get());
                }
            } else {
                n->tryRemoveChild(it->get());
            }
            break;
        }
    }
}


void
GuiApplicationManagerPrivate::removePluginToolButton(const QStringList& grouping)
{
    assert(grouping.size() > 0);
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::iterator it = _topLevelToolButtons.begin();
         it != _topLevelToolButtons.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ((*it)->getChildren().empty()) {
                    _topLevelToolButtons.erase(it);
                }
            } else {
                _topLevelToolButtons.erase(it);
            }
            break;
        }
    }
}


boost::shared_ptr<PluginGroupNode>
GuiApplicationManagerPrivate::findPluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& parent,
                                                           const QStringList & grouping,
                                                           const QString & name,
                                                           const QString & iconPath,
                                                           int major,
                                                           int minor,
                                                           bool isUserCreatable)
{
    assert(grouping.size() > 0);
    
    const std::list<boost::shared_ptr<PluginGroupNode> >& children = parent->getChildren();
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                return findPluginToolButtonInternal(*it, newGrouping, name, iconPath, major,minor, isUserCreatable);
            }
            if (major == (*it)->getMajorVersion()) {
                return *it;
            } else {
                (*it)->setNotHighestMajorVersion(true);
            }
        }
    }
    boost::shared_ptr<PluginGroupNode> ret(new PluginGroupNode(grouping[0],grouping.size() == 1 ? name : grouping[0],iconPath,major,minor, isUserCreatable));
    parent->tryAddChild(ret);
    ret->setParent(parent);
    
    if (grouping.size() > 1) {
        QStringList newGrouping;
        for (int i = 1; i < grouping.size(); ++i) {
            newGrouping.push_back(grouping[i]);
        }
        return findPluginToolButtonInternal(ret, newGrouping, name, iconPath, major,minor, isUserCreatable);
    }
    return ret;
}

void
GuiApplicationManagerPrivate::createColorPickerCursor()
{
    QImage originalImage;
    originalImage.load(NATRON_IMAGES_PATH "color_picker.png");
    originalImage = originalImage.scaled(16, 16);
    QImage dstImage(32,32,QImage::Format_ARGB32);
    dstImage.fill(QColor(0,0,0,0));
    
    int oW = originalImage.width();
    int oH = originalImage.height();
    for (int y = 0; y < oH; ++y) {
        for (int x = 0; x < oW; ++x) {
            dstImage.setPixel(x + oW, y, originalImage.pixel(x, y));
        }
    }
    QPixmap pix = QPixmap::fromImage(dstImage);
    _colorPickerCursor = new QCursor(pix);
}

void
GuiApplicationManagerPrivate::addKeybindInternal(const QString & grouping,const QString & id,
                        const QString & description,
                        const std::list<Qt::KeyboardModifiers>& modifiersList,
                        const std::list<Qt::Key>& symbolsList)
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
    
    assert(modifiersList.size() == symbolsList.size());
    std::list<Qt::KeyboardModifiers>::const_iterator mit = modifiersList.begin();
    for (std::list<Qt::Key>::const_iterator it = symbolsList.begin(); it != symbolsList.end(); ++it, ++mit) {
        if ((*it) != (Qt::Key)0) {
            kA->defaultModifiers.push_back(*mit);
            kA->modifiers.push_back(*mit);
            kA->defaultShortcut.push_back(*it);
            kA->currentShortcut.push_back(*it);
        }
    }
    
    kA->actionID = id;
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, kA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(kA);
    }
}

void
GuiApplicationManagerPrivate::addKeybind(const QString & grouping,const QString & id,
                const QString & description,
                const Qt::KeyboardModifiers & modifiers1,Qt::Key symbol1,
                const Qt::KeyboardModifiers & modifiers2,Qt::Key symbol2)
{
    std::list<Qt::KeyboardModifiers> m;
    m.push_back(modifiers1);
    m.push_back(modifiers2);
    std::list<Qt::Key> symbols;
    symbols.push_back(symbol1);
    symbols.push_back(symbol2);
    addKeybindInternal(grouping, id, description, m, symbols);
}

void
GuiApplicationManagerPrivate::addKeybind(const QString & grouping,
                                         const QString & id,
                                         const QString & description,
                                         const Qt::KeyboardModifiers & modifiers,
                                         Qt::Key symbol)
{
    std::list<Qt::KeyboardModifiers> m;
    m.push_back(modifiers);
    std::list<Qt::Key> symbols;
    symbols.push_back(symbol);
    addKeybindInternal(grouping, id, description, m, symbols);
    
}


void
GuiApplicationManagerPrivate::removeKeybind(const QString& grouping,const QString& id)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            foundGroup->second.erase(foundAction);
        }
        if (foundGroup->second.empty()) {
            _actionShortcuts.erase(foundGroup);
        }
    }
}


void
GuiApplicationManagerPrivate::addMouseShortcut(const QString & grouping,
                                               const QString & id,
                                               const QString & description,
                                               const Qt::KeyboardModifiers & modifiers,
                                               Qt::MouseButton button)
{
    
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    MouseAction* mA = new MouseAction;

    mA->grouping = grouping;
    mA->description = description;
    mA->defaultModifiers.push_back(modifiers);
    mA->actionID = id;
    if ( modifiers & (Qt::AltModifier | Qt::MetaModifier) ) {
        qDebug() << "Warning: mouse shortcut " << grouping << '/' << description << '(' << id << ')' << " uses the Alt or Meta modifier, which is reserved for three-button mouse emulation. Fix this ASAP.";
    }
    mA->modifiers.push_back(modifiers);
    mA->button = button;

    ///Mouse shortcuts are not editable.
    mA->editable = false;

    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, mA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, mA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(mA);
    }
}


void
GuiApplicationManagerPrivate::addStandardKeybind(const QString & grouping,
                                                 const QString & id,
                                                 const QString & description,
                                                 QKeySequence::StandardKey key)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    
    Qt::KeyboardModifiers modifiers;
    Qt::Key symbol;

    extractKeySequence(QKeySequence(key), modifiers, symbol);
    KeyBoundAction* kA = new KeyBoundAction;
    kA->grouping = grouping;
    kA->description = description;
    kA->defaultModifiers.push_back(modifiers);
    kA->modifiers.push_back(modifiers);
    kA->defaultShortcut.push_back(symbol);
    kA->currentShortcut.push_back(symbol);
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, kA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(kA);
    }
}

