
/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "NodePrivate.h"

#include <sstream> // stringstream

#include <QDebug>

NATRON_NAMESPACE_ENTER


static bool runAndCheckRet(const std::string& script)
{
    std::string err;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        return false;
    }

    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
    PyObject* retObj = PyObject_GetAttrString(mainModule, "ret"); //new ref
    assert(retObj);
    bool ret = PyObject_IsTrue(retObj) == 1;
    Py_XDECREF(retObj);
    return ret;
}

static bool checkFunctionPresence(const std::string& pluginID, const std::string& functionName, bool prefixed)
{
    if (prefixed) {
        static const QString script = QString::fromUtf8("import inspect\n"
                                                        "import %1\n"
                                                        "ret = True\n"
                                                        "if not hasattr(%1,\"%2\") or not inspect.isfunction(%1.%2):\n"
                                                        "    ret = False\n");
        std::string toRun = script.arg(QString::fromUtf8(pluginID.c_str())).arg(QString::fromUtf8(functionName.c_str())).toStdString();
        return runAndCheckRet(toRun);
    } else {
        static const QString script = QString::fromUtf8("ret = True\n"
                                                        "try:\n"
                                                        "    %1\n"
                                                        "except NameError:\n"
                                                        "    ret = False\n");
        std::string toRun = script.arg(QString::fromUtf8(functionName.c_str())).toStdString();
        return runAndCheckRet(toRun);

    }

}

bool
NodePrivate::figureOutCallbackName(const std::string& inCallback, std::string* outCallback)
{
    if (inCallback.empty()) {
        return false;
    }
    // Python callbacks may be in a python script with indicated by the plug-in
    // check if it exists
    std::string extScriptFile = plugin.lock()->getPropertyUnsafe<std::string>(kNatronPluginPropPyPlugExtScriptFile);
    std::string moduleName;
    if (!extScriptFile.empty()) {
        std::size_t foundDot = extScriptFile.find_last_of(".");
        if (foundDot != std::string::npos) {
            moduleName = extScriptFile.substr(0, foundDot);
        }
    }

    bool gotFunc = false;
    if (!moduleName.empty() && checkFunctionPresence(moduleName, inCallback, true)) {
        *outCallback = moduleName + "." + inCallback;
        gotFunc = true;
    }

    if (!gotFunc && checkFunctionPresence(moduleName, inCallback, false)) {
        *outCallback = inCallback;
        gotFunc = true;
    }

    if (!gotFunc) {
        _publicInterface->getApp()->appendToScriptEditor(tr("Failed to run callback: %1 does not seem to be defined").arg(QString::fromUtf8(inCallback.c_str())).toStdString());
        return false;
    }
    return true;
}


void
NodePrivate::runOnNodeCreatedCBInternal(const std::string& cb,
                                        bool userEdited)
{
    std::vector<std::string> args;
    std::string error;
    if (_publicInterface->getScriptName_mt_safe().empty()) {
        return;
    }

    std::string callbackFunction;
    if (!figureOutCallbackName(cb, &callbackFunction)) {
        return;
    }

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(callbackFunction, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onNodeCreated callback: ")
                                                         + e.what() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The on node created callback supports the following signature(s):\n");
    signatureError.append("- callback(thisNode,app,userEdited)");
    if (args.size() != 3) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + signatureError);

        return;
    }

    if ( (args[0] != "thisNode") || (args[1] != "app") || (args[2] != "userEdited") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::string scriptName = _publicInterface->getScriptName_mt_safe();
    if ( scriptName.empty() ) {
        return;
    }
    std::stringstream ss;
    ss << callbackFunction << "(" << appID << "." << _publicInterface->getFullyQualifiedName() << "," << appID << ",";
    if (userEdited) {
        ss << "True";
    } else {
        ss << "False";
    }
    ss << ")\n";
    std::string output;
    std::string script = ss.str();
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);
    } else if ( !output.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
} // Node::Implementation::runOnNodeCreatedCBInternal

void
NodePrivate::runOnNodeDeleteCBInternal(const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;

    std::string callbackFunction;
    if (!figureOutCallbackName(cb, &callbackFunction)) {
        return;
    }

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(callbackFunction, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onNodeDeletion callback: ")
                                                         + e.what() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The on node deletion callback supports the following signature(s):\n");
    signatureError.append("- callback(thisNode,app)");
    if (args.size() != 2) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + signatureError);

        return;
    }

    if ( (args[0] != "thisNode") || (args[1] != "app") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::stringstream ss;
    ss << callbackFunction << "(" << appID << "." << _publicInterface->getFullyQualifiedName() << "," << appID << ")\n";

    std::string err;
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &err, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + err);
    } else if ( !output.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
}

void
NodePrivate::runOnNodeDeleteCB()
{

    if (_publicInterface->getScriptName_mt_safe().empty()) {
        return;
    }
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeDeleteCB();
    NodeCollectionPtr group = _publicInterface->getGroup();

    if (!group) {
        return;
    }

    std::string callbackFunction;
    if (figureOutCallbackName(cb, &callbackFunction)) {
        runOnNodeDeleteCBInternal(callbackFunction);
    }




    // If this is a group, run the node deleted callback on itself
    {
        cb = effect->getBeforeNodeRemovalCallback();
        if (!cb.empty()) {
            if (figureOutCallbackName(cb, &callbackFunction)) {
                runOnNodeDeleteCBInternal(callbackFunction);
            }

        }
    }

    // if there's a parent group, run the node deletec callback on the parent
    NodeGroupPtr isParentGroup = toNodeGroup(group);
    if (isParentGroup) {
        NodePtr grpNode = isParentGroup->getNode();
        if (grpNode) {
            cb = isParentGroup->getBeforeNodeRemovalCallback();
            if (figureOutCallbackName(cb, &callbackFunction)) {
                runOnNodeDeleteCBInternal(callbackFunction);
            }
        }
    }
}


void
NodePrivate::runOnNodeCreatedCB(bool userEdited)
{

    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeCreatedCB();
    NodeCollectionPtr group = _publicInterface->getGroup();

    if (!group) {
        return;
    }
    std::string callbackFunction;
    if (figureOutCallbackName(cb, &callbackFunction)) {
        runOnNodeCreatedCBInternal(callbackFunction, userEdited);
    }

    // If this is a group, run the node created callback on itself
    cb = effect->getAfterNodeCreatedCallback();
    if (!cb.empty()) {
        if (figureOutCallbackName(cb, &callbackFunction)) {
            runOnNodeCreatedCBInternal(callbackFunction, userEdited);
        }

    }

    // if there's a parent group, run the node created callback on the parent
    NodeGroupPtr isParentGroup = toNodeGroup(group);
    if (isParentGroup) {
        NodePtr grpNode = isParentGroup->getNode();
        if (grpNode) {
            cb = isParentGroup->getAfterNodeCreatedCallback();
            if (figureOutCallbackName(cb, &callbackFunction)) {
                runOnNodeCreatedCBInternal(callbackFunction, userEdited);
            }
        }
    }

}


void
NodePrivate::runInputChangedCallback(int index,
                                     const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;

    std::string callbackFunction;
    if (!figureOutCallbackName(cb, &callbackFunction)) {
        return;
    }

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(callbackFunction, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onInputChanged callback: ")
                                                         + e.what() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The on input changed callback supports the following signature(s):\n");
    signatureError.append("- callback(inputIndex,thisNode,thisGroup,app)");
    if (args.size() != 4) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + signatureError);

        return;
    }

    if ( (args[0] != "inputIndex") || (args[1] != "thisNode") || (args[2] != "thisGroup") || (args[3] != "app") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    NodeCollectionPtr collection = _publicInterface->getGroup();
    assert(collection);
    if (!collection) {
        return;
    }

    std::string thisGroupVar;
    NodeGroupPtr isParentGrp = toNodeGroup(collection);
    if (isParentGrp) {
        std::string nodeName = isParentGrp->getNode()->getFullyQualifiedName();
        std::string nodeFullName = appID + "." + nodeName;
        thisGroupVar = nodeFullName;
    } else {
        thisGroupVar = appID;
    }

    std::stringstream ss;
    ss << callbackFunction << "(" << index << "," << appID << "." << _publicInterface->getFullyQualifiedName() << "," << thisGroupVar << "," << appID << ")\n";

    std::string script = ss.str();
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to execute callback: %1").arg( QString::fromUtf8( error.c_str() ) ).toStdString() );
    } else {
        if ( !output.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }
} //runInputChangedCallback

void
NodePrivate::runChangedParamCallback(const std::string& cb, const KnobIPtr& k, bool userEdited)
{
    std::vector<std::string> args;
    std::string error;

    if ( !k || (k->getName() == "onParamChanged") ) {
        return;
    }

    std::string callbackFunction;
    if (!figureOutCallbackName(cb, &callbackFunction)) {
        return;
    }

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(callbackFunction, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( e.what() ) ).toStdString() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( error.c_str() ) ).toStdString() );

        return;
    }

    std::string signatureError;
    signatureError.append( tr("The param changed callback supports the following signature(s):").toStdString() );
    signatureError.append("\n- callback(thisParam,thisNode,thisGroup,app,userEdited)");
    if (args.size() != 5) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( signatureError.c_str() ) ).toStdString() );

        return;
    }

    if ( ( (args[0] != "thisParam") || (args[1] != "thisNode") || (args[2] != "thisGroup") || (args[3] != "app") || (args[4] != "userEdited") ) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( signatureError.c_str() ) ).toStdString() );

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();

    assert(k);
    std::string thisNodeVar = appID + ".";
    thisNodeVar.append( _publicInterface->getFullyQualifiedName() );

    NodeCollectionPtr collection = _publicInterface->getGroup();
    assert(collection);
    if (!collection) {
        return;
    }

    std::string thisGroupVar;
    NodeGroupPtr isParentGrp = toNodeGroup(collection);
    if (isParentGrp) {
        std::string nodeName = isParentGrp->getNode()->getFullyQualifiedName();
        std::string nodeFullName = appID + "." + nodeName;
        thisGroupVar = nodeFullName;
    } else {
        thisGroupVar = appID;
    }

    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(thisNodeVar, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    if (!nodeObj || !alreadyDefined) {
        return;
    }

    if (!PyObject_HasAttrString( nodeObj, k->getName().c_str() ) ) {
        return;
    }

    std::stringstream ss;
    ss << callbackFunction << "(" << thisNodeVar << "." << k->getName() << "," << thisNodeVar << "," << thisGroupVar << "," << appID
    << ",";
    if (userEdited) {
        ss << "True";
    } else {
        ss << "False";
    }
    ss << ")\n";

    std::string script = ss.str();
    std::string err;
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to execute onParamChanged callback: %1").arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
    } else {
        if ( !output.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }

} // runChangedParamCallback

void
NodePrivate::runAfterItemsSelectionChangedCallback(const std::string& cb, const KnobItemsTablePtr& table, const std::list<KnobTableItemPtr>& deselected, const std::list<KnobTableItemPtr>& selected, TableChangeReasonEnum reason)
{
    std::vector<std::string> args;
    std::string error;

    std::string callbackFunction;
    if (!figureOutCallbackName(cb, &callbackFunction)) {
        return;
    }
    if (!table) {
        return;
    }

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(callbackFunction, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run afterItemsSelectionChanged callback: %1").arg( QString::fromUtf8( e.what() ) ).toStdString() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run afterItemsSelectionChanged callback: %1").arg( QString::fromUtf8( error.c_str() ) ).toStdString() );

        return;
    }

    std::string signatureError;
    signatureError.append( tr("The after items selection changed callback supports the following signature(s):").toStdString() );
    signatureError.append("\n- callback(thisNode, thisTable, app, deselected, selected, reason)");
    if (args.size() != 6) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run afterItemsSelectionChanged callback: %1").arg( QString::fromUtf8( signatureError.c_str() ) ).toStdString() );

        return;
    }

    if ( ( (args[0] != "thisNode") || (args[1] != "thisTable") || (args[2] != "app") || (args[3] != "deselected") || (args[4] != "selected") || (args[5] != "reason") ) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run afterItemsSelectionChanged callback: %1").arg( QString::fromUtf8( signatureError.c_str() ) ).toStdString() );

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();

    std::string thisNodeVar = appID + ".";
    thisNodeVar.append( _publicInterface->getFullyQualifiedName() );

    std::string tableVar = thisNodeVar + "." + table->getPythonPrefix();

    // Check thisNode exists
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(thisNodeVar, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    if (!nodeObj || !alreadyDefined) {
        return;
    }

    std::stringstream ss;
    ss << "deselectedItemsSequenceArg = []\n";
    ss << "selectedItemsSequenceArg = []\n";
    for (std::list<KnobTableItemPtr>::const_iterator it = deselected.begin(); it != deselected.end(); ++it) {
        ss << "itemDeselected = " << tableVar << ".getItemByFullyQualifiedScriptName(\"" << (*it)->getFullyQualifiedName() << "\")\n";
        ss << "if itemDeselected is not None:\n";
        ss << "    deselectedItemsSequenceArg.append(itemDeselected)\n";
    }
    for (std::list<KnobTableItemPtr>::const_iterator it = selected.begin(); it != selected.end(); ++it) {
        ss << "itemSelected = " << tableVar << ".getItemByFullyQualifiedScriptName(\"" << (*it)->getFullyQualifiedName() << "\")\n";
        ss << "if itemSelected is not None:\n";
        ss << "    selectedItemsSequenceArg.append(itemSelected)\n";
    }

    ss << callbackFunction << "(" << thisNodeVar << ", " << tableVar << ", " << appID << ", deselectedItemsSequenceArg, selectedItemsSequenceArg, NatronEngine.Natron.TableChangeReasonEnum.";
    switch (reason) {
        case eTableChangeReasonInternal:
            ss << "eTableChangeReasonInternal";
            break;
        case eTableChangeReasonPanel:
            ss << "eTableChangeReasonPanel";
            break;
        case eTableChangeReasonViewer:
            ss << "eTableChangeReasonViewer";
            break;
    }
    ss << ")\n";
    ss << "del deselectedItemsSequenceArg\n";
    ss << "del selectedItemsSequenceArg\n";

    std::string script = ss.str();
    std::string err;
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to execute afterItemsSelectionChanged callback: %1").arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
    } else {
        if ( !output.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }
    
} // runAfterItemsSelectionChangedCallback


void
Node::runAfterTableItemsSelectionChangedCallback(const KnobItemsTablePtr& table, const std::list<KnobTableItemPtr>& deselected, const std::list<KnobTableItemPtr>& selected, TableChangeReasonEnum reason)
{
    std::string s = _imp->effect->getAfterSelectionChangedCallback();
    if (s.empty()) {
        return;
    }
    _imp->runAfterItemsSelectionChangedCallback(s, table, deselected, selected, reason);
}


void
Node::runChangedParamCallback(const KnobIPtr& k, bool userEdited)
{
    std::string cb = _imp->effect->getKnobChangedCallback();
    if (!cb.empty()) {
        _imp->runChangedParamCallback(cb, k, userEdited);
    }
}



void
Node::runInputChangedCallback(int index)
{
    std::string cb = _imp->effect->getInputChangedCallback();

    if ( !cb.empty() ) {
        _imp->runInputChangedCallback(index, cb);
    }
}



void
Node::declareNodeVariableToPython(const std::string& nodeName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }


    PythonGILLocker pgl;
    PyObject* mainModule = appPTR->getMainModule();
    assert(mainModule);

    std::string appID = getApp()->getAppIDString();
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, mainModule, &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);

    if (!alreadyDefined) {
        std::stringstream ss;
        ss << nodeFullName << " = " << appID << ".getNode(\"" << nodeName << "\")\n";
#ifdef DEBUG
        ss << "if not " << nodeFullName << ":\n";
        ss << "    print \"[BUG]: " << nodeFullName << " does not exist!\"";
#endif
        std::string script = ss.str();
        std::string output;
        std::string err;
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::setNodeVariableToPython(const std::string& oldName,
                              const std::string& newName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    QString appID = QString::fromUtf8( getApp()->getAppIDString().c_str() );
    QString str = QString( appID + QString::fromUtf8(".%1 = ") + appID + QString::fromUtf8(".%2\ndel ") + appID + QString::fromUtf8(".%2\n") ).arg( QString::fromUtf8( newName.c_str() ) ).arg( QString::fromUtf8( oldName.c_str() ) );
    std::string script = str.toStdString();
    std::string err;
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        qDebug() << err.c_str();
    }
}

void
Node::deleteNodeVariableToPython(const std::string& nodeName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }

    AppInstancePtr app = getApp();
    if (!app) {
        return;
    }
    QString appID = QString::fromUtf8( getApp()->getAppIDString().c_str() );
    std::string nodeFullName = appID.toStdString() + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, appPTR->getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (alreadyDefined) {
        std::string script = "del " + nodeFullName;
        std::string err;
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::declarePythonKnobs()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    PythonGILLocker pgl;

    if ( !getGroup() ) {
        return;
    }

    std::locale locale;
    std::string nodeName;
    if (getIOContainer()) {
        nodeName = getIOContainer()->getFullyQualifiedName();
    } else {
        nodeName = getFullyQualifiedName();
    }
    std::string appID = getApp()->getAppIDString();
    bool alreadyDefined = false;
    std::string nodeFullName = appID + "." + nodeName;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString::fromUtf8("declarePythonKnobs(): attribute ") + QString::fromUtf8( nodeFullName.c_str() ) + QString::fromUtf8(" is not defined");
        throw std::logic_error(std::string("declarePythonKnobs(): attribute ") + nodeFullName + " is not defined");
    }


    std::stringstream ss;
#ifdef DEBUG
    ss << "if not " << nodeFullName << ":\n";
    ss << "    print \"[BUG]: " << nodeFullName << " is not defined!\"\n";
#endif
    const KnobsVec& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        const std::string& knobName = knobs[i]->getName();
        if ( !knobName.empty() && (knobName.find(" ") == std::string::npos) && !std::isdigit(knobName[0], locale) ) {
            if ( PyObject_HasAttrString( nodeObj, knobName.c_str() ) ) {
                continue;
            }
            ss << nodeFullName <<  "." << knobName << " = ";
            ss << nodeFullName << ".getParam(\"" << knobName << "\")\n";
        }
    }

    std::string script = ss.str();
    if ( !script.empty() ) {
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            qDebug() << err.c_str();
        }
    }
} // Node::declarePythonFields

void
Node::removeParameterFromPython(const std::string& parameterName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    PythonGILLocker pgl;
    std::string appID = getApp()->getAppIDString();
    std::string nodeName;
    if (getIOContainer()) {
        nodeName = getIOContainer()->getFullyQualifiedName();
    } else {
        nodeName = getFullyQualifiedName();
    }
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString::fromUtf8("removeParameterFromPython(): attribute ") + QString::fromUtf8( nodeFullName.c_str() ) + QString::fromUtf8(" is not defined");
        throw std::logic_error(std::string("removeParameterFromPython(): attribute ") + nodeFullName + " is not defined");
    }
    assert( PyObject_HasAttrString( nodeObj, parameterName.c_str() ) );
    std::string script = "del " + nodeFullName + "." + parameterName;
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    std::string err;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        qDebug() << err.c_str();
    }
}

void
Node::declareTablePythonFields()
{
    std::list<KnobItemsTablePtr> tables = _imp->effect->getAllItemsTables();

    if (getScriptName_mt_safe().empty()) {
        return;
    }
    for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it!=tables.end(); ++it) {
        (*it)->declareItemsToPython();
    }
}


void
Node::declareAllPythonAttributes()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    try {
        declareNodeVariableToPython( getFullyQualifiedName() );
        declarePythonKnobs();
        declareTablePythonFields();
    } catch (const std::exception& e) {
        qDebug() << e.what();
    }
}

NATRON_NAMESPACE_EXIT
