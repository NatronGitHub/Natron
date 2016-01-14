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

#include <QMutex>

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Settings.h"

using namespace Natron;

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
Plugin::isReader() const {
    return _isReader;
}

bool
Plugin::isWriter() const {
    return _isWriter;
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
    return getLabelWithoutSuffix() + ' ' + QString::number(_majorVersion) + '.' + QString::number(_minorVersion);
}

QString
Plugin::makeLabelWithoutSuffix(const QString& label)
{
    if (label.startsWith("Read") || label.startsWith("Write")) {
        return label;
    } else if (label.endsWith("OFX")) {
        return label.mid(0,label.size() - 3);
    } else if (label.endsWith("CImg")) {
        return label.mid(0,label.size() - 4);
    } else if (label.endsWith("OIIO")) {
        return label.mid(0,label.size() - 4);
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
    return getLabelWithoutSuffix() + ' ' + QString::number(_majorVersion);
}

QString
Plugin::generateUserFriendlyPluginID() const
{
    QString grouping = _grouping.size() > 0 ? _grouping[0] : QString();
    return getLabelWithoutSuffix() + "  [" + grouping + "]";
}

QString
Plugin::generateUserFriendlyPluginIDMajorEncoded() const
{
    QString grouping = _grouping.size() > 0 ? _grouping[0] : QString();
    return getLabelVersionMajorEncoded() + "  [" + grouping + "]";
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

QMutex*
Plugin::getPluginLock() const
{
    return _lock;
}

Natron::LibraryBinary*
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
Plugin::getPythonModule() const {
    return _pythonModule;
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
    if (!_activatedSet) {
        _activated = !appPTR->getCurrentSettings()->isPluginDeactivated(this);
        _activatedSet = true;
    }
    return !_isInternalOnly && _activated;
}

void
Plugin::setOfxDesc(OFX::Host::ImageEffect::Descriptor* desc,ContextEnum ctx)
{
    assert(ctx != Natron::eContextNone);
    _ofxDescriptor = desc;
    _ofxContext = ctx;
}

void
PluginGroupNode::tryAddChild(const boost::shared_ptr<PluginGroupNode>& plugin)
{
    for (std::list<boost::shared_ptr<PluginGroupNode> >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (*it == plugin) {
            return;
        }
    }
    _children.push_back(plugin);
}

void
PluginGroupNode::tryRemoveChild(PluginGroupNode* plugin)
{
    for (std::list<boost::shared_ptr<PluginGroupNode> >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->get() == plugin) {
            _children.erase(it);
            return;
        }
    }
}

