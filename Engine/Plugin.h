//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include <vector>
#include <set>
#include <map>
#include <QString>
#include <QStringList>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

class QMutex;
namespace Natron {
class LibraryBinary;
}

class PluginGroupNode
{
    QString _id;
    QString _label;
    QString _iconPath;
    int _major,_minor;
    std::list<boost::shared_ptr<PluginGroupNode> > _children;
    boost::weak_ptr<PluginGroupNode> _parent;
    bool _notHighestMajorVersion;
public:
    PluginGroupNode(const QString & pluginID,
                    const QString & pluginLabel,
                    const QString & iconPath,
                    int major,
                    int minor)
    : _id(pluginID)
    , _label(pluginLabel)
    , _iconPath(iconPath)
    , _major(major)
    , _minor(minor)
    , _children()
    , _parent()
    , _notHighestMajorVersion(false)
    {
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
    QString _groupIconFilePath;
    QStringList _grouping;
    QString _ofxPluginID;
    QMutex* _lock;
    int _majorVersion;
    int _minorVersion;
    mutable bool _hasShortcutSet; //< to speed up the keypress event of Nodegraph, this is used to find out quickly whether it has a shortcut or not.
    bool _isReader,_isWriter;
public:

    Plugin()
        : _binary(NULL)
          , _id()
          , _label()
          , _iconFilePath()
          , _groupIconFilePath()
          , _grouping()
          , _ofxPluginID()
          , _lock()
          , _majorVersion(0)
          , _minorVersion(0)
          , _hasShortcutSet(false)
          , _isReader(false)
          , _isWriter(false)
    {
    }

    Plugin(Natron::LibraryBinary* binary,
           const QString & id,
           const QString & label,
           const QString & iconFilePath,
           const QString & groupIconFilePath,
           const QStringList & grouping,
           const QString & ofxPluginID,
           QMutex* lock,
           int majorVersion,
           int minorVersion,
           bool isReader,
           bool isWriter)
        : _binary(binary)
          , _id(id)
          , _label(label)
          , _iconFilePath(iconFilePath)
          , _groupIconFilePath(groupIconFilePath)
          , _grouping(grouping)
          , _ofxPluginID(ofxPluginID)
          , _lock(lock)
          , _majorVersion(majorVersion)
          , _minorVersion(minorVersion)
          , _hasShortcutSet(false)
          , _isReader(isReader)
          , _isWriter(isWriter)
    {
    }

    ~Plugin();

    void setPluginID(const QString & id)
    {
        _id = id;
    }

    const QString & getPluginID() const
    {
        return _id;
    }
    
    bool isReader() const {
        return _isReader;
    }
    
    bool isWriter() const {
        return _isWriter;
    }

    void setPluginLabel(const QString & label)
    {
        _label = label;
    }

    const QString & getPluginLabel() const
    {
        return _label;
    }
    
    const QString getLabelVersionMajorMinorEncoded() const
    {
        return _label + ' ' + QString::number(_majorVersion) + '.' + QString::number(_minorVersion);
    }
    
    const QString getLabelVersionMajorEncoded() const
    {
        return _label + ' ' + QString::number(_majorVersion);
    }
    
    QString generateUserFriendlyPluginID() const
    {
        QString grouping = _grouping.size() > 0 ? _grouping[0] : QString();
        return _label + "  [" + grouping + "]";
    }
    
    QString generateUserFriendlyPluginIDMajorEncoded() const
    {
        QString grouping = _grouping.size() > 0 ? _grouping[0] : QString();
        return getLabelVersionMajorEncoded() + "  [" + grouping + "]";
    }

    
    const QString & getPluginOFXID() const
    {
        return _ofxPluginID;
    }

    const QString & getIconFilePath() const
    {
        return _iconFilePath;
    }

    const QString & getGroupIconFilePath() const
    {
        return _groupIconFilePath;
    }

    const QStringList & getGrouping() const
    {
        return _grouping;
    }

    QMutex* getPluginLock() const
    {
        return _lock;
    }

    Natron::LibraryBinary* getLibraryBinary() const
    {
        return _binary;
    }

    int getMajorVersion() const
    {
        return _majorVersion;
    }

    int getMinorVersion() const
    {
        return _minorVersion;
    }

    void setHasShortcut(bool has) const
    {
        _hasShortcutSet = has;
    }

    bool getHasShortcut() const
    {
        return _hasShortcutSet;
    }
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
