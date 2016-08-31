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

#ifndef NATRON_BOOST_SERIALIZATION_COMPAT
#error "NATRON_BOOST_SERIALIZATION_COMPAT should be defined when compiling ProjectConverter to allow with projects older than Natron 2.2"
#endif

#include <string>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QFile>


#include <QColor>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/CLArgs.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/FStreamsSupport.h"

#include "Gui/BackdropGui.h"

#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/ProjectSerialization.h"
#include "Serialization/SerializationIO.h"
#include "Serialization/NodeGuiSerialization.h"
#include "Serialization/ProjectGuiSerialization.h"
#include "Serialization/SerializationCompat.h"

NATRON_NAMESPACE_USING


static void convertNodeGuiSerializationToNodeSerialization(const std::list<SERIALIZATION_NAMESPACE::NodeGuiSerialization>& nodeGuis, SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{
    for (std::list<SERIALIZATION_NAMESPACE::NodeGuiSerialization>::const_iterator it = nodeGuis.begin(); it != nodeGuis.end(); ++it) {
        const std::string& fullName = (it)->_nodeName;
        std::string groupMasterName;
        std::string nodeName;
        {
            std::size_t foundDot = fullName.find_last_of(".");
            if (foundDot != std::string::npos) {
                groupMasterName = fullName.substr(0, foundDot);
                nodeName = fullName.substr(foundDot + 1);
            } else {
                nodeName = fullName;
            }
        }
        if (groupMasterName == serialization->_groupFullyQualifiedScriptName && nodeName == serialization->_nodeScriptName) {
            serialization->_nodePositionCoords[0] = it->_posX;
            serialization->_nodePositionCoords[1] = it->_posY;
            serialization->_nodeSize[0] = it->_width;
            serialization->_nodeSize[1] = it->_height;
            if (it->_colorWasFound) {
                serialization->_nodeColor[0] = it->_r;
                serialization->_nodeColor[1] = it->_g;
                serialization->_nodeColor[2] = it->_b;
            } else {
                serialization->_nodeColor[0] = serialization->_nodeColor[1] = serialization->_nodeColor[2] = -1;
            }
            if (it->_hasOverlayColor) {
                serialization->_overlayColor[0] = it->_r;
                serialization->_overlayColor[1] = it->_g;
                serialization->_overlayColor[2] = it->_b;
            } else {
                serialization->_overlayColor[0] = serialization->_overlayColor[1] = serialization->_overlayColor[2] = -1;
            }
            break;
        }
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
        convertNodeGuiSerializationToNodeSerialization(original._serializedNodes, it->get());
        for (std::list<SERIALIZATION_NAMESPACE::NodeSerializationPtr>::iterator it2 = (*it)->_children.begin(); it2!=(*it)->_children.end(); ++it2) {
            convertNodeGuiSerializationToNodeSerialization(original._serializedNodes, it2->get());
        }
    }

    convertWorkspaceSerialization(original._layoutSerialization, serialization->_projectWorkspace.get());
    serialization->_projectWorkspace->_histograms = original._histograms;
    serialization->_projectWorkspace->_pythonPanels = original._pythonPanels;

    serialization->_openedPanelsOrdered = original._openedPanelsOrdered;
    
    
    
} // convertToProjectSerialization


class ConverterAppManager : public AppManager
{
public:

    ConverterAppManager()
    : AppManager()
    {

    }

private:

    virtual void loadProjectFromFileFunction(std::istream& ifile, const AppInstancePtr& app, SERIALIZATION_NAMESPACE::ProjectSerialization* obj) OVERRIDE FINAL
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
                        CreateNodeArgs args( PLUGINID_NATRON_BACKDROP, app->getProject() );
                        args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                        args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

                        NodePtr node = app->createNode(args);
                        NodeGuiIPtr gui_i = node->getNodeGui();
                        assert(gui_i);
                        BackdropGuiPtr bd = toBackdropGui( gui_i );
                        assert(bd);
                        if (bd) {
                            bd->setPos(x, y);
                            bd->resize(w, h);
                            if (labelSerialization && !labelSerialization->_values.empty()) {
                                bd->onLabelChanged( QString::fromUtf8(labelSerialization->_values[0]._value.isString.c_str()));
                            }

                            float r, g, b;
                            it->getColor(r, g, b);
                            QColor c;
                            c.setRgbF(r, g, b);
                            bd->setCurrentColor(c);
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
                
                }
                };

static boost::scoped_ptr<ConverterAppManager> app;


static void
printUsage(const std::string& programName)
{

                              /* Text must hold in 80 columns ************************************************/
    QString msg = QObject::tr("%1 usage:\n"
                              "This program can convert Natron projects (.ntp files) or workspace (.nl files)\n"
                              "made with Natron version 2.1.3 and older to the new project format used in 2.2\n\n"
                              "Program options:\n\n"
                              "-i <filename>: Converts a .ntp (Project) or .nl (Workspace) file to\n"
                              "               a newer version. Upon failure an error message will be printed.\n"
                              "               In output, if the -o option is not passed, the converted\n"
                              "               file will have the same name as the input file with the\n"
                              "               \"converted\" extension before the file extension.\n\n"
                              "-i <dir name>: Converts all .ntp files under the given directory.\n\n"
                              "-r: If the path given to -i is a directory, recursively convert all recognized\n"
                              "    files in each sub-directory.\n\n"
                              "-o <filename/dirname>: Optional: If set this will instead output the\n"
                              "                       converted file or directory to this name instead.").arg(QString::fromUtf8(programName.c_str()));
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

static void parseArgs(const QStringList& appArgs, QString* inputPath, QString* outputPath, bool* recurse)
{
    QStringList localArgs = appArgs;
    {
        QStringList::iterator foundInput = hasToken(localArgs, QLatin1String("-i"));
        if (foundInput == localArgs.end()) {
            throw std::invalid_argument(QObject::tr("Missing -i switch").toStdString());
        } else {
            ++foundInput;
            if ( foundInput != localArgs.end() ) {
                *inputPath = *foundInput;
                localArgs.erase(foundInput);
            } else {
                throw std::invalid_argument(QObject::tr("-i switch without a file/directory name").toStdString());
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
                throw std::invalid_argument(QObject::tr("-o switch without a file/directory name").toStdString());
            }
        }

    }
    *recurse = false;
    {
        QStringList::iterator foundInput = hasToken(localArgs, QLatin1String("-r"));
        if (foundInput != localArgs.end()) {
            if ( foundInput != localArgs.end() ) {
                *recurse = true;
            }
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
 * @brief Attempts to read a workspace from an old project (pre Natron 2.2) encoded with boost serialization in XML format.
 * If the file could be read, the structure is then converted to the newer format post Natron 2.2.
 * Upon failure an exception is thrown.
 **/
static void tryReadAndConvertOlderProject(const QString& filename, const QString& outFileName)
{

    AppInstancePtr instance = app->getTopLevelInstance();
    assert(instance);
    if (!instance) {
        return;
    }
    instance->loadProject(filename.toStdString());
    instance->save(outFileName.toStdString());


} // tryReadAndConvertOlderProject

static void convertFile(const QString& filename, const QString& outputFileName)
{
    bool isProjectFile = filename.endsWith(QLatin1String(".ntp"));
    bool isWorkspaceFile = filename.endsWith(QLatin1String(".nl"));

    if (!isProjectFile && !isWorkspaceFile) {
        QString message = QObject::tr("%1 does not appear to be a .ntp or .nl file.");
        throw std::invalid_argument(message.toStdString());
    }

    QString outFileName = outputFileName;
    if (outFileName.isEmpty()) {
        int foundDot = filename.lastIndexOf(QLatin1Char('.'));
        if (foundDot != -1) {
            outFileName = filename.mid(0, foundDot);
            outFileName += QLatin1String("-converted.");
            outFileName += filename.mid(foundDot + 1);
        }
    }

    if (isProjectFile) {
        tryReadAndConvertOlderProject(filename, outFileName);
    } else if (isWorkspaceFile) {
        boost::shared_ptr<SERIALIZATION_NAMESPACE::WorkspaceSerialization> workspace;
        // Read old file
        {
            FStreamsSupport::ifstream ifile;
            FStreamsSupport::open(&ifile, filename.toStdString());
            if (!ifile) {
                QString message = QObject::tr("Could not open %1").arg(filename);
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
                QString message = QObject::tr("Could not open %1").arg(outFileName);
                throw std::invalid_argument(message.toStdString());
            }

            SERIALIZATION_NAMESPACE::write(ofile, *workspace);

        }

    }


} // convertFile

static void convertDirectory(const QString& dirPath, const QString& outputDirPath, bool recurse, unsigned int recursionLevel)
{
    QDir d(dirPath);
    if (!d.exists()) {
        QString message = QObject::tr("%1: No such file or directory").arg(dirPath);
        throw std::invalid_argument(message.toStdString());
    }

    QString newDirPath = outputDirPath;
    if (outputDirPath.isEmpty()) {
        newDirPath = dirPath;
        if (recursionLevel == 0) {
            newDirPath += QLatin1String("_converted");
        }
    }

    QStringList files = d.entryList(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    for (QStringList::const_iterator it = files.begin(); it!=files.end(); ++it) {

        QString absoluteFilePath = d.absolutePath() + QLatin1Char('/') + *it;
        QDir subDir(absoluteFilePath);
        if (subDir.exists()) {
            if (recurse) {
                QString childPath = newDirPath + QLatin1Char('/') + *it;
                convertDirectory(absoluteFilePath, childPath, recurse, recursionLevel + 1);
            }
            continue;
        }

        if (it->endsWith(QLatin1String(".ntp")) || it->endsWith(QLatin1String(".nl"))) {
            convertFile(absoluteFilePath, QString());
        }
    }
} // convertDirectory

int
main(int argc,
     char *argv[])
{
    //QCoreApplication app(argc,argv);
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments.push_back(QString::fromUtf8(argv[i]));
    }

    // Parse app args
    QString inputPath, outputPath;
    bool recurse;
    try {
        parseArgs(arguments, & inputPath, &outputPath, &recurse);
    } catch (const std::exception &e) {
        std::cerr << QObject::tr("Error while parsing command line arguments: %1").arg(QString::fromUtf8(e.what())).toStdString() << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Create Natron instance
    app.reset(new ConverterAppManager);
    int nArgc = 0;
    app->load(nArgc, 0, CLArgs());
    assert(app->getTopLevelInstance());



    QDir d(inputPath);
    if (d.exists()) {
        // This is a directory
        convertDirectory(inputPath, outputPath, recurse, 0);
    } else {
        if (!QFile::exists(inputPath)) {
            std::cerr << QObject::tr("%1: No such file or directory").arg(inputPath).toStdString() << std::endl;
            return 1;
        }
        try {
            convertFile(inputPath, outputPath);
        } catch (const std::exception& e) {
            std::cerr << QObject::tr("Error: %1").arg(QString::fromUtf8(e.what())).toStdString() << std::endl;
            return 1;
        }
    }
    app->quitApplication();
    app.reset();
    return 0;
} // main

