//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
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


#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

using namespace Natron;


Settings::Settings(AppInstance* appInstance)
: KnobHolder(appInstance)
, _wereChangesMadeSinceLastSave(false)
{
    
}

static QStringList getDefaultOcioConfigPaths() {
    QString binaryPath = appPTR->getApplicationBinaryPath();
    if (!binaryPath.isEmpty()) {
        binaryPath += QDir::separator();
    }
#ifdef __NATRON_LINUX__
    QStringList ret;
    ret.push_back(QString("/usr/share/OpenColorIO-Configs"));
    ret.push_back(QString(binaryPath + "../share/OpenColorIO-Configs"));x
    ret.push_back(QString(binaryPath + "../Resources/OpenColorIO-Configs"));
    return ret;
#elif defined(__NATRON_WIN32__)
    return QStringList(QString(binaryPath + "../Resources/OpenColorIO-Configs"));
#elif defined(__NATRON_OSX__)
    return QStringList(QString(binaryPath + "../Resources/OpenColorIO-Configs"));
#endif
}

#if defined(WINDOWS)
// defined in ofxhPluginCache.cpp
const TCHAR *getStdOFXPluginPath(const std::string &hostId);
#endif

void Settings::initializeKnobs(){
    
    _generalTab = Natron::createKnob<Page_Knob>(this, "General");
    
    _checkForUpdates = Natron::createKnob<Bool_Knob>(this, "Always check for updates on start-up");
    _checkForUpdates->setAnimationEnabled(false);
    _checkForUpdates->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " will check for new updates on start-up of the application.");
    _generalTab->addKnob(_checkForUpdates);
    
    _autoSaveDelay = Natron::createKnob<Int_Knob>(this, "Auto-save trigger delay");
    _autoSaveDelay->setAnimationEnabled(false);
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setMinimum(0);
    _autoSaveDelay->setMaximum(60);
    _autoSaveDelay->setHintToolTip("The number of seconds after an event that " NATRON_APPLICATION_NAME " should wait before "
                                   " auto-saving. Note that if a render is in progress, " NATRON_APPLICATION_NAME " will "
                                   " wait until it is done to actually auto-save.");
    _generalTab->addKnob(_autoSaveDelay);
    
    
    _linearPickers = Natron::createKnob<Bool_Knob>(this, "Linear color pickers");
    _linearPickers->setAnimationEnabled(false);
    _linearPickers->setHintToolTip("When activated, all colors picked from the color parameters will be converted"
                                   " to linear before being fetched. Otherwise they will be in the same color-space "
                                   " as the viewer they were picked from.");
    _generalTab->addKnob(_linearPickers);
    
    _numberOfThreads = Natron::createKnob<Int_Knob>(this, "Number of render threads");
    _numberOfThreads->setAnimationEnabled(false);
    QString numberOfThreadsToolTip = QString("Controls how many threads " NATRON_APPLICATION_NAME " should use to render. \n"
                                             "-1: Disable multi-threading totally (useful for debug) \n"
                                             "0: Guess from the number of cores. The ideal threads count for your hardware is %1.").arg(QThread::idealThreadCount());
    _numberOfThreads->setHintToolTip(numberOfThreadsToolTip.toStdString());
    _numberOfThreads->disableSlider();
    _numberOfThreads->setMinimum(-1);
    _numberOfThreads->setDisplayMinimum(-1);
    _generalTab->addKnob(_numberOfThreads);
    
    _renderInSeparateProcess = Natron::createKnob<Bool_Knob>(this, "Render in a separate process");
    _renderInSeparateProcess->setAnimationEnabled(false);
    _renderInSeparateProcess->setHintToolTip("If true, " NATRON_APPLICATION_NAME " will render (using the write nodes) in "
                                             "a separate process. Disabling it is most helpful for the dev team.");
    _generalTab->addKnob(_renderInSeparateProcess);
    
    _autoPreviewEnabledForNewProjects = Natron::createKnob<Bool_Knob>(this, "Auto-preview enabled by default for new projects");
    _autoPreviewEnabledForNewProjects->setAnimationEnabled(false);
    _autoPreviewEnabledForNewProjects->setHintToolTip("If checked then when creating a new project, the Auto-preview option"
                                                      " will be enabled.");
    _generalTab->addKnob(_autoPreviewEnabledForNewProjects);
    
    
    
    _maxPanelsOpened = Natron::createKnob<Int_Knob>(this, "Maximum number of node settings panels opened");
    _maxPanelsOpened->setHintToolTip("This property holds the number of node settings pnaels that can be "
                                     "held by the properties dock at the same time."
                                     "The special value of 0 indicates there can be an unlimited number of panels opened.");
    _maxPanelsOpened->setAnimationEnabled(false);
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setMinimum(0);
    _maxPanelsOpened->setMaximum(100);
    _generalTab->addKnob(_maxPanelsOpened);
    
    _renderOnEditingFinished = Natron::createKnob<Bool_Knob>(this, "Render on editing finished only");
    _renderOnEditingFinished->setHintToolTip("When checked, the render engine will run only when you finish editing "
                                             "a curve or a parameter. For example, for curves editing, that is only when you release the button"
                                             " of the mouse the render will be triggered."
                                             "This also works for sliders and values of parameters."
                                             " Note that this setting doesn't apply to roto splines editing.");
    _renderOnEditingFinished->setAnimationEnabled(false);
    _generalTab->addKnob(_renderOnEditingFinished);
    
    _generalTab->addKnob(Natron::createKnob<Separator_Knob>(this, "OpenFX Plugins"));
    
    _extraPluginPaths = Natron::createKnob<Path_Knob>(this, "Extra plugins search paths");
    _extraPluginPaths->setHintToolTip(std::string("All paths in this variable are separated by ';' and indicate"
                                                  " extra search paths where " NATRON_APPLICATION_NAME " should scan for plug-ins. "
                                                  "Extra plugins search paths can also be specified using the OFX_PLUGIN_PATH environment variable.\n"
                                                  "The priority order for system-wide plugins, from high to low, is:\n"
                                                  "- plugins found in OFX_PLUGIN_PATH\n"
                                                  "- plugins found in \""
#if defined(WINDOWS)
                                                  ) + getStdOFXPluginPath("") +
                                      std::string("\" and \"C:\\Program Files\\Common Files\\OFX\\Plugins"
#endif
#if defined(__linux__)
                                                  "/usr/OFX/Plugins"
#endif
#if defined(__APPLE__)
                                                  "/Library/OFX/Plugins"
#endif
                                                  "\".\n"
                                                  "Plugins bundled with the binary distribution of Natron may have either "
                                                  "higher or lower priority, depending on the \"Prefer bundled plug-ins over "
                                                  "system-wide plug-ins\" setting.\n"
                                                  "Any change will take effect on the next launch of " NATRON_APPLICATION_NAME "."));
    _extraPluginPaths->setMultiPath(true);
    _generalTab->addKnob(_extraPluginPaths);
    
    _loadBundledPlugins = Natron::createKnob<Bool_Knob>(this, "Use bundled plug-ins");
    _loadBundledPlugins->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " will also use the plug-ins bundled "
                                        "with the binary distribution.\n"
                                        "When unchecked, only system-wide plug-ins will be loaded (more information can be "
                                        "found in the help for the \"Extra plugins search paths\" setting).");
    _loadBundledPlugins->setAnimationEnabled(false);
    _generalTab->addKnob(_loadBundledPlugins);

    _preferBundledPlugins = Natron::createKnob<Bool_Knob>(this, "Prefer bundled plug-ins over system-wide plug-ins");
    _preferBundledPlugins->setHintToolTip("When checked, and if \"Use bundled plug-ins\" is also checked, plug-ins bundled with the "
                                          NATRON_APPLICATION_NAME " binary distribution will take precedence over system-wide plug-ins.");
    _preferBundledPlugins->setAnimationEnabled(false);
    _generalTab->addKnob(_preferBundledPlugins);
    

    _hostName = Natron::createKnob<String_Knob>(this, "Host name");
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
    _ocioConfigKnob->setAnimationEnabled(false);
    
    std::vector<std::string> configs;
    int defaultIndex = 0;
    
    QStringList defaultOcioConfigsPaths = getDefaultOcioConfigPaths();
    for (int i = 0; i < defaultOcioConfigsPaths.size(); ++i) {
        QDir ocioConfigsDir(defaultOcioConfigsPaths[i]);
        if (ocioConfigsDir.exists()) {
            QStringList entries = ocioConfigsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
            for (int j = 0; j < entries.size(); ++j) {
                if (entries[j].contains("nuke-default")) {
                    defaultIndex = j;
                }
                configs.push_back(entries[j].toStdString());
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
    _customOcioConfigFile->setAllDimensionsEnabled(false);
    _customOcioConfigFile->setHintToolTip("To use this, set the OpenColorIO config to custom config a point "
                                          "to a custom OpenColorIO config file (.ocio).");
    ocioTab->addKnob(_customOcioConfigFile);
    
    _viewersTab = Natron::createKnob<Page_Knob>(this, "Viewers");
    
    _texturesMode = Natron::createKnob<Choice_Knob>(this, "Viewer textures bit depth");
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
    
    /////////// Nodegraph tab
    _nodegraphTab = Natron::createKnob<Page_Knob>(this, "Nodegraph");
    
    _snapNodesToConnections = Natron::createKnob<Bool_Knob>(this, "Snap to node");
    _snapNodesToConnections->setHintToolTip("When moving nodes on the node graph, snap them to positions where it lines them up "
                                            "with the inputs and output nodes.");
    _snapNodesToConnections->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_snapNodesToConnections);
    
    _useNodeGraphHints = Natron::createKnob<Bool_Knob>(this, "Use connection hints");
    _useNodeGraphHints->setHintToolTip("When checked, moving a node which is not connected to anything to arrows "
                                       "nearby will display a hint for possible connections. Releasing the mouse on such a "
                                       "hint will perform the connection for you.");
    _useNodeGraphHints->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_useNodeGraphHints);
    
    _maxUndoRedoNodeGraph = Natron::createKnob<Int_Knob>(this, "Maximum undo/redo for the node graph");
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
    _disconnectedArrowLength->setAnimationEnabled(false);
    _disconnectedArrowLength->setHintToolTip("The size of a disconnected node input arrow in pixels.");
    _disconnectedArrowLength->disableSlider();
    
    _nodegraphTab->addKnob(_disconnectedArrowLength);
    
    _defaultNodeColor = Natron::createKnob<Color_Knob>(this, "Default node color",3);
    _defaultNodeColor->setAnimationEnabled(false);
    _defaultNodeColor->setHintToolTip("This is default color which nodes have when created.");
    
    _nodegraphTab->addKnob(_defaultNodeColor);
    
    _defaultSelectedNodeColor = Natron::createKnob<Color_Knob>(this, "Default selected node color",3);
    _defaultSelectedNodeColor->setAnimationEnabled(false);
    _defaultSelectedNodeColor->setHintToolTip("This is default color which the selected nodes have.");
   
    _nodegraphTab->addKnob(_defaultSelectedNodeColor);

    
    _defaultBackdropColor =  Natron::createKnob<Color_Knob>(this, "Default backdrop color",3);
    _defaultBackdropColor->setAnimationEnabled(false);
    _defaultBackdropColor->setHintToolTip("This is default color which backdrop nodes have when created.");
    _nodegraphTab->addKnob(_defaultBackdropColor);
    
    ///////////////////DEFAULT GROUP COLORS
    
    _defaultReaderColor =  Natron::createKnob<Color_Knob>(this, "Readers",3);
    _defaultReaderColor->setAnimationEnabled(false);
    _defaultReaderColor->setHintToolTip("This is default color which reader nodes have when created.");
    _nodegraphTab->addKnob(_defaultReaderColor);
    
    _defaultWriterColor =  Natron::createKnob<Color_Knob>(this, "Writers",3);
    _defaultWriterColor->setAnimationEnabled(false);
    _defaultWriterColor->setHintToolTip("This is default color which writer nodes have when created.");
    _nodegraphTab->addKnob(_defaultWriterColor);
    
    _defaultGeneratorColor =  Natron::createKnob<Color_Knob>(this, "Generators",3);
    _defaultGeneratorColor->setAnimationEnabled(false);
    _defaultGeneratorColor->setHintToolTip("This is default color which generator nodes have when created.");
    _nodegraphTab->addKnob(_defaultGeneratorColor);
    
    _defaultColorGroupColor =  Natron::createKnob<Color_Knob>(this, "Color group",3);
    _defaultColorGroupColor->setAnimationEnabled(false);
    _defaultColorGroupColor->setHintToolTip("This is default color which color processing nodes have when created.");
    _nodegraphTab->addKnob(_defaultColorGroupColor);
    
    _defaultFilterGroupColor =  Natron::createKnob<Color_Knob>(this, "Filter group",3);
    _defaultFilterGroupColor->setAnimationEnabled(false);
    _defaultFilterGroupColor->setHintToolTip("This is default color which filter nodes have when created.");
    _nodegraphTab->addKnob(_defaultFilterGroupColor);
    
    _defaultTransformGroupColor =  Natron::createKnob<Color_Knob>(this, "Transform group",3);
    _defaultTransformGroupColor->setAnimationEnabled(false);
    _defaultTransformGroupColor->setHintToolTip("This is default color which transform nodes have when created.");
    _nodegraphTab->addKnob(_defaultTransformGroupColor);
    
    _defaultTimeGroupColor =  Natron::createKnob<Color_Knob>(this, "Time group",3);
    _defaultTimeGroupColor->setAnimationEnabled(false);
    _defaultTimeGroupColor->setHintToolTip("This is default color which time processing nodes have when created.");
    _nodegraphTab->addKnob(_defaultTimeGroupColor);
    
    _defaultDrawGroupColor =  Natron::createKnob<Color_Knob>(this, "Draw group",3);
    _defaultDrawGroupColor->setAnimationEnabled(false);
    _defaultDrawGroupColor->setHintToolTip("This is default color which draw nodes have when created.");
    _nodegraphTab->addKnob(_defaultDrawGroupColor);
    
    _defaultKeyerGroupColor =  Natron::createKnob<Color_Knob>(this, "Keyer group",3);
    _defaultKeyerGroupColor->setAnimationEnabled(false);
    _defaultKeyerGroupColor->setHintToolTip("This is default color which keying nodes have when created.");
    _nodegraphTab->addKnob(_defaultKeyerGroupColor);
    
    _defaultChannelGroupColor =  Natron::createKnob<Color_Knob>(this, "Channel group",3);
    _defaultChannelGroupColor->setAnimationEnabled(false);
    _defaultChannelGroupColor->setHintToolTip("This is default color which channel processing nodes have when created.");
    _nodegraphTab->addKnob(_defaultChannelGroupColor);
    
    _defaultMergeGroupColor =  Natron::createKnob<Color_Knob>(this, "Merge group",3);
    _defaultMergeGroupColor->setAnimationEnabled(false);
    _defaultMergeGroupColor->setHintToolTip("This is default color which merge  nodes have when created.");
    _nodegraphTab->addKnob(_defaultMergeGroupColor);
    
    _defaultViewsGroupColor =  Natron::createKnob<Color_Knob>(this, "Views group",3);
    _defaultViewsGroupColor->setAnimationEnabled(false);
    _defaultViewsGroupColor->setHintToolTip("This is default color which multi-view nodes have when created.");
    _nodegraphTab->addKnob(_defaultViewsGroupColor);
    
    _defaultDeepGroupColor =  Natron::createKnob<Color_Knob>(this, "Deep group",3);
    _defaultDeepGroupColor->setAnimationEnabled(false);
    _defaultDeepGroupColor->setHintToolTip("This is default color which deep compositing nodes have when created.");
    _nodegraphTab->addKnob(_defaultDeepGroupColor);
    
    /////////// Caching tab
    _cachingTab = Natron::createKnob<Page_Knob>(this, "Caching");
    
    _maxRAMPercent = Natron::createKnob<Int_Knob>(this, "Maximum system's RAM for caching (% system total RAM)");
    _maxRAMPercent->setAnimationEnabled(false);
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    std::string ramHint("This setting indicates the percentage of the system's total RAM "
                        NATRON_APPLICATION_NAME "'s caches are allowed to use."
                        " Your system has ");
    ramHint.append(printAsRAM(getSystemTotalRAM()).toStdString());
    ramHint.append(" of RAM.");
    if (isApplication32Bits()) {
        ramHint.append("\n The version of " NATRON_APPLICATION_NAME " you are running is 32 bits, that means the system's total RAM "
                       "will be limited to 4Gib. In this case the percentage of RAM allowed will be min(4Gib,SystemtotalRAM) * MaxRamPercent.");
    }
    
    _maxRAMPercent->setHintToolTip(ramHint);
    _cachingTab->addKnob(_maxRAMPercent);
    
    _maxPlayBackPercent = Natron::createKnob<Int_Knob>(this, "Playback cache RAM percentage (% maximum RAM for caching");
    _maxPlayBackPercent->setAnimationEnabled(false);
    _maxPlayBackPercent->setMinimum(0);
    _maxPlayBackPercent->setMaximum(100);
    _maxPlayBackPercent->setHintToolTip("This setting indicates the percentage of the Maximum system's RAM for caching"
                                        " dedicated for the playback cache. Normally you shouldn't change this value"
                                        " as it is tuned automatically by the Maximum system's RAM for caching, but"
                                        " this is made possible for convenience.");
    _cachingTab->addKnob(_maxPlayBackPercent);
    
    _maxDiskCacheGB = Natron::createKnob<Int_Knob>(this, "Maximum disk cache size (GB)");
    _maxDiskCacheGB->setAnimationEnabled(false);
    _maxDiskCacheGB->setMinimum(0);
    _maxDiskCacheGB->setMaximum(100);
    _maxDiskCacheGB->setHintToolTip("The maximum disk space the caches can use. (in GB)");
    _cachingTab->addKnob(_maxDiskCacheGB);
    
    
    ///readers & writers settings are created in a postponed manner because we don't know
    ///their dimension yet. See populateReaderPluginsAndFormats & populateWriterPluginsAndFormats
    
    _readersTab = Natron::createKnob<Page_Knob>(this, "Readers");
    _readersTab->setName("readersTab");

    _writersTab = Natron::createKnob<Page_Knob>(this, "Writers");
    _writersTab->setName("writersTab");
    
    setDefaultValues();
}


void Settings::setDefaultValues() {
    
    beginKnobsValuesChanged(Natron::PLUGIN_EDITED);
    _hostName->setDefaultValue(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);
    _checkForUpdates->setDefaultValue(false);
    _autoSaveDelay->setDefaultValue(5, 0);
    _maxUndoRedoNodeGraph->setDefaultValue(20, 0);
    _linearPickers->setDefaultValue(true,0);
    _snapNodesToConnections->setDefaultValue(true);
    _useNodeGraphHints->setDefaultValue(true);
    _numberOfThreads->setDefaultValue(0,0);
    _renderInSeparateProcess->setDefaultValue(true,0);
    _autoPreviewEnabledForNewProjects->setDefaultValue(true,0);
    _maxPanelsOpened->setDefaultValue(10,0);
    _renderOnEditingFinished->setDefaultValue(false);
    _extraPluginPaths->setDefaultValue("",0);
    _preferBundledPlugins->setDefaultValue(true);
    _loadBundledPlugins->setDefaultValue(true);
    _texturesMode->setDefaultValue(0,0);
    _powerOf2Tiling->setDefaultValue(8,0);
    _maxRAMPercent->setDefaultValue(50,0);
    _maxPlayBackPercent->setDefaultValue(25,0);
    _maxDiskCacheGB->setDefaultValue(10,0);
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
        const std::vector<std::string>& entries = _readersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if (QString(entries[j].c_str()).contains("ReadOIIOOFX")) {
                _readersMapping[i]->setDefaultValue(j,0);
                break;
            }
        }
    }
    
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _writersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if (QString(entries[j].c_str()).contains("WriteOIIOOFX")) {
                _writersMapping[i]->setDefaultValue(j,0);
                break;
            }
        }
    }
    endKnobsValuesChanged(Natron::PLUGIN_EDITED);
}

void Settings::saveSettings(){
    
    _wereChangesMadeSinceLastSave = false;
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    settings.setValue("CheckUpdates", _checkForUpdates->getValue());
    settings.setValue("AutoSaveDelay", _autoSaveDelay->getValue());
    settings.setValue("LinearColorPickers",_linearPickers->getValue());
    settings.setValue("Number of threads", _numberOfThreads->getValue());
    settings.setValue("RenderInSeparateProcess", _renderInSeparateProcess->getValue());
    settings.setValue("AutoPreviewDefault", _autoPreviewEnabledForNewProjects->getValue());
    settings.setValue("MaxPanelsOpened", _maxPanelsOpened->getValue());
    settings.setValue("RenderOnEditingFinished",_renderOnEditingFinished->getValue());
    settings.setValue("ExtraPluginsPaths", _extraPluginPaths->getValue().c_str());
    settings.setValue("PreferBundledPlugins", _preferBundledPlugins->getValue());
    settings.setValue("LoadBundledPlugins", _loadBundledPlugins->getValue());
    settings.setValue("HostName",_hostName->getValue().c_str());
    settings.endGroup();
    
    settings.beginGroup("OpenColorIO");
    std::string configList = _customOcioConfigFile->getValue();
    if (!configList.empty()) {
        settings.setValue("OCIOConfigFile", configList.c_str());
    }
    settings.setValue("OCIOConfig", _ocioConfigKnob->getValue());
    settings.endGroup();
    
    settings.beginGroup("Caching");
    settings.setValue("MaximumRAMUsagePercentage", _maxRAMPercent->getValue());
    settings.setValue("MaximumPlaybackRAMUsage", _maxPlayBackPercent->getValue());
    settings.setValue("MaximumDiskSizeUsage", _maxDiskCacheGB->getValue());
    settings.endGroup();
    
    settings.beginGroup("Viewers");
    settings.setValue("ByteTextures", _texturesMode->getValue());
    settings.setValue("TilesPowerOf2", _powerOf2Tiling->getValue());
    settings.endGroup();
    
    settings.beginGroup("Nodegraph");
    settings.setValue("SnapToNode",_snapNodesToConnections->getValue());
    settings.setValue("ConnectionHints",_useNodeGraphHints->getValue());
    settings.setValue("MaximumUndoRedoNodeGraph", _maxUndoRedoNodeGraph->getValue());
    settings.setValue("DisconnectedArrowLength", _disconnectedArrowLength->getValue());
    settings.setValue("DefaultNodeColor_r", _defaultNodeColor->getValue(0));
    settings.setValue("DefaultNodeColor_g", _defaultNodeColor->getValue(1));
    settings.setValue("DefaultNodeColor_b", _defaultNodeColor->getValue(2));
    settings.setValue("DefaultSelectedNodeColor_r", _defaultSelectedNodeColor->getValue(0));
    settings.setValue("DefaultSelectedNodeColor_g", _defaultSelectedNodeColor->getValue(1));
    settings.setValue("DefaultSelectedNodeColor_b", _defaultSelectedNodeColor->getValue(2));
    settings.setValue("DefaultBackdropColor_r", _defaultBackdropColor->getValue(0));
    settings.setValue("DefaultBackdropColor_g", _defaultBackdropColor->getValue(1));
    settings.setValue("DefaultBackdropColor_b", _defaultBackdropColor->getValue(2));
    
    settings.setValue("DefaultGeneratorColor_r", _defaultGeneratorColor->getValue(0));
    settings.setValue("DefaultGeneratorColor_g", _defaultGeneratorColor->getValue(1));
    settings.setValue("DefaultGeneratorColor_b", _defaultGeneratorColor->getValue(2));
    
    settings.setValue("DefaultReaderColor_r", _defaultReaderColor->getValue(0));
    settings.setValue("DefaultReaderColor_g", _defaultReaderColor->getValue(1));
    settings.setValue("DefaultReaderColor_b", _defaultReaderColor->getValue(2));
    
    settings.setValue("DefaultWriterColor_r", _defaultWriterColor->getValue(0));
    settings.setValue("DefaultWriterColor_g", _defaultWriterColor->getValue(1));
    settings.setValue("DefaultWriterColor_b", _defaultWriterColor->getValue(2));
    
    settings.setValue("DefaultColorGroupColor_r", _defaultColorGroupColor->getValue(0));
    settings.setValue("DefaultColorGroupColor_g", _defaultColorGroupColor->getValue(1));
    settings.setValue("DefaultColorGroupColor_b", _defaultColorGroupColor->getValue(2));
    
    settings.setValue("DefaultFilterGroupColor_r", _defaultFilterGroupColor->getValue(0));
    settings.setValue("DefaultFilterGroupColor_g", _defaultFilterGroupColor->getValue(1));
    settings.setValue("DefaultFilterGroupColor_b", _defaultFilterGroupColor->getValue(2));
    
    settings.setValue("DefaultTransformGroupColor_r", _defaultTransformGroupColor->getValue(0));
    settings.setValue("DefaultTransformGroupColor_g", _defaultTransformGroupColor->getValue(1));
    settings.setValue("DefaultTransformGroupColor_b", _defaultTransformGroupColor->getValue(2));
    
    settings.setValue("DefaultTimeGroupColor_r", _defaultTimeGroupColor->getValue(0));
    settings.setValue("DefaultTimeGroupColor_g", _defaultTimeGroupColor->getValue(1));
    settings.setValue("DefaultTimeGroupColor_b", _defaultTimeGroupColor->getValue(2));
    
    settings.setValue("DefaultDrawGroupColor_r", _defaultDrawGroupColor->getValue(0));
    settings.setValue("DefaultDrawGroupColor_g", _defaultDrawGroupColor->getValue(1));
    settings.setValue("DefaultDrawGroupColor_b", _defaultDrawGroupColor->getValue(2));
    
    settings.setValue("DefaultKeyerGroupColor_r", _defaultKeyerGroupColor->getValue(0));
    settings.setValue("DefaultKeyerGroupColor_g", _defaultKeyerGroupColor->getValue(1));
    settings.setValue("DefaultKeyerGroupColor_b", _defaultKeyerGroupColor->getValue(2));
    
    settings.setValue("DefaultChannelGroupColor_r", _defaultChannelGroupColor->getValue(0));
    settings.setValue("DefaultChannelGroupColor_g", _defaultChannelGroupColor->getValue(1));
    settings.setValue("DefaultChannelGroupColor_b", _defaultChannelGroupColor->getValue(2));
    
    settings.setValue("DefaultMergeGroupColor_r", _defaultMergeGroupColor->getValue(0));
    settings.setValue("DefaultMergeGroupColor_g", _defaultMergeGroupColor->getValue(1));
    settings.setValue("DefaultMergeGroupColor_b", _defaultMergeGroupColor->getValue(2));
    
    settings.setValue("DefaultViewsGroupColor_r", _defaultViewsGroupColor->getValue(0));
    settings.setValue("DefaultViewsGroupColor_g", _defaultViewsGroupColor->getValue(1));
    settings.setValue("DefaultViewsGroupColor_b", _defaultViewsGroupColor->getValue(2));
    
    settings.setValue("DefaultDeepGroupColor_r", _defaultDeepGroupColor->getValue(0));
    settings.setValue("DefaultDeepGroupColor_g", _defaultDeepGroupColor->getValue(1));
    settings.setValue("DefaultDeepGroupColor_b", _defaultDeepGroupColor->getValue(2));

    settings.endGroup();
    
    settings.beginGroup("Readers");
    
    
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        settings.setValue(_readersMapping[i]->getDescription().c_str(),_readersMapping[i]->getValue());
    }
    settings.endGroup();
    
    settings.beginGroup("Writers");
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        settings.setValue(_writersMapping[i]->getDescription().c_str(),_writersMapping[i]->getValue());
    }
    settings.endGroup();
    
}

void Settings::restoreSettings(){
    
    _wereChangesMadeSinceLastSave = false;
    
    notifyProjectBeginKnobsValuesChanged(Natron::PROJECT_LOADING);
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    if (settings.contains("CheckUpdates")) {
        _checkForUpdates->setValue(settings.value("CheckUpdates").toBool(), 0);
    }
    if (settings.contains("AutoSaveDelay")) {
        _autoSaveDelay->setValue(settings.value("AutoSaveDelay").toInt(),0);
    }
    if(settings.contains("LinearColorPickers")){
        _linearPickers->setValue(settings.value("LinearColorPickers").toBool(),0);
    }
    if (settings.contains("Number of threads")) {
        _numberOfThreads->setValue(settings.value("Number of threads").toInt(),0);
    }
    if (settings.contains("RenderInSeparateProcess")) {
        _renderInSeparateProcess->setValue(settings.value("RenderInSeparateProcess").toBool(),0);
    }
    if (settings.contains("AutoPreviewDefault")) {
        _autoPreviewEnabledForNewProjects->setValue(settings.value("AutoPreviewDefault").toBool(),0);
    }

    if (settings.contains("MaxPanelsOpened")) {
        _maxPanelsOpened->setValue(settings.value("MaxPanelsOpened").toInt(), 0);
    }
    if (settings.contains("RenderOnEditingFinished")) {
        _renderOnEditingFinished->setValue(settings.value("RenderOnEditingFinished").toBool(), 0);
    }
        
    if (settings.contains("ExtraPluginsPaths")) {
        _extraPluginPaths->setValue(settings.value("ExtraPluginsPaths").toString().toStdString(),0);
    }
    if (settings.contains("PreferBundledPlugins")) {
        _preferBundledPlugins->setValue(settings.value("PreferBundledPlugins").toBool(),0);
    }
    if (settings.contains("LoadBundledPlugins")) {
        _loadBundledPlugins->setValue(settings.value("LoadBundledPlugins").toBool(), 0);
    }
    if (settings.contains("HostName")) {
        _hostName->setValue(settings.value("HostName").toString().toStdString(), 0);
    }
    settings.endGroup();
    
    settings.beginGroup("OpenColorIO");
    if (settings.contains("OCIOConfigFile")) {
        _customOcioConfigFile->setValue(settings.value("OCIOConfigFile").toString().toStdString(),0);
    }
    if (settings.contains("OCIOConfig")) {
        int activeIndex = settings.value("OCIOConfig").toInt();
        if (activeIndex < (int)_ocioConfigKnob->getEntries().size()) {
            _ocioConfigKnob->setValue(activeIndex,0);
        }
    }
    settings.endGroup();
    
    settings.beginGroup("Caching");
    if(settings.contains("MaximumRAMUsagePercentage")){
        _maxRAMPercent->setValue(settings.value("MaximumRAMUsagePercentage").toInt(),0);
    }
    if(settings.contains("MaximumPlaybackRAMUsage")){
        _maxPlayBackPercent->setValue(settings.value("MaximumPlaybackRAMUsage").toInt(),0);
    }
    if(settings.contains("MaximumDiskSizeUsage")){
        _maxDiskCacheGB->setValue(settings.value("MaximumDiskSizeUsage").toInt(),0);
    }
    settings.endGroup();
    
    settings.beginGroup("Viewers");
    if(settings.contains("ByteTextures")){
        _texturesMode->setValue(settings.value("ByteTextures").toInt(),0);
    }
    if (settings.contains("TilesPowerOf2")) {
        _powerOf2Tiling->setValue(settings.value("TilesPowerOf2").toInt(),0);
    }
    settings.endGroup();
    
    settings.beginGroup("Nodegraph");
    if (settings.contains("SnapToNode")) {
        _snapNodesToConnections->setValue(settings.value("SnapToNode").toBool(), 0);
    }
    if (settings.contains("ConnectionHints")) {
        _useNodeGraphHints->setValue(settings.value("ConnectionHints").toBool(), 0);
    }
    if (settings.contains("MaximumUndoRedoNodeGraph")) {
        _maxUndoRedoNodeGraph->setValue(settings.value("MaximumUndoRedoNodeGraph").toInt(), 0);
    }
    if (settings.contains("DisconnectedArrowLength")) {
        _disconnectedArrowLength->setValue(settings.value("DisconnectedArrowLength").toInt(), 0);
    }
    if (settings.contains("DefaultNodeColor_r")) {
        _defaultNodeColor->setValue(settings.value("DefaultNodeColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultNodeColor_g")) {
        _defaultNodeColor->setValue(settings.value("DefaultNodeColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultNodeColor_b")) {
        _defaultNodeColor->setValue(settings.value("DefaultNodeColor_b").toDouble(), 2);
    }
    if (settings.contains("DefaultSelectedNodeColor_r")) {
        _defaultSelectedNodeColor->setValue(settings.value("DefaultSelectedNodeColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultSelectedNodeColor_g")) {
        _defaultSelectedNodeColor->setValue(settings.value("DefaultSelectedNodeColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultSelectedNodeColor_b")) {
        _defaultSelectedNodeColor->setValue(settings.value("DefaultSelectedNodeColor_b").toDouble(), 2);
    }

    if (settings.contains("DefaultBackdropColor_r")) {
        _defaultBackdropColor->setValue(settings.value("DefaultBackdropColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultBackdropColor_g")) {
        _defaultBackdropColor->setValue(settings.value("DefaultBackdropColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultBackdropColor_b")) {
        _defaultBackdropColor->setValue(settings.value("DefaultBackdropColor_b").toDouble(), 2);
    }
    
    //////Default group colors
    if (settings.contains("DefaultGeneratorColor_r")) {
        _defaultGeneratorColor->setValue(settings.value("DefaultGeneratorColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultGeneratorColor_g")) {
        _defaultGeneratorColor->setValue(settings.value("DefaultGeneratorColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultGeneratorColor_b")) {
        _defaultGeneratorColor->setValue(settings.value("DefaultGeneratorColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultReaderColor_r")) {
        _defaultReaderColor->setValue(settings.value("DefaultReaderColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultReaderColor_g")) {
        _defaultReaderColor->setValue(settings.value("DefaultReaderColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultReaderColor_b")) {
        _defaultReaderColor->setValue(settings.value("DefaultReaderColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultWriterColor_r")) {
        _defaultWriterColor->setValue(settings.value("DefaultWriterColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultWriterColor_g")) {
        _defaultWriterColor->setValue(settings.value("DefaultWriterColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultWriterColor_b")) {
        _defaultWriterColor->setValue(settings.value("DefaultWriterColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultColorGroupColor_r")) {
        _defaultColorGroupColor->setValue(settings.value("DefaultColorGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultColorGroupColor_g")) {
        _defaultColorGroupColor->setValue(settings.value("DefaultColorGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultColorGroupColor_b")) {
        _defaultColorGroupColor->setValue(settings.value("DefaultColorGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultFilterGroupColor_r")) {
        _defaultFilterGroupColor->setValue(settings.value("DefaultFilterGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultFilterGroupColor_g")) {
        _defaultFilterGroupColor->setValue(settings.value("DefaultFilterGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultFilterGroupColor_b")) {
        _defaultFilterGroupColor->setValue(settings.value("DefaultFilterGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultTransformGroupColor_r")) {
        _defaultTransformGroupColor->setValue(settings.value("DefaultTransformGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultTransformGroupColor_g")) {
        _defaultTransformGroupColor->setValue(settings.value("DefaultTransformGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultTransformGroupColor_b")) {
        _defaultTransformGroupColor->setValue(settings.value("DefaultTransformGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultTimeGroupColor_r")) {
        _defaultTimeGroupColor->setValue(settings.value("DefaultTimeGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultTimeGroupColor_g")) {
        _defaultTimeGroupColor->setValue(settings.value("DefaultTimeGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultTimeGroupColor_b")) {
        _defaultTimeGroupColor->setValue(settings.value("DefaultTimeGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultDrawGroupColor_r")) {
        _defaultDrawGroupColor->setValue(settings.value("DefaultDrawGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultDrawGroupColor_g")) {
        _defaultDrawGroupColor->setValue(settings.value("DefaultDrawGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultDrawGroupColor_b")) {
        _defaultDrawGroupColor->setValue(settings.value("DefaultDrawGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultKeyerGroupColor_r")) {
        _defaultKeyerGroupColor->setValue(settings.value("DefaultKeyerGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultKeyerGroupColor_g")) {
        _defaultKeyerGroupColor->setValue(settings.value("DefaultKeyerGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultKeyerGroupColor_b")) {
        _defaultKeyerGroupColor->setValue(settings.value("DefaultKeyerGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultChannelGroupColor_r")) {
        _defaultChannelGroupColor->setValue(settings.value("DefaultChannelGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultChannelGroupColor_g")) {
        _defaultChannelGroupColor->setValue(settings.value("DefaultChannelGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultChannelGroupColor_b")) {
        _defaultChannelGroupColor->setValue(settings.value("DefaultChannelGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultMergeGroupColor_r")) {
        _defaultMergeGroupColor->setValue(settings.value("DefaultMergeGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultMergeGroupColor_g")) {
        _defaultMergeGroupColor->setValue(settings.value("DefaultMergeGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultMergeGroupColor_b")) {
        _defaultMergeGroupColor->setValue(settings.value("DefaultMergeGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultViewsGroupColor_r")) {
        _defaultViewsGroupColor->setValue(settings.value("DefaultViewsGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultViewsGroupColor_g")) {
        _defaultViewsGroupColor->setValue(settings.value("DefaultViewsGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultViewsGroupColor_b")) {
        _defaultViewsGroupColor->setValue(settings.value("DefaultViewsGroupColor_b").toDouble(), 2);
    }
    
    if (settings.contains("DefaultDeepGroupColor_r")) {
        _defaultDeepGroupColor->setValue(settings.value("DefaultDeepGroupColor_r").toDouble(), 0);
    }
    if (settings.contains("DefaultDeepGroupColor_g")) {
        _defaultDeepGroupColor->setValue(settings.value("DefaultDeepGroupColor_g").toDouble(), 1);
    }
    if (settings.contains("DefaultDeepGroupColor_b")) {
        _defaultDeepGroupColor->setValue(settings.value("DefaultDeepGroupColor_b").toDouble(), 2);
    }
    
    settings.endGroup();
    
    settings.beginGroup("Readers");
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        QString format = _readersMapping[i]->getDescription().c_str();
        if(settings.contains(format)){
            int index = settings.value(format).toInt();
            if (index < (int)_readersMapping[i]->getEntries().size()) {
                _readersMapping[i]->setValue(index,0);
            }
        }
    }
    settings.endGroup();
    
    settings.beginGroup("Writers");
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        QString format = _writersMapping[i]->getDescription().c_str();
        if(settings.contains(format)){
            int index = settings.value(format).toInt();
            if (index < (int)_writersMapping[i]->getEntries().size()) {
                _writersMapping[i]->setValue(index,0);
            }
        }
    }
    settings.endGroup();
    notifyProjectEndKnobsValuesChanged();

}

bool Settings::tryLoadOpenColorIOConfig()
{
    QString configFile;
    if (_customOcioConfigFile->isEnabled(0)) {
        ///try to load from the file
        std::string file = _customOcioConfigFile->getValue();
        if (file.empty()) {
            return false;
        }
        if (!QFile::exists(file.c_str())) {
            Natron::errorDialog("OpenColorIO", file + ": No such file.");
            return false;
        }
        configFile = file.c_str();
    } else {
        ///try to load from the combobox
        QString activeEntryText(_ocioConfigKnob->getActiveEntryText().c_str());
        QString configFileName = QString(activeEntryText + ".ocio");
        QStringList defaultConfigsPaths = getDefaultOcioConfigPaths();
        for (int i = 0; i < defaultConfigsPaths.size();++i) {
            QDir defaultConfigsDir(defaultConfigsPaths[i]);
            if (!defaultConfigsDir.exists()) {
                qDebug() << "Attempt to read an OpenColorIO configuration but the configuration directory does not exist.";
                continue;
            }
            ///try to open the .ocio config file first in the defaultConfigsDir
            ///if we can't find it, try to look in a subdirectory with the name of the config for the file config.ocio
            if (!defaultConfigsDir.exists(configFileName)) {
                QDir subDir(defaultConfigsPaths[i] + QDir::separator() + activeEntryText);
                if (!subDir.exists()) {
                    Natron::errorDialog("OpenColorIO",subDir.absoluteFilePath("config.ocio").toStdString() + ": No such file or directory.");
                    return false;
                }
                if (!subDir.exists("config.ocio")) {
                    Natron::errorDialog("OpenColorIO",subDir.absoluteFilePath("config.ocio").toStdString() + ": No such file or directory.");
                    return false;
                }
                configFile = subDir.absoluteFilePath("config.ocio");
            } else {
                configFile = defaultConfigsDir.absoluteFilePath(configFileName);
            }
        }
        if (configFile.isEmpty()) {
            return false;
        }
        
    }
    qDebug() << "setting OCIO=" << configFile;
    qputenv("OCIO", configFile.toUtf8());
    return true;
}

void Settings::onKnobValueChanged(KnobI* k,Natron::ValueChangedReason /*reason*/){

    _wereChangesMadeSinceLastSave = true;

    
    if (k == _texturesMode.get()) {
        std::map<int,AppInstanceRef> apps = appPTR->getAppInstances();
        bool isFirstViewer = true;
        for(std::map<int,AppInstanceRef>::iterator it = apps.begin();it!=apps.end();++it){
            const std::vector<boost::shared_ptr<Node> > nodes = it->second.app->getProject()->getCurrentNodes();
            for (U32 i = 0; i < nodes.size(); ++i) {
                assert(nodes[i]);
                if (nodes[i]->pluginID() == "Viewer") {
                    ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
                    assert(n);
                    if(isFirstViewer){
                        if(!n->supportsGLSL() && _texturesMode->getValue() != 0){
                            Natron::errorDialog("Viewer", "You need OpenGL GLSL in order to use 32 bit fp texutres.\n"
                                                "Reverting to 8bits textures.");
                            _texturesMode->setValue(0,0);
                            return;
                        }
                    }
                    n->updateTreeAndRender();
                }
            }
        }
    } else if(k == _maxDiskCacheGB.get()) {
        appPTR->setApplicationsCachesMaximumDiskSpace(getMaximumDiskCacheSize());
    } else if(k == _maxRAMPercent.get()) {
        appPTR->setApplicationsCachesMaximumMemoryPercent(getRamMaximumPercent());
    } else if(k == _maxPlayBackPercent.get()) {
        appPTR->setPlaybackCacheMaximumSize(getRamPlaybackMaximumPercent());
    } else if(k == _numberOfThreads.get()) {
        int nbThreads = getNumberOfThreads();
        if (nbThreads == -1) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else if (nbThreads == 0) {
            QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
        }
    } else if(k == _ocioConfigKnob.get()) {
        if (_ocioConfigKnob->getActiveEntryText() == std::string(NATRON_CUSTOM_OCIO_CONFIG_NAME)) {
            _customOcioConfigFile->setAllDimensionsEnabled(true);
        } else {
            _customOcioConfigFile->setAllDimensionsEnabled(false);
        }
        tryLoadOpenColorIOConfig();
         
    } else if (k == _customOcioConfigFile.get()) {
        tryLoadOpenColorIOConfig();
    } else if (k == _maxUndoRedoNodeGraph.get()) {
        appPTR->setUndoRedoStackLimit(_maxUndoRedoNodeGraph->getValue());
    } else if (k == _maxPanelsOpened.get()) {
        appPTR->onMaxPanelsOpenedChanged(_maxPanelsOpened->getValue());
    }
}

int Settings::getViewersBitDepth() const {
    return _texturesMode->getValue();
}

int Settings::getViewerTilesPowerOf2() const {
    return _powerOf2Tiling->getValue();
}

double Settings::getRamMaximumPercent() const {
    return (double)_maxRAMPercent->getValue() / 100.;
}

double Settings::getRamPlaybackMaximumPercent() const{
    return (double)_maxPlayBackPercent->getValue() / 100.;
}

U64 Settings::getMaximumDiskCacheSize() const {
    return ((U64)(_maxDiskCacheGB->getValue()) * std::pow(1024.,3.));
}
bool Settings::getColorPickerLinear() const {
    return _linearPickers->getValue();
}

int Settings::getNumberOfThreads() const {
    return _numberOfThreads->getValue();
}

void Settings::setNumberOfThreads(int threadsNb) {
    _numberOfThreads->setValue(threadsNb,0);
}

bool Settings::isAutoPreviewOnForNewProjects() const {
    return _autoPreviewEnabledForNewProjects->getValue();
}

const std::string& Settings::getReaderPluginIDForFileType(const std::string& extension){
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        if (_readersMapping[i]->getDescription() == extension) {
            const std::vector<std::string>& entries =  _readersMapping[i]->getEntries();
            int index = _readersMapping[i]->getValue();
            assert(index < (int)entries.size());
            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
 }

const std::string& Settings::getWriterPluginIDForFileType(const std::string& extension){
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        if (_writersMapping[i]->getDescription() == extension) {
            const std::vector<std::string>& entries =  _writersMapping[i]->getEntries();
            int index = _writersMapping[i]->getValue();
            assert(index < (int)entries.size());
            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
}

void Settings::populateReaderPluginsAndFormats(const std::map<std::string,std::vector<std::string> >& rows){
    
    for (std::map<std::string,std::vector<std::string> >::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(this, it->first);
        k->setAnimationEnabled(false);
        k->populateChoices(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote ReadOIIO !
            if (QString(it->second[i].c_str()).contains("ReadOIIOOFX")) {
                k->setValue(i,0);
                break;
            }
        }
        _readersMapping.push_back(k);
        _readersTab->addKnob(k);
    }
}

void Settings::populateWriterPluginsAndFormats(const std::map<std::string,std::vector<std::string> >& rows){
    for (std::map<std::string,std::vector<std::string> >::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(this, it->first);
        k->setAnimationEnabled(false);
        k->populateChoices(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote WriteOIIOOFX !
            if (QString(it->second[i].c_str()).contains("WriteOIIOOFX")) {
                k->setValue(i,0);
                break;
            }
        }
        _writersMapping.push_back(k);
        _writersTab->addKnob(k);
    }
}

void Settings::getFileFormatsForReadingAndReader(std::map<std::string,std::string>* formats){
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _readersMapping[i]->getEntries();
        int index = _readersMapping[i]->getValue();
        
        assert(index < (int)entries.size());
        
        formats->insert(std::make_pair(_readersMapping[i]->getDescription(),entries[index]));
    }
}

void Settings::getFileFormatsForWritingAndWriter(std::map<std::string,std::string>* formats){
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _writersMapping[i]->getEntries();
        int index = _writersMapping[i]->getValue();
        
        assert(index < (int)entries.size());
        
        formats->insert(std::make_pair(_writersMapping[i]->getDescription(),entries[index]));
    }
}

QStringList Settings::getPluginsExtraSearchPaths() const {
    QString paths = _extraPluginPaths->getValue().c_str();
    return paths.split(QChar(';'));
}

void Settings::restoreDefault() {
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    if (!QFile::remove(settings.fileName())) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }
    
    beginKnobsValuesChanged(Natron::PLUGIN_EDITED);
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        for (int j = 0; j < knobs[i]->getDimension();++j) {
            knobs[i]->resetToDefaultValue(j);
        }
    }
    endKnobsValuesChanged(Natron::PLUGIN_EDITED);
}

bool Settings::isRenderInSeparatedProcessEnabled() const {
    return _renderInSeparateProcess->getValue();
}

int Settings::getMaximumUndoRedoNodeGraph() const
{
    return _maxUndoRedoNodeGraph->getValue();
}

int Settings::getAutoSaveDelayMS() const
{
    return _autoSaveDelay->getValue() * 1000;
}

bool Settings::isSnapToNodeEnabled() const
{
    return _snapNodesToConnections->getValue();
}


bool Settings::isCheckForUpdatesEnabled() const
{
    return _checkForUpdates->getValue();
}

void Settings::setCheckUpdatesEnabled(bool enabled)
{
    _checkForUpdates->setValue(enabled, 0);
    saveSettings();
}

int Settings::getMaxPanelsOpened() const
{
    return _maxPanelsOpened->getValue();
}

void Settings::setMaxPanelsOpened(int maxPanels)
{
    _maxPanelsOpened->setValue(maxPanels, 0);
}

void Settings::setConnectionHintsEnabled(bool enabled)
{
    _useNodeGraphHints->setValue(enabled, 0);
}

bool Settings::isConnectionHintEnabled() const
{
    return _useNodeGraphHints->getValue();
}

bool Settings::loadBundledPlugins() const
{
    return _loadBundledPlugins->getValue();
}

bool Settings::preferBundledPlugins() const
{
    return _preferBundledPlugins->getValue();
}

void Settings::getDefaultNodeColor(float *r,float *g,float *b) const
{
    *r = _defaultNodeColor->getValue(0);
    *g = _defaultNodeColor->getValue(1);
    *b = _defaultNodeColor->getValue(2);
}

void Settings::getDefaultSelectedNodeColor(float *r,float *g,float *b) const
{
    *r = _defaultSelectedNodeColor->getValue(0);
    *g = _defaultSelectedNodeColor->getValue(1);
    *b = _defaultSelectedNodeColor->getValue(2);
}

void Settings::getDefaultBackDropColor(float *r,float *g,float *b) const
{
    *r = _defaultBackdropColor->getValue(0);
    *g = _defaultBackdropColor->getValue(1);
    *b = _defaultBackdropColor->getValue(2);
}

void Settings::getGeneratorColor(float *r,float *g,float *b) const
{
    *r = _defaultGeneratorColor->getValue(0);
    *g = _defaultGeneratorColor->getValue(1);
    *b = _defaultGeneratorColor->getValue(2);
}

void Settings::getReaderColor(float *r,float *g,float *b) const
{
    *r = _defaultReaderColor->getValue(0);
    *g = _defaultReaderColor->getValue(1);
    *b = _defaultReaderColor->getValue(2);
}

void Settings::getWriterColor(float *r,float *g,float *b) const
{
    *r = _defaultWriterColor->getValue(0);
    *g = _defaultWriterColor->getValue(1);
    *b = _defaultWriterColor->getValue(2);
}

void Settings::getColorGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultColorGroupColor->getValue(0);
    *g = _defaultColorGroupColor->getValue(1);
    *b = _defaultColorGroupColor->getValue(2);
}

void Settings::getFilterGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultFilterGroupColor->getValue(0);
    *g = _defaultFilterGroupColor->getValue(1);
    *b = _defaultFilterGroupColor->getValue(2);
}

void Settings::getTransformGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultTransformGroupColor->getValue(0);
    *g = _defaultTransformGroupColor->getValue(1);
    *b = _defaultTransformGroupColor->getValue(2);
}

void Settings::getTimeGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultTimeGroupColor->getValue(0);
    *g = _defaultTimeGroupColor->getValue(1);
    *b = _defaultTimeGroupColor->getValue(2);
}

void Settings::getDrawGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultDrawGroupColor->getValue(0);
    *g = _defaultDrawGroupColor->getValue(1);
    *b = _defaultDrawGroupColor->getValue(2);
}

void Settings::getKeyerGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultKeyerGroupColor->getValue(0);
    *g = _defaultKeyerGroupColor->getValue(1);
    *b = _defaultKeyerGroupColor->getValue(2);
}

void Settings::getChannelGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultChannelGroupColor->getValue(0);
    *g = _defaultChannelGroupColor->getValue(1);
    *b = _defaultChannelGroupColor->getValue(2);
}

void Settings::getMergeGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultMergeGroupColor->getValue(0);
    *g = _defaultMergeGroupColor->getValue(1);
    *b = _defaultMergeGroupColor->getValue(2);
}

void Settings::getViewsGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultViewsGroupColor->getValue(0);
    *g = _defaultViewsGroupColor->getValue(1);
    *b = _defaultViewsGroupColor->getValue(2);
}

void Settings::getDeepGroupColor(float *r,float *g,float *b) const
{
    *r = _defaultDeepGroupColor->getValue(0);
    *g = _defaultDeepGroupColor->getValue(1);
    *b = _defaultDeepGroupColor->getValue(2);
}

int Settings::getDisconnectedArrowLength() const
{
    return _disconnectedArrowLength->getValue();
}

std::string Settings::getHostName() const
{
    return _hostName->getValue();
}

bool Settings::getRenderOnEditingFinishedOnly() const
{
    return _renderOnEditingFinished->getValue();
}

void Settings::setRenderOnEditingFinishedOnly(bool render)
{
    _renderOnEditingFinished->setValue(render, 0);
}