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

NATRON_NAMESPACE_ENTER

class PluginGroupNode
{
    QString _id;
    QString _label;
    QString _iconPath;
    int _major, _minor;
    std::list<PluginGroupNodePtr> _children;
    PluginGroupNodeWPtr _parent;
    bool _notHighestMajorVersion;
    bool _isUserCreatable;

public:
    PluginGroupNode(const QString & pluginID,
                    const QString & pluginLabel,
                    const QString & iconPath,
                    int major,
                    int minor,
                    bool isUserCreatable)
        : _id(pluginID)
        , _label(pluginLabel)
        , _iconPath(iconPath)
        , _major(major)
        , _minor(minor)
        , _children()
        , _parent()
        , _notHighestMajorVersion(false)
        , _isUserCreatable(isUserCreatable)
    {
    }

    bool getIsUserCreatable() const
    {
        return _isUserCreatable;
    }

    const QString & getID() const
    {
        return _id;
    }

    const QString & getLabel() const
    {
        return _label;
    }

    const QString getLabelVersionMajorEncoded() const
    {
        return _label + QString::fromUtf8(" ") + QString::number(_major);
    }

    void setLabel(const QString & label)
    {
        _label = label;
    }

    const QString & getIconPath() const
    {
        return _iconPath;
    }

    void setIconPath(const QString & iconPath)
    {
        _iconPath = iconPath;
    }

    const std::list<PluginGroupNodePtr> & getChildren() const
    {
        return _children;
    }

    void tryAddChild(const PluginGroupNodePtr& plugin);
    void tryRemoveChild(PluginGroupNode* plugin);

    PluginGroupNodePtr getParent() const
    {
        return _parent.lock();
    }

    void setParent(const PluginGroupNodePtr& parent)
    {
        _parent = parent;
    }

    bool hasParent() const
    {
        return _parent.lock().get() != NULL;
    }

    int getMajorVersion() const
    {
        return _major;
    }

    int getMinorVersion() const
    {
        return _minor;
    }

    bool getNotHighestMajorVersion() const
    {
        return _notHighestMajorVersion;
    }

    void setNotHighestMajorVersion(bool v)
    {
        _notHighestMajorVersion = v;
    }
};


class Plugin
{
    LibraryBinary* _binary;
    QString _resourcesPath; // Path to the resources (images etc..) of the plug-in, or empty if statically linked  (expected to be found in resources)
    QString _id;
    QString _label;
    QString _iconFilePath;
    QStringList _groupIconFilePath;
    QStringList _grouping;
    QString _labelWithoutSuffix;
    QString _pythonModule;
    OFX::Host::ImageEffect::ImageEffectPlugin* _ofxPlugin;
    OFX::Host::ImageEffect::Descriptor* _ofxDescriptor;
    QMutex* _lock;
    int _majorVersion;
    int _minorVersion;
    ContextEnum _ofxContext;
    mutable bool _hasShortcutSet; //< to speed up the keypress event of Nodegraph, this is used to find out quickly whether it has a shortcut or not.
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

public:

    Plugin()
        : _binary(NULL)
        , _resourcesPath()
        , _id()
        , _label()
        , _iconFilePath()
        , _groupIconFilePath()
        , _grouping()
        , _labelWithoutSuffix()
        , _pythonModule()
        , _ofxPlugin(0)
        , _ofxDescriptor(0)
        , _lock()
        , _majorVersion(0)
        , _minorVersion(0)
        , _ofxContext(eContextNone)
        , _hasShortcutSet(false)
        , _isReader(false)
        , _isWriter(false)
        , _isDeprecated(false)
        , _isInternalOnly(false)
        , _toolSetScript(false)
        , _activated(true)
        , _renderScaleEnabled(true)
        , _multiThreadingEnabled(true)
        , _openglActivated(true)
        , _openglRenderSupport(ePluginOpenGLRenderSupportNone)
    {
    }

    Plugin(LibraryBinary* binary,
           const QString& resourcesPath,
           const QString & id,
           const QString & label,
           const QString & iconFilePath,
           const QStringList & groupIconFilePath,
           const QStringList & grouping,
           QMutex* lock,
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
        , _lock(lock)
        , _majorVersion(majorVersion)
        , _minorVersion(minorVersion)
        , _ofxContext(eContextNone)
        , _hasShortcutSet(false)
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
    {
        if ( _resourcesPath.isEmpty() ) {
            _resourcesPath = QLatin1String(":/Resources/");
        }
    }

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

    QMutex* getPluginLock() const;
    LibraryBinary* getLibraryBinary() const;

    int getMajorVersion() const;

    int getMinorVersion() const;

    void setHasShortcut(bool has) const;

    bool getHasShortcut() const;

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
};

struct Plugin_compare_version
{
    bool operator() (const Plugin* const lhs,
                     const Plugin* const rhs) const
    {
        // see also OFX::Host::Plugin::trumps()
        return ( ( lhs->getMajorVersion() < rhs->getMajorVersion() ) ||
                ( ( lhs->getMajorVersion() == rhs->getMajorVersion() ) &&
                  ( lhs->getMinorVersion() < rhs->getMinorVersion() ) ) );
    }
};

typedef std::set<Plugin*, Plugin_compare_version> PluginVersionsOrdered;
typedef std::map<std::string, PluginVersionsOrdered> PluginsMap;

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

NATRON_NAMESPACE_EXIT

#endif // PLUGIN_H
