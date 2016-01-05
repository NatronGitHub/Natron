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

#ifndef PLUGIN_H
#define PLUGIN_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <set>
#include <map>
#include <QString>
#include <QStringList>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Global/Enums.h"
#include "Engine/EngineFwd.h"


class PluginGroupNode
{
    QString _id;
    QString _label;
    QString _iconPath;
    int _major,_minor;
    std::list<boost::shared_ptr<PluginGroupNode> > _children;
    boost::weak_ptr<PluginGroupNode> _parent;
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

    bool getIsUserCreatable() const {
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
        return _label + ' ' + QString::number(_major);
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

    const std::list<boost::shared_ptr<PluginGroupNode> > & getChildren() const
    {
        return _children;
    }

    void tryAddChild(const boost::shared_ptr<PluginGroupNode>& plugin);
    void tryRemoveChild(PluginGroupNode* plugin);
    
    boost::shared_ptr<PluginGroupNode> getParent() const
    {
        return _parent.lock();
    }

    void setParent(const boost::shared_ptr<PluginGroupNode>& parent)
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

namespace Natron {
class Plugin
{
    Natron::LibraryBinary* _binary;
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
    bool _isReader,_isWriter;

    //Deprecated are by default Disabled in the Preferences.
    bool _isDeprecated;
    
    //Is not visible by the user, just for internal use
    bool _isInternalOnly;
    
    mutable  bool _activatedSet;
    mutable  bool _activated;
    
public:

    Plugin()
    : _binary(NULL)
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
    , _activatedSet(false)
    , _activated(true)
    {
    }

    Plugin(Natron::LibraryBinary* binary,
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
    , _activatedSet(false)
    , _activated(true)
    {
    }
    
    ~Plugin();
    
    bool getIsForInternalUseOnly() const { return _isInternalOnly; }
    
    void setForInternalUseOnly(bool b) { _isInternalOnly = b; }

    bool getIsDeprecated() const { return _isDeprecated; }
    
    bool getIsUserCreatable() const;
    
    void setPluginID(const QString & id);
    

    const QString & getPluginID() const;
    
    bool isReader() const ;
    
    bool isWriter() const ;

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
    
    QMutex* getPluginLock() const;

    Natron::LibraryBinary* getLibraryBinary() const;

    int getMajorVersion() const;

    int getMinorVersion() const;

    void setHasShortcut(bool has) const;

    bool getHasShortcut() const;
    
    void setPythonModule(const QString& module);
    
    const QString& getPythonModule() const ;
    
    void setOfxPlugin(OFX::Host::ImageEffect::ImageEffectPlugin* p);
    
    OFX::Host::ImageEffect::ImageEffectPlugin* getOfxPlugin() const;
    
    OFX::Host::ImageEffect::Descriptor* getOfxDesc(ContextEnum* ctx) const;
    
    void setOfxDesc(OFX::Host::ImageEffect::Descriptor* desc,ContextEnum ctx);
    
};
    
struct Plugin_compare_major
{
    bool operator() (const Plugin* const lhs,
                     const Plugin* const rhs) const
    {
        return lhs->getMajorVersion() < rhs->getMajorVersion();
    }
};
    
typedef std::set<Plugin*,Plugin_compare_major> PluginMajorsOrdered;
typedef std::map<std::string,PluginMajorsOrdered> PluginsMap;
}

#endif // PLUGIN_H
