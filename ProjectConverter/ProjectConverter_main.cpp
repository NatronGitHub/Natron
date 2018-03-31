/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef NATRON_BOOST_SERIALIZATION_COMPAT
#error "NATRON_BOOST_SERIALIZATION_COMPAT should be defined when compiling ProjectConverter to allow with projects older than Natron 2.2"
#endif

#include <string>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QDateTime>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/CLArgs.h"
#include "Engine/CreateNodeArgs.h"
#include "Global/FStreamsSupport.h"
#include "Engine/StandardPaths.h"

#include "Global/StrUtils.h"

#include "Gui/BackdropGui.h"

#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/ProjectSerialization.h"
#include "Serialization/SerializationIO.h"
#include "Serialization/NodeGuiSerialization.h"
#include "Serialization/ProjectGuiSerialization.h"
#include "Serialization/SerializationCompat.h"

NATRON_NAMESPACE_USING

static const SERIALIZATION_NAMESPACE::NodeGuiSerialization* matchNodeGuiRecursive(const SERIALIZATION_NAMESPACE::NodeGuiSerialization& nodeGui, const std::string& nodeScriptName, const SERIALIZATION_NAMESPACE::NodeSerialization& serialization)
{
    if (nodeGui._nodeName == nodeScriptName) {
        return &nodeGui;
    }
    const std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::NodeGuiSerialization> >& children = nodeGui.getChildren();
    for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::NodeGuiSerialization> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        const SERIALIZATION_NAMESPACE::NodeGuiSerialization* childRet = matchNodeGuiRecursive(**it, nodeScriptName, serialization);
        if (childRet) {
            return childRet;
        }
    }


    return 0;
}

static const SERIALIZATION_NAMESPACE::NodeGuiSerialization* matchNodeGuiRecursive(const std::list<SERIALIZATION_NAMESPACE::NodeGuiSerialization>& nodeGuis, const std::string& nodeScriptName, const SERIALIZATION_NAMESPACE::NodeSerialization& serialization)
{
    for (std::list<SERIALIZATION_NAMESPACE::NodeGuiSerialization>::const_iterator it = nodeGuis.begin(); it != nodeGuis.end(); ++it) {
        const SERIALIZATION_NAMESPACE::NodeGuiSerialization* foundNodeGui = matchNodeGuiRecursive(*it, nodeScriptName, serialization);
        if (foundNodeGui) {
            return foundNodeGui;
        }
    }
    return 0;
}


static void convertNodeGuiSerializationToNodeSerialization(const std::list<SERIALIZATION_NAMESPACE::NodeGuiSerialization>& nodeGuis, const std::string& nodeScriptName, SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{

    const SERIALIZATION_NAMESPACE::NodeGuiSerialization* foundNodeGui = matchNodeGuiRecursive(nodeGuis, nodeScriptName, *serialization);

    if (foundNodeGui) {
        serialization->_nodePositionCoords[0] = foundNodeGui->_posX;
        serialization->_nodePositionCoords[1] = foundNodeGui->_posY;
        serialization->_nodeSize[0] = foundNodeGui->_width;
        serialization->_nodeSize[1] = foundNodeGui->_height;
        if (foundNodeGui->_colorWasFound) {
            serialization->_nodeColor[0] = foundNodeGui->_r;
            serialization->_nodeColor[1] = foundNodeGui->_g;
            serialization->_nodeColor[2] = foundNodeGui->_b;
        } else {
            serialization->_nodeColor[0] = serialization->_nodeColor[1] = serialization->_nodeColor[2] = -1;
        }
        if (foundNodeGui->_hasOverlayColor) {
            serialization->_overlayColor[0] = foundNodeGui->_r;
            serialization->_overlayColor[1] = foundNodeGui->_g;
            serialization->_overlayColor[2] = foundNodeGui->_b;
        } else {
            serialization->_overlayColor[0] = serialization->_overlayColor[1] = serialization->_overlayColor[2] = -1;
        }
    }

    for (std::list<SERIALIZATION_NAMESPACE::NodeSerializationPtr>::iterator it2 = serialization->_children.begin(); it2!=serialization->_children.end(); ++it2) {
        std::string childScriptName = nodeScriptName;
        childScriptName += ".";
        childScriptName += (*it2)->_nodeScriptName;
        convertNodeGuiSerializationToNodeSerialization(nodeGuis, childScriptName, it2->get());
    }
} // convertNodeGuiSerializationToNodeSerialization

static void convertPaneLayoutToTabWidgetSerialization(const SERIALIZATION_NAMESPACE::PaneLayout& deprecated, SERIALIZATION_NAMESPACE::TabWidgetSerialization* serialization)
{
    serialization->isAnchor = deprecated.isAnchor;
    serialization->currentIndex = deprecated.currentIndex;
    serialization->tabs = deprecated.tabs;
    serialization->scriptName = deprecated.name;
} // convertPaneLayoutToTabWidgetSerialization

static void convertSplitterToProjectSplitterSerialization(const SERIALIZATION_NAMESPACE::SplitterSerialization& deprecated, SERIALIZATION_NAMESPACE::WidgetSplitterSerialization* serialization)
{
    QStringList list = QString::fromUtf8(deprecated.sizes.c_str()).split( QLatin1Char(' ') );

    assert(list.size() == 2);
    QList<int> s;
    s << list[0].toInt() << list[1].toInt();
    if ( (s[0] == 0) || (s[1] == 0) ) {
        int mean = (s[0] + s[1]) / 2;
        s[0] = s[1] = mean;
    }

    assert(deprecated.children.size() == 2);
    serialization->leftChildSize = s[0];
    serialization->rightChildSize = s[1];
    switch ((NATRON_NAMESPACE::OrientationEnum)deprecated.orientation) {
        case NATRON_NAMESPACE::eOrientationHorizontal:
            serialization->orientation = kSplitterOrientationHorizontal;
            break;
        case NATRON_NAMESPACE::eOrientationVertical:
            serialization->orientation = kSplitterOrientationVertical;
            break;
    }

    SERIALIZATION_NAMESPACE::WidgetSplitterSerialization::Child* children[2] = {&serialization->leftChild, &serialization->rightChild};
    for (int i = 0; i < 2; ++i) {
        if (deprecated.children[i]->child_asPane) {
            children[i]->childIsTabWidget.reset(new SERIALIZATION_NAMESPACE::TabWidgetSerialization);
            children[i]->type = kSplitterChildTypeTabWidget;
            convertPaneLayoutToTabWidgetSerialization(*deprecated.children[i]->child_asPane, children[i]->childIsTabWidget.get());
        } else if (deprecated.children[i]->child_asSplitter) {
            children[i]->childIsSplitter.reset(new SERIALIZATION_NAMESPACE::WidgetSplitterSerialization);
            children[i]->type = kSplitterChildTypeSplitter;
            convertSplitterToProjectSplitterSerialization(*deprecated.children[i]->child_asSplitter, children[i]->childIsSplitter.get());
        }

    }
} // convertSplitterToProjectSplitterSerialization

static void
convertWorkspaceSerialization(const SERIALIZATION_NAMESPACE::GuiLayoutSerialization& original, SERIALIZATION_NAMESPACE::WorkspaceSerialization* serialization)
{
    for (std::list<SERIALIZATION_NAMESPACE::ApplicationWindowSerialization*>::const_iterator it = original._windows.begin(); it != original._windows.end(); ++it) {
        boost::shared_ptr<SERIALIZATION_NAMESPACE::WindowSerialization> window(new SERIALIZATION_NAMESPACE::WindowSerialization);
        window->windowPosition[0] = (*it)->x;
        window->windowPosition[1] = (*it)->y;
        window->windowSize[0] = (*it)->w;
        window->windowSize[1] = (*it)->h;

        if ((*it)->child_asSplitter) {
            window->isChildSplitter.reset(new SERIALIZATION_NAMESPACE::WidgetSplitterSerialization);
            window->childType = kSplitterChildTypeSplitter;
            convertSplitterToProjectSplitterSerialization(*(*it)->child_asSplitter, window->isChildSplitter.get());
        } else if ((*it)->child_asPane) {
            window->isChildTabWidget.reset(new SERIALIZATION_NAMESPACE::TabWidgetSerialization);
            window->childType = kSplitterChildTypeTabWidget;
            convertPaneLayoutToTabWidgetSerialization(*(*it)->child_asPane, window->isChildTabWidget.get());
        } else if (!(*it)->child_asDockablePanel.empty()) {
            window->isChildSettingsPanel = (*it)->child_asDockablePanel;
        }

        if ((*it)->isMainWindow) {
            serialization->_mainWindowSerialization = window;
        } else {
            serialization->_floatingWindowsSerialization.push_back(window);
        }
    }
} // convertWorkspaceSerialization

static void
convertToProjectSerialization(const SERIALIZATION_NAMESPACE::ProjectGuiSerialization& original, SERIALIZATION_NAMESPACE::ProjectSerialization* serialization)
{

    if (!serialization->_projectWorkspace) {
        serialization->_projectWorkspace.reset(new SERIALIZATION_NAMESPACE::WorkspaceSerialization);
    }
    for (std::map<std::string, SERIALIZATION_NAMESPACE::ViewerData >::const_iterator it = original._viewersData.begin(); it != original._viewersData.end(); ++it) {
        SERIALIZATION_NAMESPACE::ViewportData& d = serialization->_viewportsData[it->first];
        d.left = it->second.zoomLeft;
        d.bottom = it->second.zoomBottom;
        d.zoomFactor = it->second.zoomFactor;
        d.par = 1.;
        d.zoomOrPanSinceLastFit = it->second.zoomOrPanSinceLastFit;
    }

    for (std::list<SERIALIZATION_NAMESPACE::NodeSerializationPtr>::iterator it = serialization->_nodes.begin(); it!=serialization->_nodes.end(); ++it) {
        convertNodeGuiSerializationToNodeSerialization(original._serializedNodes, (*it)->_nodeScriptName, it->get());
    }

    convertWorkspaceSerialization(original._layoutSerialization, serialization->_projectWorkspace.get());
    serialization->_projectWorkspace->_histograms = original._histograms;
    serialization->_projectWorkspace->_pythonPanels = original._pythonPanels;

    serialization->_openedPanelsOrdered = original._openedPanelsOrdered;
    
    
    
} // convertToProjectSerialization


class ConverterAppManager : public AppManager
{

    // Retain a copy of the loaded project before it was saved so that we can make a few properties pertain in the converted project such
    // as the software version, creation date etc...
    SERIALIZATION_NAMESPACE::ProjectSerialization _lastProjectLoadedCopy;

public:

    ConverterAppManager()
    : AppManager()
    {

    }


private:

    virtual bool checkForOlderProjectFile(const AppInstancePtr& /*app*/, const QString& filePathIn, QString* filePathOut) OVERRIDE FINAL
    {
        *filePathOut = filePathIn;
        return false;
    }

    virtual void loadProjectFromFileFunction(std::istream& ifile, const std::string& /*filename*/, const AppInstancePtr& app, SERIALIZATION_NAMESPACE::ProjectSerialization* obj) OVERRIDE FINAL
    {
        try {
            boost::archive::xml_iarchive iArchive(ifile);
            bool bgProject;
            iArchive >> boost::serialization::make_nvp("Background_project", bgProject);
            iArchive >> boost::serialization::make_nvp("Project", *obj);

            if (!bgProject) {
                SERIALIZATION_NAMESPACE::ProjectGuiSerialization deprecatedGuiSerialization;
                iArchive >> boost::serialization::make_nvp("ProjectGui", deprecatedGuiSerialization);

                // Prior to this version the layout cannot be recovered (this was when Natron 1 was still in beta)
                bool isToolOld = deprecatedGuiSerialization._version < PROJECT_GUI_SERIALIZATION_MAJOR_OVERHAUL;

                if (!isToolOld) {

                    // Restore the old backdrops from old version prior to Natron 1.1
                    const std::list<SERIALIZATION_NAMESPACE::NodeBackdropSerialization> & backdrops = deprecatedGuiSerialization._backdrops;
                    for (std::list<SERIALIZATION_NAMESPACE::NodeBackdropSerialization>::const_iterator it = backdrops.begin(); it != backdrops.end(); ++it) {

                        // Emulate the old backdrop which was not a node to a node as it is now in newer version

                        double x, y;
                        it->getPos(x, y);
                        int w, h;
                        it->getSize(w, h);

                        SERIALIZATION_NAMESPACE::KnobSerializationPtr labelSerialization = it->getLabelSerialization();
                        CreateNodeArgsPtr args(CreateNodeArgs::create( PLUGINID_NATRON_BACKDROP, app->getProject() ));
                        args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

                        NodePtr node = app->createNode(args);
                        assert(node);
                        if (node) {
                            node->setPosition(x, y);
                            node->setSize(w, h);
                            if (labelSerialization && !labelSerialization->_values.empty()) {
                                KnobStringPtr labelKnob = node->getEffectInstance()->getExtraLabelKnob();
                                if (labelKnob) {
                                    labelKnob->setValue(labelSerialization->_values.begin()->second[0]._value.isString);
                                }
                            }

                            float r, g, b;
                            it->getColor(r, g, b);
                            node->setColor(r, g, b);
                            node->setLabel( it->getFullySpecifiedName() );
                        }
                    }


                    // Now convert what we can convert to our newer format...
                    convertToProjectSerialization(deprecatedGuiSerialization, obj);
        }
                
        }
        } catch (...) {
            throw std::invalid_argument("Invalid project file");
        }

        _lastProjectLoadedCopy = *obj;

    } // loadProjectFromFileFunction

    virtual void aboutToSaveProject(SERIALIZATION_NAMESPACE::ProjectSerialization* serialization) OVERRIDE FINAL
    {
        // Hi-hack the version of the project to preserve the one written in the project originally
        serialization->_projectLoadedInfo = _lastProjectLoadedCopy._projectLoadedInfo;
    }
};

static boost::scoped_ptr<ConverterAppManager> g_app;

static void initializeAppOnce()
{
    // Create Natron instance
    if (g_app) {
        return;
    }
    g_app.reset(new ConverterAppManager);
    int nArgc = 0;
    g_app->load(nArgc, 0, CLArgs());
    assert(g_app->getTopLevelInstance());


}


static void
printUsage(const std::string& programName)
{

                              /* Text must hold in 80 columns ************************************************/
    QString msg = QString::fromUtf8("%1 usage:\n"
                              "This program can convert Natron projects (.ntp files) or workspace (.nl files)\n"
                              "or PyPlugs (.py files) made with Natron version 2.1.x and older to the new\n"
                              "project format used in 2.2.\n\n"
                              "Program options:\n\n"
                              "-i <filename>: Converts a .ntp (Project) or .nl (Workspace) file to\n"
                              "               a newer version. Upon failure an error message will be printed.\n\n"
                              "-i <dir name>: Converts all .ntp or .nl files under the given directory.\n\n"
                              "-r: Optional: If the path given to -i is a directory, recursively convert all\n"
                              "              recognized files in each sub-directory.\n\n"
                              "-o <filename>: Optional: Only valid when used with -i <filename>. If set,\n"
                              "               this the absolute file path of the output file.\n\n"
                              "-c: Optional: If set the original file(s) will be replaced by the converted file(s).\n"
                              "              The original file(s) will be renamed with the .bak extension.\n"
                              "              If not set the converted file(s) will have the same name as \n"
                              "              the input file with the \"-converted\" suffix before the file\n"
                              "              extension. When the -o option is set, this option has no effect.\n\n").arg(QString::fromUtf8(programName.c_str()));
    std::cout << msg.toStdString() << std::endl;
} // printUsage

static QStringList::iterator hasToken(QStringList &localArgs, const QString& token)
{
    for (QStringList::iterator it = localArgs.begin(); it!=localArgs.end(); ++it) {
        if (*it == token) {
            return it;
        }
    }
    return localArgs.end();
} // hasToken

static void parseArgs(const QStringList& appArgs, QString* inputPath, QString* outputPath, bool* replaceOriginal, bool* recurse)
{
    *recurse = false;
    *replaceOriginal = false;
    QStringList localArgs = appArgs;
    {
        QStringList::iterator foundInput = hasToken(localArgs, QLatin1String("-i"));
        if (foundInput == localArgs.end()) {
            throw std::invalid_argument(QString::fromUtf8("Missing -i switch").toStdString());
        } else {
            ++foundInput;
            if ( foundInput != localArgs.end() ) {
                *inputPath = *foundInput;
                localArgs.erase(foundInput);
            } else {
                throw std::invalid_argument(QString::fromUtf8("-i switch without a file/directory name").toStdString());
            }
        }
    }
    {
        QStringList::iterator foundInput = hasToken(localArgs, QLatin1String("-o"));
        if (foundInput != localArgs.end()) {
            ++foundInput;
            if ( foundInput != localArgs.end() ) {
                *outputPath = *foundInput;
                localArgs.erase(foundInput);
            } else {
                throw std::invalid_argument(QString::fromUtf8("-o switch without a file name").toStdString());
            }
        }

    }

    {
        QStringList::iterator foundInput = hasToken(localArgs, QLatin1String("-c"));
        if (foundInput != localArgs.end()) {
            *replaceOriginal = true;
        }

    }
    *recurse = false;
    {
        QStringList::iterator foundInput = hasToken(localArgs, QLatin1String("-r"));
        if (foundInput != localArgs.end()) {
            *recurse = true;
        }

    }
} // parseArgs






/**
 * @brief Attempts to read a workspace from an old layout (pre Natron 2.2) encoded with boost serialization in XML format.
 * If the file could be read, the structure is then converted to the newer format post Natron 2.2.
 * Upon failure an exception is thrown.
 **/
static void tryReadAndConvertOlderWorkspace(std::istream& stream, SERIALIZATION_NAMESPACE::WorkspaceSerialization* obj)
{
    try {
        boost::archive::xml_iarchive iArchive(stream);
        // Try first to load an old gui layout
        SERIALIZATION_NAMESPACE::GuiLayoutSerialization s;
        iArchive >> boost::serialization::make_nvp("Layout", s);
        convertWorkspaceSerialization(s, obj);
    } catch (...) {
        throw std::invalid_argument("Invalid workspace file");
    }

} // void tryReadAndConvertOlderWorkspace



                

/**
 * @brief Attempts to read a project from an old project (pre Natron 2.2) encoded with boost serialization in XML format.
 * If the file could be read, the structure is then converted to the newer format post Natron 2.2.
 * Upon failure an exception is thrown.
 **/
static void tryReadAndConvertOlderProject(const QString& filename, const QString& outFileName)
{
    initializeAppOnce();
    AppInstancePtr instance = g_app->getTopLevelInstance();
    assert(instance);
    if (!instance) {
        return;
    }

    AppInstancePtr couldLoadProj = instance->loadProject(filename.toStdString());
    if (couldLoadProj) {
        instance->saveTemp(outFileName.toStdString());
    } else {
        throw std::invalid_argument("Could not load project.");
    }


} // tryReadAndConvertOlderProject

/**
 * @brief Attemps to read a workspace from an old project.
 * Upon failure an exception is thrown.
**/
static void tryReadAndConvertOlderPyPlugFile(const QString& filename, const QString& outFileName)
{
    initializeAppOnce();
    AppInstancePtr instance = g_app->getTopLevelInstance();
    assert(instance);
    if (!instance) {
        return;
    }

    QFileInfo info(filename);
    QString baseName = info.fileName();
    if (!baseName.endsWith(QLatin1String(".py"))) {
        throw std::invalid_argument(QString::fromUtf8("%1: Invalid PyPlug file").arg(filename).toStdString());
    }

    baseName.remove(QLatin1String(".py"));

    std::string pythonModule = baseName.toStdString();
    // The NATRON_PATH_ENV_VAR should have been set, ensuring that the PyPlug to convert is loaded.
    // We now just try to instantiate the node from the PyPlug.
    std::string pluginID, pluginLabel, pluginIcon, pluginGrouping, pluginDescription, pythonScriptDirPath;
    bool isToolset;
    unsigned int version;
    if (!NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::getGroupInfos(pythonModule, &pluginID, &pluginLabel, &pluginIcon, &pluginGrouping, &pluginDescription, &pythonScriptDirPath, &isToolset, &version)) {
        return;
    }
    if (isToolset) {
        return;
    }
    CreateNodeArgsPtr args(CreateNodeArgs::create(pluginID, NodeCollectionPtr() ));
    args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
    args->setProperty(kCreateNodeArgsPropSilent, true);
    NodePtr node = instance->createNode(args);
    if (!node) {
        throw std::invalid_argument(QString::fromUtf8("%1: Failure to instanciate PyPlug").arg(baseName).toStdString());
    }

    // Convert all knobs in the group to user knobs
    const KnobsVec& knobs = node->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->getKnobDeclarationType() == KnobI::eKnobDeclarationTypePlugin) {
            (*it)->setKnobDeclarationType(KnobI::eKnobDeclarationTypeUser);
        }
    }

    NodeGroupPtr group = node->isEffectNodeGroup();
    assert(group);
    group->setSubGraphEditedByUser(true);

    node->exportNodeToPyPlug(outFileName.toStdString());
    
} // tryReadAndConvertOlderPyPlugFile

/**
 * @brief Attemps to read a workspace from an old project.
 * Upon failure an exception is thrown.
 **/
static void tryReadAndConvertOlderLayoutFile(const QString& filename, const QString& outFileName)
{
    boost::shared_ptr<SERIALIZATION_NAMESPACE::WorkspaceSerialization> workspace;
    // Read old file
    {
        FStreamsSupport::ifstream ifile;
        FStreamsSupport::open(&ifile, filename.toStdString());
        if (!ifile) {
            QString message = QString::fromUtf8("Could not open %1").arg(filename);
            throw std::invalid_argument(message.toStdString());
        }

        workspace.reset(new SERIALIZATION_NAMESPACE::WorkspaceSerialization);
        tryReadAndConvertOlderWorkspace(ifile, workspace.get());

    }

    // Write to converted file
    {
        FStreamsSupport::ofstream ofile;
        FStreamsSupport::open(&ofile, outFileName.toStdString());
        if (!ofile) {
            QString message = QString::fromUtf8("Could not open %1").arg(outFileName);
            throw std::invalid_argument(message.toStdString());
        }

        SERIALIZATION_NAMESPACE::write(ofile, *workspace, NATRON_PROJECT_FILE_HEADER);

    }

} // tryReadAndConvertOlderLayoutFile

struct ProcessData
{
    std::list<std::string> bakFiles;
    std::list<std::string> convertedFiles;
};


static void convertFile(const QString& filename, const QString& outputFilePathArgs, bool replaceOriginal, ProcessData* data)
{

    if (!QFile::exists(filename)) {
        QString message = QString::fromUtf8("%1: No such file or directory").arg(filename);
        throw std::invalid_argument(message.toStdString());
    }

    bool isProjectFile = filename.endsWith(QLatin1String(".ntp")) || filename.endsWith(QLatin1String(".ntp.autosave"));
    bool isWorkspaceFile = filename.endsWith(QLatin1String(".nl"));
    bool isPyPlugFile = filename.endsWith(QLatin1String(".py"));

    if (!isPyPlugFile && !isProjectFile && !isWorkspaceFile) {
        QString message = QString::fromUtf8("%1 does not appear to be a project file or PyPlug script or layout file.").arg(filename);
        throw std::invalid_argument(message.toStdString());
    }


    QString outFileName;
    if (!outputFilePathArgs.isEmpty()) {
        outFileName = outputFilePathArgs;
        // Remove any existing file with the same name
        if (QFile::exists(outFileName)) {
            QFile::remove(outFileName);
        }
    } else {
        if (replaceOriginal) {
            // To replace the original, first we save to a temporary file then we save onto the original file
            QString tmpFilename = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);
            StrUtils::ensureLastPathSeparator(tmpFilename);
            tmpFilename.append( QString::number( QDateTime::currentDateTime().toMSecsSinceEpoch() ) );
            outFileName = tmpFilename;
        } else {
            int foundDot = filename.lastIndexOf(QLatin1Char('.'));
            assert(foundDot != -1);
            if (foundDot != -1) {
                QString baseName = filename.mid(0, foundDot);
                outFileName += baseName;
                outFileName += QLatin1String("-converted.");
                QString ext;
                if (isPyPlugFile) {
                    ext = QLatin1String(NATRON_PRESETS_FILE_EXT);
                } else {
                    ext = filename.mid(foundDot + 1);
                }
                outFileName += ext;
            }

            // Remove any existing file with the same name
            if (QFile::exists(outFileName)) {
                QFile::remove(outFileName);
            }
        }
    }


    if (isProjectFile) {
        tryReadAndConvertOlderProject(filename, outFileName);
    } else if (isWorkspaceFile) {
        tryReadAndConvertOlderLayoutFile(filename, outFileName);
    } else if (isPyPlugFile) {
        tryReadAndConvertOlderPyPlugFile(filename, outFileName);
    }
    if (!QFile::exists(outFileName)) {
        return;
    }
    if (!replaceOriginal || !outputFilePathArgs.isEmpty()) {
        data->convertedFiles.push_back(outFileName.toStdString());
    } else {

        bool extensionChanged = false;
        QString finalFileName;
        {
            // Even if we replace the original file, make sure it has the
            // correct extension: old PyPlugs are .py files and in Natron >= 2.2
            // they are .nps

            int foundDot = filename.lastIndexOf(QLatin1Char('.'));
            assert(foundDot != -1);
            if (foundDot != -1) {
                finalFileName = filename.mid(0, foundDot);
                QString ext;
                if (isPyPlugFile) {
                    ext = QLatin1String("." NATRON_PRESETS_FILE_EXT);
                } else {
                    ext = filename.mid(foundDot);
                }
                finalFileName += ext;
                extensionChanged = filename.mid(foundDot) != ext;
            }
        }


        // Copy the original file to a backup (if the extension did not change)
        if (!extensionChanged) {
            QString backupFileName = filename;
            backupFileName += QLatin1String(".bak");
            if (QFile::exists(backupFileName)) {
                QFile::remove(backupFileName);
            }
            QFile::copy(filename, backupFileName);

            data->bakFiles.push_back(backupFileName.toStdString());

            // Remove the original file
            QFile::remove(filename);
        }


        if (QFile::exists(finalFileName)) {
            QFile::remove(finalFileName);
        }

        // Copy the temporary file onto the original file
        QFile::copy(outFileName, finalFileName);

        QFile::remove(outFileName);
    }

} // convertFile

static bool convertDirectory(const QString& dirPath, bool replaceOriginal, bool recurse, unsigned int recursionLevel, ProcessData* data)
{
    QDir originalDir(dirPath);
    if (!originalDir.exists()) {
        QString message = QString::fromUtf8("%1: No such file or directory").arg(dirPath);
        throw std::invalid_argument(message.toStdString());
    }
    QStringList originalFiles = originalDir.entryList(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    QString originalDirAbsolutePath = originalDir.absolutePath();

    bool didSomething = false;
    for (QStringList::const_iterator it = originalFiles.begin(); it!=originalFiles.end(); ++it) {
        QString absoluteOriginalFilePath = originalDirAbsolutePath + QLatin1Char('/') + *it;

        // Check for sub-directory
        {
            QDir subDir(absoluteOriginalFilePath);
            if (subDir.exists()) {
                if (recurse) {
                    didSomething |= convertDirectory(absoluteOriginalFilePath, replaceOriginal, recurse, recursionLevel + 1, data);
                }
                continue;
            }
        }

        if (it->endsWith(QLatin1String(".ntp")) || it->endsWith(QLatin1String(".nl")) || it->endsWith(QLatin1String(".py"))) {
            try {
                convertFile(absoluteOriginalFilePath, QString(),replaceOriginal, data);
            } catch (const std::exception& e) {
                std::cerr << QString::fromUtf8("Error: %1").arg(QString::fromUtf8(e.what())).toStdString() << std::endl;
                continue;
            }
            didSomething = true;
        }
    }

    return didSomething;
} // convertDirectory

static void cleanupApp()
{
    if (g_app) {
        g_app->quitApplication();
        g_app.reset();
    }
} // cleanupApp

static void cleanupCreatedFiles(const ProcessData& data)
{
    for (std::list<std::string>::const_iterator it = data.bakFiles.begin(); it!= data.bakFiles.end(); ++it) {
        QString filename(QString::fromUtf8(it->c_str()));
        if (!QFile::exists(filename)) {
            continue;
        }
        QString nonBakFilename = filename;
        assert(filename.endsWith(QLatin1String(".bak")));
        nonBakFilename.remove(nonBakFilename.size() - 4, 4);

        // Remove the created file
        if (QFile::exists(nonBakFilename)) {
            QFile::remove(nonBakFilename);
        }
        QFile::copy(filename, nonBakFilename);
        QFile::remove(filename);
    }

    for (std::list<std::string>::const_iterator it = data.convertedFiles.begin(); it!= data.convertedFiles.end(); ++it) {
        QString filename(QString::fromUtf8(it->c_str()));
        if (!QFile::exists(filename)) {
            continue;
        }
        QFile::remove(filename);
    }
} // cleanupCreatedFiles

/**
 * @brief Force Natron to load any PyPlug in this directory (and sub-directories) so 
 * that if we convert a PyPlug they get loaded by Natron.
 **/
static void setNatronPathEnvVar(const QString& filePath)
{
    qputenv(NATRON_PATH_ENV_VAR, filePath.toStdString().c_str());
}

#ifdef Q_OS_WIN
// g++ knows nothing about wmain
// https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/
// If it fails to compile it means either UNICODE or _UNICODE is not defined (it should be in global.pri) and
// the project is not linking against -municode
extern "C" {
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char *argv[])
#endif
{
    //QCoreApplication app(argc,argv);
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
#ifdef Q_OS_WIN
        std::wstring ws(argv[i]);
        arguments.push_back(QString::fromStdWString(ws));
#else
        arguments.push_back(QString::fromUtf8(argv[i]));
#endif
    }

    // Parse app args
    QString inputPath, outputPath;
    bool recurse, replaceOriginal;
    try {
        parseArgs(arguments, &inputPath, &outputPath, &replaceOriginal, &recurse);
    } catch (const std::exception &e) {
        std::cerr << QString::fromUtf8("Error while parsing command line arguments: %1").arg(QString::fromUtf8(e.what())).toStdString() << std::endl;
        printUsage(arguments[0].toStdString());
        return 1;
    }


    QFileInfo info(inputPath);
    if (!info.exists()) {
        std::cerr << QString::fromUtf8("%1: No such file or directory.").arg(inputPath).toStdString() << std::endl;
        return 1;
    }

    ProcessData convertData;

    try {

        if (info.isDir()) {
            setNatronPathEnvVar(inputPath);
            convertDirectory(inputPath, replaceOriginal, recurse, 0, &convertData);
        } else {
            setNatronPathEnvVar(info.path());
            convertFile(inputPath, outputPath, replaceOriginal, &convertData);
        }
    } catch (const std::exception& e) {
        cleanupCreatedFiles(convertData);
        cleanupApp();
        std::cerr << QString::fromUtf8("Error: %1").arg(QString::fromUtf8(e.what())).toStdString() << std::endl;
        return 1;
    }
    cleanupApp();
    return 0;
} // main
#ifdef Q_OS_WIN
} // extern "C"
#endif
