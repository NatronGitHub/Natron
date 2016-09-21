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

#ifndef PLUGIN_H
#define PLUGIN_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <set>
#include <map>
#include <list>
#include <QtCore/QString>
#include <QtCore/QStringList>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Global/Enums.h"
#include "Engine/EngineFwd.h"
#include "Engine/PluginActionShortcut.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief A node of a tree data structure representing the grouping hierarchy of plug-ins. This is mainly for the GUI so it can create its toolbuttons and menus
 * If this node is a leaf it will have a valid plugin, otherwise it has just a name for menus
 **/
class PluginGroupNode
{

    std::list<PluginGroupNodePtr> _children;
    PluginGroupNodeWPtr _parent;
    PluginWPtr _plugin;

    // the name of the node in the hierarchy
    QString _name;

    // the icon file path of this menu item
    QString _iconFilePath;

public:
    PluginGroupNode(const PluginPtr& plugin,
                    const QString& name,
                    const QString& iconFilePath)
        : _children()
        , _parent()
        , _plugin(plugin)
        , _name(name)
        , _iconFilePath(iconFilePath)
    {
    }

    PluginPtr getPlugin() const
    {
        return _plugin.lock();
    }


    const std::list<PluginGroupNodePtr> & getChildren() const
    {
        return _children;
    }

    void tryAddChild(const PluginGroupNodePtr& plugin);

    void tryRemoveChild(const PluginGroupNodePtr& plugin);

    PluginGroupNodePtr getParent() const
    {
        return _parent.lock();
    }

    void setParent(const PluginGroupNodePtr& parent)
    {
        _parent = parent;
    }

    const QString& getTreeNodeName() const
    {
        return _name;
    }

    const QString& getTreeNodeIconFilePath() const
    {
        return _iconFilePath;
    }

    // the node ID is the same as name for menu items and the pluginID if this is a leaf
    const QString& getTreeNodeID() const;
    
};

// Identify a preset for a node
struct PluginPresetDescriptor
{
    QString presetFilePath;
    QString presetLabel;
    QString presetIconFile;
    Key symbol;
    KeyboardModifiers modifiers;
};

/**
 * @brief A Natron plug-in that can be instantiated in nodes
 **/
class Plugin
{
    LibraryBinary* _binary;
    QString _resourcesPath; // Path to the resources (images etc..) of the plug-in, or empty if statically linked  (epxected to be found in resources)
    QString _id;
    QString _label;
    QString _iconFilePath;
    QStringList _groupIconFilePath;
    QStringList _grouping;
    QString _labelWithoutSuffix;

    // Deprecated, PyPlugs are no longer python scripts
    QString _pythonModule;
    OFX::Host::ImageEffect::ImageEffectPlugin* _ofxPlugin;
    OFX::Host::ImageEffect::Descriptor* _ofxDescriptor;
    boost::shared_ptr<QMutex> _lock;
    int _majorVersion;
    int _minorVersion;
    ContextEnum _ofxContext;

    bool _isReader, _isWriter;

    //Deprecated are by default Disabled in the Preferences.
    bool _isDeprecated;

    //Is not visible by the user, just for internal use
    bool _isInternalOnly;

    //When the plug-in is a PyPlug, if this is set to true, the script will not be embedded into a group
    bool _toolSetScript;
    mutable bool _activated;

    /*
       These are shortcuts that the plug-in registered
     */
    std::list<PluginActionShortcut> _shortcuts;
    bool _renderScaleEnabled;
    bool _multiThreadingEnabled;
    bool _openglActivated;

    PluginOpenGLRenderSupport _openglRenderSupport;

    std::vector<PluginPresetDescriptor> _presetsFiles;

    bool _isHighestVersion;

public:

    Plugin(LibraryBinary* binary,
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
           bool isDeprecated);

    ~Plugin();

    void setShorcuts(const std::list<PluginActionShortcut>& shortcuts)
    {
        _shortcuts = shortcuts;
    }

    const std::list<PluginActionShortcut>& getShortcuts() const
    {
        return _shortcuts;
    }

    const QString& getResourcesPath() const
    {
        return _resourcesPath;
    }

    void addPresetFile(const QString& filePath, const QString& presetLabel, const QString& iconFilePath, Key symbol, const KeyboardModifiers& mods);

    const std::vector<PluginPresetDescriptor>& getPresetFiles() const;

    void sortPresetsByLabel();

    QString getPluginShortcutGroup() const;

    bool getIsForInternalUseOnly() const { return _isInternalOnly; }

    void setForInternalUseOnly(bool b) { _isInternalOnly = b; }

    bool getIsDeprecated() const { return _isDeprecated; }

    bool getIsUserCreatable() const;

    void setPluginID(const QString & id);


    const QString & getPluginID() const;

    bool isReader() const;

    bool isWriter() const;

    void setPluginLabel(const QString & label);

    const QString & getPluginLabel() const;
    const QString getLabelVersionMajorMinorEncoded() const;
    static QString makeLabelWithoutSuffix(const QString& label);
    const QString& getLabelWithoutSuffix() const;
    void setLabelWithoutSuffix(const QString& label);

    const QString getLabelVersionMajorEncoded() const;

    QString generateUserFriendlyPluginID() const;

    QString generateUserFriendlyPluginIDMajorEncoded() const;

    const QString & getIconFilePath() const;

    void setIconFilePath(const QString& filePath);

    const QStringList & getGroupIconFilePath() const;
    const QStringList & getGrouping() const;

    void setToolsetScript(bool isToolset);

    bool getToolsetScript() const;

    boost::shared_ptr<QMutex> getPluginLock() const;
    LibraryBinary* getLibraryBinary() const;

    int getMajorVersion() const;

    int getMinorVersion() const;

    void setPythonModule(const QString& module);

    const QString& getPythonModule() const;

    void getPythonModuleNameAndPath(QString* moduleName, QString* modulePath) const;

    void setOfxPlugin(OFX::Host::ImageEffect::ImageEffectPlugin* p);

    OFX::Host::ImageEffect::ImageEffectPlugin* getOfxPlugin() const;
    OFX::Host::ImageEffect::Descriptor* getOfxDesc(ContextEnum* ctx) const;

    void setOfxDesc(OFX::Host::ImageEffect::Descriptor* desc, ContextEnum ctx);

    bool isRenderScaleEnabled() const;
    void setRenderScaleEnabled(bool b);

    bool isMultiThreadingEnabled() const;
    void setMultiThreadingEnabled(bool b);

    bool isActivated() const;
    void setActivated(bool b);

    bool isOpenGLEnabled() const;
    void setOpenGLEnabled(bool b);

    void setOpenGLRenderSupport(PluginOpenGLRenderSupport support);
    PluginOpenGLRenderSupport getPluginOpenGLRenderSupport() const;

    void setIsHighestMajorVersion(bool isHighest);
    bool getIsHighestMajorVersion() const;
};

struct Plugin_compare_major
{
    bool operator() (const PluginPtr& lhs,
                     const PluginPtr& rhs) const
    {
        return lhs->getMajorVersion() < rhs->getMajorVersion();
    }
};

typedef std::set<PluginPtr, Plugin_compare_major> PluginMajorsOrdered;
typedef std::map<std::string, PluginMajorsOrdered> PluginsMap;

struct IOPluginEvaluation
{
    std::string pluginID;
    double evaluation;

    IOPluginEvaluation()
        : pluginID()
        , evaluation(0)
    {
    }

    IOPluginEvaluation(const std::string& p,
                       double e)
        : pluginID(p)
        , evaluation(e)
    {}
};

struct IOPluginEvaluation_CompareLess
{
    bool operator() (const IOPluginEvaluation& lhs,
                     const IOPluginEvaluation& rhs) const
    {
        return lhs.evaluation < rhs.evaluation;
    }
};

struct FormatExtensionCompareCaseInsensitive
{
    bool operator() (const std::string& lhs, const std::string& rhs) const;
};

typedef std::set<IOPluginEvaluation, IOPluginEvaluation_CompareLess> IOPluginSetForFormat;
// For each extension format (lower case) a set of plug-in IDs sorted by increasing evaluation order.
// The best plug-in for a format is the set.rbegin()
typedef std::map<std::string, IOPluginSetForFormat, FormatExtensionCompareCaseInsensitive> IOPluginsMap;

NATRON_NAMESPACE_EXIT;

#endif // PLUGIN_H
