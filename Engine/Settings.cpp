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

#include "Settings.h"

#include <QtCore/QDebug>
#include <QDir>
#include <QSettings>
#include <QThreadPool>
#include <QThread>
#include "Global/MemoryInfo.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/LibraryBinary.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobFactory.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "SequenceParsing.h"

#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

using namespace Natron;


Settings::Settings(AppInstance* appInstance)
    : KnobHolder(appInstance)
      , _wereChangesMadeSinceLastSave(false)
      , _restoringSettings(false)
{
}

static QStringList
getDefaultOcioConfigPaths()
{
    QString binaryPath = appPTR->getApplicationBinaryPath();

    if ( !binaryPath.isEmpty() ) {
        binaryPath += QDir::separator();
    }
#ifdef __NATRON_LINUX__
    QStringList ret;
    ret.push_back( QString("/usr/share/OpenColorIO-Configs") );
    ret.push_back( QString(binaryPath + "../share/OpenColorIO-Configs") );
    ret.push_back( QString(binaryPath + "../Resources/OpenColorIO-Configs") );

    return ret;
#elif defined(__NATRON_WIN32__)

    return QStringList( QString(binaryPath + "../Resources/OpenColorIO-Configs") );
#elif defined(__NATRON_OSX__)

    return QStringList( QString(binaryPath + "../Resources/OpenColorIO-Configs") );
#endif
}

#if defined(WINDOWS)
// defined in ofxhPluginCache.cpp
const TCHAR * getStdOFXPluginPath(const std::string &hostId);
#endif

void
Settings::initializeKnobs()
{
    _generalTab = Natron::createKnob<Page_Knob>(this, "General");

    _checkForUpdates = Natron::createKnob<Bool_Knob>(this, "Always check for updates on start-up");
    _checkForUpdates->setName("checkForUpdates");
    _checkForUpdates->setAnimationEnabled(false);
    _checkForUpdates->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " will check for new updates on start-up of the application.");
    _generalTab->addKnob(_checkForUpdates);

    _autoSaveDelay = Natron::createKnob<Int_Knob>(this, "Auto-save trigger delay");
    _autoSaveDelay->setName("autoSaveDelay");
    _autoSaveDelay->setAnimationEnabled(false);
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setMinimum(0);
    _autoSaveDelay->setMaximum(60);
    _autoSaveDelay->setHintToolTip("The number of seconds after an event that " NATRON_APPLICATION_NAME " should wait before "
                                   " auto-saving. Note that if a render is in progress, " NATRON_APPLICATION_NAME " will "
                                   " wait until it is done to actually auto-save.");
    _generalTab->addKnob(_autoSaveDelay);


    _linearPickers = Natron::createKnob<Bool_Knob>(this, "Linear color pickers");
    _linearPickers->setName("linearPickers");
    _linearPickers->setAnimationEnabled(false);
    _linearPickers->setHintToolTip("When activated, all colors picked from the color parameters will be converted"
                                   " to linear before being fetched. Otherwise they will be in the same color-space "
                                   " as the viewer they were picked from.");
    _generalTab->addKnob(_linearPickers);

    _numberOfThreads = Natron::createKnob<Int_Knob>(this, "Number of render threads");
    _numberOfThreads->setName("noRenderThreads");
    _numberOfThreads->setAnimationEnabled(false);
    QString numberOfThreadsToolTip = QString("Controls how many threads " NATRON_APPLICATION_NAME " should use to render. \n"
                                             "-1: Disable multi-threading totally (useful for debug) \n"
                                             "0: Guess from the number of cores. The ideal threads count for your hardware is %1.").arg( QThread::idealThreadCount() );
    _numberOfThreads->setHintToolTip( numberOfThreadsToolTip.toStdString() );
    _numberOfThreads->disableSlider();
    _numberOfThreads->setMinimum(-1);
    _numberOfThreads->setDisplayMinimum(-1);
    _generalTab->addKnob(_numberOfThreads);

    _renderInSeparateProcess = Natron::createKnob<Bool_Knob>(this, "Render in a separate process");
    _renderInSeparateProcess->setName("renderNewProcess");
    _renderInSeparateProcess->setAnimationEnabled(false);
    _renderInSeparateProcess->setHintToolTip("If true, " NATRON_APPLICATION_NAME " will render (using the write nodes) in "
                                             "a separate process. Disabling it is most helpful for the dev team.");
    _generalTab->addKnob(_renderInSeparateProcess);

    _autoPreviewEnabledForNewProjects = Natron::createKnob<Bool_Knob>(this, "Auto-preview enabled by default for new projects");
    _autoPreviewEnabledForNewProjects->setName("enableAutoPreviewNewProjects");
    _autoPreviewEnabledForNewProjects->setAnimationEnabled(false);
    _autoPreviewEnabledForNewProjects->setHintToolTip("If checked then when creating a new project, the Auto-preview option"
                                                      " will be enabled.");
    _generalTab->addKnob(_autoPreviewEnabledForNewProjects);


    _firstReadSetProjectFormat = Natron::createKnob<Bool_Knob>(this, "First image read set project format");
    _firstReadSetProjectFormat->setName("autoProjectFormat");
    _firstReadSetProjectFormat->setAnimationEnabled(false);
    _firstReadSetProjectFormat->setHintToolTip("If checked, the first image you read in the project will set the project format to the "
                                               "image size.");
    _generalTab->addKnob(_firstReadSetProjectFormat);
    
    _fixPathsOnProjectPathChanged = Natron::createKnob<Bool_Knob>(this, "Auto fix relative file-paths");
    _fixPathsOnProjectPathChanged->setAnimationEnabled(false);
    _fixPathsOnProjectPathChanged->setHintToolTip("If checked, when a project-path changes (either the name or the value pointed to), " NATRON_APPLICATION_NAME " will check all file-path parameters in the project and try to fix them.");
    _fixPathsOnProjectPathChanged->setName("autoFixRelativePaths");
    _generalTab->addKnob(_fixPathsOnProjectPathChanged);
    
    _maxPanelsOpened = Natron::createKnob<Int_Knob>(this, "Maximum number of node settings panels opened");
    _maxPanelsOpened->setName("maxPanels");
    _maxPanelsOpened->setHintToolTip("This property holds the number of node settings pnaels that can be "
                                     "held by the properties dock at the same time."
                                     "The special value of 0 indicates there can be an unlimited number of panels opened.");
    _maxPanelsOpened->setAnimationEnabled(false);
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setMinimum(0);
    _maxPanelsOpened->setMaximum(100);
    _generalTab->addKnob(_maxPanelsOpened);

    _useCursorPositionIncrements = Natron::createKnob<Bool_Knob>(this, "Cursor position aware value fields");
    _useCursorPositionIncrements->setName("cursorPositionAwareFields");
    _useCursorPositionIncrements->setHintToolTip("When enabled, incrementing the value fields of parameters with the "
                                                 "mouse wheel or with arrow keys will increment the digits on the right "
                                                 "of the cursor. \n"
                                                 "When disabled, the value fields are incremented given what the plug-in "
                                                 "decided it should be. You can alter this increment by holding "
                                                 "shift (x10) or control (/10) while incrementing.");
    _useCursorPositionIncrements->setAnimationEnabled(false);
    _generalTab->addKnob(_useCursorPositionIncrements);

    _defaultLayoutFile = Natron::createKnob<File_Knob>(this, "Default layout file");
    _defaultLayoutFile->setName("defaultLayout");
    _defaultLayoutFile->setHintToolTip("When set, " NATRON_APPLICATION_NAME " will use the layout indicated in the file "
                                       "as default layout for new projects. You can export/import a layout to/from a file "
                                       "from the Layout menu. If empty, the default layout of the application will be used.");
    _defaultLayoutFile->setAnimationEnabled(false);
    _generalTab->addKnob(_defaultLayoutFile);

    _renderOnEditingFinished = Natron::createKnob<Bool_Knob>(this, "Render on editing finished only");
    _renderOnEditingFinished->setName("renderOnEditingFinished");
    _renderOnEditingFinished->setHintToolTip("When checked, the render engine will run only when you finish editing "
                                             "a curve or a parameter. For example, for curves editing, that is only when you release the button"
                                             " of the mouse the render will be triggered."
                                             "This also works for sliders and values of parameters."
                                             " Note that this setting doesn't apply to roto splines editing.");
    _renderOnEditingFinished->setAnimationEnabled(false);
    _generalTab->addKnob(_renderOnEditingFinished);

    _generalTab->addKnob( Natron::createKnob<Separator_Knob>(this, "OpenFX Plugins") );

    _extraPluginPaths = Natron::createKnob<Path_Knob>(this, "Extra plugins search paths");
    _extraPluginPaths->setName("extraPluginsSearchPaths");
    _extraPluginPaths->setHintToolTip( std::string("Extra search paths where " NATRON_APPLICATION_NAME " should scan for plug-ins. "
                                                   "Extra plugins search paths can also be specified using the OFX_PLUGIN_PATH environment variable.\n"
                                                   "The priority order for system-wide plugins, from high to low, is:\n"
                                                   "- plugins found in OFX_PLUGIN_PATH\n"
                                                   "- plugins found in \""
#if defined(WINDOWS)
                                                   ) + getStdOFXPluginPath("") +
                                       std::string("\" and \"C:\\Program Files\\Common Files\\OFX\\Plugins"
#endif
#if defined(__linux__) || defined(__FreeBSD__)
                                                   "/usr/OFX/Plugins"
#endif
#if defined(__APPLE__)
                                                   "/Library/OFX/Plugins"
#endif
                                                   "\".\n"
                                                   "Plugins bundled with the binary distribution of Natron may have either "
                                                   "higher or lower priority, depending on the \"Prefer bundled plug-ins over "
                                                   "system-wide plug-ins\" setting.\n"
                                                   "Any change will take effect on the next launch of " NATRON_APPLICATION_NAME ".") );
    _extraPluginPaths->setMultiPath(true);
    _generalTab->addKnob(_extraPluginPaths);

    _loadBundledPlugins = Natron::createKnob<Bool_Knob>(this, "Use bundled plug-ins");
    _loadBundledPlugins->setName("useBundledPlugins");
    _loadBundledPlugins->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " will also use the plug-ins bundled "
                                        "with the binary distribution.\n"
                                        "When unchecked, only system-wide plug-ins will be loaded (more information can be "
                                        "found in the help for the \"Extra plugins search paths\" setting).");
    _loadBundledPlugins->setAnimationEnabled(false);
    _generalTab->addKnob(_loadBundledPlugins);

    _preferBundledPlugins = Natron::createKnob<Bool_Knob>(this, "Prefer bundled plug-ins over system-wide plug-ins");
    _preferBundledPlugins->setName("preferBundledPlugins");
    _preferBundledPlugins->setHintToolTip("When checked, and if \"Use bundled plug-ins\" is also checked, plug-ins bundled with the "
                                          NATRON_APPLICATION_NAME " binary distribution will take precedence over system-wide plug-ins.");
    _preferBundledPlugins->setAnimationEnabled(false);
    _generalTab->addKnob(_preferBundledPlugins);


    _hostName = Natron::createKnob<String_Knob>(this, "Host name");
    _hostName->setName("hostName");
    _hostName->setHintToolTip("This is the name of the application as it appears to the OpenFX plug-ins. "
                              "Changing it to the name of another application can help loading some plug-ins which "
                              "restrict their usage to some particular application. This is kind of a hack and you shoud leave "
                              "it to its original value, unless you know a plug-in that should load but doesn't. "
                              "Changing this will take effect on the next application launch and only if you cleared "
                              "the OpenFX plug-ins cache from the Cache menu. "
                              "Here is a list of known OpenFX hosts: \n"
                              "uk.co.thefoundry.nuke \n"
                              "com.eyeonline.Fusion \n"
                              "com.sonycreativesoftware.vegas \n"
                              "Autodesk Toxik \n"
                              "Assimilator \n"
                              "Dustbuster \n"
                              "DaVinciResolve \n"
                              "Mistika \n"
                              "com.apple.shake \n"
                              "Baselight \n"
                              "IRIDAS Framecycler \n"
                              "Ramen \n"
                              "\n"
                              "And finally the default host name being: \n"
                              NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);
    _hostName->setAnimationEnabled(false);
    _generalTab->addKnob(_hostName);

    boost::shared_ptr<Page_Knob> ocioTab = Natron::createKnob<Page_Knob>(this, "OpenColorIO");


    _ocioConfigKnob = Natron::createKnob<Choice_Knob>(this, "OpenColorIO config");
    _ocioConfigKnob->setName("ocioConfig");
    _ocioConfigKnob->setAnimationEnabled(false);

    std::vector<std::string> configs;
    int defaultIndex = 0;
    QStringList defaultOcioConfigsPaths = getDefaultOcioConfigPaths();
    for (int i = 0; i < defaultOcioConfigsPaths.size(); ++i) {
        QDir ocioConfigsDir(defaultOcioConfigsPaths[i]);
        if ( ocioConfigsDir.exists() ) {
            QStringList entries = ocioConfigsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
            for (int j = 0; j < entries.size(); ++j) {
                if ( entries[j].contains("nuke-default") ) {
                    defaultIndex = j;
                }
                configs.push_back( entries[j].toStdString() );
            }

            break; //if we found 1 OpenColorIO-Configs directory, skip the next
        }
    }
    configs.push_back(NATRON_CUSTOM_OCIO_CONFIG_NAME);
    _ocioConfigKnob->populateChoices(configs);
    _ocioConfigKnob->setDefaultValue(defaultIndex,0);
    _ocioConfigKnob->setHintToolTip("Select the OpenColorIO config you would like to use globally for all "
                                    "operators that use OpenColorIO. Note that changing it will set the OCIO "
                                    "environment variable, hence any change to this parameter will be "
                                    "taken into account on the next application launch. "
                                    "When custom config is selected, you can use the custom OpenColorIO config file "
                                    "setting to point to the config you would like to use.");

    ocioTab->addKnob(_ocioConfigKnob);

    _customOcioConfigFile = Natron::createKnob<File_Knob>(this, "Custom OpenColorIO config file");
    _customOcioConfigFile->setName("ocioCustomConfigFile");
    _customOcioConfigFile->setAllDimensionsEnabled(false);
    _customOcioConfigFile->setHintToolTip("To use this, set the OpenColorIO config to custom config a point "
                                          "to a custom OpenColorIO config file (.ocio).");
    ocioTab->addKnob(_customOcioConfigFile);

    _viewersTab = Natron::createKnob<Page_Knob>(this, "Viewers");

    _texturesMode = Natron::createKnob<Choice_Knob>(this, "Viewer textures bit depth");
    _texturesMode->setName("texturesBitDepth");
    _texturesMode->setAnimationEnabled(false);
    std::vector<std::string> textureModes;
    std::vector<std::string> helpStringsTextureModes;
    textureModes.push_back("Byte");
    helpStringsTextureModes.push_back("Viewer's post-process like color-space conversion will be done\n"
                                      "by the software. Cached textures will be smaller in  the viewer cache.");
    textureModes.push_back("16bits half-float");
    helpStringsTextureModes.push_back("Not available yet. Similar to 32bits fp.");
    textureModes.push_back("32bits floating-point");
    helpStringsTextureModes.push_back("Viewer's post-process like color-space conversion will be done\n"
                                      "by the hardware using GLSL. Cached textures will be larger in the viewer cache.");
    _texturesMode->populateChoices(textureModes,helpStringsTextureModes);
    _texturesMode->setHintToolTip("Bitdepth of the viewer textures used for rendering."
                                  " Hover each option with the mouse for a more detailed comprehension.");
    _viewersTab->addKnob(_texturesMode);

    _powerOf2Tiling = Natron::createKnob<Int_Knob>(this, "Viewer tile size is 2 to the power of...");
    _powerOf2Tiling->setName("viewerTiling");
    _powerOf2Tiling->setHintToolTip("The power of 2 of the tiles size used by the Viewer to render."
                                    " A high value means that the viewer will usually render big tiles, which means"
                                    " you have good chances when panning/zooming to find an already rendered texture in the cache."
                                    " On the other hand a small value means that the tiles will be closer to the real size of"
                                    " images to be rendered and as a result of this there might be more cache misses." );
    _powerOf2Tiling->setMinimum(4);
    _powerOf2Tiling->setDisplayMinimum(4);
    _powerOf2Tiling->setMaximum(9);
    _powerOf2Tiling->setDisplayMaximum(9);

    _powerOf2Tiling->setAnimationEnabled(false);
    _viewersTab->addKnob(_powerOf2Tiling);
    
    _checkerboardTileSize = Natron::createKnob<Int_Knob>(this, "Checkerboard tile size (pixels)");
    _checkerboardTileSize->setName("checkerboardTileSize");
    _checkerboardTileSize->setMinimum(1);
    _checkerboardTileSize->setAnimationEnabled(false);
    _checkerboardTileSize->setHintToolTip("The size in pixel of 1 tile of the checkerboard in pixels.");
    _viewersTab->addKnob(_checkerboardTileSize);
    
    _checkerboardColor1 = Natron::createKnob<Color_Knob>(this, "Checkerboard color 1",4);
    _checkerboardColor1->setName("checkerboardColor1");
    _checkerboardColor1->setAnimationEnabled(false);
    _checkerboardColor1->setHintToolTip("The first color used by the checkerboard");
    _viewersTab->addKnob(_checkerboardColor1);
    
    _checkerboardColor2 = Natron::createKnob<Color_Knob>(this, "Checkerboard color 2",4);
    _checkerboardColor2->setName("checkerboardColor2");
    _checkerboardColor2->setAnimationEnabled(false);
    _checkerboardColor2->setHintToolTip("The second color used by the checkerboard");
    _viewersTab->addKnob(_checkerboardColor2);
    
    
    /////////// Nodegraph tab
    _nodegraphTab = Natron::createKnob<Page_Knob>(this, "Nodegraph");

    _snapNodesToConnections = Natron::createKnob<Bool_Knob>(this, "Snap to node");
    _snapNodesToConnections->setName("enableSnapToNode");
    _snapNodesToConnections->setHintToolTip("When moving nodes on the node graph, snap them to positions where it lines them up "
                                            "with the inputs and output nodes.");
    _snapNodesToConnections->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_snapNodesToConnections);

    _useBWIcons = Natron::createKnob<Bool_Knob>(this, "Use black & white toolbutton icons");
    _useBWIcons->setName("useBwIcons");
    _useBWIcons->setHintToolTip("When checked, the tools icons in the left toolbar will be in black and white. Changing this takes "
                                "effect upon the next launch of the application.");
    _useBWIcons->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_useBWIcons);

    _useNodeGraphHints = Natron::createKnob<Bool_Knob>(this, "Use connection hints");
    _useNodeGraphHints->setName("useHints");
    _useNodeGraphHints->setHintToolTip("When checked, moving a node which is not connected to anything to arrows "
                                       "nearby will display a hint for possible connections. Releasing the mouse on such a "
                                       "hint will perform the connection for you.");
    _useNodeGraphHints->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_useNodeGraphHints);

    _maxUndoRedoNodeGraph = Natron::createKnob<Int_Knob>(this, "Maximum undo/redo for the node graph");
    _maxUndoRedoNodeGraph->setName("maxUndoRedo");
    _maxUndoRedoNodeGraph->setAnimationEnabled(false);
    _maxUndoRedoNodeGraph->disableSlider();
    _maxUndoRedoNodeGraph->setMinimum(0);
    _maxUndoRedoNodeGraph->setMaximum(100);
    _maxUndoRedoNodeGraph->setHintToolTip("Set the maximum of events related to the node graph " NATRON_APPLICATION_NAME
                                          " will remember. Past this limit, older events will be deleted permanantly "
                                          " allowing to re-use the RAM for better purposes since Nodes can hold a significant amount of RAM. \n"
                                          "Changing this value will clear the undo/redo stack.");
    _nodegraphTab->addKnob(_maxUndoRedoNodeGraph);


    _disconnectedArrowLength = Natron::createKnob<Int_Knob>(this, "Disconnected arrow length");
    _disconnectedArrowLength->setName("disconnectedArrowLength");
    _disconnectedArrowLength->setAnimationEnabled(false);
    _disconnectedArrowLength->setHintToolTip("The size of a disconnected node input arrow in pixels.");
    _disconnectedArrowLength->disableSlider();

    _nodegraphTab->addKnob(_disconnectedArrowLength);

    _defaultNodeColor = Natron::createKnob<Color_Knob>(this, "Default node color",3);
    _defaultNodeColor->setName("defaultNodeColor");
    _defaultNodeColor->setAnimationEnabled(false);
    _defaultNodeColor->setHintToolTip("This is default color which nodes have when created.");

    _nodegraphTab->addKnob(_defaultNodeColor);

    _defaultSelectedNodeColor = Natron::createKnob<Color_Knob>(this, "Default selected node color",3);
    _defaultSelectedNodeColor->setName("selectedNodeColor");
    _defaultSelectedNodeColor->setAnimationEnabled(false);
    _defaultSelectedNodeColor->setHintToolTip("This is default color which the selected nodes have.");

    _nodegraphTab->addKnob(_defaultSelectedNodeColor);


    _defaultBackdropColor =  Natron::createKnob<Color_Knob>(this, "Default backdrop color",3);
    _defaultBackdropColor->setName("backdropColor");
    _defaultBackdropColor->setAnimationEnabled(false);
    _defaultBackdropColor->setHintToolTip("This is default color which backdrop nodes have when created.");
    _nodegraphTab->addKnob(_defaultBackdropColor);

    ///////////////////DEFAULT GROUP COLORS

    _defaultReaderColor =  Natron::createKnob<Color_Knob>(this, PLUGIN_GROUP_IMAGE_READERS,3);
    _defaultReaderColor->setName("readerColor");
    _defaultReaderColor->setAnimationEnabled(false);
    _defaultReaderColor->setHintToolTip("This is default color which reader nodes have when created.");
    _nodegraphTab->addKnob(_defaultReaderColor);

    _defaultWriterColor =  Natron::createKnob<Color_Knob>(this, PLUGIN_GROUP_IMAGE_WRITERS,3);
    _defaultWriterColor->setName("writerColor");
    _defaultWriterColor->setAnimationEnabled(false);
    _defaultWriterColor->setHintToolTip("This is default color which writer nodes have when created.");
    _nodegraphTab->addKnob(_defaultWriterColor);

    _defaultGeneratorColor =  Natron::createKnob<Color_Knob>(this, "Generators",3);
    _defaultGeneratorColor->setName("generatorColor");
    _defaultGeneratorColor->setAnimationEnabled(false);
    _defaultGeneratorColor->setHintToolTip("This is default color which generator nodes have when created.");
    _nodegraphTab->addKnob(_defaultGeneratorColor);

    _defaultColorGroupColor =  Natron::createKnob<Color_Knob>(this, "Color group",3);
    _defaultColorGroupColor->setName("colorNodesColor");
    _defaultColorGroupColor->setAnimationEnabled(false);
    _defaultColorGroupColor->setHintToolTip("This is default color which color processing nodes have when created.");
    _nodegraphTab->addKnob(_defaultColorGroupColor);

    _defaultFilterGroupColor =  Natron::createKnob<Color_Knob>(this, "Filter group",3);
    _defaultFilterGroupColor->setName("filterNodesColor");
    _defaultFilterGroupColor->setAnimationEnabled(false);
    _defaultFilterGroupColor->setHintToolTip("This is default color which filter nodes have when created.");
    _nodegraphTab->addKnob(_defaultFilterGroupColor);

    _defaultTransformGroupColor =  Natron::createKnob<Color_Knob>(this, "Transform group",3);
    _defaultTransformGroupColor->setName("transformNodesColor");
    _defaultTransformGroupColor->setAnimationEnabled(false);
    _defaultTransformGroupColor->setHintToolTip("This is default color which transform nodes have when created.");
    _nodegraphTab->addKnob(_defaultTransformGroupColor);

    _defaultTimeGroupColor =  Natron::createKnob<Color_Knob>(this, "Time group",3);
    _defaultTimeGroupColor->setName("timeNodesColor");
    _defaultTimeGroupColor->setAnimationEnabled(false);
    _defaultTimeGroupColor->setHintToolTip("This is default color which time processing nodes have when created.");
    _nodegraphTab->addKnob(_defaultTimeGroupColor);

    _defaultDrawGroupColor =  Natron::createKnob<Color_Knob>(this, "Draw group",3);
    _defaultDrawGroupColor->setName("drawNodesColor");
    _defaultDrawGroupColor->setAnimationEnabled(false);
    _defaultDrawGroupColor->setHintToolTip("This is default color which draw nodes have when created.");
    _nodegraphTab->addKnob(_defaultDrawGroupColor);

    _defaultKeyerGroupColor =  Natron::createKnob<Color_Knob>(this, "Keyer group",3);
    _defaultKeyerGroupColor->setName("keyerNodesColor");
    _defaultKeyerGroupColor->setAnimationEnabled(false);
    _defaultKeyerGroupColor->setHintToolTip("This is default color which keying nodes have when created.");
    _nodegraphTab->addKnob(_defaultKeyerGroupColor);

    _defaultChannelGroupColor =  Natron::createKnob<Color_Knob>(this, "Channel group",3);
    _defaultChannelGroupColor->setName("channelNodesColor");
    _defaultChannelGroupColor->setAnimationEnabled(false);
    _defaultChannelGroupColor->setHintToolTip("This is default color which channel processing nodes have when created.");
    _nodegraphTab->addKnob(_defaultChannelGroupColor);

    _defaultMergeGroupColor =  Natron::createKnob<Color_Knob>(this, "Merge group",3);
    _defaultMergeGroupColor->setName("defaultMergeColor");
    _defaultMergeGroupColor->setAnimationEnabled(false);
    _defaultMergeGroupColor->setHintToolTip("This is default color which merge  nodes have when created.");
    _nodegraphTab->addKnob(_defaultMergeGroupColor);

    _defaultViewsGroupColor =  Natron::createKnob<Color_Knob>(this, "Views group",3);
    _defaultViewsGroupColor->setName("defaultViewsColor");
    _defaultViewsGroupColor->setAnimationEnabled(false);
    _defaultViewsGroupColor->setHintToolTip("This is default color which multi-view nodes have when created.");
    _nodegraphTab->addKnob(_defaultViewsGroupColor);

    _defaultDeepGroupColor =  Natron::createKnob<Color_Knob>(this, "Deep group",3);
    _defaultDeepGroupColor->setName("defaultDeepColor");
    _defaultDeepGroupColor->setAnimationEnabled(false);
    _defaultDeepGroupColor->setHintToolTip("This is default color which deep compositing nodes have when created.");
    _nodegraphTab->addKnob(_defaultDeepGroupColor);

    /////////// Caching tab
    _cachingTab = Natron::createKnob<Page_Knob>(this, "Caching");

    _maxRAMPercent = Natron::createKnob<Int_Knob>(this, "Maximum system's RAM for caching (% system total RAM)");
    _maxRAMPercent->setName("maxRAMPercent");
    _maxRAMPercent->setAnimationEnabled(false);
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    std::string ramHint("This setting indicates the percentage of the system's total RAM "
                        NATRON_APPLICATION_NAME "'s caches are allowed to use."
                        " Your system has ");
    ramHint.append( printAsRAM( getSystemTotalRAM() ).toStdString() );
    ramHint.append(" of RAM.");
    if ( isApplication32Bits() ) {
        ramHint.append("\n The version of " NATRON_APPLICATION_NAME " you are running is 32 bits, that means the system's total RAM "
                       "will be limited to 4Gib. In this case the percentage of RAM allowed will be min(4Gib,SystemtotalRAM) * MaxRamPercent.");
    }

    _maxRAMPercent->setHintToolTip(ramHint);
    _maxRAMPercent->turnOffNewLine();
    _cachingTab->addKnob(_maxRAMPercent);

    _maxRAMLabel = Natron::createKnob<String_Knob>(this, " = ");
    _maxRAMLabel->setName("maxRamLabel");
    _maxRAMLabel->setIsPersistant(false);
    _maxRAMLabel->setAsLabel();
    _maxRAMLabel->setAnimationEnabled(false);
    _cachingTab->addKnob(_maxRAMLabel);

    _maxPlayBackPercent = Natron::createKnob<Int_Knob>(this, "Playback cache RAM percentage (% maximum RAM for caching");
    _maxPlayBackPercent->setName("maxPlaybackPercent");
    _maxPlayBackPercent->setAnimationEnabled(false);
    _maxPlayBackPercent->setMinimum(0);
    _maxPlayBackPercent->setMaximum(100);
    _maxPlayBackPercent->setHintToolTip("This setting indicates the percentage of the Maximum system's RAM for caching"
                                        " dedicated for the playback cache. Normally you shouldn't change this value"
                                        " as it is tuned automatically by the Maximum system's RAM for caching, but"
                                        " this is made possible for convenience.");
    _maxPlayBackPercent->turnOffNewLine();
    _cachingTab->addKnob(_maxPlayBackPercent);

    _maxPlaybackLabel = Natron::createKnob<String_Knob>(this, " = ");
    _maxPlaybackLabel->setName("maxPlaybackLabel");
    _maxPlaybackLabel->setIsPersistant(false);
    _maxPlaybackLabel->setAsLabel();
    _maxPlaybackLabel->setAnimationEnabled(false);
    _cachingTab->addKnob(_maxPlaybackLabel);

    _unreachableRAMPercent = Natron::createKnob<Int_Knob>(this, "System RAM to keep free (% system total RAM)");
    _unreachableRAMPercent->setName("unreachableRAMPercent");
    _unreachableRAMPercent->setAnimationEnabled(false);
    _unreachableRAMPercent->setMinimum(0);
    _unreachableRAMPercent->setMaximum(90);
    _unreachableRAMPercent->setHintToolTip("This determines how much system's RAM Natron's caches should keep free. "
                                           "This overrides completely the 2 settings above in a sense that when falling "
                                           "under that threshold Natron's caches will start recycling memory instead of growing "
                                           "regardless of their size limit. "
                                           "A reasonable value should be set for it allowing the caches to stay in physical RAM "
                                           "and avoid being swapped-out on disk. This value should reflect the amount of memory "
                                           "you want to keep available on your computer for other usage. "
                                           "Failing to supply a good value will result in caches being swapped on disk "
                                           "hence bad performances.");
    _unreachableRAMPercent->turnOffNewLine();
    _cachingTab->addKnob(_unreachableRAMPercent);
    _unreachableRAMLabel = Natron::createKnob<String_Knob>(this, " = ");
    _unreachableRAMLabel->setName("unreachableRAMLabel");
    _unreachableRAMLabel->setIsPersistant(false);
    _unreachableRAMLabel->setAsLabel();
    _unreachableRAMLabel->setAnimationEnabled(false);
    _cachingTab->addKnob(_unreachableRAMLabel);

    _maxDiskCacheGB = Natron::createKnob<Int_Knob>(this, "Maximum disk cache size (GB)");
    _maxDiskCacheGB->setName("maxDiskCache");
    _maxDiskCacheGB->setAnimationEnabled(false);
    _maxDiskCacheGB->setMinimum(0);
    _maxDiskCacheGB->setMaximum(100);
    _maxDiskCacheGB->setHintToolTip("The maximum disk space the caches can use. (in GB)");
    _cachingTab->addKnob(_maxDiskCacheGB);


    ///readers & writers settings are created in a postponed manner because we don't know
    ///their dimension yet. See populateReaderPluginsAndFormats & populateWriterPluginsAndFormats

    _readersTab = Natron::createKnob<Page_Knob>(this, PLUGIN_GROUP_IMAGE_READERS);
    _readersTab->setName("readersTab");

    _writersTab = Natron::createKnob<Page_Knob>(this, PLUGIN_GROUP_IMAGE_WRITERS);
    _writersTab->setName("writersTab");

    setDefaultValues();
} // initializeKnobs

void
Settings::setCachingLabels()
{
    int maxPlaybackPercent = _maxPlayBackPercent->getValue();
    int maxTotalRam = _maxRAMPercent->getValue();
    U64 systemTotalRam = getSystemTotalRAM();
    U64 maxRAM = (U64)( ( (double)maxTotalRam / 100. ) * systemTotalRam );

    _maxRAMLabel->setValue(printAsRAM(maxRAM).toStdString(), 0);
    _maxPlaybackLabel->setValue(printAsRAM( (U64)( maxRAM * ( (double)maxPlaybackPercent / 100. ) ) ).toStdString(), 0);

    _unreachableRAMLabel->setValue(printAsRAM( (double)systemTotalRam * ( (double)_unreachableRAMPercent->getValue() / 100. ) ).toStdString(), 0);
}

void
Settings::setDefaultValues()
{
    beginKnobsValuesChanged(Natron::PLUGIN_EDITED);
    _hostName->setDefaultValue(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);
    _checkForUpdates->setDefaultValue(false);
    _autoSaveDelay->setDefaultValue(5, 0);
    _maxUndoRedoNodeGraph->setDefaultValue(20, 0);
    _linearPickers->setDefaultValue(true,0);
    _snapNodesToConnections->setDefaultValue(true);
    _useBWIcons->setDefaultValue(false);
    _useNodeGraphHints->setDefaultValue(true);
    _numberOfThreads->setDefaultValue(0,0);
    _renderInSeparateProcess->setDefaultValue(true,0);
    _autoPreviewEnabledForNewProjects->setDefaultValue(true,0);
    _firstReadSetProjectFormat->setDefaultValue(true);
    _fixPathsOnProjectPathChanged->setDefaultValue(false);
    _maxPanelsOpened->setDefaultValue(10,0);
    _useCursorPositionIncrements->setDefaultValue(true);
    _renderOnEditingFinished->setDefaultValue(false);
    _extraPluginPaths->setDefaultValue("",0);
    _preferBundledPlugins->setDefaultValue(true);
    _loadBundledPlugins->setDefaultValue(true);
    _texturesMode->setDefaultValue(0,0);
    _powerOf2Tiling->setDefaultValue(8,0);
    _checkerboardTileSize->setDefaultValue(5);
    _checkerboardColor1->setDefaultValue(0.5,0);
    _checkerboardColor1->setDefaultValue(0.5,1);
    _checkerboardColor1->setDefaultValue(0.5,2);
    _checkerboardColor1->setDefaultValue(0.5,3);
    _checkerboardColor2->setDefaultValue(0.,0);
    _checkerboardColor2->setDefaultValue(0.,1);
    _checkerboardColor2->setDefaultValue(0.,2);
    _checkerboardColor2->setDefaultValue(0.,3);

    _maxRAMPercent->setDefaultValue(50,0);
    _maxPlayBackPercent->setDefaultValue(25,0);
    _unreachableRAMPercent->setDefaultValue(5);
    _maxDiskCacheGB->setDefaultValue(10,0);
    setCachingLabels();
    _defaultNodeColor->setDefaultValue(0.6,0);
    _defaultNodeColor->setDefaultValue(0.6,1);
    _defaultNodeColor->setDefaultValue(0.6,2);
    _defaultSelectedNodeColor->setDefaultValue(0.7,0);
    _defaultSelectedNodeColor->setDefaultValue(0.6,1);
    _defaultSelectedNodeColor->setDefaultValue(0.3,2);
    _defaultBackdropColor->setDefaultValue(0.5,0);
    _defaultBackdropColor->setDefaultValue(0.5,1);
    _defaultBackdropColor->setDefaultValue(0.2,2);
    _disconnectedArrowLength->setDefaultValue(30);

    _defaultGeneratorColor->setDefaultValue(0.3,0);
    _defaultGeneratorColor->setDefaultValue(0.5,1);
    _defaultGeneratorColor->setDefaultValue(0.2,2);

    _defaultReaderColor->setDefaultValue(0.6,0);
    _defaultReaderColor->setDefaultValue(0.6,1);
    _defaultReaderColor->setDefaultValue(0.6,2);

    _defaultWriterColor->setDefaultValue(0.75,0);
    _defaultWriterColor->setDefaultValue(0.75,1);
    _defaultWriterColor->setDefaultValue(0.,2);

    _defaultColorGroupColor->setDefaultValue(0.48,0);
    _defaultColorGroupColor->setDefaultValue(0.66,1);
    _defaultColorGroupColor->setDefaultValue(1.,2);

    _defaultFilterGroupColor->setDefaultValue(0.8,0);
    _defaultFilterGroupColor->setDefaultValue(0.5,1);
    _defaultFilterGroupColor->setDefaultValue(0.3,2);

    _defaultTransformGroupColor->setDefaultValue(0.7,0);
    _defaultTransformGroupColor->setDefaultValue(0.3,1);
    _defaultTransformGroupColor->setDefaultValue(0.1,2);

    _defaultTimeGroupColor->setDefaultValue(0.7,0);
    _defaultTimeGroupColor->setDefaultValue(0.65,1);
    _defaultTimeGroupColor->setDefaultValue(0.35,2);

    _defaultDrawGroupColor->setDefaultValue(0.75,0);
    _defaultDrawGroupColor->setDefaultValue(0.75,1);
    _defaultDrawGroupColor->setDefaultValue(0.75,2);

    _defaultKeyerGroupColor->setDefaultValue(0.,0);
    _defaultKeyerGroupColor->setDefaultValue(1,1);
    _defaultKeyerGroupColor->setDefaultValue(0.,2);

    _defaultChannelGroupColor->setDefaultValue(0.6,0);
    _defaultChannelGroupColor->setDefaultValue(0.24,1);
    _defaultChannelGroupColor->setDefaultValue(0.39,2);

    _defaultMergeGroupColor->setDefaultValue(0.3,0);
    _defaultMergeGroupColor->setDefaultValue(0.37,1);
    _defaultMergeGroupColor->setDefaultValue(0.776,2);

    _defaultViewsGroupColor->setDefaultValue(0.5,0);
    _defaultViewsGroupColor->setDefaultValue(0.9,1);
    _defaultViewsGroupColor->setDefaultValue(0.7,2);

    _defaultDeepGroupColor->setDefaultValue(0.,0);
    _defaultDeepGroupColor->setDefaultValue(0.,1);
    _defaultDeepGroupColor->setDefaultValue(0.38,2);

#pragma message WARN("This is kinda a big hack to promote the OpenImageIO plug-in, we should use Tuttle's notation extension")
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        const std::vector<std::string> & entries = _readersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if ( QString( entries[j].c_str() ).contains("ReadOIIOOFX") ) {
                _readersMapping[i]->setDefaultValue(j,0);
                break;
            }
        }
    }

    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string> & entries = _writersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if ( QString( entries[j].c_str() ).contains("WriteOIIOOFX") ) {
                _writersMapping[i]->setDefaultValue(j,0);
                break;
            }
        }
    }
    endKnobsValuesChanged(Natron::PLUGIN_EDITED);
} // setDefaultValues

void
Settings::saveSettings()
{
    _wereChangesMadeSinceLastSave = false;
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knobs[i].get());
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knobs[i].get());
        Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knobs[i].get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knobs[i].get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knobs[i].get());
        const std::string& name = knobs[i]->getName();
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            std::string dimensionName = knobs[i]->getDimension() > 1 ? name + '.' + knobs[i]->getDimensionName(j) : name;
            if (isString) {
                settings.setValue(dimensionName.c_str(), QVariant(isString->getValue(j).c_str()));
            } else if (isInt) {
                if (isChoice) {
                    ///For choices,serialize the choice name instead
                    int index = isChoice->getValue(j);
                    const std::vector<std::string>& entries = isChoice->getEntries();
                    assert((int)entries.size() > index);
                    settings.setValue(dimensionName.c_str(), QVariant(entries[index].c_str()));
                } else {
                    settings.setValue(dimensionName.c_str(), QVariant(isInt->getValue(j)));
                }
            } else if (isDouble) {
                settings.setValue(dimensionName.c_str(), QVariant(isDouble->getValue(j)));
            } else if (isBool) {
                settings.setValue(dimensionName.c_str(), QVariant(isBool->getValue(j)));
            } else {
                assert(false);
            }
        }
    }
} // saveSettings

void
Settings::restoreSettings()
{
    _restoringSettings = true;
    _wereChangesMadeSinceLastSave = false;
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knobs[i].get());
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knobs[i].get());
        Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knobs[i].get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knobs[i].get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knobs[i].get());
        const std::string& name = knobs[i]->getName();
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            std::string dimensionName = knobs[i]->getDimension() > 1 ? name + '.' + knobs[i]->getDimensionName(j) : name;
            QString qDimName(dimensionName.c_str());
            if (settings.contains(qDimName)) {
                if (isString) {
                    isString->setValue(settings.value(qDimName).toString().toStdString(), j);
                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        std::string value = settings.value(qDimName).toString().toStdString();
                        const std::vector<std::string>& entries = isChoice->getEntries();
                        int found = -1;
                        for (U32 k = 0; k < entries.size(); ++k) {
                            if (entries[k] == value) {
                                found = (int)k;
                                break;
                            }
                        }
                        if (found >= 0) {
                            isChoice->setValue(found, j);
                        }
                    } else {
                        isInt->setValue(settings.value(qDimName).toInt(), j);
                    }
                } else if (isDouble) {
                    isDouble->setValue(settings.value(qDimName).toDouble(), j);
                } else if (isBool) {
                    isBool->setValue(settings.value(qDimName).toBool(), j);
                } else {
                    assert(false);
                }
            }
        }
    }
    if ( !settings.contains("OCIOConfig") ) {
        ///Load even though there's no settings!
        tryLoadOpenColorIOConfig();
    }
    _restoringSettings = false;
} // restoreSettings

bool
Settings::tryLoadOpenColorIOConfig()
{
    QString configFile;


    if ( _customOcioConfigFile->isEnabled(0) ) {
        ///try to load from the file
        std::string file = _customOcioConfigFile->getValue();
        if ( file.empty() ) {
            return false;
        }
        if ( !QFile::exists( file.c_str() ) ) {
            Natron::errorDialog( "OpenColorIO", file + QObject::tr(": No such file.").toStdString() );

            return false;
        }
        configFile = file.c_str();
    } else {
        ///try to load from the combobox
        QString activeEntryText( _ocioConfigKnob->getActiveEntryText().c_str() );
        QString configFileName = QString(activeEntryText + ".ocio");
        QStringList defaultConfigsPaths = getDefaultOcioConfigPaths();
        for (int i = 0; i < defaultConfigsPaths.size(); ++i) {
            QDir defaultConfigsDir(defaultConfigsPaths[i]);
            if ( !defaultConfigsDir.exists() ) {
                qDebug() << "Attempt to read an OpenColorIO configuration but the configuration directory does not exist.";
                continue;
            }
            ///try to open the .ocio config file first in the defaultConfigsDir
            ///if we can't find it, try to look in a subdirectory with the name of the config for the file config.ocio
            if ( !defaultConfigsDir.exists(configFileName) ) {
                QDir subDir(defaultConfigsPaths[i] + QDir::separator() + activeEntryText);
                if ( !subDir.exists() ) {
                    Natron::errorDialog( "OpenColorIO",subDir.absoluteFilePath("config.ocio").toStdString() + QObject::tr(": No such file or directory.").toStdString() );

                    return false;
                }
                if ( !subDir.exists("config.ocio") ) {
                    Natron::errorDialog( "OpenColorIO",subDir.absoluteFilePath("config.ocio").toStdString() + QObject::tr(": No such file or directory.").toStdString() );

                    return false;
                }
                configFile = subDir.absoluteFilePath("config.ocio");
            } else {
                configFile = defaultConfigsDir.absoluteFilePath(configFileName);
            }
        }
        if ( configFile.isEmpty() ) {
            return false;
        }
    }
    qDebug() << "setting OCIO=" << configFile;
    qputenv( NATRON_OCIO_ENV_VAR_NAME, configFile.toUtf8() );

    std::string stdConfigFile = configFile.toStdString();
    std::string configPath = SequenceParsing::removePath(stdConfigFile);
    if (!configPath.empty() && configPath[configPath.size() - 1] == '/') {
        configPath.erase(configPath.size() - 1, 1);
    }
    appPTR->onOCIOConfigPathChanged(configPath);
    return true;
} // tryLoadOpenColorIOConfig

void
Settings::onKnobValueChanged(KnobI* k,
                             Natron::ValueChangedReason /*reason*/,
                             SequenceTime /*time*/)
{
    _wereChangesMadeSinceLastSave = true;

    if ( k == _texturesMode.get() ) {
        std::map<int,AppInstanceRef> apps = appPTR->getAppInstances();
        bool isFirstViewer = true;
        for (std::map<int,AppInstanceRef>::iterator it = apps.begin(); it != apps.end(); ++it) {
            const std::vector<boost::shared_ptr<Node> > nodes = it->second.app->getProject()->getCurrentNodes();
            for (U32 i = 0; i < nodes.size(); ++i) {
                assert(nodes[i]);
                if (nodes[i]->getPluginID() == "Viewer") {
                    ViewerInstance* n = dynamic_cast<ViewerInstance*>( nodes[i]->getLiveInstance() );
                    assert(n);
                    if (isFirstViewer) {
                        if ( !n->supportsGLSL() && (_texturesMode->getValue() != 0) ) {
                            Natron::errorDialog( QObject::tr("Viewer").toStdString(), QObject::tr("You need OpenGL GLSL in order to use 32 bit fp textures.\n"
                                                                                                  "Reverting to 8bits textures.").toStdString() );
                            _texturesMode->setValue(0,0);

                            return;
                        }
                    }
                    n->updateTreeAndRender();
                }
            }
        }
    } else if ( k == _maxDiskCacheGB.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumDiskSpace( getMaximumDiskCacheSize() );
        }
    } else if ( k == _maxRAMPercent.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumMemoryPercent( getRamMaximumPercent() );
        }
        setCachingLabels();
    } else if ( k == _maxPlayBackPercent.get() ) {
        if (!_restoringSettings) {
            appPTR->setPlaybackCacheMaximumSize( getRamPlaybackMaximumPercent() );
        }
        setCachingLabels();
    } else if ( k == _numberOfThreads.get() ) {
        int nbThreads = getNumberOfThreads();
        if (nbThreads == -1) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else if (nbThreads == 0) {
            QThreadPool::globalInstance()->setMaxThreadCount( QThread::idealThreadCount() );
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
        }
    } else if ( k == _ocioConfigKnob.get() ) {
        if ( _ocioConfigKnob->getActiveEntryText() == std::string(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) {
            _customOcioConfigFile->setAllDimensionsEnabled(true);
        } else {
            _customOcioConfigFile->setAllDimensionsEnabled(false);
        }
        tryLoadOpenColorIOConfig();
    } else if ( k == _customOcioConfigFile.get() ) {
        tryLoadOpenColorIOConfig();
    } else if ( k == _maxUndoRedoNodeGraph.get() ) {
        appPTR->setUndoRedoStackLimit( _maxUndoRedoNodeGraph->getValue() );
    } else if ( k == _maxPanelsOpened.get() ) {
        appPTR->onMaxPanelsOpenedChanged( _maxPanelsOpened->getValue() );
    } else if ( k == _checkerboardTileSize.get() || k == _checkerboardColor1.get() || k == _checkerboardColor2.get() ) {
        appPTR->onCheckerboardSettingsChanged();
    }
} // onKnobValueChanged

int
Settings::getViewersBitDepth() const
{
    return _texturesMode->getValue();
}

int
Settings::getViewerTilesPowerOf2() const
{
    return _powerOf2Tiling->getValue();
}

double
Settings::getRamMaximumPercent() const
{
    return (double)_maxRAMPercent->getValue() / 100.;
}

double
Settings::getRamPlaybackMaximumPercent() const
{
    return (double)_maxPlayBackPercent->getValue() / 100.;
}

U64
Settings::getMaximumDiskCacheSize() const
{
    return (U64)( _maxDiskCacheGB->getValue() ) * std::pow(1024.,3.);
}

double
Settings::getUnreachableRamPercent() const
{
    return (double)_unreachableRAMPercent->getValue() / 100.;
}

bool
Settings::getColorPickerLinear() const
{
    return _linearPickers->getValue();
}

int
Settings::getNumberOfThreads() const
{
    return _numberOfThreads->getValue();
}

void
Settings::setNumberOfThreads(int threadsNb)
{
    _numberOfThreads->setValue(threadsNb,0);
}

bool
Settings::isAutoPreviewOnForNewProjects() const
{
    return _autoPreviewEnabledForNewProjects->getValue();
}

const std::string &
Settings::getReaderPluginIDForFileType(const std::string & extension)
{
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        if (_readersMapping[i]->getDescription() == extension) {
            const std::vector<std::string> & entries =  _readersMapping[i]->getEntries();
            int index = _readersMapping[i]->getValue();
            assert( index < (int)entries.size() );

            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
}

const std::string &
Settings::getWriterPluginIDForFileType(const std::string & extension)
{
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        if (_writersMapping[i]->getDescription() == extension) {
            const std::vector<std::string> & entries =  _writersMapping[i]->getEntries();
            int index = _writersMapping[i]->getValue();
            assert( index < (int)entries.size() );

            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
}

void
Settings::populateReaderPluginsAndFormats(const std::map<std::string,std::vector<std::string> > & rows)
{
    for (std::map<std::string,std::vector<std::string> >::const_iterator it = rows.begin(); it != rows.end(); ++it) {
        boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(this, it->first);
        k->setAnimationEnabled(false);
        k->populateChoices(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote ReadOIIO !
            if ( QString( it->second[i].c_str() ).contains("ReadOIIOOFX") ) {
                k->setValue(i,0);
                break;
            }
        }
        _readersMapping.push_back(k);
        _readersTab->addKnob(k);
    }
}

void
Settings::populateWriterPluginsAndFormats(const std::map<std::string,std::vector<std::string> > & rows)
{
    for (std::map<std::string,std::vector<std::string> >::const_iterator it = rows.begin(); it != rows.end(); ++it) {
        boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(this, it->first);
        k->setAnimationEnabled(false);
        k->populateChoices(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote WriteOIIOOFX !
            if ( QString( it->second[i].c_str() ).contains("WriteOIIOOFX") ) {
                k->setValue(i,0);
                break;
            }
        }
        _writersMapping.push_back(k);
        _writersTab->addKnob(k);
    }
}

void
Settings::getFileFormatsForReadingAndReader(std::map<std::string,std::string>* formats)
{
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        const std::vector<std::string> & entries = _readersMapping[i]->getEntries();
        int index = _readersMapping[i]->getValue();

        assert( index < (int)entries.size() );

        formats->insert( std::make_pair(_readersMapping[i]->getDescription(),entries[index]) );
    }
}

void
Settings::getFileFormatsForWritingAndWriter(std::map<std::string,std::string>* formats)
{
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string> & entries = _writersMapping[i]->getEntries();
        int index = _writersMapping[i]->getValue();

        assert( index < (int)entries.size() );

        formats->insert( std::make_pair(_writersMapping[i]->getDescription(),entries[index]) );
    }
}

QStringList
Settings::getPluginsExtraSearchPaths() const
{
    QString paths = _extraPluginPaths->getValue().c_str();

    return paths.split( QChar(';') );
}

void
Settings::restoreDefault()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    if ( !QFile::remove( settings.fileName() ) ) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }

    beginKnobsValuesChanged(Natron::PLUGIN_EDITED);
    const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            knobs[i]->resetToDefaultValue(j);
        }
    }
    setCachingLabels();
    endKnobsValuesChanged(Natron::PLUGIN_EDITED);
}

bool
Settings::isRenderInSeparatedProcessEnabled() const
{
    return _renderInSeparateProcess->getValue();
}

int
Settings::getMaximumUndoRedoNodeGraph() const
{
    return _maxUndoRedoNodeGraph->getValue();
}

int
Settings::getAutoSaveDelayMS() const
{
    return _autoSaveDelay->getValue() * 1000;
}

bool
Settings::isSnapToNodeEnabled() const
{
    return _snapNodesToConnections->getValue();
}

bool
Settings::isCheckForUpdatesEnabled() const
{
    return _checkForUpdates->getValue();
}

void
Settings::setCheckUpdatesEnabled(bool enabled)
{
    _checkForUpdates->setValue(enabled, 0);
    saveSettings();
}

int
Settings::getMaxPanelsOpened() const
{
    return _maxPanelsOpened->getValue();
}

void
Settings::setMaxPanelsOpened(int maxPanels)
{
    _maxPanelsOpened->setValue(maxPanels, 0);
}

void
Settings::setConnectionHintsEnabled(bool enabled)
{
    _useNodeGraphHints->setValue(enabled, 0);
}

bool
Settings::isConnectionHintEnabled() const
{
    return _useNodeGraphHints->getValue();
}

bool
Settings::loadBundledPlugins() const
{
    return _loadBundledPlugins->getValue();
}

bool
Settings::preferBundledPlugins() const
{
    return _preferBundledPlugins->getValue();
}

void
Settings::getDefaultNodeColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _defaultNodeColor->getValue(0);
    *g = _defaultNodeColor->getValue(1);
    *b = _defaultNodeColor->getValue(2);
}

void
Settings::getDefaultSelectedNodeColor(float *r,
                                      float *g,
                                      float *b) const
{
    *r = _defaultSelectedNodeColor->getValue(0);
    *g = _defaultSelectedNodeColor->getValue(1);
    *b = _defaultSelectedNodeColor->getValue(2);
}

void
Settings::getDefaultBackDropColor(float *r,
                                  float *g,
                                  float *b) const
{
    *r = _defaultBackdropColor->getValue(0);
    *g = _defaultBackdropColor->getValue(1);
    *b = _defaultBackdropColor->getValue(2);
}

void
Settings::getGeneratorColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultGeneratorColor->getValue(0);
    *g = _defaultGeneratorColor->getValue(1);
    *b = _defaultGeneratorColor->getValue(2);
}

void
Settings::getReaderColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _defaultReaderColor->getValue(0);
    *g = _defaultReaderColor->getValue(1);
    *b = _defaultReaderColor->getValue(2);
}

void
Settings::getWriterColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _defaultWriterColor->getValue(0);
    *g = _defaultWriterColor->getValue(1);
    *b = _defaultWriterColor->getValue(2);
}

void
Settings::getColorGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultColorGroupColor->getValue(0);
    *g = _defaultColorGroupColor->getValue(1);
    *b = _defaultColorGroupColor->getValue(2);
}

void
Settings::getFilterGroupColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _defaultFilterGroupColor->getValue(0);
    *g = _defaultFilterGroupColor->getValue(1);
    *b = _defaultFilterGroupColor->getValue(2);
}

void
Settings::getTransformGroupColor(float *r,
                                 float *g,
                                 float *b) const
{
    *r = _defaultTransformGroupColor->getValue(0);
    *g = _defaultTransformGroupColor->getValue(1);
    *b = _defaultTransformGroupColor->getValue(2);
}

void
Settings::getTimeGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultTimeGroupColor->getValue(0);
    *g = _defaultTimeGroupColor->getValue(1);
    *b = _defaultTimeGroupColor->getValue(2);
}

void
Settings::getDrawGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultDrawGroupColor->getValue(0);
    *g = _defaultDrawGroupColor->getValue(1);
    *b = _defaultDrawGroupColor->getValue(2);
}

void
Settings::getKeyerGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultKeyerGroupColor->getValue(0);
    *g = _defaultKeyerGroupColor->getValue(1);
    *b = _defaultKeyerGroupColor->getValue(2);
}

void
Settings::getChannelGroupColor(float *r,
                               float *g,
                               float *b) const
{
    *r = _defaultChannelGroupColor->getValue(0);
    *g = _defaultChannelGroupColor->getValue(1);
    *b = _defaultChannelGroupColor->getValue(2);
}

void
Settings::getMergeGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultMergeGroupColor->getValue(0);
    *g = _defaultMergeGroupColor->getValue(1);
    *b = _defaultMergeGroupColor->getValue(2);
}

void
Settings::getViewsGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultViewsGroupColor->getValue(0);
    *g = _defaultViewsGroupColor->getValue(1);
    *b = _defaultViewsGroupColor->getValue(2);
}

void
Settings::getDeepGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultDeepGroupColor->getValue(0);
    *g = _defaultDeepGroupColor->getValue(1);
    *b = _defaultDeepGroupColor->getValue(2);
}

int
Settings::getDisconnectedArrowLength() const
{
    return _disconnectedArrowLength->getValue();
}

std::string
Settings::getHostName() const
{
    return _hostName->getValue();
}

bool
Settings::getRenderOnEditingFinishedOnly() const
{
    return _renderOnEditingFinished->getValue();
}

void
Settings::setRenderOnEditingFinishedOnly(bool render)
{
    _renderOnEditingFinished->setValue(render, 0);
}

bool
Settings::getIconsBlackAndWhite() const
{
    return _useBWIcons->getValue();
}

std::string
Settings::getDefaultLayoutFile() const
{
    return _defaultLayoutFile->getValue();
}

bool
Settings::useCursorPositionIncrements() const
{
    return _useCursorPositionIncrements->getValue();
}

bool
Settings::isAutoProjectFormatEnabled() const
{
    return _firstReadSetProjectFormat->getValue();
}

bool
Settings::isAutoFixRelativeFilePathEnabled() const
{
    return _fixPathsOnProjectPathChanged->getValue();
}

int
Settings::getCheckerboardTileSize() const
{
    return _checkerboardTileSize->getValue();
}

void
Settings::getCheckerboardColor1(double* r,double* g,double* b,double* a) const
{
    *r = _checkerboardColor1->getValue(0);
    *g = _checkerboardColor1->getValue(1);
    *b = _checkerboardColor1->getValue(2);
    *a = _checkerboardColor1->getValue(3);
}

void
Settings::getCheckerboardColor2(double* r,double* g,double* b,double* a) const
{
    *r = _checkerboardColor2->getValue(0);
    *g = _checkerboardColor2->getValue(1);
    *b = _checkerboardColor2->getValue(2);
    *a = _checkerboardColor2->getValue(3);
}
