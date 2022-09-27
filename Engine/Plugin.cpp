/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Plugin.h"

#include <algorithm>
#include <cassert>
#include <locale>
#include <stdexcept>

#include <QtCore/QMutex>

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER

Plugin::~Plugin()
{
    delete _lock;
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

QString
Plugin::getPluginShortcutGroup() const
{
    return getPluginLabel() + QLatin1String(" Viewer Interface");
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
QRecursiveMutex*
#else
QMutex*
#endif
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
Plugin::setHasShortcut(bool has) const
{
    _hasShortcutSet = has;
}

bool
Plugin::getHasShortcut() const
{
    return _hasShortcutSet;
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
PluginGroupNode::tryRemoveChild(PluginGroupNode* plugin)
{
    for (std::list<PluginGroupNodePtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->get() == plugin) {
            _children.erase(it);

            return;
        }
    }
}

static bool
iless(char l, char r) {
    return std::tolower(l, std::locale()) < std::tolower(r, std::locale());
}

bool
FormatExtensionCompareCaseInsensitive::operator() (const std::string& lhs,
                                                   const std::string& rhs) const
{
    return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), iless );
}

NATRON_NAMESPACE_EXIT
