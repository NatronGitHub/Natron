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

#include "Plugin.h"

#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QtCore/QMutex>

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/InputDescription.h"
#include "Engine/EffectDescription.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER

void
Plugin::initializeProperties() const
{
    createProperty<void*>(kNatronPluginPropCreateFunc, 0);
    createProperty<void*>(kNatronPluginPropCreateRenderCloneFunc, 0);
    createProperty<std::string>(kNatronPluginPropID, std::string());
    createProperty<std::string>(kNatronPluginPropLabel, std::string());
    createProperty<std::string>(kNatronPluginPropDescription, std::string());
    createProperty<unsigned int>(kNatronPluginPropVersion, 0, 0);
    createProperty<bool>(kNatronPluginPropDescriptionIsMarkdown, false);
    createProperty<std::string>(kNatronPluginPropResourcesPath, PLUGIN_DEFAULT_RESOURCES_PATH);
    createProperty<std::string>(kNatronPluginPropIconFilePath, std::string());
    createProperty<std::string>(kNatronPluginPropGrouping, PLUGIN_GROUP_DEFAULT);
    createProperty<int>(kNatronPluginPropShortcut, 0, 0);
    createProperty<std::string>(kNatronPluginPropGroupIconFilePath, std::vector<std::string>());
    createProperty<std::string>(kNatronPluginPropPyPlugScriptAbsoluteFilePath, std::string());
    createProperty<std::string>(kNatronPluginPropPyPlugContainerID, std::string());
    createProperty<std::string>(kNatronPluginPropPyPlugExtScriptFile, std::string());
    createProperty<bool>(kNatronPluginPropPyPlugIsPythonScript, false);
    createProperty<bool>(kNatronPluginPropPyPlugIsToolset, false);
    createProperty<void*>(kNatronPluginPropOpenFXPluginPtr, 0);
    createProperty<bool>(kNatronPluginPropIsDeprecated, false);
    createProperty<bool>(kNatronPluginPropIsInternalOnly, false);
    createProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>());
    createPropertyInternal<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths);
    createProperty<bool>(kNatronPluginPropViewAware, false);
    createProperty<ViewInvarianceLevel>(kNatronPluginPropViewInvariant, eViewInvarianceAllViewsVariant);
    createProperty<PlanePassThroughEnum>(kNatronPluginPropPlanesPassThrough, ePassThroughPassThroughNonRenderedPlanes);
    createProperty<bool>(kNatronPluginPropMultiPlanar, false);
    createProperty<bool>(kNatronPluginPropSupportsDraftRender, false);
    createProperty<bool>(kNatronPluginPropHostChannelSelector, false);
    createProperty<std::bitset<4> >(kNatronPluginPropHostChannelSelectorValue, std::bitset<4>());
    createProperty<bool>(kNatronPluginPropHostMix, false);
    createProperty<bool>(kNatronPluginPropHostMask, false);
    createProperty<bool>(kNatronPluginPropHostPlaneSelector, false);
    createProperty<bool>(kNatronPluginPropSupportsMultiInputsPAR, false);
    createProperty<bool>(kNatronPluginPropSupportsMultiInputsBitDepths, false);
    createProperty<bool>(kNatronPluginPropSupportsMultiInputsFPS, false);
    createProperty<bool>(kNatronPluginPropRenderAllPlanesAtOnce, false);
    createProperty<std::string>(kNatronPluginPropSupportedExtensions, std::vector<std::string>());
    createProperty<double>(kNatronPluginPropIOEvaluation, -1.);
}

PluginPtr
Plugin::create(EffectBuilder createEffectFunc,
               EffectRenderCloneBuilder createCloneFunc,
               const std::string &pluginID,
               const std::string &pluginLabel,
               unsigned int majorVersion,
               unsigned int minorVersion,
               const std::vector<std::string> &pluginGrouping,
               const std::vector<std::string> &groupIconFilePath)
{
    if (pluginID.empty()) {
        throw std::invalid_argument("Plugin::create: plugin ID cannot be empty");
    }
    if (pluginLabel.empty()) {
        throw std::invalid_argument("Plugin::create: plugin label cannot be empty");
    }
    PluginPtr ret(new Plugin);
    GCC_DIAG_PEDANTIC_OFF
    ret->setProperty<void*>(kNatronPluginPropCreateFunc, (void*)createEffectFunc);
    ret->setProperty<void*>(kNatronPluginPropCreateRenderCloneFunc, (void*)createCloneFunc);
    GCC_DIAG_PEDANTIC_ON
    ret->setProperty<std::string>(kNatronPluginPropID, pluginID);
    ret->setProperty<std::string>(kNatronPluginPropLabel, pluginLabel);
    ret->setProperty<unsigned int>(kNatronPluginPropVersion, majorVersion);
    ret->setProperty<unsigned int>(kNatronPluginPropVersion, minorVersion, 1);
    if (!pluginGrouping.empty()) {
        ret->setPropertyN<std::string>(kNatronPluginPropGrouping, pluginGrouping);
    }
    if (!groupIconFilePath.empty()) {
        ret->setPropertyN<std::string>(kNatronPluginPropGroupIconFilePath, groupIconFilePath);
    } else {

        std::vector<std::string> grouping = ret->getPropertyNUnsafe<std::string>(kNatronPluginPropGrouping);
        std::string iconPath;
        const std::string& mainGroup = grouping[0];
        if ( mainGroup == PLUGIN_GROUP_COLOR) {
            iconPath = PLUGIN_GROUP_COLOR_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_FILTER) {
            iconPath = PLUGIN_GROUP_FILTER_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_IMAGE) {
            iconPath = PLUGIN_GROUP_IMAGE_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_TRANSFORM) {
            iconPath = PLUGIN_GROUP_TRANSFORM_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_DEEP) {
            iconPath = PLUGIN_GROUP_DEEP_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_MULTIVIEW) {
            iconPath = PLUGIN_GROUP_VIEWS_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_TIME) {
            iconPath = PLUGIN_GROUP_TIME_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_PAINT) {
            iconPath = PLUGIN_GROUP_PAINT_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_OTHER) {
            iconPath = PLUGIN_GROUP_MISC_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_KEYER) {
            iconPath = PLUGIN_GROUP_KEYER_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_TOOLSETS) {
            iconPath = PLUGIN_GROUP_TOOLSETS_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_3D) {
            iconPath = PLUGIN_GROUP_3D_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_CHANNEL) {
            iconPath = PLUGIN_GROUP_CHANNEL_ICON_PATH;
        } else if ( mainGroup == PLUGIN_GROUP_MERGE) {
            iconPath = PLUGIN_GROUP_MERGE_ICON_PATH;
        } else {
            iconPath = PLUGIN_GROUP_DEFAULT_ICON_PATH;
        }

        std::vector<std::string> groupIcon;
        groupIcon.push_back(iconPath);
        ret->setPropertyN<std::string>(kNatronPluginPropGroupIconFilePath, groupIcon);
    }
    return ret;
}

Plugin::Plugin()
: PropertiesHolder()
, _actionShortcuts()
, _presets()
, _pluginLock(new QMutex(QMutex::Recursive))
, _openfxContext(eContextNone)
, _openfxDescriptor(0)
, _isEnabled(true)
, _isHighestVersion(true)
, _openGLEnabled(true)
, _multiThreadEnabled(true)
, _renderScaleEnabled(true)
, _effectDescription(new EffectDescription)
{


}

Plugin::~Plugin()
{
}

std::string
Plugin::getPluginID() const
{
    return getPropertyUnsafe<std::string>(kNatronPluginPropID);
}

void
Plugin::addPresetFile(const PluginPresetDescriptor& preset)
{
    for (std::vector<PluginPresetDescriptor>::iterator it = _presets.begin(); it!= _presets.end(); ++it) {
        if (it->presetLabel == preset.presetLabel) {
            // Another preset with the same label already exists for this plug-in, ignore it
            return;
        }
    }
    _presets.push_back(preset);
}


struct PresetsSortByLabelFunctor
{
    bool operator() (const PluginPresetDescriptor& lhs,
                     const PluginPresetDescriptor& rhs)
    {
        return lhs.presetLabel < rhs.presetLabel;
    }
};


void
Plugin::sortPresetsByLabel()
{

    std::sort(_presets.begin(), _presets.end(), PresetsSortByLabelFunctor());
}

const std::vector<PluginPresetDescriptor>&
Plugin::getPresetFiles() const
{
    return _presets;
}

std::string
Plugin::getPluginShortcutGroup() const
{
    std::string ret = getPropertyUnsafe<std::string>(kNatronPluginPropLabel);
    bool isViewer = ret == "Viewer";
    ret += ' ';
    if (!isViewer) {
        ret +=  "Viewer";
        ret += ' ';
    }

    ret += "Interface";
    return ret;
}

std::string
Plugin::getPluginLabel() const
{
    return getPropertyUnsafe<std::string>(kNatronPluginPropLabel);
}

int
Plugin::getMajorVersion() const
{
    return getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 0);
}

int
Plugin::getMinorVersion() const
{
    return getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 1);
}

std::string
Plugin::getLabelVersionMajorMinorEncoded() const
{
    std::stringstream ss;
    ss << getLabelWithoutSuffix();
    ss << ' ';
    ss << getMajorVersion();
    ss << '.';
    ss << getMinorVersion();
    return ss.str();
}

std::string
Plugin::getLabelVersionMajorEncoded() const
{
    std::stringstream ss;
    ss << getLabelWithoutSuffix();
    ss << ' ';
    ss << getMajorVersion();
    return ss.str();
}

std::string
Plugin::makeLabelWithoutSuffix(const std::string& label)
{
    QString qLabel(QString::fromUtf8(label.c_str()));
    if ( qLabel.startsWith( QString::fromUtf8("Read") ) || qLabel.startsWith( QString::fromUtf8("Write") ) ) {
        return label;
    } else if ( qLabel.endsWith( QString::fromUtf8("OFX") ) ) {
        return qLabel.mid(0, label.size() - 3).toStdString();
    } else if ( qLabel.endsWith( QString::fromUtf8("CImg") ) ) {
        return qLabel.mid(0, label.size() - 4).toStdString();
    } else if ( qLabel.endsWith( QString::fromUtf8("OIIO") ) ) {
        return qLabel.mid(0, label.size() - 4).toStdString();
    }

    return label;
}

std::string
Plugin::getLabelWithoutSuffix() const
{
    return _labelWithoutSuffix;
}

void
Plugin::setLabelWithoutSuffix(const std::string& label)
{
    _labelWithoutSuffix = label;
}

QStringList
Plugin::getGroupingAsQStringList() const
{
    QStringList ret;
    std::vector<std::string> groupingStd = getPropertyNUnsafe<std::string>(kNatronPluginPropGrouping);
    for (std::size_t i = 0; i < groupingStd.size(); ++i) {
        ret.push_back(QString::fromUtf8(groupingStd[i].c_str()));
    }
    return ret;
}

std::string
Plugin::getGroupingString() const
{
    std::vector<std::string> groupingStd = getPropertyNUnsafe<std::string>(kNatronPluginPropGrouping);
    std::string ret;
    for (std::size_t i = 0; i < groupingStd.size(); ++i) {
        ret += groupingStd[i];
        if (i < groupingStd.size() - 1) {
            ret += "/";
        }
    }
    return ret;
}

std::string
Plugin::generateUserFriendlyPluginID() const
{
    std::stringstream ss;
    ss << getLabelWithoutSuffix();
    ss << "  [";
    ss << getPropertyUnsafe<std::string>(kNatronPluginPropGrouping, 0);
    ss << ']';
    return ss.str();
}

std::string
Plugin::generateUserFriendlyPluginIDMajorEncoded() const
{
    std::stringstream ss;
    ss << getLabelVersionMajorEncoded();
    ss << "  [";
    ss << getPropertyUnsafe<std::string>(kNatronPluginPropGrouping, 0);
    ss << ']';
    return ss.str();
}

boost::shared_ptr<QMutex>
Plugin::getPluginLock() const
{
    return _pluginLock;
}

OFX::Host::ImageEffect::Descriptor*
Plugin::getOfxDesc(ContextEnum* ctx) const
{
    *ctx = _openfxContext;
    return _openfxDescriptor;
}

bool
Plugin::isEnabled() const
{
    return _isEnabled;
}

void
Plugin::setEnabled(bool b)
{
    _isEnabled = b;
}

bool
Plugin::getIsUserCreatable() const
{
    return _isEnabled &&
    !getPropertyUnsafe<bool>(kNatronPluginPropIsInternalOnly) &&
    !getPropertyUnsafe<bool>(kNatronPluginPropIsDeprecated);
}

void
Plugin::setOfxDesc(OFX::Host::ImageEffect::Descriptor* desc,
                   ContextEnum ctx)
{
    assert(ctx != eContextNone);
    _openfxDescriptor = desc;
    _openfxContext = ctx;
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
    return _multiThreadEnabled;
}

void
Plugin::setMultiThreadingEnabled(bool b)
{
    _multiThreadEnabled = b;
}

bool
Plugin::isOpenGLEnabled() const
{
    return _openGLEnabled;
}

void
Plugin::setOpenGLEnabled(bool b)
{
    _openGLEnabled = b;
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

const
std::vector<InputDescriptionPtr>&
Plugin::getInputsDescription()
{
    return _inputsDescription;
}

void
Plugin::addInputDescription(const InputDescriptionPtr& desc)
{
    _inputsDescription.push_back(desc);
}

void
Plugin::addActionShortcut(const PluginActionShortcut& shortcut)
{
    _actionShortcuts.push_back(shortcut);
}

const std::list<PluginActionShortcut>&
Plugin::getShortcuts() const
{
    return _actionShortcuts;
}

QString
PluginGroupNode::getTreeNodeID() const
{
    PluginPtr plugin = getPlugin();
    if (plugin) {
        return QString::fromUtf8(plugin->getPluginID().c_str());
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

EffectDescriptionPtr
Plugin::getEffectDescriptor() const
{
    return _effectDescription;
}

bool
FormatExtensionCompareCaseInsensitive::operator() (const std::string& lhs,
                                                   const std::string& rhs) const
{
    return boost::algorithm::lexicographical_compare( lhs, rhs, boost::algorithm::is_iless() );
}

NATRON_NAMESPACE_EXIT
