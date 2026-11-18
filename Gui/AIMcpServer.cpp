/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "AIMcpServer.h"

#include <cassert>
#include <map>
#include <string>
#include <vector>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QThread>
#include <QtCore/QUuid>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QUndoStack>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/ChoiceOption.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"

#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"

NATRON_NAMESPACE_ENTER

// MCP protocol revision we advertise in the initialize handshake.
#define AI_MCP_PROTOCOL_VERSION "2025-06-18"

// JSON-RPC 2.0 reserved error codes.
#define AI_JSONRPC_PARSE_ERROR      (-32700)
#define AI_JSONRPC_INVALID_REQUEST  (-32600)
#define AI_JSONRPC_METHOD_NOT_FOUND (-32601)
#define AI_JSONRPC_INVALID_PARAMS   (-32602)
#define AI_JSONRPC_INTERNAL_ERROR   (-32603)

// Refuse absurd bodies rather than buffering without bound.
#define AI_MCP_MAX_REQUEST_BYTES (4 * 1024 * 1024)

namespace {
/**
 * @brief A domain-level failure. Carries the structured shape the agent sees:
 * code / message / hint. Never escapes as a JSON-RPC protocol error -- it is
 * reported as an MCP tool result with isError:true, so the model can read it
 * and recover.
 */
struct ToolError
{
    QString code;
    QString message;
    QString hint;

    ToolError(const QString& c,
              const QString& m,
              const QString& h = QString())
        : code(c), message(m), hint(h) {}
};
} // anonymous namespace

struct AIMcpServerPrivate
{
    AIMcpServer* _publicInterface;
    Gui* gui;
    QTcpServer* server;
    QString token;
    std::map<QTcpSocket*, QByteArray> buffers;

    // Undo transaction state. txStack is remembered rather than re-resolved so
    // that endMacro() always lands on the very stack beginMacro() was called on,
    // even if the user switches graph mid-turn.
    int txDepth;
    QUndoStack* txStack;

    AIMcpServerPrivate(AIMcpServer* publicInterface,
                       Gui* g)
        : _publicInterface(publicInterface)
        , gui(g)
        , server(0)
        , token()
        , buffers()
        , txDepth(0)
        , txStack(0)
    {
    }

    /// The undo stack agent mutations should be grouped on, or NULL.
    QUndoStack* agentUndoStack() const;

    /// True for tools that mutate the graph and therefore need a transaction.
    static bool toolMutates(const QString& toolName);

    /// Runs one request, hopping to the GUI thread when necessary.
    QString dispatch(const QString& requestJson);

    QJsonObject handleRequest(const QJsonObject& request);
    QJsonObject handleToolCall(const QString& toolName,
                               const QJsonObject& args);
    QJsonObject toolsList() const;

    // --- tools -------------------------------------------------------------
    QJsonObject toolNatronStatus();
    QJsonObject toolGraphListNodes();
    QJsonObject toolNodeCreate(const QJsonObject& a);
    QJsonObject toolNodeConnect(const QJsonObject& a);
    QJsonObject toolNodeDelete(const QJsonObject& a);
    QJsonObject toolParamGet(const QJsonObject& a);
    QJsonObject toolParamSet(const QJsonObject& a);
    QJsonObject toolParamSetExpression(const QJsonObject& a);
    QJsonObject toolParamSetKeyframe(const QJsonObject& a);
    QJsonObject toolParamClearAnimation(const QJsonObject& a);
    QJsonObject toolNodeSetPosition(const QJsonObject& a);
    QJsonObject toolNodeRename(const QJsonObject& a);
    QJsonObject toolPluginSearch(const QJsonObject& a);
    QJsonObject toolScriptExec(const QJsonObject& a);
    QJsonObject toolRenderStart(const QJsonObject& a);
    QJsonObject toolRenderStatus(const QJsonObject& a);

    // --- helpers -----------------------------------------------------------
    AppInstancePtr app() const;
    ProjectPtr project() const;

    /// Resolves a node by script name. Throws ToolError with close matches.
    NodePtr requireNode(const QString& scriptName) const;

    /// Resolves a knob by name on a node. Throws ToolError.
    KnobIPtr requireKnob(const NodePtr& node,
                         const QString& paramName) const;

    static QJsonObject describeNode(const NodePtr& node);
    static QJsonObject readKnob(const KnobIPtr& knob);
    static void writeKnob(const KnobIPtr& knob,
                          const QJsonValue& value,
                          int dimension);

    void sendHttpResponse(QTcpSocket* socket,
                          int status,
                          const QByteArray& body);
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QUndoStack*
AIMcpServerPrivate::agentUndoStack() const
{
    if (!gui) {
        return 0;
    }

    NodeGraph* graph = gui->getNodeGraph();

    return graph ? graph->getUndoStack() : 0;
}

bool
AIMcpServerPrivate::toolMutates(const QString& toolName)
{
    return ( toolName == QString::fromUtf8("node_create") ) ||
           ( toolName == QString::fromUtf8("node_connect") ) ||
           ( toolName == QString::fromUtf8("node_delete") ) ||
           ( toolName == QString::fromUtf8("node_rename") ) ||
           ( toolName == QString::fromUtf8("node_set_position") ) ||
           ( toolName == QString::fromUtf8("param_set") ) ||
           ( toolName == QString::fromUtf8("param_set_expression") ) ||
           ( toolName == QString::fromUtf8("param_set_keyframe") ) ||
           ( toolName == QString::fromUtf8("param_clear_animation") ) ||
           ( toolName == QString::fromUtf8("script_exec") ) ||
           ( toolName == QString::fromUtf8("render_start") );
}

AppInstancePtr
AIMcpServerPrivate::app() const
{
    if (!gui) {
        return AppInstancePtr();
    }

    return std::dynamic_pointer_cast<AppInstance>( gui->getApp() );
}

ProjectPtr
AIMcpServerPrivate::project() const
{
    AppInstancePtr a = app();

    if (!a) {
        return ProjectPtr();
    }

    return a->getProject();
}

NodePtr
AIMcpServerPrivate::requireNode(const QString& scriptName) const
{
    ProjectPtr proj = project();

    if (!proj) {
        throw ToolError( QString::fromUtf8("NO_PROJECT"),
                         QString::fromUtf8("No project is open"),
                         QString::fromUtf8("Open or create a project first.") );
    }

    const std::string wanted = scriptName.toStdString();
    NodesList nodes = proj->getNodes();

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it) && ( (*it)->getScriptName_mt_safe() == wanted ) ) {
            return *it;
        }
    }

    // Build a short "did you mean" list rather than a bare failure.
    QStringList available;
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it) {
            available.push_back( QString::fromUtf8( (*it)->getScriptName_mt_safe().c_str() ) );
        }
        if (available.size() >= 12) {
            break;
        }
    }

    throw ToolError( QString::fromUtf8("NODE_NOT_FOUND"),
                     QString::fromUtf8("No node named '%1' in the current project").arg(scriptName),
                     QString::fromUtf8("Call graph_list_nodes to see available nodes. Present: %1")
                     .arg( available.join( QString::fromUtf8(", ") ) ) );
}

KnobIPtr
AIMcpServerPrivate::requireKnob(const NodePtr& node,
                                const QString& paramName) const
{
    KnobIPtr knob = node->getKnobByName( paramName.toStdString() );

    if (knob) {
        return knob;
    }

    QStringList available;
    const KnobsVec& knobs = node->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (*it) {
            available.push_back( QString::fromUtf8( (*it)->getName().c_str() ) );
        }
        if (available.size() >= 20) {
            break;
        }
    }

    throw ToolError( QString::fromUtf8("PARAM_NOT_FOUND"),
                     QString::fromUtf8("Node '%1' has no parameter '%2'")
                     .arg( QString::fromUtf8( node->getScriptName_mt_safe().c_str() ) )
                     .arg(paramName),
                     QString::fromUtf8("Available: %1").arg( available.join( QString::fromUtf8(", ") ) ) );
}

QJsonObject
AIMcpServerPrivate::describeNode(const NodePtr& node)
{
    QJsonObject o;

    o[QString::fromUtf8("scriptName")] = QString::fromUtf8( node->getScriptName_mt_safe().c_str() );
    o[QString::fromUtf8("label")] = QString::fromUtf8( node->getLabel_mt_safe().c_str() );
    o[QString::fromUtf8("pluginID")] = QString::fromUtf8( node->getPluginID().c_str() );

    QJsonArray inputs;
    const int n = node->getNInputs();
    for (int i = 0; i < n; ++i) {
        QJsonObject in;
        in[QString::fromUtf8("index")] = i;
        in[QString::fromUtf8("label")] = QString::fromUtf8( node->getInputLabel(i).c_str() );
        NodePtr src = node->getInput(i);
        if (src) {
            in[QString::fromUtf8("source")] = QString::fromUtf8( src->getScriptName_mt_safe().c_str() );
        } else {
            in[QString::fromUtf8("source")] = QJsonValue();
        }
        inputs.push_back(in);
    }
    o[QString::fromUtf8("inputs")] = inputs;

    return o;
}

QJsonObject
AIMcpServerPrivate::readKnob(const KnobIPtr& knob)
{
    QJsonObject o;
    const std::string type = knob->typeName();

    o[QString::fromUtf8("scriptName")] = QString::fromUtf8( knob->getName().c_str() );
    o[QString::fromUtf8("label")] = QString::fromUtf8( knob->getLabel().c_str() );
    o[QString::fromUtf8("type")] = QString::fromUtf8( type.c_str() );

    const int dims = knob->getDimension();
    o[QString::fromUtf8("dimensions")] = dims;

    QJsonArray values;
    KnobIntBasePtr asInt = std::dynamic_pointer_cast<KnobIntBase>(knob);
    KnobDoubleBasePtr asDouble = std::dynamic_pointer_cast<KnobDoubleBase>(knob);
    KnobBoolBasePtr asBool = std::dynamic_pointer_cast<KnobBoolBase>(knob);
    KnobStringBasePtr asString = std::dynamic_pointer_cast<KnobStringBase>(knob);

    for (int i = 0; i < dims; ++i) {
        if (asInt) {
            values.push_back( asInt->getValue(i) );
        } else if (asDouble) {
            values.push_back( asDouble->getValue(i) );
        } else if (asBool) {
            values.push_back( asBool->getValue(i) );
        } else if (asString) {
            values.push_back( QString::fromUtf8( asString->getValue(i).c_str() ) );
        } else {
            values.push_back( QJsonValue() );
        }
    }

    // Fold single-dimension params to a scalar; the agent should not have to
    // unwrap a one-element array for every ordinary parameter.
    if (dims == 1) {
        o[QString::fromUtf8("value")] = values.at(0);
    } else {
        o[QString::fromUtf8("value")] = values;
    }

    // A Choice's options are what the agent must pick from, so surface them.
    KnobChoicePtr asChoice = std::dynamic_pointer_cast<KnobChoice>(knob);
    if (asChoice) {
        // A ChoiceOption carries id / label / tooltip. The *id* is what
        // identifies the option internally, so that is what param_set expects;
        // the label is only what the GUI shows.
        QJsonArray options;
        std::vector<ChoiceOption> entries = asChoice->getEntries_mt_safe();
        for (std::size_t i = 0; i < entries.size(); ++i) {
            QJsonObject opt;
            opt[QString::fromUtf8("id")] = QString::fromUtf8( entries[i].id.c_str() );
            opt[QString::fromUtf8("label")] = QString::fromUtf8( entries[i].label.empty()
                                                                 ? entries[i].id.c_str()
                                                                 : entries[i].label.c_str() );
            options.push_back(opt);
        }
        o[QString::fromUtf8("options")] = options;
    }

    return o;
}

void
AIMcpServerPrivate::writeKnob(const KnobIPtr& knob,
                              const QJsonValue& value,
                              int dimension)
{
    const int dims = knob->getDimension();

    if ( (dimension < 0) || (dimension >= dims) ) {
        throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                         QString::fromUtf8("Dimension %1 is out of range for '%2' (%3 dimension(s))")
                         .arg(dimension)
                         .arg( QString::fromUtf8( knob->getName().c_str() ) )
                         .arg(dims),
                         QString::fromUtf8("Valid dimensions are 0 to %1.").arg(dims - 1) );
    }

    const QString name = QString::fromUtf8( knob->getName().c_str() );

    // A Choice is an int knob underneath, but the agent will usually send the
    // option label. Resolve it here so a bad label is a real error rather than
    // a silently ignored write.
    KnobChoicePtr asChoice = std::dynamic_pointer_cast<KnobChoice>(knob);
    if (asChoice) {
        std::vector<ChoiceOption> entries = asChoice->getEntries_mt_safe();
        int index = -1;

        if ( value.isDouble() ) {
            index = (int)value.toDouble();
            if ( ( index < 0 ) || ( index >= (int)entries.size() ) ) {
                throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                                 QString::fromUtf8("Index %1 is out of range for choice '%2' (%3 options)")
                                 .arg(index).arg(name).arg( (int)entries.size() ),
                                 QString::fromUtf8("Call param_get to list the options.") );
            }
        } else if ( value.isString() ) {
            // Match the id first (that is the canonical identifier), then fall
            // back to the display label. Resolving here rather than calling
            // KnobChoice's string setter means a bad option is a real error
            // instead of a silently ignored write.
            const QString wanted = value.toString();
            for (std::size_t i = 0; i < entries.size(); ++i) {
                const QString id = QString::fromUtf8( entries[i].id.c_str() );
                if ( id.compare(wanted, Qt::CaseInsensitive) == 0 ) {
                    index = (int)i;
                    break;
                }
            }
            if (index < 0) {
                for (std::size_t i = 0; i < entries.size(); ++i) {
                    const QString label = QString::fromUtf8( entries[i].label.c_str() );
                    if ( !label.isEmpty() && ( label.compare(wanted, Qt::CaseInsensitive) == 0 ) ) {
                        index = (int)i;
                        break;
                    }
                }
            }
            if (index < 0) {
                QStringList opts;
                for (std::size_t i = 0; i < entries.size(); ++i) {
                    opts.push_back( QString::fromUtf8( entries[i].id.c_str() ) );
                }
                throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                                 QString::fromUtf8("'%1' is not a valid option for '%2'").arg(wanted).arg(name),
                                 QString::fromUtf8("Valid options: %1").arg( opts.join( QString::fromUtf8(", ") ) ) );
            }
        } else {
            throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                             QString::fromUtf8("Choice '%1' takes an option label or an index").arg(name) );
        }

        asChoice->setValue(index, ViewSpec::all(), dimension);

        return;
    }

    KnobIntBasePtr asInt = std::dynamic_pointer_cast<KnobIntBase>(knob);
    if (asInt) {
        if ( !value.isDouble() ) {
            throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                             QString::fromUtf8("Parameter '%1' is an Int; expected a number").arg(name) );
        }
        asInt->setValue( (int)value.toDouble(), ViewSpec::all(), dimension );

        return;
    }

    KnobBoolBasePtr asBool = std::dynamic_pointer_cast<KnobBoolBase>(knob);
    if (asBool) {
        if ( !value.isBool() ) {
            throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                             QString::fromUtf8("Parameter '%1' is a Bool; expected true or false").arg(name) );
        }
        asBool->setValue( value.toBool(), ViewSpec::all(), dimension );

        return;
    }

    KnobDoubleBasePtr asDouble = std::dynamic_pointer_cast<KnobDoubleBase>(knob);
    if (asDouble) {
        if ( !value.isDouble() ) {
            throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                             QString::fromUtf8("Parameter '%1' is a Double; expected a number").arg(name) );
        }
        asDouble->setValue( value.toDouble(), ViewSpec::all(), dimension );

        return;
    }

    KnobStringBasePtr asString = std::dynamic_pointer_cast<KnobStringBase>(knob);
    if (asString) {
        if ( !value.isString() ) {
            throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                             QString::fromUtf8("Parameter '%1' is a String; expected a string").arg(name) );
        }
        asString->setValue( value.toString().toStdString(), ViewSpec::all(), dimension );

        return;
    }

    throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                     QString::fromUtf8("Parameter '%1' has type '%2', which cannot be set through this tool")
                     .arg(name).arg( QString::fromUtf8( knob->typeName().c_str() ) ),
                     QString::fromUtf8("Button, Page, Group and Parametric parameters hold no settable value.") );
}

// ---------------------------------------------------------------------------
// Tools
// ---------------------------------------------------------------------------

QJsonObject
AIMcpServerPrivate::toolNatronStatus()
{
    QJsonObject o;

    o[QString::fromUtf8("version")] = QString::fromUtf8(NATRON_VERSION_STRING);

    ProjectPtr proj = project();
    if (proj) {
        // Both already return QString.
        o[QString::fromUtf8("projectPath")] = proj->getProjectPath() + proj->getProjectFilename();
        o[QString::fromUtf8("nodeCount")] = (int)proj->getNodes().size();
        o[QString::fromUtf8("hasProject")] = true;
    } else {
        o[QString::fromUtf8("projectPath")] = QJsonValue();
        o[QString::fromUtf8("nodeCount")] = 0;
        o[QString::fromUtf8("hasProject")] = false;
    }

    // This server only ever runs inside the GUI process, which is exactly why
    // it can offer node positions and grouped undo.
    o[QString::fromUtf8("hasGui")] = true;

    return o;
}

QJsonObject
AIMcpServerPrivate::toolGraphListNodes()
{
    ProjectPtr proj = project();

    if (!proj) {
        throw ToolError( QString::fromUtf8("NO_PROJECT"),
                         QString::fromUtf8("No project is open"),
                         QString::fromUtf8("Open or create a project first.") );
    }

    QJsonArray arr;
    NodesList nodes = proj->getNodes();
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it) {
            arr.push_back( describeNode(*it) );
        }
    }

    QJsonObject o;
    o[QString::fromUtf8("nodes")] = arr;
    o[QString::fromUtf8("count")] = arr.size();

    return o;
}

QJsonObject
AIMcpServerPrivate::toolNodeCreate(const QJsonObject& a)
{
    AppInstancePtr instance = app();

    if (!instance) {
        throw ToolError( QString::fromUtf8("NO_PROJECT"),
                         QString::fromUtf8("No application instance is available") );
    }

    const QString pluginID = a[QString::fromUtf8("pluginID")].toString();
    if ( pluginID.isEmpty() ) {
        throw ToolError( QString::fromUtf8("PLUGIN_NOT_FOUND"),
                         QString::fromUtf8("pluginID is required"),
                         QString::fromUtf8("For example 'net.sf.openfx.GradePlugin'.") );
    }

    CreateNodeArgs args( pluginID.toStdString(), project() );

    // Never pop a settings panel or a dialog: this call is driven by an agent,
    // not by a click.
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
    // Let Natron push its own undo command; AIUndoTransaction groups it.
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, true);

    if ( a.contains( QString::fromUtf8("label") ) ) {
        const QString label = a[QString::fromUtf8("label")].toString();
        if ( !label.isEmpty() ) {
            args.setProperty<std::string>( kCreateNodeArgsPropNodeInitialName, label.toStdString() );
        }
    }

    if ( a.contains( QString::fromUtf8("x") ) && a.contains( QString::fromUtf8("y") ) ) {
        args.setProperty<double>( kCreateNodeArgsPropNodeInitialPosition, a[QString::fromUtf8("x")].toDouble(), 0 );
        args.setProperty<double>( kCreateNodeArgsPropNodeInitialPosition, a[QString::fromUtf8("y")].toDouble(), 1 );
    }

    // createNode returns NULL on failure rather than throwing.
    NodePtr node = instance->createNode(args);
    if (!node) {
        throw ToolError( QString::fromUtf8("PLUGIN_NOT_FOUND"),
                         QString::fromUtf8("Could not create a node for plugin '%1'").arg(pluginID),
                         QString::fromUtf8("Plugin IDs are exact and case-sensitive. Check natron_status for the plugin count, and verify the ID.") );
    }

    return describeNode(node);
}

QJsonObject
AIMcpServerPrivate::toolNodeConnect(const QJsonObject& a)
{
    const QString nodeName = a[QString::fromUtf8("node")].toString();
    const QString sourceName = a[QString::fromUtf8("source")].toString();
    const int inputIndex = (int)a[QString::fromUtf8("inputIndex")].toDouble();

    NodePtr node = requireNode(nodeName);
    NodePtr source = requireNode(sourceName);

    if ( ( inputIndex < 0 ) || ( inputIndex >= node->getNInputs() ) ) {
        throw ToolError( QString::fromUtf8("CONNECT_INVALID"),
                         QString::fromUtf8("Input index %1 is out of range for '%2', which has %3 input(s)")
                         .arg(inputIndex).arg(nodeName).arg( node->getNInputs() ),
                         QString::fromUtf8("Call graph_list_nodes to see each node's inputs.") );
    }

    // connectInput does not auto-disconnect: an occupied input makes it fail.
    if ( node->getInput(inputIndex) ) {
        node->disconnectInput(inputIndex);
    }

    if ( !node->connectInput(source, inputIndex) ) {
        throw ToolError( QString::fromUtf8("CONNECT_INVALID"),
                         QString::fromUtf8("'%1' cannot be connected to input %2 of '%3'")
                         .arg(sourceName).arg(inputIndex).arg(nodeName),
                         QString::fromUtf8("Usual causes: the connection would create a cycle, or the component types are incompatible.") );
    }

    return describeNode(node);
}

QJsonObject
AIMcpServerPrivate::toolNodeDelete(const QJsonObject& a)
{
    const QString nodeName = a[QString::fromUtf8("node")].toString();
    NodePtr node = requireNode(nodeName);

    // Destructive: give the panel a chance to raise a native approval dialog.
    bool allowed = true;
    Q_EMIT _publicInterface->destructiveToolRequested(
        QString::fromUtf8("node_delete"),
        QString::fromUtf8("Delete node '%1'").arg(nodeName),
        &allowed);

    if (!allowed) {
        throw ToolError( QString::fromUtf8("PERMISSION_DENIED"),
                         QString::fromUtf8("The user declined deletion of '%1'").arg(nodeName),
                         QString::fromUtf8("Ask the user what they would prefer before retrying.") );
    }

    node->destroyNode(false, false);

    QJsonObject o;
    o[QString::fromUtf8("deleted")] = nodeName;

    return o;
}

QJsonObject
AIMcpServerPrivate::toolParamGet(const QJsonObject& a)
{
    NodePtr node = requireNode( a[QString::fromUtf8("node")].toString() );
    KnobIPtr knob = requireKnob( node, a[QString::fromUtf8("param")].toString() );

    return readKnob(knob);
}

QJsonObject
AIMcpServerPrivate::toolParamSet(const QJsonObject& a)
{
    NodePtr node = requireNode( a[QString::fromUtf8("node")].toString() );
    KnobIPtr knob = requireKnob( node, a[QString::fromUtf8("param")].toString() );

    int dimension = 0;
    if ( a.contains( QString::fromUtf8("dimension") ) ) {
        dimension = (int)a[QString::fromUtf8("dimension")].toDouble();
    }

    writeKnob(knob, a[QString::fromUtf8("value")], dimension);

    // Read back: most Natron setters return void or an ignored status, so the
    // only honest confirmation is to re-read the knob.
    return readKnob(knob);
}

QJsonObject
AIMcpServerPrivate::toolPluginSearch(const QJsonObject& a)
{
    // Without this the agent has to guess plugin IDs, and a wrong guess is
    // indistinguishable from a broken tool. Everything installed is listed here.
    const QString query = a[QString::fromUtf8("query")].toString().trimmed();
    int limit = a.contains( QString::fromUtf8("limit") )
                ? (int)a[QString::fromUtf8("limit")].toDouble() : 40;

    if (limit <= 0) {
        limit = 40;
    }

    // AppManager::getPluginIDs() hands back std::list<std::string>.
    const std::list<std::string> all = appPTR->getPluginIDs();
    QStringList ids;
    for (std::list<std::string>::const_iterator it = all.begin(); it != all.end(); ++it) {
        const QString id = QString::fromUtf8( it->c_str() );
        if ( query.isEmpty() || id.contains(query, Qt::CaseInsensitive) ) {
            ids.push_back(id);
        }
    }
    ids.sort(Qt::CaseInsensitive);

    QJsonArray arr;
    for (int i = 0; i < ids.size() && i < limit; ++i) {
        arr.push_back( ids.at(i) );
    }

    QJsonObject o;
    o[QString::fromUtf8("plugins")] = arr;
    o[QString::fromUtf8("matched")] = ids.size();
    o[QString::fromUtf8("returned")] = arr.size();
    o[QString::fromUtf8("totalInstalled")] = (int)all.size();
    if ( ids.size() > arr.size() ) {
        o[QString::fromUtf8("note")] =
            QString::fromUtf8("Truncated; refine 'query' or raise 'limit'.");
    }

    return o;
}

QJsonObject
AIMcpServerPrivate::toolParamSetExpression(const QJsonObject& a)
{
    NodePtr node = requireNode( a[QString::fromUtf8("node")].toString() );
    KnobIPtr knob = requireKnob( node, a[QString::fromUtf8("param")].toString() );

    const QString expr = a[QString::fromUtf8("expr")].toString();
    const int dimension = a.contains( QString::fromUtf8("dimension") )
                          ? (int)a[QString::fromUtf8("dimension")].toDouble() : 0;
    // A multi-line script must assign to `ret`; a single expression must not.
    const bool hasRet = a.contains( QString::fromUtf8("hasRetVariable") )
                        ? a[QString::fromUtf8("hasRetVariable")].toBool()
                        : expr.contains( QString::fromUtf8("ret") ) && expr.contains( QLatin1Char('\n') );

    if ( ( dimension < 0 ) || ( dimension >= knob->getDimension() ) ) {
        throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                         QString::fromUtf8("Dimension %1 is out of range for '%2' (%3 dimension(s))")
                         .arg(dimension)
                         .arg( QString::fromUtf8( knob->getName().c_str() ) )
                         .arg( knob->getDimension() ) );
    }

    try {
        knob->setExpression(dimension, expr.toStdString(), hasRet, false);
    } catch (const std::exception& e) {
        throw ToolError( QString::fromUtf8("EXPRESSION_INVALID"),
                         QString::fromUtf8("Natron rejected the expression: %1").arg( QString::fromUtf8( e.what() ) ),
                         QString::fromUtf8("Natron expressions are Python evaluated in the parameter's context; "
                                           "'frame' is the current frame. A single expression must evaluate to a "
                                           "value; a multi-line script must assign to 'ret' and be sent with "
                                           "hasRetVariable=true.") );
    }

    return readKnob(knob);
}

QJsonObject
AIMcpServerPrivate::toolParamSetKeyframe(const QJsonObject& a)
{
    NodePtr node = requireNode( a[QString::fromUtf8("node")].toString() );
    KnobIPtr knob = requireKnob( node, a[QString::fromUtf8("param")].toString() );

    const double frame = a[QString::fromUtf8("frame")].toDouble();
    const int dimension = a.contains( QString::fromUtf8("dimension") )
                          ? (int)a[QString::fromUtf8("dimension")].toDouble() : 0;
    const QJsonValue value = a[QString::fromUtf8("value")];
    const QString name = QString::fromUtf8( knob->getName().c_str() );

    if ( ( dimension < 0 ) || ( dimension >= knob->getDimension() ) ) {
        throw ToolError( QString::fromUtf8("PARAM_TYPE_MISMATCH"),
                         QString::fromUtf8("Dimension %1 is out of range for '%2'").arg(dimension).arg(name) );
    }

    KnobIntBasePtr    asInt    = std::dynamic_pointer_cast<KnobIntBase>(knob);
    KnobBoolBasePtr   asBool   = std::dynamic_pointer_cast<KnobBoolBase>(knob);
    KnobDoubleBasePtr asDouble = std::dynamic_pointer_cast<KnobDoubleBase>(knob);
    KnobStringBasePtr asString = std::dynamic_pointer_cast<KnobStringBase>(knob);

    // Choice derives from KnobIntBase, so it is handled by the int branch using
    // the option index.
    if (asInt) {
        asInt->setValueAtTime(frame, (int)value.toDouble(), ViewSpec::all(), dimension);
    } else if (asBool) {
        asBool->setValueAtTime(frame, value.toBool(), ViewSpec::all(), dimension);
    } else if (asDouble) {
        asDouble->setValueAtTime(frame, value.toDouble(), ViewSpec::all(), dimension);
    } else if (asString) {
        asString->setValueAtTime(frame, value.toString().toStdString(), ViewSpec::all(), dimension);
    } else {
        throw ToolError( QString::fromUtf8("PARAM_NOT_ANIMATABLE"),
                         QString::fromUtf8("Parameter '%1' (type %2) cannot hold keyframes")
                         .arg(name).arg( QString::fromUtf8( knob->typeName().c_str() ) ) );
    }

    QJsonObject o = readKnob(knob);
    o[QString::fromUtf8("keyframeSetAt")] = frame;

    return o;
}

QJsonObject
AIMcpServerPrivate::toolParamClearAnimation(const QJsonObject& a)
{
    NodePtr node = requireNode( a[QString::fromUtf8("node")].toString() );
    KnobIPtr knob = requireKnob( node, a[QString::fromUtf8("param")].toString() );

    const int dims = knob->getDimension();
    const int only = a.contains( QString::fromUtf8("dimension") )
                     ? (int)a[QString::fromUtf8("dimension")].toDouble() : -1;

    for (int d = 0; d < dims; ++d) {
        if ( ( only >= 0 ) && ( d != only ) ) {
            continue;
        }
        knob->removeAnimation(ViewSpec::all(), d);
    }

    return readKnob(knob);
}

QJsonObject
AIMcpServerPrivate::toolNodeSetPosition(const QJsonObject& a)
{
    NodePtr node = requireNode( a[QString::fromUtf8("node")].toString() );

    // Unlike a headless session, this server always runs inside the GUI, so the
    // node really does have a NodeGui and the position sticks.
    node->setPosition( a[QString::fromUtf8("x")].toDouble(),
                       a[QString::fromUtf8("y")].toDouble() );

    double x = 0., y = 0.;
    node->getPosition(&x, &y);

    QJsonObject o = describeNode(node);
    QJsonArray pos;
    pos.push_back(x);
    pos.push_back(y);
    o[QString::fromUtf8("position")] = pos;

    return o;
}

QJsonObject
AIMcpServerPrivate::toolNodeRename(const QJsonObject& a)
{
    NodePtr node = requireNode( a[QString::fromUtf8("node")].toString() );

    if ( a.contains( QString::fromUtf8("label") ) ) {
        node->setLabel( a[QString::fromUtf8("label")].toString().toStdString() );
    }
    if ( a.contains( QString::fromUtf8("scriptName") ) ) {
        const QString wanted = a[QString::fromUtf8("scriptName")].toString();
        try {
            node->setScriptName( wanted.toStdString() );
        } catch (const std::exception& e) {
            throw ToolError( QString::fromUtf8("SCRIPTNAME_INVALID"),
                             QString::fromUtf8("Could not rename to '%1': %2")
                             .arg(wanted).arg( QString::fromUtf8( e.what() ) ),
                             QString::fromUtf8("Script names must be unique and contain only letters, digits and underscores.") );
        }
    }

    return describeNode(node);
}

QJsonObject
AIMcpServerPrivate::toolScriptExec(const QJsonObject& a)
{
    // The escape hatch. Anything the typed tools do not cover -- expressions on
    // exotic knobs, roto shapes, trackers, rendering, batch edits -- is reachable
    // from here, because this is the same interpreter the Script Editor uses.
    const QString code = a[QString::fromUtf8("code")].toString();

    if ( code.trimmed().isEmpty() ) {
        throw ToolError( QString::fromUtf8("SCRIPT_EMPTY"),
                         QString::fromUtf8("No code was supplied") );
    }

    std::string error, output;
    const bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(code.toStdString(), &error, &output);

    QJsonObject o;
    o[QString::fromUtf8("ok")] = ok;
    o[QString::fromUtf8("stdout")] = QString::fromUtf8( output.c_str() );

    if (!ok) {
        throw ToolError( QString::fromUtf8("SCRIPT_FAILED"),
                         QString::fromUtf8( error.empty()
                                            ? "The script failed with no message"
                                            : error.c_str() ),
                         QString::fromUtf8("'app' is the current project. Output printed with print() "
                                           "is returned in 'stdout'.") );
    }

    return o;
}

QJsonObject
AIMcpServerPrivate::toolRenderStart(const QJsonObject& a)
{
    AppInstancePtr instance = app();

    if (!instance) {
        throw ToolError( QString::fromUtf8("NO_PROJECT"),
                         QString::fromUtf8("No project is open") );
    }

    const QString writerName = a[QString::fromUtf8("writeNode")].toString();
    NodePtr writer = requireNode(writerName);

    if ( !writer->isOutputNode() ) {
        throw ToolError( QString::fromUtf8("NOT_A_WRITER"),
                         QString::fromUtf8("'%1' is not a Write node").arg(writerName),
                         QString::fromUtf8("Call graph_list_nodes and pick a node whose pluginID is a Write.") );
    }

    const int first = (int)a[QString::fromUtf8("first")].toDouble();
    const int last  = (int)a[QString::fromUtf8("last")].toDouble();

    if (last < first) {
        throw ToolError( QString::fromUtf8("RENDER_RANGE_INVALID"),
                         QString::fromUtf8("last (%1) is before first (%2)").arg(last).arg(first) );
    }

    std::list<std::string> writers;
    writers.push_back( writerName.toStdString() );

    std::list<std::pair<int, std::pair<int, int> > > ranges;
    ranges.push_back( std::make_pair( 1, std::make_pair(first, last) ) );

    // doBlockingRender = false. This is the whole point of the tool: in GUI mode
    // the render is queued onto Natron's scheduler threads and this returns at
    // once, so the GUI thread -- which this handler occupies -- is released
    // immediately.
    //
    // Rendering from inside script_exec instead is what freezes Natron: that
    // handler holds the GUI thread, so a script that waits for the render
    // deadlocks (the scheduler needs the main thread to make progress), the MCP
    // client eventually times out, and Natron's own watchdog puts up "A Render
    // is not responding anymore" (Engine/AbortableRenderInfo.cpp:272).
    instance->startWritersRenderingFromNames(false, false, writers, ranges);

    QJsonObject o;
    o[QString::fromUtf8("started")] = true;
    o[QString::fromUtf8("writeNode")] = writerName;
    o[QString::fromUtf8("first")] = first;
    o[QString::fromUtf8("last")] = last;
    o[QString::fromUtf8("note")] =
        QString::fromUtf8("Queued. This returned immediately; poll render_status to follow it.");

    return o;
}

QJsonObject
AIMcpServerPrivate::toolRenderStatus(const QJsonObject& a)
{
    QJsonObject o;

    if ( a.contains( QString::fromUtf8("writeNode") ) ) {
        NodePtr writer = requireNode( a[QString::fromUtf8("writeNode")].toString() );
        o[QString::fromUtf8("writeNode")] = a[QString::fromUtf8("writeNode")];
        o[QString::fromUtf8("rendering")] = writer->isNodeRendering();

        return o;
    }

    // No node given: report anything in the project that is currently rendering.
    ProjectPtr proj = project();
    QJsonArray busy;
    if (proj) {
        NodesList nodes = proj->getNodes();
        for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if ( (*it) && (*it)->isNodeRendering() ) {
                busy.push_back( QString::fromUtf8( (*it)->getScriptName_mt_safe().c_str() ) );
            }
        }
    }
    o[QString::fromUtf8("renderingNodes")] = busy;
    o[QString::fromUtf8("rendering")] = ( busy.size() > 0 );

    return o;
}

// ---------------------------------------------------------------------------
// Tool catalogue
// ---------------------------------------------------------------------------

namespace {
QJsonObject
makeProperty(const QString& type,
             const QString& description)
{
    QJsonObject p;

    p[QString::fromUtf8("type")] = type;
    p[QString::fromUtf8("description")] = description;

    return p;
}

QJsonObject
makeTool(const QString& name,
         const QString& description,
         const QJsonObject& properties,
         const QJsonArray& required,
         bool readOnly,
         bool destructive)
{
    QJsonObject schema;

    schema[QString::fromUtf8("type")] = QString::fromUtf8("object");
    schema[QString::fromUtf8("properties")] = properties;
    schema[QString::fromUtf8("required")] = required;

    QJsonObject annotations;
    annotations[QString::fromUtf8("readOnlyHint")] = readOnly;
    annotations[QString::fromUtf8("destructiveHint")] = destructive;
    annotations[QString::fromUtf8("openWorldHint")] = false;

    QJsonObject tool;
    tool[QString::fromUtf8("name")] = name;
    tool[QString::fromUtf8("description")] = description;
    tool[QString::fromUtf8("inputSchema")] = schema;
    tool[QString::fromUtf8("annotations")] = annotations;

    return tool;
}
} // anonymous namespace

QJsonObject
AIMcpServerPrivate::toolsList() const
{
    QJsonArray tools;

    tools.push_back( makeTool( QString::fromUtf8("natron_status"),
                               QString::fromUtf8("Version, open project and node count of the running Natron instance."),
                               QJsonObject(), QJsonArray(), true, false ) );

    tools.push_back( makeTool( QString::fromUtf8("graph_list_nodes"),
                               QString::fromUtf8("List every node in the current project with its plugin ID and input connections."),
                               QJsonObject(), QJsonArray(), true, false ) );

    {
        QJsonObject props;
        props[QString::fromUtf8("pluginID")] = makeProperty( QString::fromUtf8("string"),
                                                             QString::fromUtf8("Exact plugin ID, e.g. 'net.sf.openfx.GradePlugin'.") );
        props[QString::fromUtf8("label")] = makeProperty( QString::fromUtf8("string"),
                                                          QString::fromUtf8("Optional initial name for the node.") );
        props[QString::fromUtf8("x")] = makeProperty( QString::fromUtf8("number"),
                                                      QString::fromUtf8("Optional x position in the node graph.") );
        props[QString::fromUtf8("y")] = makeProperty( QString::fromUtf8("number"),
                                                      QString::fromUtf8("Optional y position in the node graph.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("pluginID") );
        tools.push_back( makeTool( QString::fromUtf8("node_create"),
                                   QString::fromUtf8("Create a node in the current project."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"),
                                                         QString::fromUtf8("Script name of the node whose input is being connected.") );
        props[QString::fromUtf8("inputIndex")] = makeProperty( QString::fromUtf8("integer"),
                                                               QString::fromUtf8("Zero-based input index.") );
        props[QString::fromUtf8("source")] = makeProperty( QString::fromUtf8("string"),
                                                           QString::fromUtf8("Script name of the upstream node.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        req.push_back( QString::fromUtf8("inputIndex") );
        req.push_back( QString::fromUtf8("source") );
        tools.push_back( makeTool( QString::fromUtf8("node_connect"),
                                   QString::fromUtf8("Connect one node's input to another node's output. Replaces any existing connection on that input."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"),
                                                         QString::fromUtf8("Script name of the node to delete.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        tools.push_back( makeTool( QString::fromUtf8("node_delete"),
                                   QString::fromUtf8("Delete a node from the project. Asks the user for confirmation inside Natron."),
                                   props, req, false, true ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"),
                                                         QString::fromUtf8("Script name of the node.") );
        props[QString::fromUtf8("param")] = makeProperty( QString::fromUtf8("string"),
                                                          QString::fromUtf8("Script name of the parameter.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        req.push_back( QString::fromUtf8("param") );
        tools.push_back( makeTool( QString::fromUtf8("param_get"),
                                   QString::fromUtf8("Read a parameter's type, value and (for choices) its options."),
                                   props, req, true, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"),
                                                         QString::fromUtf8("Script name of the node.") );
        props[QString::fromUtf8("param")] = makeProperty( QString::fromUtf8("string"),
                                                          QString::fromUtf8("Script name of the parameter.") );
        props[QString::fromUtf8("value")] = makeProperty( QString::fromUtf8("string"),
                                                          QString::fromUtf8("New value. Number, boolean or string depending on the parameter type; a Choice accepts either its option label or its index.") );
        props[QString::fromUtf8("dimension")] = makeProperty( QString::fromUtf8("integer"),
                                                              QString::fromUtf8("Dimension to write, default 0.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        req.push_back( QString::fromUtf8("param") );
        req.push_back( QString::fromUtf8("value") );
        tools.push_back( makeTool( QString::fromUtf8("param_set"),
                                   QString::fromUtf8("Set one dimension of a parameter and return the re-read parameter."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("query")] = makeProperty( QString::fromUtf8("string"),
                                                          QString::fromUtf8("Case-insensitive substring of the plugin ID, e.g. 'text', 'blur', 'merge'. Omit to list everything.") );
        props[QString::fromUtf8("limit")] = makeProperty( QString::fromUtf8("integer"),
                                                          QString::fromUtf8("Maximum results, default 40.") );
        tools.push_back( makeTool( QString::fromUtf8("plugin_search"),
                                   QString::fromUtf8("List or search the plugin IDs actually installed. Call this before node_create instead of guessing an ID."),
                                   props, QJsonArray(), true, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Script name of the node.") );
        props[QString::fromUtf8("param")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Script name of the parameter.") );
        props[QString::fromUtf8("expr")] = makeProperty( QString::fromUtf8("string"),
                                                         QString::fromUtf8("Natron expression, Python evaluated per frame. 'frame' is the current frame, e.g. 'frame / 144.0 * 250000'.") );
        props[QString::fromUtf8("dimension")] = makeProperty( QString::fromUtf8("integer"), QString::fromUtf8("Dimension to drive, default 0.") );
        props[QString::fromUtf8("hasRetVariable")] = makeProperty( QString::fromUtf8("boolean"),
                                                                   QString::fromUtf8("True when the code is a multi-line script assigning to 'ret'.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        req.push_back( QString::fromUtf8("param") );
        req.push_back( QString::fromUtf8("expr") );
        tools.push_back( makeTool( QString::fromUtf8("param_set_expression"),
                                   QString::fromUtf8("Drive a parameter with an expression, for values that change over time."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Script name of the node.") );
        props[QString::fromUtf8("param")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Script name of the parameter.") );
        props[QString::fromUtf8("value")] = makeProperty( QString::fromUtf8("number"), QString::fromUtf8("Value at that frame.") );
        props[QString::fromUtf8("frame")] = makeProperty( QString::fromUtf8("number"), QString::fromUtf8("Frame number.") );
        props[QString::fromUtf8("dimension")] = makeProperty( QString::fromUtf8("integer"), QString::fromUtf8("Dimension, default 0.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        req.push_back( QString::fromUtf8("param") );
        req.push_back( QString::fromUtf8("value") );
        req.push_back( QString::fromUtf8("frame") );
        tools.push_back( makeTool( QString::fromUtf8("param_set_keyframe"),
                                   QString::fromUtf8("Set a keyframe on a parameter at a given frame."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Script name of the node.") );
        props[QString::fromUtf8("param")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Script name of the parameter.") );
        props[QString::fromUtf8("dimension")] = makeProperty( QString::fromUtf8("integer"), QString::fromUtf8("Only this dimension; omit for all.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        req.push_back( QString::fromUtf8("param") );
        tools.push_back( makeTool( QString::fromUtf8("param_clear_animation"),
                                   QString::fromUtf8("Remove all keyframes from a parameter."),
                                   props, req, false, true ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Script name of the node.") );
        props[QString::fromUtf8("x")] = makeProperty( QString::fromUtf8("number"), QString::fromUtf8("X position in the node graph.") );
        props[QString::fromUtf8("y")] = makeProperty( QString::fromUtf8("number"), QString::fromUtf8("Y position in the node graph.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        req.push_back( QString::fromUtf8("x") );
        req.push_back( QString::fromUtf8("y") );
        tools.push_back( makeTool( QString::fromUtf8("node_set_position"),
                                   QString::fromUtf8("Move a node in the node graph so the layout stays readable."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("node")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("Current script name.") );
        props[QString::fromUtf8("label")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("New display label.") );
        props[QString::fromUtf8("scriptName")] = makeProperty( QString::fromUtf8("string"), QString::fromUtf8("New script name (letters, digits, underscore; must be unique).") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("node") );
        tools.push_back( makeTool( QString::fromUtf8("node_rename"),
                                   QString::fromUtf8("Rename a node's label and/or script name."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("code")] = makeProperty( QString::fromUtf8("string"),
                                                         QString::fromUtf8("Python source, run in Natron's own interpreter. 'app' is the current project.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("code") );
        tools.push_back( makeTool( QString::fromUtf8("script_exec"),
                                   QString::fromUtf8("Run Python inside Natron for anything the other tools do not cover: roto, tracking, batch edits, any parameter API. Same interpreter as the Script Editor. "
                                                     "IMPORTANT: this runs on Natron's GUI thread and blocks it until it returns, so the code must finish quickly. "
                                                     "Never render or wait/sleep/poll in here -- that freezes Natron and the render can never finish. Use render_start instead."),
                                   props, req, false, true ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("writeNode")] = makeProperty( QString::fromUtf8("string"),
                                                              QString::fromUtf8("Script name of the Write node.") );
        props[QString::fromUtf8("first")] = makeProperty( QString::fromUtf8("integer"), QString::fromUtf8("First frame.") );
        props[QString::fromUtf8("last")] = makeProperty( QString::fromUtf8("integer"), QString::fromUtf8("Last frame.") );
        QJsonArray req;
        req.push_back( QString::fromUtf8("writeNode") );
        req.push_back( QString::fromUtf8("first") );
        req.push_back( QString::fromUtf8("last") );
        tools.push_back( makeTool( QString::fromUtf8("render_start"),
                                   QString::fromUtf8("Start a render and return immediately. This is the only correct way to render: it queues the work on Natron's scheduler instead of blocking. Poll render_status to follow progress."),
                                   props, req, false, false ) );
    }

    {
        QJsonObject props;
        props[QString::fromUtf8("writeNode")] = makeProperty( QString::fromUtf8("string"),
                                                              QString::fromUtf8("Optional: check just this node. Omit to list everything currently rendering.") );
        tools.push_back( makeTool( QString::fromUtf8("render_status"),
                                   QString::fromUtf8("Report whether a render is still running."),
                                   props, QJsonArray(), true, false ) );
    }

    QJsonObject result;
    result[QString::fromUtf8("tools")] = tools;

    return result;
}

// ---------------------------------------------------------------------------
// JSON-RPC dispatch
// ---------------------------------------------------------------------------

QJsonObject
AIMcpServerPrivate::handleToolCall(const QString& toolName,
                                   const QJsonObject& args)
{
    if ( toolName == QString::fromUtf8("natron_status") ) {
        return toolNatronStatus();
    }
    if ( toolName == QString::fromUtf8("graph_list_nodes") ) {
        return toolGraphListNodes();
    }
    if ( toolName == QString::fromUtf8("node_create") ) {
        return toolNodeCreate(args);
    }
    if ( toolName == QString::fromUtf8("node_connect") ) {
        return toolNodeConnect(args);
    }
    if ( toolName == QString::fromUtf8("node_delete") ) {
        return toolNodeDelete(args);
    }
    if ( toolName == QString::fromUtf8("param_get") ) {
        return toolParamGet(args);
    }
    if ( toolName == QString::fromUtf8("param_set") ) {
        return toolParamSet(args);
    }
    if ( toolName == QString::fromUtf8("param_set_expression") ) {
        return toolParamSetExpression(args);
    }
    if ( toolName == QString::fromUtf8("param_set_keyframe") ) {
        return toolParamSetKeyframe(args);
    }
    if ( toolName == QString::fromUtf8("param_clear_animation") ) {
        return toolParamClearAnimation(args);
    }
    if ( toolName == QString::fromUtf8("node_set_position") ) {
        return toolNodeSetPosition(args);
    }
    if ( toolName == QString::fromUtf8("node_rename") ) {
        return toolNodeRename(args);
    }
    if ( toolName == QString::fromUtf8("plugin_search") ) {
        return toolPluginSearch(args);
    }
    if ( toolName == QString::fromUtf8("script_exec") ) {
        return toolScriptExec(args);
    }
    if ( toolName == QString::fromUtf8("render_start") ) {
        return toolRenderStart(args);
    }
    if ( toolName == QString::fromUtf8("render_status") ) {
        return toolRenderStatus(args);
    }

    throw ToolError( QString::fromUtf8("UNKNOWN_TOOL"),
                     QString::fromUtf8("No tool named '%1'").arg(toolName),
                     QString::fromUtf8("Call tools/list for the available tools.") );
}

QJsonObject
AIMcpServerPrivate::handleRequest(const QJsonObject& request)
{
    QJsonObject response;

    response[QString::fromUtf8("jsonrpc")] = QString::fromUtf8("2.0");
    if ( request.contains( QString::fromUtf8("id") ) ) {
        response[QString::fromUtf8("id")] = request[QString::fromUtf8("id")];
    }

    const QString method = request[QString::fromUtf8("method")].toString();

    if ( method == QString::fromUtf8("initialize") ) {
        QJsonObject caps;
        QJsonObject toolsCap;
        toolsCap[QString::fromUtf8("listChanged")] = false;
        caps[QString::fromUtf8("tools")] = toolsCap;

        QJsonObject info;
        info[QString::fromUtf8("name")] = QString::fromUtf8("natron");
        info[QString::fromUtf8("version")] = QString::fromUtf8(NATRON_VERSION_STRING);

        QJsonObject result;
        result[QString::fromUtf8("protocolVersion")] = QString::fromUtf8(AI_MCP_PROTOCOL_VERSION);
        result[QString::fromUtf8("capabilities")] = caps;
        result[QString::fromUtf8("serverInfo")] = info;

        response[QString::fromUtf8("result")] = result;

        return response;
    }

    if ( method == QString::fromUtf8("tools/list") ) {
        response[QString::fromUtf8("result")] = toolsList();

        return response;
    }

    if ( method == QString::fromUtf8("tools/call") ) {
        const QJsonObject params = request[QString::fromUtf8("params")].toObject();
        const QString toolName = params[QString::fromUtf8("name")].toString();
        const QJsonObject args = params[QString::fromUtf8("arguments")].toObject();

        QJsonObject result;
        QJsonArray content;
        QJsonObject textBlock;
        textBlock[QString::fromUtf8("type")] = QString::fromUtf8("text");

        bool ok = true;
        try {
            // Group this tool's undo commands. The guard is re-entrant, so when
            // the chat panel has already opened a transaction for the whole
            // conversation turn this nests harmlessly and the turn stays a
            // single Ctrl+Z; when the server is driven directly by an external
            // CLI (no panel) each mutating tool still gets one clean entry.
            //
            // The guard is a stack object on purpose: if handleToolCall throws
            // -- which it does for every domain error -- the destructor still
            // closes the macro. An unmatched beginMacro would wedge undo for the
            // rest of the session.
            std::unique_ptr<AIUndoTransaction> tx;
            if ( toolMutates(toolName) ) {
                tx.reset( new AIUndoTransaction(_publicInterface, toolName) );
            }

            const QJsonObject payload = handleToolCall(toolName, args);
            textBlock[QString::fromUtf8("text")] =
                QString::fromUtf8( QJsonDocument(payload).toJson(QJsonDocument::Compact) );
            result[QString::fromUtf8("structuredContent")] = payload;
            result[QString::fromUtf8("isError")] = false;
        } catch (const ToolError& e) {
            ok = false;
            QJsonObject err;
            err[QString::fromUtf8("code")] = e.code;
            err[QString::fromUtf8("message")] = e.message;
            if ( !e.hint.isEmpty() ) {
                err[QString::fromUtf8("hint")] = e.hint;
            }
            textBlock[QString::fromUtf8("text")] =
                QString::fromUtf8( QJsonDocument(err).toJson(QJsonDocument::Compact) );
            result[QString::fromUtf8("isError")] = true;
        } catch (const std::exception& e) {
            // A domain failure must never escape as a protocol error: the model
            // can act on an isError result, but not on a transport failure.
            ok = false;
            QJsonObject err;
            err[QString::fromUtf8("code")] = QString::fromUtf8("INTERNAL_ERROR");
            err[QString::fromUtf8("message")] = QString::fromUtf8( e.what() );
            textBlock[QString::fromUtf8("text")] =
                QString::fromUtf8( QJsonDocument(err).toJson(QJsonDocument::Compact) );
            result[QString::fromUtf8("isError")] = true;
        } catch (...) {
            ok = false;
            QJsonObject err;
            err[QString::fromUtf8("code")] = QString::fromUtf8("INTERNAL_ERROR");
            err[QString::fromUtf8("message")] = QString::fromUtf8("Unknown failure while running the tool");
            textBlock[QString::fromUtf8("text")] =
                QString::fromUtf8( QJsonDocument(err).toJson(QJsonDocument::Compact) );
            result[QString::fromUtf8("isError")] = true;
        }

        content.push_back(textBlock);
        result[QString::fromUtf8("content")] = content;
        response[QString::fromUtf8("result")] = result;

        Q_EMIT _publicInterface->toolCallFinished(toolName, ok);

        return response;
    }

    // Notifications (no id) need no reply; the caller drops empty responses.
    if ( method.startsWith( QString::fromUtf8("notifications/") ) ) {
        return QJsonObject();
    }

    QJsonObject err;
    err[QString::fromUtf8("code")] = AI_JSONRPC_METHOD_NOT_FOUND;
    err[QString::fromUtf8("message")] = QString::fromUtf8("Method not found: %1").arg(method);
    response[QString::fromUtf8("error")] = err;

    return response;
}

QString
AIMcpServerPrivate::dispatch(const QString& requestJson)
{
    // The node graph is GUI-thread affine. When this server object lives in the
    // GUI thread the socket callbacks already run there, so calling directly is
    // both correct and necessary -- a BlockingQueuedConnection onto the calling
    // thread deadlocks.
    if ( QThread::currentThread() == qApp->thread() ) {
        return _publicInterface->dispatchOnGuiThread(requestJson);
    }

    QString response;
    QMetaObject::invokeMethod( _publicInterface,
                               "dispatchOnGuiThread",
                               Qt::BlockingQueuedConnection,
                               Q_RETURN_ARG(QString, response),
                               Q_ARG(QString, requestJson) );

    return response;
}

QString
AIMcpServer::dispatchOnGuiThread(const QString& requestJson)
{
    assert( QThread::currentThread() == qApp->thread() );

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(requestJson.toUtf8(), &parseError);

    if ( parseError.error != QJsonParseError::NoError ) {
        QJsonObject err;
        err[QString::fromUtf8("code")] = AI_JSONRPC_PARSE_ERROR;
        err[QString::fromUtf8("message")] = parseError.errorString();

        QJsonObject response;
        response[QString::fromUtf8("jsonrpc")] = QString::fromUtf8("2.0");
        response[QString::fromUtf8("id")] = QJsonValue();
        response[QString::fromUtf8("error")] = err;

        return QString::fromUtf8( QJsonDocument(response).toJson(QJsonDocument::Compact) );
    }

    if ( !doc.isObject() ) {
        QJsonObject err;
        err[QString::fromUtf8("code")] = AI_JSONRPC_INVALID_REQUEST;
        err[QString::fromUtf8("message")] = QString::fromUtf8("Request must be a JSON object");

        QJsonObject response;
        response[QString::fromUtf8("jsonrpc")] = QString::fromUtf8("2.0");
        response[QString::fromUtf8("id")] = QJsonValue();
        response[QString::fromUtf8("error")] = err;

        return QString::fromUtf8( QJsonDocument(response).toJson(QJsonDocument::Compact) );
    }

    const QJsonObject response = _imp->handleRequest( doc.object() );
    if ( response.isEmpty() ) {
        return QString();
    }

    return QString::fromUtf8( QJsonDocument(response).toJson(QJsonDocument::Compact) );
}

// ---------------------------------------------------------------------------
// HTTP transport
// ---------------------------------------------------------------------------

void
AIMcpServerPrivate::sendHttpResponse(QTcpSocket* socket,
                                     int status,
                                     const QByteArray& body)
{
    const char* reason = "OK";

    switch (status) {
    case 400: reason = "Bad Request"; break;
    case 401: reason = "Unauthorized"; break;
    case 404: reason = "Not Found"; break;
    case 405: reason = "Method Not Allowed"; break;
    case 413: reason = "Payload Too Large"; break;
    default:  reason = "OK"; break;
    }

    QByteArray header;
    header += "HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\n";
    header += "Content-Type: application/json\r\n";
    header += "Content-Length: " + QByteArray::number( body.size() ) + "\r\n";
    header += "Connection: close\r\n";
    header += "\r\n";

    socket->write(header);
    socket->write(body);
    socket->flush();
    socket->disconnectFromHost();
}

AIMcpServer::AIMcpServer(Gui* gui,
                         QObject* parent)
    : QObject(parent)
    , _imp( new AIMcpServerPrivate(this, gui) )
{
}

AIMcpServer::~AIMcpServer()
{
    stop();
}

bool
AIMcpServer::start()
{
    if (_imp->server) {
        return true;
    }

    _imp->server = new QTcpServer(this);

    // Loopback only. Without this an agent on the local network -- or any other
    // process on a shared host -- could drive the artist's Natron.
    if ( !_imp->server->listen(QHostAddress::LocalHost, 0) ) {
        delete _imp->server;
        _imp->server = 0;

        return false;
    }

    _imp->token = QUuid::createUuid().toString( QUuid::WithoutBraces ) +
                  QUuid::createUuid().toString( QUuid::WithoutBraces );

    connect( _imp->server, SIGNAL( newConnection() ), this, SLOT( onNewConnection() ) );

    return true;
}

void
AIMcpServer::stop()
{
    if (!_imp->server) {
        return;
    }

    for (std::map<QTcpSocket*, QByteArray>::iterator it = _imp->buffers.begin();
         it != _imp->buffers.end(); ++it) {
        it->first->abort();
        it->first->deleteLater();
    }
    _imp->buffers.clear();

    _imp->server->close();
    _imp->server->deleteLater();
    _imp->server = 0;
    _imp->token.clear();
}

bool
AIMcpServer::isRunning() const
{
    return _imp->server && _imp->server->isListening();
}

quint16
AIMcpServer::port() const
{
    return _imp->server ? _imp->server->serverPort() : 0;
}

QString
AIMcpServer::token() const
{
    return _imp->token;
}

void
AIMcpServer::beginAgentTransaction(const QString& label)
{
    // Must run where the undo stack lives.
    assert( QThread::currentThread() == qApp->thread() );

    if (_imp->txDepth == 0) {
        QUndoStack* stack = _imp->agentUndoStack();
        if (stack) {
            // Qt still calls redo() on each pushed command inside a macro, so
            // the graph updates live; only the *stack* collapses to one entry.
            stack->beginMacro( tr("AI: %1").arg(label) );
            _imp->txStack = stack;
        } else {
            // No graph yet (no project open). Still count the nesting so the
            // matching end() call stays balanced.
            _imp->txStack = 0;
        }
    }
    ++_imp->txDepth;
}

void
AIMcpServer::endAgentTransaction()
{
    assert( QThread::currentThread() == qApp->thread() );

    if (_imp->txDepth == 0) {
        return;
    }

    --_imp->txDepth;

    if (_imp->txDepth == 0) {
        if (_imp->txStack) {
            // Close on the stack we opened, not on whatever is current now.
            //
            // Known wart: Qt pushes the macro command even when nothing was
            // added to it, so a turn in which every tool failed before mutating
            // anything leaves one inert "AI: ..." entry on the stack. QUndoStack
            // offers no way to cancel a macro once begun, and undoing it here
            // would just move the empty command to the redo side. It is
            // cosmetic -- undoing it is a no-op -- and it is strictly preferable
            // to the alternative failure mode of leaving the macro open.
            _imp->txStack->endMacro();
            _imp->txStack = 0;
        }
    }
}

bool
AIMcpServer::isInAgentTransaction() const
{
    return _imp->txDepth > 0;
}

QString
AIMcpServer::url() const
{
    if ( !isRunning() ) {
        return QString();
    }

    return QString::fromUtf8("http://127.0.0.1:%1/mcp").arg( port() );
}

void
AIMcpServer::onNewConnection()
{
    while ( _imp->server && _imp->server->hasPendingConnections() ) {
        QTcpSocket* socket = _imp->server->nextPendingConnection();
        if (!socket) {
            continue;
        }
        _imp->buffers[socket] = QByteArray();
        connect( socket, SIGNAL( readyRead() ), this, SLOT( onSocketReadyRead() ) );
        connect( socket, SIGNAL( disconnected() ), this, SLOT( onSocketDisconnected() ) );
    }
}

void
AIMcpServer::onSocketDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>( sender() );

    if (!socket) {
        return;
    }
    _imp->buffers.erase(socket);
    socket->deleteLater();
}

void
AIMcpServer::onSocketReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>( sender() );

    if (!socket) {
        return;
    }

    std::map<QTcpSocket*, QByteArray>::iterator found = _imp->buffers.find(socket);
    if ( found == _imp->buffers.end() ) {
        return;
    }

    found->second += socket->readAll();
    QByteArray& buffer = found->second;

    if ( buffer.size() > AI_MCP_MAX_REQUEST_BYTES ) {
        _imp->sendHttpResponse( socket, 413, QByteArray("{\"error\":\"request too large\"}") );

        return;
    }

    // Wait for the full header block.
    const int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;
    }

    const QByteArray header = buffer.left(headerEnd);
    const QList<QByteArray> lines = header.split('\n');

    if ( lines.isEmpty() ) {
        return;
    }

    // Request line: METHOD PATH VERSION
    const QList<QByteArray> requestLine = lines.at(0).trimmed().split(' ');
    const QByteArray verb = requestLine.isEmpty() ? QByteArray() : requestLine.at(0).toUpper();

    int contentLength = 0;
    QByteArray authorization;
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        const int colon = line.indexOf(':');
        if (colon < 0) {
            continue;
        }
        const QByteArray name = line.left(colon).trimmed().toLower();
        const QByteArray value = line.mid(colon + 1).trimmed();
        if (name == "content-length") {
            contentLength = value.toInt();
        } else if (name == "authorization") {
            authorization = value;
        }
    }

    // Wait for the whole body before doing anything.
    const int bodyStart = headerEnd + 4;
    if ( buffer.size() - bodyStart < contentLength ) {
        return;
    }

    const QByteArray body = buffer.mid(bodyStart, contentLength);
    buffer.clear();

    // Authenticate before parsing the body, and before any tool can run.
    const QByteArray expected = QByteArray("Bearer ") + _imp->token.toUtf8();
    if ( _imp->token.isEmpty() || (authorization != expected) ) {
        _imp->sendHttpResponse( socket, 401, QByteArray("{\"error\":\"invalid or missing bearer token\"}") );

        return;
    }

    if (verb == "GET") {
        // Cheap liveness probe for the panel's status indicator.
        _imp->sendHttpResponse( socket, 200, QByteArray("{\"status\":\"ok\",\"server\":\"natron\"}") );

        return;
    }

    if (verb != "POST") {
        _imp->sendHttpResponse( socket, 405, QByteArray("{\"error\":\"use POST\"}") );

        return;
    }

    const QString response = _imp->dispatch( QString::fromUtf8(body) );

    if ( response.isEmpty() ) {
        // JSON-RPC notification: acknowledge with 202 and no body.
        _imp->sendHttpResponse( socket, 200, QByteArray() );

        return;
    }

    _imp->sendHttpResponse( socket, 200, response.toUtf8() );
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AIMcpServer.cpp"
