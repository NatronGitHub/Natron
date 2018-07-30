/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
    , _fontFamily()
    , _fontSize(0)
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
                                                             const std::vector<std::string>& grouping)
{
    assert(grouping.size() > 0);

    const std::list<PluginGroupNodePtr>& children = n->getChildren();
    for (std::list<PluginGroupNodePtr>::const_iterator it = children.begin();
         it != children.end(); ++it) {
        if ( (*it)->getTreeNodeID().toStdString() == grouping[0] ) {
            if (grouping.size() > 1) {
                std::vector<std::string> newGrouping;
                for (std::size_t i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ( (*it)->getChildren().empty() ) {
                    n->tryRemoveChild(*it);
                }
            } else {
                n->tryRemoveChild(*it);
            }
            break;
        }
    }
}

void
GuiApplicationManagerPrivate::removePluginToolButton(const std::vector<std::string>& grouping)
{
    assert(grouping.size() > 0);

    for (std::list<PluginGroupNodePtr>::iterator it = _topLevelToolButtons.begin();
         it != _topLevelToolButtons.end(); ++it) {
        if ( (*it)->getTreeNodeID().toStdString() == grouping[0] ) {
            if (grouping.size() > 1) {
                std::vector<std::string> newGrouping;
                for (std::size_t i = 1; i < grouping.size(); ++i) {
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
GuiApplicationManagerPrivate::findPluginToolButtonOrCreateInternal(const std::list<PluginGroupNodePtr>& children,
                                                                   const PluginGroupNodePtr& parent,
                                                                   const PluginPtr& plugin,
                                                                   const QStringList& grouping,
                                                                   const QStringList& groupingIcon)
{
    assert(plugin);
    assert(groupingIcon.size() <= grouping.size());

    // On first call of this function, children are top-level toolbuttons
    // Otherwise this tree node has children
    // We ensure that the path in the tree leading to the plugin in parameter is created by recursing on the children
    // If there are no children that means we reached the wanted PluginGroupNode
    QString nodeIDToFind;
    if (grouping.empty()) {
        // Look for plugin ID
        nodeIDToFind = QString::fromUtf8(plugin->getPluginID().c_str());
    } else {
        // Look for grouping menu item
        nodeIDToFind = grouping[0];
    }

    for (std::list<PluginGroupNodePtr>::const_iterator it = children.begin(); it != children.end(); ++it) {

        // If we find a node with the same ID, then we found it already.
        if ( (*it)->getTreeNodeID() == nodeIDToFind ) {
            if (grouping.empty()) {
                // This is a leaf (plug-in), return it
                return *it;
            } else {

                // This is an intermidiate menu item, recurse
                QStringList newGrouping, newIconsGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                for (int i = 1; i < groupingIcon.size(); ++i) {
                    newIconsGrouping.push_back(groupingIcon[i]);
                }

                return findPluginToolButtonOrCreateInternal( (*it)->getChildren(), *it, plugin, newGrouping, newIconsGrouping);
            }
        }
    }

    // Ok the PluginGroupNode does not exist yet, create it
    QString treeNodeName, iconFilePath;
    if (grouping.empty()) {
        // This is a leaf (plug-in), take the plug-in label and icon
        treeNodeName = QString::fromUtf8(plugin->getLabelWithoutSuffix().c_str());
        iconFilePath = QString::fromUtf8(plugin->getPropertyUnsafe<std::string>(kNatronPluginPropIconFilePath).c_str());
    } else {
        // For menu items, take from grouping
        treeNodeName = grouping[0];
        iconFilePath = groupingIcon.isEmpty() ? QString() : groupingIcon[0];
    }
    PluginGroupNodePtr ret(new PluginGroupNode(grouping.empty() ? plugin : PluginPtr(), treeNodeName, iconFilePath));

    // If there is a parent, add it as a child
    if (parent) {
        parent->tryAddChild(ret);
        ret->setParent(parent);
    } else {
        // No parent, this is a top-level toolbutton
        _topLevelToolButtons.push_back(ret);
    }

    // If we still did not reach the desired tree node, find it, advancing (removing the first item) in the grouping
    if (!grouping.empty()) {
        QStringList newGrouping, newIconsGrouping;
        for (int i = 1; i < grouping.size(); ++i) {
            newGrouping.push_back(grouping[i]);
        }
        for (int i = 1; i < groupingIcon.size(); ++i) {
            newIconsGrouping.push_back(groupingIcon[i]);
        }

        return findPluginToolButtonOrCreateInternal(ret->getChildren(), ret, plugin, newGrouping, newIconsGrouping);
    }

    return ret;
} // GuiApplicationManagerPrivate::findPluginToolButtonOrCreateInternal

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
GuiApplicationManagerPrivate::updateFontConfigCache()
{
    qDebug() << "Building Fontconfig fonts...";
    FcConfig *fcConfig = FcInitLoadConfig();
    FcConfigBuildFonts(fcConfig);
    qDebug() << "Fontconfig fonts built";
}

NATRON_NAMESPACE_EXIT
