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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Plugin.h"

#include <cassert>
#include <stdexcept>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QtCore/QMutex>

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER;

Plugin::Plugin(LibraryBinary* binary,
       const QString& resourcesPath,
       const QString & id,
       const QString & label,
       const QString & iconFilePath,
       const QStringList & groupIconFilePath,
       const QStringList & grouping,
       bool mustCreateMutex,
       int majorVersion,
       int minorVersion,
       bool isReader,
       bool isWriter,
       bool isDeprecated)
: _binary(binary)
, _resourcesPath(resourcesPath)
, _id(id)
, _label(label)
, _iconFilePath(iconFilePath)
, _groupIconFilePath(groupIconFilePath)
, _grouping(grouping)
, _labelWithoutSuffix()
, _pythonModule()
, _ofxPlugin(0)
, _ofxDescriptor(0)
, _lock()
, _majorVersion(majorVersion)
, _minorVersion(minorVersion)
, _ofxContext(eContextNone)
, _isReader(isReader)
, _isWriter(isWriter)
, _isDeprecated(isDeprecated)
, _isInternalOnly(false)
, _toolSetScript(false)
, _activated(true)
, _renderScaleEnabled(true)
, _multiThreadingEnabled(true)
, _openglActivated(true)
, _openglRenderSupport(ePluginOpenGLRenderSupportNone)
, _presetsFiles()
, _isHighestVersion(false)
{
    if ( _resourcesPath.isEmpty() ) {
        _resourcesPath = QString::fromUtf8(PLUGIN_DEFAULT_RESOURCES_PATH);
    }

    if (_grouping.isEmpty()) {
        _grouping.push_back(QString::fromUtf8(PLUGIN_GROUP_DEFAULT));
    }
    if (_groupIconFilePath.isEmpty()) {
        std::string iconPath;
        const QString& mainGroup = _grouping[0];
        if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_COLOR) ) {
            iconPath = PLUGIN_GROUP_COLOR_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_FILTER) ) {
            iconPath = PLUGIN_GROUP_FILTER_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_IMAGE) ) {
            iconPath = PLUGIN_GROUP_IMAGE_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_TRANSFORM) ) {
            iconPath = PLUGIN_GROUP_TRANSFORM_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_DEEP) ) {
            iconPath = PLUGIN_GROUP_DEEP_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_MULTIVIEW) ) {
            iconPath = PLUGIN_GROUP_VIEWS_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_TIME) ) {
            iconPath = PLUGIN_GROUP_TIME_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_PAINT) ) {
            iconPath = PLUGIN_GROUP_PAINT_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_OTHER) ) {
            iconPath = PLUGIN_GROUP_MISC_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_KEYER) ) {
            iconPath = PLUGIN_GROUP_KEYER_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_TOOLSETS) ) {
            iconPath = PLUGIN_GROUP_TOOLSETS_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_3D) ) {
            iconPath = PLUGIN_GROUP_3D_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_CHANNEL) ) {
            iconPath = PLUGIN_GROUP_CHANNEL_ICON_PATH;
        } else if ( mainGroup == QString::fromUtf8(PLUGIN_GROUP_MERGE) ) {
            iconPath = PLUGIN_GROUP_MERGE_ICON_PATH;
        } else {
            iconPath = PLUGIN_GROUP_DEFAULT_ICON_PATH;
        }

        _groupIconFilePath.push_back(QString::fromUtf8(iconPath.c_str()));
    }

    if (mustCreateMutex) {
        _lock.reset(new QMutex(QMutex::Recursive));
    }
}

Plugin::~Plugin()
{
    delete _binary;
}

void
Plugin::setPluginID(const QString & id)
{
    _id = id;
}

const QString &
Plugin::getPluginID() const
{
    return _id;
}

bool
Plugin::isReader() const
{
    return _isReader;
}

bool
Plugin::isWriter() const
{
    return _isWriter;
}

void
Plugin::addPresetFile(const QString& filePath, const QString& presetLabel, const QString& iconFilePath, Key symbol, const KeyboardModifiers& mods)
{
    for (std::vector<PluginPresetDescriptor>::iterator it = _presetsFiles.begin(); it!=_presetsFiles.end(); ++it) {
        if (it->presetLabel == presetLabel) {
            // Another preset with the same label already exists for this plug-in, ignore it
            return;
        }
    }
    PluginPresetDescriptor s;
    s.presetFilePath = filePath;
    s.presetLabel = presetLabel;
    s.presetIconFile = iconFilePath;
    s.symbol = symbol;
    s.modifiers = mods;
    _presetsFiles.push_back(s);
}

struct PresetsSortByLabelFunctor
{
    bool operator() (const PluginPresetDescriptor& lhs,
                     PluginPresetDescriptor& rhs)
    {
        return lhs.presetLabel < rhs.presetLabel;
    }
};

void
Plugin::sortPresetsByLabel()
{
    
    std::sort(_presetsFiles.begin(), _presetsFiles.end(), PresetsSortByLabelFunctor());
}

const std::vector<PluginPresetDescriptor>&
Plugin::getPresetFiles() const
{
    return _presetsFiles;
}

QString
Plugin::getPluginShortcutGroup() const
{
    QString ret = getPluginLabel();
    bool isViewer = ret == QLatin1String("Viewer");
    ret += QLatin1Char(' ');
    if (!isViewer) {
        ret +=  QLatin1String("Viewer");
        ret += QLatin1Char(' ');
    }

    ret += QLatin1String("Interface");
    return ret;
}

void
Plugin::setPluginLabel(const QString & label)
{
    _label = label;
}

const QString &
Plugin::getPluginLabel() const
{
    return _label;
}

const QString
Plugin::getLabelVersionMajorMinorEncoded() const
{
    return getLabelWithoutSuffix() + QLatin1Char(' ') + QString::number(_majorVersion) + QLatin1Char('.') + QString::number(_minorVersion);
}

QString
Plugin::makeLabelWithoutSuffix(const QString& label)
{
    if ( label.startsWith( QString::fromUtf8("Read") ) || label.startsWith( QString::fromUtf8("Write") ) ) {
        return label;
    } else if ( label.endsWith( QString::fromUtf8("OFX") ) ) {
        return label.mid(0, label.size() - 3);
    } else if ( label.endsWith( QString::fromUtf8("CImg") ) ) {
        return label.mid(0, label.size() - 4);
    } else if ( label.endsWith( QString::fromUtf8("OIIO") ) ) {
        return label.mid(0, label.size() - 4);
    }

    return label;
}

const QString&
Plugin::getLabelWithoutSuffix() const
{
    return _labelWithoutSuffix;
}

void
Plugin::setLabelWithoutSuffix(const QString& label)
{
    _labelWithoutSuffix = label;
}

const QString
Plugin::getLabelVersionMajorEncoded() const
{
    return getLabelWithoutSuffix() + QLatin1Char(' ') + QString::number(_majorVersion);
}

QString
Plugin::generateUserFriendlyPluginID() const
{
    QString grouping = _grouping.size() > 0 ? _grouping[0] : QString();

    return getLabelWithoutSuffix() + QString::fromUtf8("  [") + grouping + QLatin1Char(']');
}

QString
Plugin::generateUserFriendlyPluginIDMajorEncoded() const
{
    QString grouping = _grouping.size() > 0 ? _grouping[0] : QString();

    return getLabelVersionMajorEncoded() + QString::fromUtf8("  [") + grouping + QLatin1Char(']');
}

void
Plugin::setToolsetScript(bool isToolset)
{
    _toolSetScript = isToolset;
}

bool
Plugin::getToolsetScript() const
{
    return _toolSetScript;
}

const QString&
Plugin::getIconFilePath() const
{
    return _iconFilePath;
}

void
Plugin::setIconFilePath(const QString& filePath)
{
    _iconFilePath = filePath;
}

const QStringList&
Plugin::getGroupIconFilePath() const
{
    return _groupIconFilePath;
}

const QStringList&
Plugin::getGrouping() const
{
    return _grouping;
}

boost::shared_ptr<QMutex>
Plugin::getPluginLock() const
{
    return _lock;
}

LibraryBinary*
Plugin::getLibraryBinary() const
{
    return _binary;
}

int
Plugin::getMajorVersion() const
{
    return _majorVersion;
}

int
Plugin::getMinorVersion() const
{
    return _minorVersion;
}

void
Plugin::setPythonModule(const QString& module)
{
    _pythonModule = module;
}

const QString&
Plugin::getPythonModule() const
{
    return _pythonModule; // < does not end with .py
}

void
Plugin::getPythonModuleNameAndPath(QString* moduleName,
                                   QString* modulePath) const
{
    int foundLastSlash = _pythonModule.lastIndexOf( QLatin1Char('/') );

    if (foundLastSlash != -1) {
        *modulePath = _pythonModule.mid(0, foundLastSlash + 1);
        *moduleName = _pythonModule.mid(foundLastSlash + 1);
    } else {
        *moduleName = _pythonModule;
    }
}

void
Plugin::setOfxPlugin(OFX::Host::ImageEffect::ImageEffectPlugin* p)
{
    _ofxPlugin = p;
}

OFX::Host::ImageEffect::ImageEffectPlugin*
Plugin::getOfxPlugin() const
{
    return _ofxPlugin;
}

OFX::Host::ImageEffect::Descriptor*
Plugin::getOfxDesc(ContextEnum* ctx) const
{
    *ctx = _ofxContext;

    return _ofxDescriptor;
}

bool
Plugin::getIsUserCreatable() const
{
    return !_isInternalOnly && _activated && !_isDeprecated;
}

void
Plugin::setOfxDesc(OFX::Host::ImageEffect::Descriptor* desc,
                   ContextEnum ctx)
{
    assert(ctx != eContextNone);
    _ofxDescriptor = desc;
    _ofxContext = ctx;
}

bool
Plugin::isRenderScaleEnabled() const
{
    return _renderScaleEnabled;
}

void
Plugin::setRenderScaleEnabled(bool b)
{
    _renderScaleEnabled = b;
}

bool
Plugin::isMultiThreadingEnabled() const
{
    return _multiThreadingEnabled;
}

void
Plugin::setMultiThreadingEnabled(bool b)
{
    _multiThreadingEnabled = b;
}

bool
Plugin::isOpenGLEnabled() const
{
    return _openglActivated;
}

void
Plugin::setOpenGLEnabled(bool b)
{
    _openglActivated = b;
}

void
Plugin::setOpenGLRenderSupport(PluginOpenGLRenderSupport support)
{
    _openglRenderSupport = support;
}

PluginOpenGLRenderSupport
Plugin::getPluginOpenGLRenderSupport() const
{
    return _openglRenderSupport;
}

bool
Plugin::isActivated() const
{
    return _activated;
}

void
Plugin::setActivated(bool b)
{
    _activated = b;
}

void
Plugin::setIsHighestMajorVersion(bool isHighest)
{
    _isHighestVersion = isHighest;
}

bool
Plugin::getIsHighestMajorVersion() const
{
    return _isHighestVersion;
}

const QString&
PluginGroupNode::getTreeNodeID() const
{
    PluginPtr plugin = getPlugin();
    if (plugin) {
        return plugin->getPluginID();
    } else {
        return _name;
    }
}

void
PluginGroupNode::tryAddChild(const PluginGroupNodePtr& plugin)
{
    for (std::list<PluginGroupNodePtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (*it == plugin) {
            return;
        }
    }
    _children.push_back(plugin);
}

void
PluginGroupNode::tryRemoveChild(const PluginGroupNodePtr& plugin)
{
    for (std::list<PluginGroupNodePtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (*it == plugin) {
            _children.erase(it);

            return;
        }
    }
}

bool
FormatExtensionCompareCaseInsensitive::operator() (const std::string& lhs,
                                                   const std::string& rhs) const
{
    return boost::algorithm::lexicographical_compare( lhs, rhs, boost::algorithm::is_iless() );
}

NATRON_NAMESPACE_EXIT;
