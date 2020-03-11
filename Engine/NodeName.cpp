/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "NodePrivate.h"

NATRON_NAMESPACE_ENTER


static bool findAvailableName(const std::string &baseName, const NodeCollectionPtr& group, const NodePtr& node, std::string* name)
{
    *name = baseName;
    int no = 1;
    do {
        if (no > 1) {
            std::stringstream ss;
            ss << baseName;
            ss << '_';
            ss << no;
            *name = ss.str();
        }
        ++no;
    } while ( group && group->checkIfNodeNameExists(*name, node) );

    // When no == 2 the name was available
    return no == 2;
}

void
Node::initNodeScriptName(const SERIALIZATION_NAMESPACE::NodeSerialization* serialization, const QString& fixedName)
{
    // If the serialization is not null, we are either pasting a node or loading it from a project.
    if (!fixedName.isEmpty()) {

        NodeCollectionPtr group = getGroup();
        std::string name;
        findAvailableName(fixedName.toStdString(), group, shared_from_this(), &name);

        //This version of setScriptName will not error if the name is invalid or already taken
        setScriptName_no_error_check(name);

        setLabel(name);


    } else if (serialization) {

        if (serialization->_nodeScriptName.empty()) {
            // The serialized script name may be empty in the case we are loading from a PyPlug/Preset directly
            initNodeNameFallbackOnPluginDefault();
        } else {
            NodeCollectionPtr group = getGroup();
            std::string name;
            bool nameAvailable = findAvailableName(serialization->_nodeScriptName, group, shared_from_this(), &name);

            // This version of setScriptName will not error if the name is invalid or already taken
            setScriptName_no_error_check(name);


            // If the script name was not available, give the same script name to the label, most likely this is a copy/paste operation.
            if (!nameAvailable) {
                setLabel(name);
            } else {
                setLabel(serialization->_nodeLabel);
            }
        }
        
    } else {
        initNodeNameFallbackOnPluginDefault();
    }
    
    
    
}

const std::string &
Node::getScriptName() const
{
    ////Only called by the main-thread
    //assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->nameMutex);

    return _imp->scriptName;
}

std::string
Node::getScriptName_mt_safe() const
{
    QMutexLocker l(&_imp->nameMutex);

    return _imp->scriptName;
}

static void
prependGroupNameRecursive(const NodePtr& group,
                          std::string& name)
{
    name.insert(0, ".");
    name.insert( 0, group->getScriptName_mt_safe() );
    NodeCollectionPtr hasParentGroup = group->getGroup();
    NodeGroupPtr isGrp = toNodeGroup(hasParentGroup);
    if (isGrp) {
        prependGroupNameRecursive(isGrp->getNode(), name);
    }
}

std::string
Node::getFullyQualifiedNameInternal(const std::string& scriptName) const
{
    std::string ret = scriptName;


    NodeCollectionPtr hasParentGroup = getGroup();
    NodeGroupPtr isGrp = toNodeGroup(hasParentGroup);
    if (isGrp) {
        NodePtr grpNode = isGrp->getNode();
        if (grpNode) {
            prependGroupNameRecursive(grpNode, ret);
        }
    }


    return ret;
}


std::string
Node::getFullyQualifiedName() const
{
    return getFullyQualifiedNameInternal( getScriptName_mt_safe() );
}


std::string
Node::getContainerGroupFullyQualifiedName() const
{
    NodeCollectionPtr collection = getGroup();
    NodeGroupPtr containerIsGroup = toNodeGroup(collection);
    if (containerIsGroup) {
        return  containerIsGroup->getNode()->getFullyQualifiedName();
    }
    return std::string();
}


void
Node::setLabel(const std::string& label)
{
    assert( QThread::currentThread() == qApp->thread() );

    QString curLabel;
    QString newLabel = QString::fromUtf8(label.c_str());
    {
        QMutexLocker k(&_imp->nameMutex);
        if (label == _imp->label) {
            return;
        }
        curLabel = QString::fromUtf8(_imp->label.c_str());
        _imp->label = label;
    }
    NodeCollectionPtr collection = getGroup();
    if (collection) {
        collection->notifyNodeLabelChanged( shared_from_this() );
    }
    Q_EMIT labelChanged(curLabel, newLabel );
}

const std::string&
Node::getLabel() const
{
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker k(&_imp->nameMutex);

    return _imp->label;
}

std::string
Node::getLabel_mt_safe() const
{
    QMutexLocker k(&_imp->nameMutex);

    return _imp->label;
}

void
Node::setScriptName_no_error_check(const std::string & name)
{
    setNameInternal(name, false);
}

static void
insertDependenciesRecursive(Node* node,
                            KnobDimViewKeySet* dependencies)
{
    const KnobsVec & knobs = node->getKnobs();

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobDimViewKeySet dimDeps;
        knobs[i]->getListeners(dimDeps, KnobI::eListenersTypeExpression);
        for (KnobDimViewKeySet::iterator it = dimDeps.begin(); it != dimDeps.end(); ++it) {
            dependencies->insert(*it);
        }
    }

    NodeGroupPtr isGroup = node->isEffectNodeGroup();
    if (isGroup) {
        NodesList nodes = isGroup->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            insertDependenciesRecursive(it->get(), dependencies);
        }
    }
}

void
Node::setNameInternal(const std::string& name,
                      bool throwErrors)
{
    std::string oldName = getScriptName_mt_safe();
    std::string fullOldName = getFullyQualifiedName();
    std::string newName = name;
    bool onlySpaces = true;

    for (std::size_t i = 0; i < name.size(); ++i) {
        if (name[i] != '_') {
            onlySpaces = false;
            break;
        }
    }
    if (onlySpaces) {
        QString err = tr("The name must at least contain a character");
        if (throwErrors) {
            throw std::runtime_error(err.toStdString());
        } else {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getFullyQualifiedName().c_str()), QDateTime::currentDateTime(), err, false, c);
            std::cerr << err.toStdString() << std::endl;

            return;
        }
    }

    NodeCollectionPtr collection = getGroup();
    if (collection) {
        if (throwErrors) {
            try {
                collection->checkNodeName(shared_from_this(), name, false, false, &newName);
            } catch (const std::exception& e) {
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getFullyQualifiedName().c_str()), QDateTime::currentDateTime(), QString::fromUtf8( e.what() ), false, c );
                std::cerr << e.what() << std::endl;

                return;
            }
        } else {
            collection->checkNodeName(shared_from_this(), name, false, false, &newName);
        }
    }


    if ( !newName.empty() ) {
        bool isAttrDefined = false;
        std::string newPotentialQualifiedName = getApp()->getAppIDString() + "." + getFullyQualifiedNameInternal(newName);
        PyObject* obj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(newPotentialQualifiedName, appPTR->getMainModule(), &isAttrDefined);
        Q_UNUSED(obj);
        if (isAttrDefined) {
            std::stringstream ss;
            ss << "A Python attribute with the same name (" << newPotentialQualifiedName << ") already exists.";
            if (throwErrors) {
                throw std::runtime_error( ss.str() );
            } else {
                std::string err = ss.str();
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(oldName.c_str()), QDateTime::currentDateTime(), QString::fromUtf8( err.c_str() ), false, c );
                std::cerr << err << std::endl;

                return;
            }
        }
    }
    QString oldLabel;
    bool labelSet = false;
    {
        QMutexLocker l(&_imp->nameMutex);
        _imp->scriptName = newName;
        oldLabel = QString::fromUtf8(_imp->label.c_str());
        ///Set the label at the same time if the label is empty
        if ( _imp->label.empty() ) {
            _imp->label = newName;
            labelSet = true;
        }
    }
    std::string fullySpecifiedName = getFullyQualifiedName();

    if (collection) {
        if ( !oldName.empty() ) {
            if (fullOldName != fullySpecifiedName) {
                try {
                    setNodeVariableToPython(fullOldName, fullySpecifiedName);
                } catch (const std::exception& e) {
                    qDebug() << e.what();
                }
            }
        } else { //if (!oldName.empty()) {
            declareNodeVariableToPython(fullySpecifiedName);
        }

        if (_imp->nodeCreated) {
            ///For all knobs that have listeners, change in the expressions of listeners this knob script-name
            KnobDimViewKeySet dependencies;
            insertDependenciesRecursive(this, &dependencies);
            for (KnobDimViewKeySet::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
                KnobIPtr listener = it->knob.lock();
                if (!listener) {
                    continue;
                }
                listener->replaceNodeNameInExpression(it->dimension, it->view, oldName, newName);

            }
        }
    }

    QString qnewName = QString::fromUtf8( newName.c_str() );
    Q_EMIT scriptNameChanged(qnewName);
    if (labelSet) {
        Q_EMIT labelChanged(oldLabel, qnewName);
    }
} // Node::setNameInternal


void
Node::setScriptName(const std::string& name)
{
    std::string newName;

    if ( getGroup() ) {
        getGroup()->checkNodeName(shared_from_this(), name, false, true, &newName);
    } else {
        newName = name;
    }
    //We do not allow setting the script-name of output nodes because we rely on it with NatronRenderer
    if ( isEffectGroupOutput() ) {
        throw std::runtime_error( tr("Changing the script-name of an Output node is not a valid operation.").toStdString() );
        
        return;
    }
    
    
    setNameInternal(newName, true);
}



NATRON_NAMESPACE_EXIT
