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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Settings.h"

#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QThreadPool>
#include <QtCore/QThread>

#ifdef WINDOWS
#include <tchar.h>
#include <ofxhUtilities.h> // for wideStringToString
#endif

#include "Global/MemoryInfo.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/StandardPaths.h"
#include "Engine/ViewerInstance.h"

#include "Gui/GuiDefines.h"

#include "SequenceParsing.h"

#ifdef WINDOWS
#include <ofxhPluginCache.h>
#endif

#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

#define NATRON_DEFAULT_APPEARANCE_VERSION 1

using namespace Natron;


Settings::Settings(AppInstance* appInstance)
    : KnobHolder(appInstance)
    , _restoringSettings(false)
    , _ocioRestored(false)
    , _settingsExisted(false)
    , _defaultAppearanceOutdated(false)
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

void
Settings::initializeKnobs()
{
    _generalTab = Natron::createKnob<KnobPage>(this, "General");

    _natronSettingsExist = Natron::createKnob<KnobBool>(this, "Existing settings");
    _natronSettingsExist->setAnimationEnabled(false);
    _natronSettingsExist->setName("existingSettings");
    _natronSettingsExist->setSecret(true);
    _generalTab->addKnob(_natronSettingsExist);
    

    _checkForUpdates = Natron::createKnob<KnobBool>(this, "Always check for updates on start-up");
    _checkForUpdates->setName("checkForUpdates");
    _checkForUpdates->setAnimationEnabled(false);
    _checkForUpdates->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " will check for new updates on start-up of the application.");
    _generalTab->addKnob(_checkForUpdates);
    
    _notifyOnFileChange = Natron::createKnob<KnobBool>(this, "Warn when a file changes externally");
    _notifyOnFileChange->setName("warnOnExternalChange");
    _notifyOnFileChange->setAnimationEnabled(false);
    _notifyOnFileChange->setHintToolTip("When checked, if a file read from a file parameter changes externally, a warning will be displayed "
                                        "on the viewer. Turning this off will suspend the notification system.");
    _generalTab->addKnob(_notifyOnFileChange);

    _autoSaveDelay = Natron::createKnob<KnobInt>(this, "Auto-save trigger delay");
    _autoSaveDelay->setName("autoSaveDelay");
    _autoSaveDelay->setAnimationEnabled(false);
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setMinimum(0);
    _autoSaveDelay->setMaximum(60);
    _autoSaveDelay->setHintToolTip("The number of seconds after an event that " NATRON_APPLICATION_NAME " should wait before "
                                                                                                        " auto-saving. Note that if a render is in progress, " NATRON_APPLICATION_NAME " will "
                                                                                                                                                                                       " wait until it is done to actually auto-save.");
    _generalTab->addKnob(_autoSaveDelay);


    _linearPickers = Natron::createKnob<KnobBool>(this, "Linear color pickers");
    _linearPickers->setName("linearPickers");
    _linearPickers->setAnimationEnabled(false);
    _linearPickers->setHintToolTip("When activated, all colors picked from the color parameters are linearized "
                                   "before being fetched. Otherwise they are in the same colorspace "
                                   "as the viewer they were picked from.");
    _generalTab->addKnob(_linearPickers);
    
    _convertNaNValues = Natron::createKnob<KnobBool>(this, "Convert NaN values");
    _convertNaNValues->setName("convertNaNs");
    _convertNaNValues->setAnimationEnabled(false);
    _convertNaNValues->setHintToolTip("When activated, any pixel that is a Not-a-Number will be converted to 1 to avoid potential crashes from "
                                      "downstream nodes. These values can be produced by faulty plug-ins when they use wrong arithmetic such as "
                                      "division by zero. Disabling this option will keep the NaN(s) in the buffers: this may lead to an "
                                      "undefined behavior.");
    _generalTab->addKnob(_convertNaNValues);

    _numberOfThreads = Natron::createKnob<KnobInt>(this, "Number of render threads (0=\"guess\")");
    _numberOfThreads->setName("noRenderThreads");
    _numberOfThreads->setAnimationEnabled(false);

    QString numberOfThreadsToolTip = QString("Controls how many threads " NATRON_APPLICATION_NAME " should use to render. \n"
                                                                                                  "-1: Disable multithreading totally (useful for debugging) \n"
                                                                                                  "0: Guess the thread count from the number of cores. The ideal threads count for this hardware is %1.").arg( QThread::idealThreadCount() );
    _numberOfThreads->setHintToolTip( numberOfThreadsToolTip.toStdString() );
    _numberOfThreads->disableSlider();
    _numberOfThreads->setMinimum(-1);
    _numberOfThreads->setDisplayMinimum(-1);
    _generalTab->addKnob(_numberOfThreads);
    
    _numberOfParallelRenders = Natron::createKnob<KnobInt>(this, "Number of parallel renders (0=\"guess\")");
    _numberOfParallelRenders->setHintToolTip("Controls the number of parallel frame that will be rendered at the same time by the renderer."
                                             "A value of 0 indicate that " NATRON_APPLICATION_NAME " should automatically determine "
                                                                                                   "the best number of parallel renders to launch given your CPU activity. "
                                                                                                   "Setting a value different than 0 should be done only if you know what you're doing and can lead "
                                                                                                   "in some situations to worse performances. Overall to get the best performances you should have your "
                                                                                                   "CPU at 100% activity without idle times. ");
    _numberOfParallelRenders->setName("nParallelRenders");
    _numberOfParallelRenders->setMinimum(0);
    _numberOfParallelRenders->disableSlider();
    _numberOfParallelRenders->setAnimationEnabled(false);
    _generalTab->addKnob(_numberOfParallelRenders);
    
    _useThreadPool = Natron::createKnob<KnobBool>(this, "Effects use thread-pool");
    _useThreadPool->setName("useThreadPool");
    _useThreadPool->setHintToolTip("When checked, all effects will use a global thread-pool to do their processing instead of launching "
                                   "their own threads. "
                                   "This suppresses the overhead created by the operating system creating new threads on demand for "
                                   "each rendering of a special effect. As a result of this, the rendering might be faster on systems "
                                   "with a lot of cores (>= 8). \n"
                                   "WARNING: This is known not to work when using The Foundry's Furnace plug-ins (and potentially "
                                   "some other plug-ins that the dev team hasn't not tested against it). When using these plug-ins, "
                                   "make sure to uncheck this option first otherwise it will crash " NATRON_APPLICATION_NAME);
    _useThreadPool->setAnimationEnabled(false);
    _generalTab->addKnob(_useThreadPool);

    _nThreadsPerEffect = Natron::createKnob<KnobInt>(this, "Max threads usable per effect (0=\"guess\")");
    _nThreadsPerEffect->setName("nThreadsPerEffect");
    _nThreadsPerEffect->setAnimationEnabled(false);
    _nThreadsPerEffect->setHintToolTip("Controls how many threads a specific effect can use at most to do its processing. "
                                       "A high value will allow 1 effect to spawn lots of thread and might not be efficient because "
                                       "the time spent to launch all the threads might exceed the time spent actually processing."
                                       "By default (0) the renderer applies an heuristic to determine what's the best number of threads "
                                       "for an effect.");
    
    _nThreadsPerEffect->setMinimum(0);
    _nThreadsPerEffect->disableSlider();
    _generalTab->addKnob(_nThreadsPerEffect);

    _renderInSeparateProcess = Natron::createKnob<KnobBool>(this, "Render in a separate process");
    _renderInSeparateProcess->setName("renderNewProcess");
    _renderInSeparateProcess->setAnimationEnabled(false);
    _renderInSeparateProcess->setHintToolTip("If true, " NATRON_APPLICATION_NAME " will render frames to disk in "
                                                                                 "a separate process so that if the main application crashes, the render goes on.");
    _generalTab->addKnob(_renderInSeparateProcess);

    _autoPreviewEnabledForNewProjects = Natron::createKnob<KnobBool>(this, "Auto-preview enabled by default for new projects");
    _autoPreviewEnabledForNewProjects->setName("enableAutoPreviewNewProjects");
    _autoPreviewEnabledForNewProjects->setAnimationEnabled(false);
    _autoPreviewEnabledForNewProjects->setHintToolTip("If checked, then when creating a new project, the Auto-preview option"
                                                      " is enabled.");
    _generalTab->addKnob(_autoPreviewEnabledForNewProjects);


    _firstReadSetProjectFormat = Natron::createKnob<KnobBool>(this, "First image read set project format");
    _firstReadSetProjectFormat->setName("autoProjectFormat");
    _firstReadSetProjectFormat->setAnimationEnabled(false);
    _firstReadSetProjectFormat->setHintToolTip("If checked, the first image you read in the project sets the project format to the "
                                               "image size.");
    _generalTab->addKnob(_firstReadSetProjectFormat);
    
    _fixPathsOnProjectPathChanged = Natron::createKnob<KnobBool>(this, "Auto fix relative file-paths");
    _fixPathsOnProjectPathChanged->setAnimationEnabled(false);
    _fixPathsOnProjectPathChanged->setHintToolTip("If checked, when a project-path changes (either the name or the value pointed to), "
                                                  NATRON_APPLICATION_NAME " checks all file-path parameters in the project and tries to fix them.");
    _fixPathsOnProjectPathChanged->setName("autoFixRelativePaths");
    _generalTab->addKnob(_fixPathsOnProjectPathChanged);
    
    _maxPanelsOpened = Natron::createKnob<KnobInt>(this, "Maximum number of open settings panels (0=\"unlimited\")");
    _maxPanelsOpened->setName("maxPanels");
    _maxPanelsOpened->setHintToolTip("This property holds the maximum number of settings panels that can be "
                                     "held by the properties dock at the same time."
                                     "The special value of 0 indicates there can be an unlimited number of panels opened.");
    _maxPanelsOpened->setAnimationEnabled(false);
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setMinimum(1);
    _maxPanelsOpened->setMaximum(100);
    _generalTab->addKnob(_maxPanelsOpened);

    _useCursorPositionIncrements = Natron::createKnob<KnobBool>(this, "Value increments based on cursor position");
    _useCursorPositionIncrements->setName("cursorPositionAwareFields");
    _useCursorPositionIncrements->setHintToolTip("When enabled, incrementing the value fields of parameters with the "
                                                 "mouse wheel or with arrow keys will increment the digits on the right "
                                                 "of the cursor. \n"
                                                 "When disabled, the value fields are incremented given what the plug-in "
                                                 "decided it should be. You can alter this increment by holding "
                                                 "Shift (x10) or Control (/10) while incrementing.");
    _useCursorPositionIncrements->setAnimationEnabled(false);
    _generalTab->addKnob(_useCursorPositionIncrements);

    _defaultLayoutFile = Natron::createKnob<KnobFile>(this, "Default layout file");
    _defaultLayoutFile->setName("defaultLayout");
    _defaultLayoutFile->setHintToolTip("When set, " NATRON_APPLICATION_NAME " uses the given layout file "
                                                                            "as default layout for new projects. You can export/import a layout to/from a file "
                                                                            "from the Layout menu. If empty, the default application layout is used.");
    _defaultLayoutFile->setAnimationEnabled(false);
    _generalTab->addKnob(_defaultLayoutFile);

    _renderOnEditingFinished = Natron::createKnob<KnobBool>(this, "Refresh viewer only when editing is finished");
    _renderOnEditingFinished->setName("renderOnEditingFinished");
    _renderOnEditingFinished->setHintToolTip("When checked, the viewer triggers a new render only when mouse is released when editing parameters, curves "
                                             " or the timeline. This setting doesn't apply to roto splines editing.");
    _renderOnEditingFinished->setAnimationEnabled(false);
    _generalTab->addKnob(_renderOnEditingFinished);
    
    _activateRGBSupport = Natron::createKnob<KnobBool>(this, "RGB components support");
    _activateRGBSupport->setHintToolTip("When checked " NATRON_APPLICATION_NAME " is able to process images with only RGB components "
                                                                                "(support for images with RGBA and Alpha components is always enabled). "
                                                                                "Un-checking this option may prevent plugins that do not well support RGB components from crashing " NATRON_APPLICATION_NAME ". "
                                                                                                                                                                                                             "Changing this option requires a restart of the application.");
    _activateRGBSupport->setAnimationEnabled(false);
    _activateRGBSupport->setName("rgbSupport");
    _generalTab->addKnob(_activateRGBSupport);


    _activateTransformConcatenationSupport = Natron::createKnob<KnobBool>(this, "Transforms concatenation support");
    _activateTransformConcatenationSupport->setHintToolTip("When checked " NATRON_APPLICATION_NAME " is able to concatenate transform effects "
                                                                                                   "when they are chained in the compositing tree. This yields better results and faster "
                                                                                                   "render times because the image is only filtered once instead of as many times as there are "
                                                                                                   "transformations.");
    _activateTransformConcatenationSupport->setAnimationEnabled(false);
    _activateTransformConcatenationSupport->setName("transformCatSupport");
    _generalTab->addKnob(_activateTransformConcatenationSupport);
    
    
    _hostName = Natron::createKnob<KnobString>(this, "Host name");
    _hostName->setName("hostName");
#pragma message WARN("TODO: make a choice menu out of these host names (with a 'custom host' entry), and tell the user to restart Natron when it is changed.")
    _hostName->setHintToolTip("This is the name of the OpenFX host (application) as it appears to the OpenFX plugins. "
                              "Changing it to the name of another application can help loading some plugins which "
                              "restrict their usage to specific OpenFX hosts. You shoud leave "
                              "this to its default value, unless a specific plugin refuses to load or run. "
                              "Changing this takes effect upon the next application launch, and requires clearing "
                              "the OpenFX plugins cache from the Cache menu. "
                              "Here is a list of known OpenFX hosts: \n"
                              "uk.co.thefoundry.nuke\n"
                              "com.eyeonline.Fusion\n"
                              "com.sonycreativesoftware.vegas\n"
                              "Autodesk Toxik\n"
                              "Assimilator\n"
                              "Dustbuster\n"
                              "DaVinciResolve\n"
                              "DaVinciResolveLite\n"
                              "Mistika\n"
                              "com.apple.shake\n"
                              "Baselight\n"
                              "IRIDAS Framecycler\n"
                              "Ramen\n"
                              "TuttleOfx\n"
                              "\n"
                              "The default host name is: \n"
                              NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);
    _hostName->setAnimationEnabled(false);
    _generalTab->addKnob(_hostName);
    
    //////////////APPEARANCE TAB/////////////////
    _appearanceTab = Natron::createKnob<KnobPage>(this, "Appearance");
    
    _defaultAppearanceVersion = Natron::createKnob<KnobInt>(this, "Appearance version");
    _defaultAppearanceVersion->setName("appearanceVersion");
    _defaultAppearanceVersion->setAnimationEnabled(false);
    _defaultAppearanceVersion->setSecret(true);
    _appearanceTab->addKnob(_defaultAppearanceVersion);
    
    _systemFontChoice = Natron::createKnob<KnobChoice>(this, "Font");
    _systemFontChoice->setHintToolTip("List of all fonts available on your system");
    _systemFontChoice->setName("systemFont");
    _systemFontChoice->setAddNewLine(false);
    _systemFontChoice->setAnimationEnabled(false);
    _appearanceTab->addKnob(_systemFontChoice);
    
    _fontSize = Natron::createKnob<KnobInt>(this, "Font size");
    _fontSize->setName("fontSize");
    _fontSize->setAnimationEnabled(false);
    _appearanceTab->addKnob(_fontSize);
    
    _qssFile = Natron::createKnob<KnobFile>(this, "Stylesheet file (.qss)");
    _qssFile->setName("stylesheetFile");
    _qssFile->setHintToolTip("When pointing to a valid .qss file, the stylesheet of the application will be set according to this file instead of the default "
                             "stylesheet. You can adapt the default stylesheet that can be found in your distribution of " NATRON_APPLICATION_NAME ".");
    _qssFile->setAnimationEnabled(false);
    _appearanceTab->addKnob(_qssFile);
    
    _guiColors = Natron::createKnob<KnobGroup>(this, "GUI colors");
    _guiColors->setAsTab();
    _appearanceTab->addKnob(_guiColors);
    
    _curveEditorColors = Natron::createKnob<KnobGroup>(this, "Curve Editor");
    _curveEditorColors->setAsTab();
    _appearanceTab->addKnob(_curveEditorColors);

    _dopeSheetEditorColors = Natron::createKnob<KnobGroup>(this, "Dope Sheet");
    _dopeSheetEditorColors->setAsTab();
    _appearanceTab->addKnob(_dopeSheetEditorColors);
    
    _sunkenColor =  Natron::createKnob<KnobColor>(this, "Sunken",3);
    _sunkenColor->setName("sunken");
    _sunkenColor->setAnimationEnabled(false);
    _sunkenColor->setSimplified(true);
    _sunkenColor->setAddNewLine(false);
    _guiColors->addKnob(_sunkenColor);
    
    _baseColor =  Natron::createKnob<KnobColor>(this, "Base",3);
    _baseColor->setName("base");
    _baseColor->setAnimationEnabled(false);
    _baseColor->setSimplified(true);
    _baseColor->setAddNewLine(false);
    _guiColors->addKnob(_baseColor);
    
    _raisedColor =  Natron::createKnob<KnobColor>(this, "Raised",3);
    _raisedColor->setName("raised");
    _raisedColor->setAnimationEnabled(false);
    _raisedColor->setSimplified(true);
    _raisedColor->setAddNewLine(false);
    _guiColors->addKnob(_raisedColor);
    
    _selectionColor =  Natron::createKnob<KnobColor>(this, "Selection",3);
    _selectionColor->setName("selection");
    _selectionColor->setAnimationEnabled(false);
    _selectionColor->setSimplified(true);
    _selectionColor->setAddNewLine(false);
    _guiColors->addKnob(_selectionColor);
    
    _textColor =  Natron::createKnob<KnobColor>(this, "Text",3);
    _textColor->setName("text");
    _textColor->setAnimationEnabled(false);
    _textColor->setSimplified(true);
    _textColor->setAddNewLine(false);
    _guiColors->addKnob(_textColor);
    
    _altTextColor =  Natron::createKnob<KnobColor>(this, "Unmodified text",3);
    _altTextColor->setName("unmodifiedText");
    _altTextColor->setAnimationEnabled(false);
    _altTextColor->setSimplified(true);
    _guiColors->addKnob(_altTextColor);
    
    _timelinePlayheadColor =  Natron::createKnob<KnobColor>(this, "Timeline playhead",3);
    _timelinePlayheadColor->setName("timelinePlayhead");
    _timelinePlayheadColor->setAnimationEnabled(false);
    _timelinePlayheadColor->setSimplified(true);
    _timelinePlayheadColor->setAddNewLine(false);
    _guiColors->addKnob(_timelinePlayheadColor);
    
    
    _timelineBGColor =  Natron::createKnob<KnobColor>(this, "Timeline background",3);
    _timelineBGColor->setName("timelineBG");
    _timelineBGColor->setAnimationEnabled(false);
    _timelineBGColor->setSimplified(true);
    _timelineBGColor->setAddNewLine(false);
    _guiColors->addKnob(_timelineBGColor);
    
    _timelineBoundsColor =  Natron::createKnob<KnobColor>(this, "Timeline bounds",3);
    _timelineBoundsColor->setName("timelineBound");
    _timelineBoundsColor->setAnimationEnabled(false);
    _timelineBoundsColor->setSimplified(true);
    _timelineBoundsColor->setAddNewLine(false);
    _guiColors->addKnob(_timelineBoundsColor);
    
    _cachedFrameColor =  Natron::createKnob<KnobColor>(this, "Cached frame",3);
    _cachedFrameColor->setName("cachedFrame");
    _cachedFrameColor->setAnimationEnabled(false);
    _cachedFrameColor->setSimplified(true);
    _cachedFrameColor->setAddNewLine(false);
    _guiColors->addKnob(_cachedFrameColor);
    
    _diskCachedFrameColor =  Natron::createKnob<KnobColor>(this, "Disk cached frame",3);
    _diskCachedFrameColor->setName("diskCachedFrame");
    _diskCachedFrameColor->setAnimationEnabled(false);
    _diskCachedFrameColor->setSimplified(true);
    _guiColors->addKnob(_diskCachedFrameColor);
    
    _interpolatedColor =  Natron::createKnob<KnobColor>(this, "Interpolated value",3);
    _interpolatedColor->setName("interpValue");
    _interpolatedColor->setAnimationEnabled(false);
    _interpolatedColor->setSimplified(true);
    _interpolatedColor->setAddNewLine(false);
    _guiColors->addKnob(_interpolatedColor);
    
    _keyframeColor =  Natron::createKnob<KnobColor>(this, "Keyframe",3);
    _keyframeColor->setName("keyframe");
    _keyframeColor->setAnimationEnabled(false);
    _keyframeColor->setSimplified(true);
    _keyframeColor->setAddNewLine(false);
    _guiColors->addKnob(_keyframeColor);
    
    _exprColor =  Natron::createKnob<KnobColor>(this, "Expression",3);
    _exprColor->setName("exprColor");
    _exprColor->setAnimationEnabled(false);
    _exprColor->setSimplified(true);
    _guiColors->addKnob(_exprColor);
    
    
    _curveEditorBGColor =  Natron::createKnob<KnobColor>(this, "Background color",3);
    _curveEditorBGColor->setName("curveEditorBG");
    _curveEditorBGColor->setAnimationEnabled(false);
    _curveEditorBGColor->setSimplified(true);
    _curveEditorBGColor->setAddNewLine(false);
    _curveEditorColors->addKnob(_curveEditorBGColor);

    
    _gridColor =  Natron::createKnob<KnobColor>(this, "Grid color",3);
    _gridColor->setName("curveditorGrid");
    _gridColor->setAnimationEnabled(false);
    _gridColor->setSimplified(true);
    _gridColor->setAddNewLine(false);
    _curveEditorColors->addKnob(_gridColor);


    _curveEditorScaleColor =  Natron::createKnob<KnobColor>(this, "Scale color",3);
    _curveEditorScaleColor->setName("curveeditorScale");
    _curveEditorScaleColor->setAnimationEnabled(false);
    _curveEditorScaleColor->setSimplified(true);
    _curveEditorColors->addKnob(_curveEditorScaleColor);

    // Create the dope sheet editor settings page
    _dopeSheetEditorBackgroundColor = Natron::createKnob<KnobColor>(this, "Sheet background color", 3);
    _dopeSheetEditorBackgroundColor->setName("dopesheetBackground");
    _dopeSheetEditorBackgroundColor->setAnimationEnabled(false);
    _dopeSheetEditorBackgroundColor->setSimplified(true);
    _dopeSheetEditorColors->addKnob(_dopeSheetEditorBackgroundColor);

    _dopeSheetEditorRootSectionBackgroundColor = Natron::createKnob<KnobColor>(this, "Root section background color", 4);
    _dopeSheetEditorRootSectionBackgroundColor->setName("dopesheetRootSectionBackground");
    _dopeSheetEditorRootSectionBackgroundColor->setAnimationEnabled(false);
    _dopeSheetEditorRootSectionBackgroundColor->setSimplified(true);
    _dopeSheetEditorRootSectionBackgroundColor->setAddNewLine(false);
    _dopeSheetEditorColors->addKnob(_dopeSheetEditorRootSectionBackgroundColor);

    _dopeSheetEditorKnobSectionBackgroundColor = Natron::createKnob<KnobColor>(this, "Knob section background color", 4);
    _dopeSheetEditorKnobSectionBackgroundColor->setName("dopesheetKnobSectionBackground");
    _dopeSheetEditorKnobSectionBackgroundColor->setAnimationEnabled(false);
    _dopeSheetEditorKnobSectionBackgroundColor->setSimplified(true);
    _dopeSheetEditorColors->addKnob(_dopeSheetEditorKnobSectionBackgroundColor);

    _dopeSheetEditorScaleColor = Natron::createKnob<KnobColor>(this, "Sheet scale color", 3);
    _dopeSheetEditorScaleColor->setName("dopesheetScale");
    _dopeSheetEditorScaleColor->setAnimationEnabled(false);
    _dopeSheetEditorScaleColor->setSimplified(true);
    _dopeSheetEditorScaleColor->setAddNewLine(false);
    _dopeSheetEditorColors->addKnob(_dopeSheetEditorScaleColor);

    _dopeSheetEditorGridColor = Natron::createKnob<KnobColor>(this, "Sheet grid color", 3);
    _dopeSheetEditorGridColor->setName("dopesheetGrid");
    _dopeSheetEditorGridColor->setAnimationEnabled(false);
    _dopeSheetEditorGridColor->setSimplified(true);
    _dopeSheetEditorColors->addKnob(_dopeSheetEditorGridColor);


    boost::shared_ptr<KnobPage> ocioTab = Natron::createKnob<KnobPage>(this, "OpenColorIO");
    _ocioConfigKnob = Natron::createKnob<KnobChoice>(this, "OpenColorIO config");
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
                if ( entries[j].contains(NATRON_DEFAULT_OCIO_CONFIG_NAME) ) {
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
    _ocioConfigKnob->setHintToolTip("Select the OpenColorIO configuration you would like to use globally for all "
                                    "operators and plugins that use OpenColorIO, by setting the \"OCIO\" "
                                    "environment variable. Only nodes created after changing this parameter will take "
                                    "it into account, and it is better to restart the application after changing it. "
                                    "When \"" NATRON_CUSTOM_OCIO_CONFIG_NAME "\" is selected, the "
                                                                             "\"Custom OpenColorIO config file\" parameter is used.");

    ocioTab->addKnob(_ocioConfigKnob);

    _customOcioConfigFile = Natron::createKnob<KnobFile>(this, "Custom OpenColorIO config file");
    _customOcioConfigFile->setName("ocioCustomConfigFile");
    _customOcioConfigFile->setAllDimensionsEnabled(false);
    _customOcioConfigFile->setAnimationEnabled(false);
    _customOcioConfigFile->setHintToolTip("OpenColorIO configuration file (*.ocio) to use when \"" NATRON_CUSTOM_OCIO_CONFIG_NAME "\" "
                                                                                                                                  "is selected as the OpenColorIO config.");
    ocioTab->addKnob(_customOcioConfigFile);
    
    _warnOcioConfigKnobChanged = Natron::createKnob<KnobBool>(this, "Warn on OpenColorIO config change");
    _warnOcioConfigKnobChanged->setName("warnOCIOChanged");
    _warnOcioConfigKnobChanged->setHintToolTip("Show a warning dialog when changing the OpenColorIO config to remember that a restart is required.");
    _warnOcioConfigKnobChanged->setAnimationEnabled(false);
    ocioTab->addKnob(_warnOcioConfigKnobChanged);
    
    _ocioStartupCheck = Natron::createKnob<KnobBool>(this, "Warn on startup if OpenColorIO config is not the default");
    _ocioStartupCheck->setName("startupCheckOCIO");
    _ocioStartupCheck->setAnimationEnabled(false);
    ocioTab->addKnob(_ocioStartupCheck);

    _viewersTab = Natron::createKnob<KnobPage>(this, "Viewers");

    _texturesMode = Natron::createKnob<KnobChoice>(this, "Viewer textures bit depth");
    _texturesMode->setName("texturesBitDepth");
    _texturesMode->setAnimationEnabled(false);
    std::vector<std::string> textureModes;
    std::vector<std::string> helpStringsTextureModes;
    textureModes.push_back("Byte");
    helpStringsTextureModes.push_back("Post-processing done by the viewer (such as colorspace conversion) is done "
                                      "by the CPU. As a results, the size of cached textures is smaller.");
    //textureModes.push_back("16bits half-float");
    //helpStringsTextureModes.push_back("Not available yet. Similar to 32bits fp.");
    textureModes.push_back("32bits floating-point");
    helpStringsTextureModes.push_back("Post-processing done by the viewer (such as colorspace conversion) is done "
                                      "by the GPU, using GLSL. As a results, the size of cached textures is larger.");
    _texturesMode->populateChoices(textureModes,helpStringsTextureModes);
    _texturesMode->setHintToolTip("Bit depth of the viewer textures used for rendering."
                                  " Hover each option with the mouse for a detailed description.");
    _viewersTab->addKnob(_texturesMode);

    _powerOf2Tiling = Natron::createKnob<KnobInt>(this, "Viewer tile size is 2 to the power of...");
    _powerOf2Tiling->setName("viewerTiling");
    _powerOf2Tiling->setHintToolTip("The dimension of the viewer tiles is 2^n by 2^n (i.e. 256 by 256 pixels for n=8). "
                                    "A high value means that the viewer renders large tiles, so that "
                                    "rendering is done less often, but on larger areas." );
    _powerOf2Tiling->setMinimum(4);
    _powerOf2Tiling->setDisplayMinimum(4);
    _powerOf2Tiling->setMaximum(9);
    _powerOf2Tiling->setDisplayMaximum(9);

    _powerOf2Tiling->setAnimationEnabled(false);
    _viewersTab->addKnob(_powerOf2Tiling);
    
    _checkerboardTileSize = Natron::createKnob<KnobInt>(this, "Checkerboard tile size (pixels)");
    _checkerboardTileSize->setName("checkerboardTileSize");
    _checkerboardTileSize->setMinimum(1);
    _checkerboardTileSize->setAnimationEnabled(false);
    _checkerboardTileSize->setHintToolTip("The size (in screen pixels) of one tile of the checkerboard.");
    _viewersTab->addKnob(_checkerboardTileSize);
    
    _checkerboardColor1 = Natron::createKnob<KnobColor>(this, "Checkerboard color 1",4);
    _checkerboardColor1->setName("checkerboardColor1");
    _checkerboardColor1->setAnimationEnabled(false);
    _checkerboardColor1->setHintToolTip("The first color used by the checkerboard.");
    _viewersTab->addKnob(_checkerboardColor1);
    
    _checkerboardColor2 = Natron::createKnob<KnobColor>(this, "Checkerboard color 2",4);
    _checkerboardColor2->setName("checkerboardColor2");
    _checkerboardColor2->setAnimationEnabled(false);
    _checkerboardColor2->setHintToolTip("The second color used by the checkerboard.");
    _viewersTab->addKnob(_checkerboardColor2);
    

    
    _autoProxyWhenScrubbingTimeline = Natron::createKnob<KnobBool>(this, "Automatically enable proxy when scrubbing the timeline");
    _autoProxyWhenScrubbingTimeline->setName("autoProxyScrubbing");
    _autoProxyWhenScrubbingTimeline->setHintToolTip("When checked, the proxy mode will be at least at the level "
                                                    "indicated by the auto-proxy parameter.");
    _autoProxyWhenScrubbingTimeline->setAnimationEnabled(false);
    _autoProxyWhenScrubbingTimeline->setAddNewLine(false);
    _viewersTab->addKnob(_autoProxyWhenScrubbingTimeline);
    
    
    _autoProxyLevel = Natron::createKnob<KnobChoice>(this, "Auto-proxy level");
    _autoProxyLevel->setName("autoProxyLevel");
    _autoProxyLevel->setAnimationEnabled(false);
    std::vector<std::string> autoProxyChoices;
    autoProxyChoices.push_back("2");
    autoProxyChoices.push_back("4");
    autoProxyChoices.push_back("8");
    autoProxyChoices.push_back("16");
    autoProxyChoices.push_back("32");
    _autoProxyLevel->populateChoices(autoProxyChoices);
    _viewersTab->addKnob(_autoProxyLevel);
    
    
    _enableProgressReport = Natron::createKnob<KnobBool>(this, "Enable progress-report (experimental, slower)");
    _enableProgressReport->setName("inViewerProgress");
    _enableProgressReport->setAnimationEnabled(false);
    _enableProgressReport->setHintToolTip("When enabled, the viewer will decompose the portion to render in small tiles and will "
                                          "display them whenever they are ready to be displayed. This should provide faster results "
                                          "on partial images. Note that this option does not support well all effects that have "
                                          "a larger region to compute that what the real render window is (e.g: Blur). As a result of this, "
                                          "any graph containing such effect(s) will be slower to render on the viewer than when this option "
                                          "is unchecked.");
    _viewersTab->addKnob(_enableProgressReport);
    
    /////////// Nodegraph tab
    _nodegraphTab = Natron::createKnob<KnobPage>(this, "Nodegraph");
    
    _autoTurbo = Natron::createKnob<KnobBool>(this, "Auto-turbo");
    _autoTurbo->setName("autoTurbo");
    _autoTurbo->setHintToolTip("When checked the Turbo-mode will be enabled automatically when playback is started and disabled "
                               "when finished.");
    _autoTurbo->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_autoTurbo);

    _snapNodesToConnections = Natron::createKnob<KnobBool>(this, "Snap to node");
    _snapNodesToConnections->setName("enableSnapToNode");
    _snapNodesToConnections->setHintToolTip("When moving nodes on the node graph, snap to positions where they are lined up "
                                            "with the inputs and output nodes.");
    _snapNodesToConnections->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_snapNodesToConnections);

    _useBWIcons = Natron::createKnob<KnobBool>(this, "Use black & white toolbutton icons");
    _useBWIcons->setName("useBwIcons");
    _useBWIcons->setHintToolTip("When checked, the tools icons in the left toolbar are greyscale. Changing this takes "
                                "effect upon the next launch of the application.");
    _useBWIcons->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_useBWIcons);

    _useNodeGraphHints = Natron::createKnob<KnobBool>(this, "Use connection hints");
    _useNodeGraphHints->setName("useHints");
    _useNodeGraphHints->setHintToolTip("When checked, moving a node which is not connected to anything to arrows "
                                       "nearby displays a hint for possible connections. Releasing the mouse when "
                                       "hints are shown connects the node.");
    _useNodeGraphHints->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_useNodeGraphHints);

    _maxUndoRedoNodeGraph = Natron::createKnob<KnobInt>(this, "Maximum undo/redo for the node graph");
    _maxUndoRedoNodeGraph->setName("maxUndoRedo");
    _maxUndoRedoNodeGraph->setAnimationEnabled(false);
    _maxUndoRedoNodeGraph->disableSlider();
    _maxUndoRedoNodeGraph->setMinimum(0);
    _maxUndoRedoNodeGraph->setHintToolTip("Set the maximum of events related to the node graph " NATRON_APPLICATION_NAME " "
                                                                                                                         "remembers. Past this limit, older events will be deleted forever, "
                                                                                                                         "allowing to re-use the RAM for other purposes. \n"
                                                                                                                         "Changing this value will clear the undo/redo stack.");
    _nodegraphTab->addKnob(_maxUndoRedoNodeGraph);


    _disconnectedArrowLength = Natron::createKnob<KnobInt>(this, "Disconnected arrow length");
    _disconnectedArrowLength->setName("disconnectedArrowLength");
    _disconnectedArrowLength->setAnimationEnabled(false);
    _disconnectedArrowLength->setHintToolTip("The size of a disconnected node input arrow in pixels.");
    _disconnectedArrowLength->disableSlider();

    _nodegraphTab->addKnob(_disconnectedArrowLength);
    
    _hideOptionalInputsAutomatically = Natron::createKnob<KnobBool>(this, "Auto hide masks inputs");
    _hideOptionalInputsAutomatically->setName("autoHideInputs");
    _hideOptionalInputsAutomatically->setAnimationEnabled(false);
    _hideOptionalInputsAutomatically->setHintToolTip("When checked, any diconnected mask input of a node in the nodegraph "
                                                     "will be visible only when the mouse is hovering the node or when it is "
                                                     "selected.");
    _nodegraphTab->addKnob(_hideOptionalInputsAutomatically);
    
    _useInputAForMergeAutoConnect = Natron::createKnob<KnobBool>(this,"Merge node connect to A input");
    _useInputAForMergeAutoConnect->setName("mergeConnectToA");
    _useInputAForMergeAutoConnect->setAnimationEnabled(false);
    _useInputAForMergeAutoConnect->setHintToolTip("If checked, upon creation of a new Merge node, the input A will be preferred "
                                                  "for auto-connection and when disabling the node instead of the input B. "
                                                  "This also applies to any other node with inputs named A and B.");
    _nodegraphTab->addKnob(_useInputAForMergeAutoConnect);
    
    _usePluginIconsInNodeGraph = Natron::createKnob<KnobBool>(this, "Display plug-in icon on node-graph");
    _usePluginIconsInNodeGraph->setHintToolTip("When checked, each node that has a plug-in icon will display it in the node-graph."
                                               "Changing this option will not affect already existing nodes, unless a restart of Natron is made.");
    _usePluginIconsInNodeGraph->setAnimationEnabled(false);
    _nodegraphTab->addKnob(_usePluginIconsInNodeGraph);
    
   
    _defaultNodeColor = Natron::createKnob<KnobColor>(this, "Default node color",3);
    _defaultNodeColor->setName("defaultNodeColor");
    _defaultNodeColor->setAnimationEnabled(false);
    _defaultNodeColor->setSimplified(true);
    _defaultNodeColor->setAddNewLine(false);
    _defaultNodeColor->setHintToolTip("The default color used for newly created nodes.");

    _nodegraphTab->addKnob(_defaultNodeColor);


    _defaultBackdropColor =  Natron::createKnob<KnobColor>(this, "Default backdrop color",3);
    _defaultBackdropColor->setName("backdropColor");
    _defaultBackdropColor->setAnimationEnabled(false);
    _defaultBackdropColor->setSimplified(true);
    _defaultBackdropColor->setAddNewLine(false);
    _defaultBackdropColor->setHintToolTip("The default color used for newly created backdrop nodes.");
    _nodegraphTab->addKnob(_defaultBackdropColor);

    ///////////////////DEFAULT GROUP COLORS

    _defaultReaderColor =  Natron::createKnob<KnobColor>(this, PLUGIN_GROUP_IMAGE_READERS,3);
    _defaultReaderColor->setName("readerColor");
    _defaultReaderColor->setAnimationEnabled(false);
    _defaultReaderColor->setSimplified(true);
    _defaultReaderColor->setAddNewLine(false);
    _defaultReaderColor->setHintToolTip("The color used for newly created Reader nodes.");
    _nodegraphTab->addKnob(_defaultReaderColor);

    _defaultWriterColor =  Natron::createKnob<KnobColor>(this, PLUGIN_GROUP_IMAGE_WRITERS,3);
    _defaultWriterColor->setName("writerColor");
    _defaultWriterColor->setAnimationEnabled(false);
    _defaultWriterColor->setSimplified(true);
    _defaultWriterColor->setAddNewLine(false);
    _defaultWriterColor->setHintToolTip("The color used for newly created Writer nodes.");
    _nodegraphTab->addKnob(_defaultWriterColor);

    _defaultGeneratorColor =  Natron::createKnob<KnobColor>(this, "Generators",3);
    _defaultGeneratorColor->setName("generatorColor");
    _defaultGeneratorColor->setAnimationEnabled(false);
    _defaultGeneratorColor->setSimplified(true);
    _defaultGeneratorColor->setHintToolTip("The color used for newly created Generator nodes.");
    _nodegraphTab->addKnob(_defaultGeneratorColor);

    _defaultColorGroupColor =  Natron::createKnob<KnobColor>(this, "Color group",3);
    _defaultColorGroupColor->setName("colorNodesColor");
    _defaultColorGroupColor->setAnimationEnabled(false);
    _defaultColorGroupColor->setSimplified(true);
    _defaultColorGroupColor->setAddNewLine(false);
    _defaultColorGroupColor->setHintToolTip("The color used for newly created Color nodes.");
    _nodegraphTab->addKnob(_defaultColorGroupColor);

    _defaultFilterGroupColor =  Natron::createKnob<KnobColor>(this, "Filter group",3);
    _defaultFilterGroupColor->setName("filterNodesColor");
    _defaultFilterGroupColor->setAnimationEnabled(false);
    _defaultFilterGroupColor->setSimplified(true);
    _defaultFilterGroupColor->setAddNewLine(false);
    _defaultFilterGroupColor->setHintToolTip("The color used for newly created Filter nodes.");
    _nodegraphTab->addKnob(_defaultFilterGroupColor);

    _defaultTransformGroupColor =  Natron::createKnob<KnobColor>(this, "Transform group",3);
    _defaultTransformGroupColor->setName("transformNodesColor");
    _defaultTransformGroupColor->setAnimationEnabled(false);
    _defaultTransformGroupColor->setSimplified(true);
    _defaultTransformGroupColor->setAddNewLine(false);
    _defaultTransformGroupColor->setHintToolTip("The color used for newly created Transform nodes.");
    _nodegraphTab->addKnob(_defaultTransformGroupColor);

    _defaultTimeGroupColor =  Natron::createKnob<KnobColor>(this, "Time group",3);
    _defaultTimeGroupColor->setName("timeNodesColor");
    _defaultTimeGroupColor->setAnimationEnabled(false);
    _defaultTimeGroupColor->setSimplified(true);
    _defaultTimeGroupColor->setAddNewLine(false);
    _defaultTimeGroupColor->setHintToolTip("The color used for newly created Time nodes.");
    _nodegraphTab->addKnob(_defaultTimeGroupColor);

    _defaultDrawGroupColor =  Natron::createKnob<KnobColor>(this, "Draw group",3);
    _defaultDrawGroupColor->setName("drawNodesColor");
    _defaultDrawGroupColor->setAnimationEnabled(false);
    _defaultDrawGroupColor->setSimplified(true);
    _defaultDrawGroupColor->setHintToolTip("The color used for newly created Draw nodes.");
    _nodegraphTab->addKnob(_defaultDrawGroupColor);

    _defaultKeyerGroupColor =  Natron::createKnob<KnobColor>(this, "Keyer group",3);
    _defaultKeyerGroupColor->setName("keyerNodesColor");
    _defaultKeyerGroupColor->setAnimationEnabled(false);
    _defaultKeyerGroupColor->setSimplified(true);
    _defaultKeyerGroupColor->setAddNewLine(false);
    _defaultKeyerGroupColor->setHintToolTip("The color used for newly created Keyer nodes.");
    _nodegraphTab->addKnob(_defaultKeyerGroupColor);

    _defaultChannelGroupColor =  Natron::createKnob<KnobColor>(this, "Channel group",3);
    _defaultChannelGroupColor->setName("channelNodesColor");
    _defaultChannelGroupColor->setAnimationEnabled(false);
    _defaultChannelGroupColor->setSimplified(true);
    _defaultChannelGroupColor->setAddNewLine(false);
    _defaultChannelGroupColor->setHintToolTip("The color used for newly created Channel nodes.");
    _nodegraphTab->addKnob(_defaultChannelGroupColor);

    _defaultMergeGroupColor =  Natron::createKnob<KnobColor>(this, "Merge group",3);
    _defaultMergeGroupColor->setName("defaultMergeColor");
    _defaultMergeGroupColor->setAnimationEnabled(false);
    _defaultMergeGroupColor->setSimplified(true);
    _defaultMergeGroupColor->setAddNewLine(false);
    _defaultMergeGroupColor->setHintToolTip("The color used for newly created Merge nodes.");
    _nodegraphTab->addKnob(_defaultMergeGroupColor);

    _defaultViewsGroupColor =  Natron::createKnob<KnobColor>(this, "Views group",3);
    _defaultViewsGroupColor->setName("defaultViewsColor");
    _defaultViewsGroupColor->setAnimationEnabled(false);
    _defaultViewsGroupColor->setSimplified(true);
    _defaultViewsGroupColor->setAddNewLine(false);
    _defaultViewsGroupColor->setHintToolTip("The color used for newly created Views nodes.");
    _nodegraphTab->addKnob(_defaultViewsGroupColor);

    _defaultDeepGroupColor =  Natron::createKnob<KnobColor>(this, "Deep group",3);
    _defaultDeepGroupColor->setName("defaultDeepColor");
    _defaultDeepGroupColor->setAnimationEnabled(false);
    _defaultDeepGroupColor->setSimplified(true);
    _defaultDeepGroupColor->setHintToolTip("The color used for newly created Deep nodes.");
    _nodegraphTab->addKnob(_defaultDeepGroupColor);

    /////////// Caching tab
    _cachingTab = Natron::createKnob<KnobPage>(this, "Caching");

    _aggressiveCaching = Natron::createKnob<KnobBool>(this, "Aggressive caching");
    _aggressiveCaching->setName("aggressiveCaching");
    _aggressiveCaching->setAnimationEnabled(false);
    _aggressiveCaching->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " will cache the output of all images "
                                                                                "rendered by all nodes, regardless of their \"Force caching\" parameter. When enabling this option "
                                                                                "you need to have at least 8GiB of RAM, and 16GiB is recommended.\n"
                                                                                "If not checked, " NATRON_APPLICATION_NAME " will only cache the  nodes "
                                                                                                                           "which have multiple outputs, or their parameter \"Force caching\" checked or if one of its "
                                                                                                                           "output has its settings panel opened.");
    _cachingTab->addKnob(_aggressiveCaching);
    
    _maxRAMPercent = Natron::createKnob<KnobInt>(this, "Maximum amount of RAM memory used for caching (% of total RAM)");
    _maxRAMPercent->setName("maxRAMPercent");
    _maxRAMPercent->setAnimationEnabled(false);
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    std::string ramHint("This setting indicates the percentage of the total RAM which can be used by the memory caches. "
                        "This system has ");
    ramHint.append( printAsRAM( getSystemTotalRAM() ).toStdString() );
    ramHint.append(" of RAM.");
    if ( isApplication32Bits() && getSystemTotalRAM() > 4ULL * 1024ULL * 1024ULL * 1024ULL) {
        ramHint.append("\nThe version of " NATRON_APPLICATION_NAME " you are running is 32 bits, which means the available RAM "
                                                                   "is limited to 4GiB. The amount of RAM used for caching is 4GiB * MaxRamPercent.");
    }

    _maxRAMPercent->setHintToolTip(ramHint);
    _maxRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_maxRAMPercent);

    _maxRAMLabel = Natron::createKnob<KnobString>(this, "");
    _maxRAMLabel->setName("maxRamLabel");
    _maxRAMLabel->setIsPersistant(false);
    _maxRAMLabel->setAsLabel();
    _maxRAMLabel->setAnimationEnabled(false);
    _cachingTab->addKnob(_maxRAMLabel);

    _maxPlayBackPercent = Natron::createKnob<KnobInt>(this, "Playback cache RAM percentage (% of maximum RAM used for caching)");
    _maxPlayBackPercent->setName("maxPlaybackPercent");
    _maxPlayBackPercent->setAnimationEnabled(false);
    _maxPlayBackPercent->setMinimum(0);
    _maxPlayBackPercent->setMaximum(100);
    _maxPlayBackPercent->setHintToolTip("This setting indicates the percentage of the maximum RAM used for caching "
                                        "dedicated to the playback cache. This is available for debugging purposes.");
    _maxPlayBackPercent->setAddNewLine(false);
    _cachingTab->addKnob(_maxPlayBackPercent);

    _maxPlaybackLabel = Natron::createKnob<KnobString>(this, "");
    _maxPlaybackLabel->setName("maxPlaybackLabel");
    _maxPlaybackLabel->setIsPersistant(false);
    _maxPlaybackLabel->setAsLabel();
    _maxPlaybackLabel->setAnimationEnabled(false);
    _cachingTab->addKnob(_maxPlaybackLabel);

    _unreachableRAMPercent = Natron::createKnob<KnobInt>(this, "System RAM to keep free (% of total RAM)");
    _unreachableRAMPercent->setName("unreachableRAMPercent");
    _unreachableRAMPercent->setAnimationEnabled(false);
    _unreachableRAMPercent->setMinimum(0);
    _unreachableRAMPercent->setMaximum(90);
    _unreachableRAMPercent->setHintToolTip("This determines how much RAM should be kept free for other applications "
                                           "running on the same system. "
                                           "When this limit is reached, the caches start recycling memory instead of growing. "
                                           //"A reasonable value should be set for it allowing the caches to stay in physical RAM " // users don't understand what swap is
                                           //"and avoid being swapped-out on disk. "
                                           "This value should reflect the amount of memory "
                                           "you want to keep available on your computer for other usage. "
                                           "A low value may result in a massive slowdown and high disk usage."
                                           );
    _unreachableRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_unreachableRAMPercent);
    _unreachableRAMLabel = Natron::createKnob<KnobString>(this, "");
    _unreachableRAMLabel->setName("unreachableRAMLabel");
    _unreachableRAMLabel->setIsPersistant(false);
    _unreachableRAMLabel->setAsLabel();
    _unreachableRAMLabel->setAnimationEnabled(false);
    _cachingTab->addKnob(_unreachableRAMLabel);

    _maxViewerDiskCacheGB = Natron::createKnob<KnobInt>(this, "Maximum playback disk cache size (GiB)");
    _maxViewerDiskCacheGB->setName("maxViewerDiskCache");
    _maxViewerDiskCacheGB->setAnimationEnabled(false);
    _maxViewerDiskCacheGB->setMinimum(0);
    _maxViewerDiskCacheGB->setMaximum(100);
    _maxViewerDiskCacheGB->setHintToolTip("The maximum size that may be used by the playback cache on disk (in GiB)");
    _cachingTab->addKnob(_maxViewerDiskCacheGB);
    
    _maxDiskCacheNodeGB = Natron::createKnob<KnobInt>(this, "Maximum DiskCache node disk usage (GiB)");
    _maxDiskCacheNodeGB->setName("maxDiskCacheNode");
    _maxDiskCacheNodeGB->setAnimationEnabled(false);
    _maxDiskCacheNodeGB->setMinimum(0);
    _maxDiskCacheNodeGB->setMaximum(100);
    _maxDiskCacheNodeGB->setHintToolTip("The maximum size that may be used by the DiskCache node on disk (in GiB)");
    _cachingTab->addKnob(_maxDiskCacheNodeGB);


    _diskCachePath = Natron::createKnob<KnobPath>(this, "Disk cache path (empty = default)");
    _diskCachePath->setName("diskCachePath");
    _diskCachePath->setAnimationEnabled(false);
    _diskCachePath->setMultiPath(false);
    
    QString defaultLocation = Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationCache);
    std::string diskCacheTt("WARNING: Changing this parameter requires a restart of the application. \n"
                            "This is points to the location where " NATRON_APPLICATION_NAME " on-disk caches will be. "
                                                                                            "This variable should point to your fastest disk. If the parameter is left empty or the location set is invalid, "
                                                                                            "the default location will be used. The default location is: \n");
    
    _diskCachePath->setHintToolTip(diskCacheTt + defaultLocation.toStdString());
    _cachingTab->addKnob(_diskCachePath);
    
    ///readers & writers settings are created in a postponed manner because we don't know
    ///their dimension yet. See populateReaderPluginsAndFormats & populateWriterPluginsAndFormats

    _readersTab = Natron::createKnob<KnobPage>(this, PLUGIN_GROUP_IMAGE_READERS);
    _readersTab->setName("readersTab");

    _writersTab = Natron::createKnob<KnobPage>(this, PLUGIN_GROUP_IMAGE_WRITERS);
    _writersTab->setName("writersTab");

    
    _pluginsTab = Natron::createKnob<KnobPage>(this, "Plug-ins");
    _pluginsTab->setName("plugins");
    
    _extraPluginPaths = Natron::createKnob<KnobPath>(this, "OpenFX plugins search path");
    _extraPluginPaths->setName("extraPluginsSearchPaths");
    
#if defined(__linux__) || defined(__FreeBSD__)
    std::string searchPath("/usr/OFX/Plugins");
#elif defined(__APPLE__)
    std::string searchPath("/Library/OFX/Plugins");
#elif defined(WINDOWS)
    
#ifdef UNICODE
    std::wstring basePath = std::wstring(OFX::Host::PluginCache::getStdOFXPluginPath(""));
	basePath.append(std::wstring(__T(" and C:\\Program Files\\Common Files\\OFX\\Plugins")));
    std::string searchPath = OFX::wideStringToString(basePath);
#else
    std::string searchPath(OFX::Host::PluginCache::getStdOFXPluginPath("")  + std::string(" and C:\\Program Files\\Common Files\\OFX\\Plugins"));
#endif
#endif
    
    _extraPluginPaths->setHintToolTip( std::string("Extra search paths where " NATRON_APPLICATION_NAME
                                                   " should scan for OpenFX plugins. "
                                                   "Extra plugins search paths can also be specified using the OFX_PLUGIN_PATH environment variable.\n"
                                                   "The priority order for system-wide plugins, from high to low, is:\n"
                                                   "- plugins found in OFX_PLUGIN_PATH\n"
                                                   "- plugins found in "
                                                   + searchPath + "\n"
                                                   "Plugins bundled with the binary distribution of Natron may have either "
                                                   "higher or lower priority, depending on the \"Prefer bundled plugins over "
                                                   "system-wide plugins\" setting.\n"
                                                   "Any change will take effect on the next launch of " NATRON_APPLICATION_NAME ".") );
    _extraPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_extraPluginPaths);
    
    _templatesPluginPaths = Natron::createKnob<KnobPath>(this, "PyPlugs search path");
    _templatesPluginPaths->setName("groupPluginsSearchPath");
    _templatesPluginPaths->setHintToolTip("Search path where " NATRON_APPLICATION_NAME " should scan for Python group scripts (PyPlugs). "
                                                                                       "The search paths for groups can also be specified using the "
                                                                                       "NATRON_PLUGIN_PATH environment variable.");
    _templatesPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_templatesPluginPaths);
    
    _loadBundledPlugins = Natron::createKnob<KnobBool>(this, "Use bundled plugins");
    _loadBundledPlugins->setName("useBundledPlugins");
    _loadBundledPlugins->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " also uses the plugins bundled "
                                                                                 "with the binary distribution.\n"
                                                                                 "When unchecked, only system-wide plugins are loaded (more information can be "
                                                                                 "found in the help for the \"Extra plugins search paths\" setting).");
    _loadBundledPlugins->setAnimationEnabled(false);
    _pluginsTab->addKnob(_loadBundledPlugins);
    
    _preferBundledPlugins = Natron::createKnob<KnobBool>(this, "Prefer bundled plugins over system-wide plugins");
    _preferBundledPlugins->setName("preferBundledPlugins");
    _preferBundledPlugins->setHintToolTip("When checked, and if \"Use bundled plugins\" is also checked, plugins bundled with the "
                                          NATRON_APPLICATION_NAME " binary distribution will take precedence over system-wide plugins "
                                                                  "if they have the same internal ID.");
    _preferBundledPlugins->setAnimationEnabled(false);
    _pluginsTab->addKnob(_preferBundledPlugins);
    
    
    _pythonPage = Natron::createKnob<KnobPage>(this, "Python");
    
    
    _onProjectCreated = Natron::createKnob<KnobString>(this, "After project created");
    _onProjectCreated->setName("afterProjectCreated");
    _onProjectCreated->setHintToolTip("Callback called once a new project is created (this is never called "
                                      "when \"After project loaded\" is called.)\n"
                                      "The signature of the callback is : callback(app) where:\n"
                                      "- app: points to the current application instance\n");
    _onProjectCreated->setAnimationEnabled(false);
    _pythonPage->addKnob(_onProjectCreated);
    
    
    _defaultOnProjectLoaded = Natron::createKnob<KnobString>(this, "Default after project loaded");
    _defaultOnProjectLoaded->setName("defOnProjectLoaded");
    _defaultOnProjectLoaded->setHintToolTip("The default afterProjectLoad callback that will be set for new projects.");
    _defaultOnProjectLoaded->setAnimationEnabled(false);
    _pythonPage->addKnob(_defaultOnProjectLoaded);
    
    _defaultOnProjectSave = Natron::createKnob<KnobString>(this, "Default before project save");
    _defaultOnProjectSave->setName("defOnProjectSave");
    _defaultOnProjectSave->setHintToolTip("The default beforeProjectSave callback that will be set for new projects.");
    _defaultOnProjectSave->setAnimationEnabled(false);
    _pythonPage->addKnob(_defaultOnProjectSave);

    
    _defaultOnProjectClose = Natron::createKnob<KnobString>(this, "Default before project close");
    _defaultOnProjectClose->setName("defOnProjectClose");
    _defaultOnProjectClose->setHintToolTip("The default beforeProjectClose callback that will be set for new projects.");
    _defaultOnProjectClose->setAnimationEnabled(false);
    _pythonPage->addKnob(_defaultOnProjectClose);

    
    _defaultOnNodeCreated = Natron::createKnob<KnobString>(this, "Default after node created");
    _defaultOnNodeCreated->setName("defOnNodeCreated");
    _defaultOnNodeCreated->setHintToolTip("The default afterNodeCreated callback that will be set for new projects.");
    _defaultOnNodeCreated->setAnimationEnabled(false);
    _pythonPage->addKnob(_defaultOnNodeCreated);

    
    _defaultOnNodeDelete = Natron::createKnob<KnobString>(this, "Default before node removal");
    _defaultOnNodeDelete->setName("defOnNodeDelete");
    _defaultOnNodeDelete->setHintToolTip("The default beforeNodeRemoval callback that will be set for new projects.");
    _defaultOnNodeDelete->setAnimationEnabled(false);
    _pythonPage->addKnob(_defaultOnNodeDelete);
    
    _loadPyPlugsFromPythonScript = Natron::createKnob<KnobBool>(this, "Load PyPlugs in projects from .py if possible");
    _loadPyPlugsFromPythonScript->setName("loadFromPyFile");
    _loadPyPlugsFromPythonScript->setHintToolTip("When checked, if a project contains a PyPlug, it will try to first load the PyPlug "
                                                 "from the .py file. If the version of the PyPlug has changed Natron will ask you "
                                                 "whether you want to upgrade to the new version of the PyPlug in your project. "
                                                 "If the .py file is not found, it will fallback to the same behavior "
                                                 "as when this option is unchecked. When unchecked the PyPlug will load as a regular group "
                                                 "with the informations embedded in the project file.");
    _loadPyPlugsFromPythonScript->setDefaultValue(true);
    _loadPyPlugsFromPythonScript->setAnimationEnabled(false);
    _pythonPage->addKnob(_loadPyPlugsFromPythonScript);

    _echoVariableDeclarationToPython = Natron::createKnob<KnobBool>(this, "Print auto-declared variables in the Script Editor");
    _echoVariableDeclarationToPython->setHintToolTip("When checked, Natron will print in the Script Editor all variables that are "
                                                     "automatically declared, such as the app variable or node attributes.");
    _echoVariableDeclarationToPython->setAnimationEnabled(false);
    _pythonPage->addKnob(_echoVariableDeclarationToPython);
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
    beginKnobsValuesChanged(Natron::eValueChangedReasonPluginEdited);
    _hostName->setDefaultValue(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);
    _natronSettingsExist->setDefaultValue(false);
    _systemFontChoice->setDefaultValue(0);
    _fontSize->setDefaultValue(NATRON_FONT_SIZE_10);
    _checkForUpdates->setDefaultValue(false);
    _notifyOnFileChange->setDefaultValue(true);
    _autoSaveDelay->setDefaultValue(5, 0);
    _maxUndoRedoNodeGraph->setDefaultValue(20, 0);
    _linearPickers->setDefaultValue(true,0);
    _convertNaNValues->setDefaultValue(true);
    _snapNodesToConnections->setDefaultValue(true);
    _useBWIcons->setDefaultValue(false);
    _useNodeGraphHints->setDefaultValue(true);
    _numberOfThreads->setDefaultValue(0,0);
    _numberOfParallelRenders->setDefaultValue(0,0);
    _useThreadPool->setDefaultValue(true);
    _nThreadsPerEffect->setDefaultValue(0);
    _renderInSeparateProcess->setDefaultValue(false,0);
    _autoPreviewEnabledForNewProjects->setDefaultValue(true,0);
    _firstReadSetProjectFormat->setDefaultValue(true);
    _fixPathsOnProjectPathChanged->setDefaultValue(true);
    _maxPanelsOpened->setDefaultValue(10,0);
    _useCursorPositionIncrements->setDefaultValue(true);
    _renderOnEditingFinished->setDefaultValue(false);
    _activateRGBSupport->setDefaultValue(true);
    _activateTransformConcatenationSupport->setDefaultValue(true);
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
    _autoProxyWhenScrubbingTimeline->setDefaultValue(true);
    _autoProxyLevel->setDefaultValue(1);
    _enableProgressReport->setDefaultValue(false);
    
    _warnOcioConfigKnobChanged->setDefaultValue(true);
    _ocioStartupCheck->setDefaultValue(true);

    _aggressiveCaching->setDefaultValue(false);
    _maxRAMPercent->setDefaultValue(50,0);
    _maxPlayBackPercent->setDefaultValue(25,0);
    _unreachableRAMPercent->setDefaultValue(5);
    _maxViewerDiskCacheGB->setDefaultValue(5,0);
    _maxDiskCacheNodeGB->setDefaultValue(10,0);
    setCachingLabels();
    _autoTurbo->setDefaultValue(false);
    _usePluginIconsInNodeGraph->setDefaultValue(true);
    _defaultNodeColor->setDefaultValue(0.7,0);
    _defaultNodeColor->setDefaultValue(0.7,1);
    _defaultNodeColor->setDefaultValue(0.7,2);
    _defaultBackdropColor->setDefaultValue(0.45,0);
    _defaultBackdropColor->setDefaultValue(0.45,1);
    _defaultBackdropColor->setDefaultValue(0.45,2);
    _disconnectedArrowLength->setDefaultValue(30);
    _hideOptionalInputsAutomatically->setDefaultValue(true);
    _useInputAForMergeAutoConnect->setDefaultValue(false);

    _defaultGeneratorColor->setDefaultValue(0.3,0);
    _defaultGeneratorColor->setDefaultValue(0.5,1);
    _defaultGeneratorColor->setDefaultValue(0.2,2);

    _defaultReaderColor->setDefaultValue(0.7,0);
    _defaultReaderColor->setDefaultValue(0.7,1);
    _defaultReaderColor->setDefaultValue(0.7,2);

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
    
    _echoVariableDeclarationToPython->setDefaultValue(true);

    
    _sunkenColor->setDefaultValue(0.12,0);
    _sunkenColor->setDefaultValue(0.12,1);
    _sunkenColor->setDefaultValue(0.12,2);
    
    _baseColor->setDefaultValue(0.19,0);
    _baseColor->setDefaultValue(0.19,1);
    _baseColor->setDefaultValue(0.19,2);
    
    _raisedColor->setDefaultValue(0.28,0);
    _raisedColor->setDefaultValue(0.28,1);
    _raisedColor->setDefaultValue(0.28,2);
    
    _selectionColor->setDefaultValue(0.95,0);
    _selectionColor->setDefaultValue(0.54,1);
    _selectionColor->setDefaultValue(0.,2);
    
    _textColor->setDefaultValue(0.78,0);
    _textColor->setDefaultValue(0.78,1);
    _textColor->setDefaultValue(0.78,2);
    
    _altTextColor->setDefaultValue(0.6,0);
    _altTextColor->setDefaultValue(0.6,1);
    _altTextColor->setDefaultValue(0.6,2);
    
    _timelinePlayheadColor->setDefaultValue(0.95,0);
    _timelinePlayheadColor->setDefaultValue(0.54,1);
    _timelinePlayheadColor->setDefaultValue(0.,2);
    
    _timelineBGColor->setDefaultValue(0,0);
    _timelineBGColor->setDefaultValue(0,1);
    _timelineBGColor->setDefaultValue(0.,2);
    
    _timelineBoundsColor->setDefaultValue(0.81,0);
    _timelineBoundsColor->setDefaultValue(0.27,1);
    _timelineBoundsColor->setDefaultValue(0.02,2);
    
    _cachedFrameColor->setDefaultValue(0.56,0);
    _cachedFrameColor->setDefaultValue(0.79,1);
    _cachedFrameColor->setDefaultValue(0.4,2);
    
    _diskCachedFrameColor->setDefaultValue(0.27,0);
    _diskCachedFrameColor->setDefaultValue(0.38,1);
    _diskCachedFrameColor->setDefaultValue(0.25,2);
    
    _interpolatedColor->setDefaultValue(0.34,0);
    _interpolatedColor->setDefaultValue(0.46,1);
    _interpolatedColor->setDefaultValue(0.6,2);
    
    _keyframeColor->setDefaultValue(0.08,0);
    _keyframeColor->setDefaultValue(0.38,1);
    _keyframeColor->setDefaultValue(0.97,2);
    
    _exprColor->setDefaultValue(0.7,0);
    _exprColor->setDefaultValue(0.78,1);
    _exprColor->setDefaultValue(0.39,2);
    
    _curveEditorBGColor->setDefaultValue(0.,0);
    _curveEditorBGColor->setDefaultValue(0.,1);
    _curveEditorBGColor->setDefaultValue(0.,2);

    _gridColor->setDefaultValue(0.46,0);
    _gridColor->setDefaultValue(0.84,1);
    _gridColor->setDefaultValue(0.35,2);
    
    _curveEditorScaleColor->setDefaultValue(0.26,0);
    _curveEditorScaleColor->setDefaultValue(0.48,1);
    _curveEditorScaleColor->setDefaultValue(0.2,2);

    // Initialize Dope sheet editor Settings knobs
    _dopeSheetEditorBackgroundColor->setDefaultValue(0.208, 0);
    _dopeSheetEditorBackgroundColor->setDefaultValue(0.208, 1);
    _dopeSheetEditorBackgroundColor->setDefaultValue(0.208, 2);

    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.204, 0);
    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.204, 1);
    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.204, 2);
    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.2, 3);

    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.443, 0);
    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.443, 1);
    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.443, 2);
    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.2, 3);

    _dopeSheetEditorScaleColor->setDefaultValue(0.714, 0);
    _dopeSheetEditorScaleColor->setDefaultValue(0.718, 1);
    _dopeSheetEditorScaleColor->setDefaultValue(0.714, 2);

    _dopeSheetEditorGridColor->setDefaultValue(0.714, 0);
    _dopeSheetEditorGridColor->setDefaultValue(0.714, 1);
    _dopeSheetEditorGridColor->setDefaultValue(0.714, 2);


    endKnobsValuesChanged(Natron::eValueChangedReasonPluginEdited);
} // setDefaultValues

void
Settings::warnChangedKnobs(const std::vector<KnobI*>& knobs)
{
    bool didFontWarn = false;
    bool didOCIOWarn = false;
    
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ((knobs[i] == _fontSize.get() ||
             knobs[i] == _systemFontChoice.get())
                && !didFontWarn) {
            
            didOCIOWarn = true;
            Natron::warningDialog(QObject::tr("Font change").toStdString(),
                                  QObject::tr("Changing the font requires a restart of " NATRON_APPLICATION_NAME).toStdString());
            
            
            
        } else if ((knobs[i] == _ocioConfigKnob.get() ||
                    knobs[i] == _customOcioConfigFile.get())
                   && !didOCIOWarn) {
            didOCIOWarn = true;
            bool warnOcioChanged = _warnOcioConfigKnobChanged->getValue();
            if (warnOcioChanged) {
                bool stopAsking = false;
                Natron::warningDialog(QObject::tr("OCIO config changed").toStdString(),
                                      QObject::tr("The OpenColorIO config change requires a restart of "
                                                  NATRON_APPLICATION_NAME " to be effective.").toStdString(),&stopAsking);
                if (stopAsking) {
                    _warnOcioConfigKnobChanged->setValue(false,0);
                    saveSetting(_warnOcioConfigKnobChanged.get());
                }

            }
        } else if (knobs[i] == _texturesMode.get()) {
            std::map<int,AppInstanceRef> apps = appPTR->getAppInstances();
            bool isFirstViewer = true;
            for (std::map<int,AppInstanceRef>::iterator it = apps.begin(); it != apps.end(); ++it) {
                
                std::list<ViewerInstance*> allViewers;
                it->second.app->getProject()->getViewers(&allViewers);
                for (std::list<ViewerInstance*>::iterator it = allViewers.begin(); it != allViewers.end(); ++it) {
                    
                    
                    if (isFirstViewer) {
                        if ( !(*it)->supportsGLSL() && (_texturesMode->getValue() != 0) ) {
                            Natron::errorDialog( QObject::tr("Viewer").toStdString(), QObject::tr("You need OpenGL GLSL in order to use 32 bit fp textures.\n"
                                                                                                  "Reverting to 8bits textures.").toStdString() );
                            _texturesMode->setValue(0,0);
                            saveSetting(_texturesMode.get());
                            return;
                        }
                    }
                    (*it)->renderCurrentFrame(true);
                    
                }
            }
        }

    }
}

void
Settings::saveAllSettings()
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = getKnobs();
    std::vector<KnobI*> k(knobs.size());
    for (U32 i = 0; i < knobs.size(); ++i) {
        k[i] = knobs[i].get();
    }
    saveSettings(k, false);
}

void
Settings::saveSettings(const std::vector<KnobI*>& knobs,bool doWarnings)
{
    
    std::vector<KnobI*> changedKnobs;
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.setValue(kQSettingsSoftwareMajorVersionSettingName, NATRON_VERSION_MAJOR);
    for (U32 i = 0; i < knobs.size(); ++i) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knobs[i]);
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knobs[i]);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knobs[i]);
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knobs[i]);
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knobs[i]);

        const std::string& name = knobs[i]->getName();
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            QString dimensionName;
            if (knobs[i]->getDimension() > 1) {
                dimensionName =  QString(name.c_str()) + '.' + knobs[i]->getDimensionName(j).c_str();
            } else {
                dimensionName = name.c_str();
            }
            try {
                if (isString) {
                    QString old = settings.value(dimensionName).toString();
                    QString newValue(isString->getValue(j).c_str());
                    if (old != newValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue(dimensionName, QVariant(newValue));

                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        int newIndex = isChoice->getValue(j);
                        const std::vector<std::string> entries = isChoice->getEntries_mt_safe();
                        if (newIndex < (int)entries.size() ) {
                            QString oldValue = settings.value(dimensionName).toString();
                            QString newValue(entries[newIndex].c_str());
                            if (oldValue != newValue) {
                                changedKnobs.push_back(knobs[i]);
                            }
                            settings.setValue(dimensionName, QVariant(newValue));

                        }
                    } else {
                        
                        int newValue = isInt->getValue(j);
                        int oldValue = settings.value(dimensionName, QVariant(INT_MIN)).toInt();
                        if (newValue != oldValue) {
                            changedKnobs.push_back(knobs[i]);
                        }
                        settings.setValue(dimensionName, QVariant(newValue));
                    }
                    
                } else if (isDouble) {
                    
                    double newValue = isDouble->getValue(j);
                    double oldValue = settings.value(dimensionName, QVariant(INT_MIN)).toDouble();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue(dimensionName, QVariant(newValue));
                } else if (isBool) {
                    
                    bool newValue = isBool->getValue(j);
                    bool oldValue = settings.value(dimensionName).toBool();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue(dimensionName, QVariant(newValue));
                } else {
                    assert(false);
                }
            } catch (std::logic_error) {
                // ignore
            }
        } // for (int j = 0; j < knobs[i]->getDimension(); ++j) {
    } // for (U32 i = 0; i < knobs.size(); ++i) {
    
    if (doWarnings) {
        warnChangedKnobs(changedKnobs);
    }
    
} // saveSettings

void
Settings::restoreKnobsFromSettings(const std::vector<KnobI*>& knobs)
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    
    for (U32 i = 0; i < knobs.size(); ++i) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knobs[i]);
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knobs[i]);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knobs[i]);
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knobs[i]);
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knobs[i]);
        
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
                        const std::vector<std::string> entries = isChoice->getEntries_mt_safe();
                        
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

}

void
Settings::restoreKnobsFromSettings(const std::vector<boost::shared_ptr<KnobI> >& knobs) {
    
    std::vector<KnobI*> k(knobs.size());
    for (U32 i = 0; i < knobs.size(); ++i) {
        k[i] = knobs[i].get();
    }
    restoreKnobsFromSettings(k);
}

void
Settings::restoreSettings()
{
    _restoringSettings = true;
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    restoreKnobsFromSettings(knobs);

    if (!_ocioRestored) {
        ///Load even though there's no settings!
        tryLoadOpenColorIOConfig();
    }
    
    _settingsExisted = false;
    try {
        _settingsExisted = _natronSettingsExist->getValue();

        if (!_settingsExisted) {
            _natronSettingsExist->setValue(true, 0);
            saveSetting(_natronSettingsExist.get());
        }
        
        int appearanceVersion = _defaultAppearanceVersion->getValue();
        if (_settingsExisted && appearanceVersion < NATRON_DEFAULT_APPEARANCE_VERSION) {
            _defaultAppearanceOutdated = true;
            _defaultAppearanceVersion->setValue(NATRON_DEFAULT_APPEARANCE_VERSION, 0);
            saveSetting(_defaultAppearanceVersion.get());
        }

        appPTR->setNThreadsPerEffect(getNumberOfThreadsPerEffect());
        appPTR->setNThreadsToRender(getNumberOfThreads());
        appPTR->setUseThreadPool(_useThreadPool->getValue());
    } catch (std::logic_error) {
        // ignore
    }

    
    _restoringSettings = false;
} // restoreSettings

bool
Settings::tryLoadOpenColorIOConfig()
{
    QString configFile;


    if ( _customOcioConfigFile->isEnabled(0) ) {
        ///try to load from the file
        std::string file;
        try {
            file = _customOcioConfigFile->getValue();
        } catch (...) {
            // ignore exceptions
        }
        if ( file.empty() ) {
            return false;
        }
        if ( !QFile::exists( file.c_str() ) ) {
            Natron::errorDialog( "OpenColorIO", file + QObject::tr(": No such file.").toStdString() );

            return false;
        }
        configFile = file.c_str();
    } else {
        try {
            ///try to load from the combobox
            QString activeEntryText( _ocioConfigKnob->getActiveEntryText_mt_safe().c_str() );
            QString configFileName = QString(activeEntryText + ".ocio");
            QStringList defaultConfigsPaths = getDefaultOcioConfigPaths();
            for (int i = 0; i < defaultConfigsPaths.size(); ++i) {
                QDir defaultConfigsDir(defaultConfigsPaths[i]);
                if ( !defaultConfigsDir.exists() ) {
                    qDebug() << "Attempt to read an OpenColorIO configuration but the configuration directory"
                    << defaultConfigsPaths[i] << "does not exist.";
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
        } catch (...) {
            // ignore exceptions
        }
        if ( configFile.isEmpty() ) {
            return false;
        }
    }
    _ocioRestored = true;
#ifdef DEBUG
    qDebug() << "setting OCIO=" << configFile;
#endif
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
                             Natron::ValueChangedReasonEnum reason,
                             SequenceTime /*time*/,
                             bool /*originatedFromMainThread*/)
{
    
    Q_EMIT settingChanged(k);
    
    if ( k == _maxViewerDiskCacheGB.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumViewerDiskSpace( getMaximumViewerDiskCacheSize() );
        }
    } else if ( k == _maxDiskCacheNodeGB.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumDiskSpace(getMaximumDiskCacheNodeSize());
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
    } else if ( k == _diskCachePath.get() ) {
        appPTR->setDiskCacheLocation(_diskCachePath->getValue().c_str());
    } else if ( k == _numberOfThreads.get() ) {
        int nbThreads = getNumberOfThreads();
        appPTR->setNThreadsToRender(nbThreads);
        if (nbThreads == -1) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else if (nbThreads == 0) {
            QThreadPool::globalInstance()->setMaxThreadCount( QThread::idealThreadCount() );
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
        }
    } else if ( k == _nThreadsPerEffect.get() ) {
        appPTR->setNThreadsPerEffect( getNumberOfThreadsPerEffect() );
    } else if ( k == _ocioConfigKnob.get() ) {
        if (_ocioConfigKnob->getActiveEntryText_mt_safe() == NATRON_CUSTOM_OCIO_CONFIG_NAME) {
            _customOcioConfigFile->setAllDimensionsEnabled(true);
        } else {
            _customOcioConfigFile->setAllDimensionsEnabled(false);
        }
        tryLoadOpenColorIOConfig();
        
    } else if ( k == _useThreadPool.get() ) {
        bool useTP = _useThreadPool->getValue();
        appPTR->setUseThreadPool(useTP);
    } else if ( k == _customOcioConfigFile.get() ) {
        if (_customOcioConfigFile->isEnabled(0)) {
            tryLoadOpenColorIOConfig();
            bool warnOcioChanged = _warnOcioConfigKnobChanged->getValue();
            if (warnOcioChanged && appPTR->getTopLevelInstance()) {
                bool stopAsking = false;
                Natron::warningDialog(QObject::tr("OCIO config changed").toStdString(),
                                      QObject::tr("The OpenColorIO config change requires a restart of "
                                                  NATRON_APPLICATION_NAME " to be effective.").toStdString(),&stopAsking);
                if (stopAsking) {
                    _warnOcioConfigKnobChanged->setValue(false,0);
                }
            }

        }
    } else if ( k == _maxUndoRedoNodeGraph.get() ) {
        appPTR->setUndoRedoStackLimit( _maxUndoRedoNodeGraph->getValue() );
    } else if ( k == _maxPanelsOpened.get() ) {
        appPTR->onMaxPanelsOpenedChanged( _maxPanelsOpened->getValue() );
    } else if ( k == _checkerboardTileSize.get() || k == _checkerboardColor1.get() || k == _checkerboardColor2.get() ) {
        appPTR->onCheckerboardSettingsChanged();
    }  else if (k == _hideOptionalInputsAutomatically.get() && !_restoringSettings && reason == Natron::eValueChangedReasonUserEdited) {
        appPTR->toggleAutoHideGraphInputs();
    } else if (k == _autoProxyWhenScrubbingTimeline.get()) {
        _autoProxyLevel->setSecret(!_autoProxyWhenScrubbingTimeline->getValue());
    } else if (!_restoringSettings &&
               (k == _sunkenColor.get() ||
                k == _baseColor.get() ||
                k == _raisedColor.get() ||
                k == _selectionColor.get() ||
                k == _textColor.get() ||
                k == _altTextColor.get() ||
                k == _timelinePlayheadColor.get() ||
                k == _timelineBoundsColor.get() ||
                k == _timelineBGColor.get() ||
                k == _interpolatedColor.get() ||
                k == _keyframeColor.get() ||
                k == _cachedFrameColor.get() ||
                k == _diskCachedFrameColor.get() ||
                k == _curveEditorBGColor.get() ||
                k == _gridColor.get() ||
                k == _curveEditorScaleColor.get() ||
                k == _dopeSheetEditorBackgroundColor.get() ||
                k == _dopeSheetEditorRootSectionBackgroundColor.get() ||
                k == _dopeSheetEditorKnobSectionBackgroundColor.get() ||
                k == _dopeSheetEditorScaleColor.get() ||
                k == _dopeSheetEditorGridColor.get())) {
                    appPTR->reloadStylesheets();
                  
                }
    else if (k == _qssFile.get()) {
        appPTR->reloadStylesheets();
    }
} // onKnobValueChanged

Natron::ImageBitDepthEnum
Settings::getViewersBitDepth() const
{
    int v = _texturesMode->getValue();
    if (v == 0) {
        return Natron::eImageBitDepthByte;
    } else if (v == 1) {
        return Natron::eImageBitDepthFloat;
    } else {
        return Natron::eImageBitDepthByte;
    }
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
Settings::getMaximumViewerDiskCacheSize() const
{
    return (U64)( _maxViewerDiskCacheGB->getValue() ) * std::pow(1024.,3.);
}

U64
Settings::getMaximumDiskCacheNodeSize() const
{
    return (U64)( _maxDiskCacheNodeGB->getValue() ) * std::pow(1024.,3.);
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
Settings::getNumberOfThreadsPerEffect() const
{
    return _nThreadsPerEffect->getValue();
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

std::string
Settings::getReaderPluginIDForFileType(const std::string & extension)
{
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        if (_readersMapping[i]->getDescription() == extension) {
            const std::vector<std::string> entries =  _readersMapping[i]->getEntries_mt_safe();
            int index = _readersMapping[i]->getValue();
            assert( index < (int)entries.size() );

            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
}

std::string
Settings::getWriterPluginIDForFileType(const std::string & extension)
{
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        if (_writersMapping[i]->getDescription() == extension) {
            const std::vector<std::string>  entries =  _writersMapping[i]->getEntries_mt_safe();
            int index = _writersMapping[i]->getValue();
            assert( index < (int)entries.size() );

            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
}

void
Settings::populateReaderPluginsAndFormats(const std::map<std::string,std::vector< std::pair<std::string,double> > > & rows)
{
    std::vector<boost::shared_ptr<KnobI> > knobs;
    for (std::map<std::string,std::vector< std::pair<std::string,double> > >::const_iterator it = rows.begin(); it != rows.end(); ++it) {
        boost::shared_ptr<KnobChoice> k = Natron::createKnob<KnobChoice>(this, it->first);
        k->setName("Reader_" + it->first);
        k->setAnimationEnabled(false);

        std::vector<std::string> entries;
        double bestPluginEvaluation = -2; //< tuttle's notation extension starts at -1
        int bestPluginIndex = -1;

        for (U32 i = 0; i < it->second.size(); ++i) {
            //qDebug() << it->first.c_str() << "candidate" << i << it->second[i].first.c_str() << it->second[i].second;
            if (it->second[i].second > bestPluginEvaluation) {
                bestPluginIndex = i;
                bestPluginEvaluation = it->second[i].second;
            }
            entries.push_back(it->second[i].first);
        }
        if (bestPluginIndex > -1) {
            k->setDefaultValue(bestPluginIndex,0);
        }
        k->populateChoices(entries);
        _readersMapping.push_back(k);
        _readersTab->addKnob(k);
        knobs.push_back(k);
    }
    restoreKnobsFromSettings(knobs);

}

void
Settings::populateWriterPluginsAndFormats(const std::map<std::string,std::vector< std::pair<std::string,double> > > & rows)
{
    std::vector<boost::shared_ptr<KnobI> > knobs;

    for (std::map<std::string,std::vector< std::pair<std::string,double> > >::const_iterator it = rows.begin(); it != rows.end(); ++it) {
        boost::shared_ptr<KnobChoice> k = Natron::createKnob<KnobChoice>(this, it->first);
        k->setName("Writer_" + it->first);
        k->setAnimationEnabled(false);

        std::vector<std::string> entries;
        double bestPluginEvaluation = -2; //< tuttle's notation extension starts at -1
        int bestPluginIndex = -1;

        for (U32 i = 0; i < it->second.size(); ++i) {
            //qDebug() << it->first.c_str() << "candidate" << i << it->second[i].first.c_str() << it->second[i].second;
            if (it->second[i].second > bestPluginEvaluation) {
                bestPluginIndex = i;
                bestPluginEvaluation = it->second[i].second;
            }
            entries.push_back(it->second[i].first);
        }
        if (bestPluginIndex > -1) {
            k->setDefaultValue(bestPluginIndex,0);
        }
        k->populateChoices(entries);
        _writersMapping.push_back(k);
        _writersTab->addKnob(k);
        knobs.push_back(k);

    }
    restoreKnobsFromSettings(knobs);
}

static bool filterDefaultActivatedPlugin(const QString& /*ofxPluginID*/)
{
#if 0
#pragma message WARN("WHY censor this list of plugins? This is open source fer chrissake! Let the user take control!")
    if (
            //Tuttle Readers/Writers
            ofxPluginID == "tuttle.avreader" ||
            ofxPluginID == "tuttle.avwriter" ||
            ofxPluginID == "tuttle.dpxwriter" ||
            ofxPluginID == "tuttle.exrreader" ||
            ofxPluginID == "tuttle.exrwriter" ||
            ofxPluginID == "tuttle.imagemagickreader" ||
            ofxPluginID == "tuttle.jpeg2000reader" ||
            ofxPluginID == "tuttle.jpeg2000writer" ||
            ofxPluginID == "tuttle.jpegreader" ||
            ofxPluginID == "tuttle.jpegwriter" ||
            ofxPluginID == "tuttle.oiioreader" ||
            ofxPluginID == "tuttle.oiiowriter" ||
            ofxPluginID == "tuttle.pngreader" ||
            ofxPluginID == "tuttle.pngwriter" ||
            ofxPluginID == "tuttle.rawreader" ||
            ofxPluginID == "tuttle.turbojpegreader" ||
            ofxPluginID == "tuttle.turbojpegwriter" ||

            //Other Tuttle plug-ins
            ofxPluginID == "tuttle.bitdepth" ||
            ofxPluginID == "tuttle.colorgradation" ||
            ofxPluginID == "tuttle.gamma" ||
            ofxPluginID == "tuttle.invert" ||
            ofxPluginID == "tuttle.histogramkeyer" ||
            ofxPluginID == "tuttle.idkeyer" ||
            ofxPluginID == "tuttle.ocio.colorspace" ||
            ofxPluginID == "tuttle.ocio.lut" ||
            ofxPluginID == "tuttle.constant" ||
            ofxPluginID == "tuttle.inputbuffer" ||
            ofxPluginID == "tuttle.outputbuffer" ||
            ofxPluginID == "tuttle.ramp" ||
            ofxPluginID == "tuttle.lut" ||
            ofxPluginID == "tuttle.print" ||
            ofxPluginID == "tuttle.colorgradient" ||
            ofxPluginID == "tuttle.component" ||
            ofxPluginID == "tuttle.merge" ||
            ofxPluginID == "tuttle.crop" ||
            ofxPluginID == "tuttle.flip" ||
            ofxPluginID == "tuttle.resize" ||
            ofxPluginID == "tuttle.pinning" ||
            ofxPluginID == "tuttle.timeshift" ||
            ofxPluginID == "tuttle.diff" ||
            ofxPluginID == "tuttle.dummy" ||
            ofxPluginID == "tuttle.histogram" ||
            ofxPluginID == "tuttle.imagestatistics" ||
            ofxPluginID == "tuttle.debugimageeffectapi" ||
            ofxPluginID == "tuttle.viewer"
            ) {
        //These plug-ins of TuttleOFX achieve the same as plug-ins bundled with Natron, deactivate them by default.
        return false;
    }
#endif
    return true;
}

/**
 * @brief Returns whether the given plug-in should by default have it's default render-scale support (0) or
 * it should be deactivated (1).
 **/
static int filterDefaultRenderScaleSupportPlugin(const QString& ofxPluginID)
{
    if (ofxPluginID == "tuttle.colorbars" ||
            ofxPluginID == "tuttle.checkerboard" ||
            ofxPluginID == "tuttle.colorcube" ||
            ofxPluginID == "tuttle.colorwheel" ||
            ofxPluginID == "tuttle.ramp" ||
            ofxPluginID == "tuttle.constant") {
        return 1;
    }
    return 0;
}

struct PerPluginKnobs
{
    boost::shared_ptr<KnobBool> enabled;
    boost::shared_ptr<KnobChoice> renderScaleSupport;
    
    PerPluginKnobs(const boost::shared_ptr<KnobBool>& enabled,
                   const boost::shared_ptr<KnobChoice>& renderScaleSupport)
        : enabled(enabled)
        , renderScaleSupport(renderScaleSupport)
    {
        
    }
    
    PerPluginKnobs()
        : enabled() , renderScaleSupport()
    {
        
    }
};

void
Settings::populatePluginsTab(std::vector<Natron::Plugin*>& pluginsToIgnore)
{
    
    const PluginsMap& plugins = appPTR->getPluginsList();
    
    std::vector<boost::shared_ptr<KnobI> > knobsToRestore;
    
    std::map<Natron::Plugin*,PerPluginKnobs> pluginsMap;
    
    std::map< std::string,std::string > groupNames;
    ///First pass to exctract all groups
    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {

        if (it->first.empty()) {
            continue;
        }
        assert(it->second.size() > 0);
        
        const QStringList& grouping = (*it->second.rbegin())->getGrouping();
        if (grouping.size() > 0) {
            
            groupNames.insert(std::make_pair(Natron::makeNameScriptFriendly(grouping[0].toStdString()),grouping[0].toStdString()));
        }
    }
    
    ///Now create all groups

    std::list< boost::shared_ptr<KnobGroup> > groups;
    for (std::map< std::string,std::string >::iterator it = groupNames.begin(); it != groupNames.end(); ++it) {
        boost::shared_ptr<KnobGroup>  g = Natron::createKnob<KnobGroup>(this, it->second);
        g->setName(it->first);
        _pluginsTab->addKnob(g);
        groups.push_back(g);
    }
    
    std::vector<std::string> zoomSupportEntries;
    zoomSupportEntries.push_back("Plugin default");
    zoomSupportEntries.push_back("Deactivated");
    
    ///Create per-plugin knobs and add them to groups
    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        
        if (it->first.empty()) {
            continue;
        }
        assert(it->second.size() > 0);
        
        Natron::Plugin* plugin  = *it->second.rbegin();
        assert(plugin);
        
        if (plugin->getIsForInternalUseOnly()) {
            continue;
        }
        
        boost::shared_ptr<KnobGroup> group;
        const QStringList& grouping = plugin->getGrouping();
        if (grouping.size() > 0) {
            
            std::string mainGroup = Natron::makeNameScriptFriendly(grouping[0].toStdString());
            
            ///Find the corresponding group
            for (std::list< boost::shared_ptr<KnobGroup> >::const_iterator it2 = groups.begin(); it2 != groups.end(); ++it2) {
                if ((*it2)->getName() == mainGroup) {
                    group  = *it2;
                    break;
                }
            }
        }
        
        ///Create checkbox to activate/deactivate the plug-in
        std::string pluginName = plugin->getPluginID().toStdString();
        
        boost::shared_ptr<KnobString> pluginLabel = Natron::createKnob<KnobString>(this, pluginName);
        pluginLabel->setAsLabel();
        pluginLabel->setName(it->first);
        pluginLabel->setAnimationEnabled(false);
        pluginLabel->setDefaultValue(pluginName);
        pluginLabel->setAddNewLine(false);
        pluginLabel->hideDescription();
        pluginLabel->setIsPersistant(false);
        if (group) {
            group->addKnob(pluginLabel);
        }
        
        
        boost::shared_ptr<KnobBool> pluginActivation = Natron::createKnob<KnobBool>(this, "Enabled");
        pluginActivation->setDefaultValue(filterDefaultActivatedPlugin(plugin->getPluginID()) && plugin->getIsUserCreatable());
        pluginActivation->setName(it->first + ".enabled");
        pluginActivation->setAnimationEnabled(false);
        pluginActivation->setAddNewLine(false);
        pluginActivation->setHintToolTip("When checked, " + pluginName + " will be activated and you can create a node using this plug-in in " NATRON_APPLICATION_NAME ". When unchecked, you'll be unable to create a node for this plug-in. Changing this parameter requires a restart of the application.");
        if (group) {
            group->addKnob(pluginActivation);
        }
        
        knobsToRestore.push_back(pluginActivation);
        
        boost::shared_ptr<KnobChoice> zoomSupport = Natron::createKnob<KnobChoice>(this, "Zoom support");
        zoomSupport->populateChoices(zoomSupportEntries);
        zoomSupport->setName(it->first + ".zoomSupport");
        zoomSupport->setDefaultValue(filterDefaultRenderScaleSupportPlugin(plugin->getPluginID()));
        zoomSupport->setHintToolTip("Controls whether the plug-in should have its default zoom support or it should be activated. "
                                    "This parameter is useful because some plug-ins flag that they can support different level of zoom "
                                    "scale for rendering but in reality they don't. This enables you to explicitly turn-off that flag for a particular "
                                    "plug-in, hence making it work at different zoom levels."
                                    "Changes to this parameter will not be applied to existing instances of the plug-in (nodes) unless you "
                                    "restart the application.");
        zoomSupport->setAnimationEnabled(false);
        if (group) {
            group->addKnob(zoomSupport);
        }
        
        knobsToRestore.push_back(zoomSupport);

        
        pluginsMap.insert(std::make_pair(plugin, PerPluginKnobs(pluginActivation,zoomSupport)));

    }
    
    for (std::map<Natron::Plugin*,PerPluginKnobs>::iterator it = pluginsMap.begin(); it != pluginsMap.end(); ++it) {
        if (!it->second.enabled->getValue()) {
            pluginsToIgnore.push_back(it->first);
        } else {
            _perPluginRenderScaleSupport.insert(std::make_pair(it->first->getPluginID().toStdString(), it->second.renderScaleSupport));
        }
    }
}

void
Settings::populateSystemFonts(const QSettings& settings,const std::vector<std::string>& fonts)
{
    _systemFontChoice->populateChoices(fonts);
    
    for (U32 i = 0; i < fonts.size(); ++i) {
        if (fonts[i] == NATRON_FONT) {
            _systemFontChoice->setDefaultValue(i);
            break;
        }
    }
    ///Now restore properly the system font choice
    QString name(_systemFontChoice->getName().c_str());
    if (settings.contains(name)) {
        std::string value = settings.value(name).toString().toStdString();
        for (U32 i = 0; i < fonts.size(); ++i) {
            if (fonts[i] == value) {
                _systemFontChoice->setValue(i, 0);
                break;
            }
        }
    }
}

void
Settings::getFileFormatsForReadingAndReader(std::map<std::string,std::string>* formats)
{
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        const std::vector<std::string>  entries = _readersMapping[i]->getEntries_mt_safe();
        int index = _readersMapping[i]->getValue();

        assert( index < (int)entries.size() );
        std::string name = _readersMapping[i]->getName();
        std::size_t prefix = name.find("Reader_");
        if (prefix != std::string::npos) {
            name.erase(prefix,7);
            formats->insert( std::make_pair(name,entries[index]) );
        }
    }
}

void
Settings::getFileFormatsForWritingAndWriter(std::map<std::string,std::string>* formats)
{
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string>  entries = _writersMapping[i]->getEntries_mt_safe();
        int index = _writersMapping[i]->getValue();

        assert( index < (int)entries.size() );
        std::string name = _writersMapping[i]->getName();
        std::size_t prefix = name.find("Writer_");
        if (prefix != std::string::npos) {
            name.erase(prefix,7);
            formats->insert( std::make_pair(name,entries[index]) );
        }
    }
}

void
Settings::getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const
{
    assert(paths);
    try {
        _extraPluginPaths->getPaths(paths);
    } catch (std::logic_error) {
        paths->clear();
    }
}

void
Settings::restoreDefault()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    if ( !QFile::remove( settings.fileName() ) ) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }

    beginKnobsValuesChanged(Natron::eValueChangedReasonPluginEdited);
    const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            knobs[i]->resetToDefaultValue(j);
        }
    }
    setCachingLabels();
    endKnobsValuesChanged(Natron::eValueChangedReasonPluginEdited);
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
    saveSetting(_checkForUpdates.get());
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

int Settings::getNumberOfParallelRenders() const
{
    return _numberOfParallelRenders->getValue();

}

void
Settings::setNumberOfParallelRenders(int nb)
{
    _numberOfParallelRenders->setValue(nb, 0);
}

bool
Settings::areRGBPixelComponentsSupported() const
{
    return _activateRGBSupport->getValue();
}

bool
Settings::isTransformConcatenationEnabled() const
{
    return _activateTransformConcatenationSupport->getValue();
}

bool
Settings::useGlobalThreadPool() const
{
    return _useThreadPool->getValue();
}

void
Settings::setUseGlobalThreadPool(bool use)
{
    _useThreadPool->setValue(use,0);
}

bool
Settings::isMergeAutoConnectingToAInput() const
{
    return _useInputAForMergeAutoConnect->getValue();
}

void
Settings::doOCIOStartupCheckIfNeeded()
{
    bool docheck = _ocioStartupCheck->getValue();
    AppInstance* mainInstance = appPTR->getTopLevelInstance();
    
    if (!mainInstance) {
        qDebug() << "WARNING: doOCIOStartupCheckIfNeeded() called without a AppInstance";
        return;
    }
    
    if (docheck && mainInstance) {
        int entry_i = _ocioConfigKnob->getValue();
        std::vector<std::string> entries = _ocioConfigKnob->getEntries_mt_safe();
        
        std::string warnText;
        if (entry_i < 0 || entry_i >= (int)entries.size()) {
            warnText = "The current OCIO config selected in the preferences is invalid, would you like to set it to the default config (" NATRON_DEFAULT_OCIO_CONFIG_NAME ") ?";
        } else if (entries[entry_i] != NATRON_DEFAULT_OCIO_CONFIG_NAME) {
            warnText = "The current OCIO config selected in the preferences is not the default one (" NATRON_DEFAULT_OCIO_CONFIG_NAME "),"
                                                                                                                                      " would you like to set it to the default config ?";
        } else {
            return;
        }
        
        bool stopAsking = false;
        Natron::StandardButtonEnum reply = mainInstance->questionDialog("OCIO config", QObject::tr(warnText.c_str()).toStdString(),false,
                                                                        Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo),
                                                                        Natron::eStandardButtonYes,
                                                                        &stopAsking);
        if (stopAsking != !docheck) {
            _ocioStartupCheck->setValue(!stopAsking,0);
            saveSetting(_ocioStartupCheck.get());
        }
        
        if (reply == Natron::eStandardButtonYes) {
            
            int defaultIndex = -1;
            for (int i = 0; i < (int)entries.size(); ++i) {
                if ( entries[i].find(NATRON_DEFAULT_OCIO_CONFIG_NAME) != std::string::npos ) {
                    defaultIndex = i;
                    break;
                }
            }
            if (defaultIndex != -1) {
                _ocioConfigKnob->setValue(defaultIndex,0);
                saveSetting(_ocioConfigKnob.get());
            } else {
                Natron::warningDialog("OCIO config", QObject::tr("The " NATRON_DEFAULT_OCIO_CONFIG_NAME " config could not be found. "
                                                                                                        "This is probably because you're not using the OpenColorIO-Configs folder that should "
                                                                                                        "be bundled with your " NATRON_APPLICATION_NAME " installation.").toStdString());
            }
        }

    }
}

bool
Settings::didSettingsExistOnStartup() const
{
    return _settingsExisted;
}


int
Settings::getRenderScaleSupportPreference(const std::string& pluginID) const
{
    std::map<std::string,boost::shared_ptr<KnobChoice> >::const_iterator found = _perPluginRenderScaleSupport.find(pluginID);
    if (found != _perPluginRenderScaleSupport.end()) {
        return found->second->getValue();
    }
    return -1;
}

bool
Settings::notifyOnFileChange() const
{
    return _notifyOnFileChange->getValue();
}

bool
Settings::isAggressiveCachingEnabled() const
{
    return _aggressiveCaching->getValue();
}

bool
Settings::isAutoTurboEnabled() const
{
    return _autoTurbo->getValue();
}

void
Settings::setAutoTurboModeEnabled(bool e)
{
    _autoTurbo->setValue(e, 0);
}

void
Settings::setOptionalInputsAutoHidden(bool hidden)
{
    _hideOptionalInputsAutomatically->setValue(hidden, 0);
}

bool
Settings::areOptionalInputsAutoHidden() const
{
    return _hideOptionalInputsAutomatically->getValue();
}

void
Settings::getPythonGroupsSearchPaths(std::list<std::string>* templates) const
{
    _templatesPluginPaths->getPaths(templates);
}

void
Settings::appendPythonGroupsPath(const std::string& path)
{
    _templatesPluginPaths->appendPath(path);
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.setValue(_templatesPluginPaths->getName().c_str(), QVariant(_templatesPluginPaths->getValue(0).c_str()));
}

std::string
Settings::getDefaultOnProjectLoadedCB()
{
    return _defaultOnProjectLoaded->getValue();
}

std::string
Settings::getDefaultOnProjectSaveCB()
{
    return _defaultOnProjectSave->getValue();
}

std::string
Settings::getDefaultOnProjectCloseCB()
{
    return _defaultOnProjectClose->getValue();
}

std::string
Settings::getDefaultOnNodeCreatedCB()
{
    return _defaultOnNodeCreated->getValue();
}

std::string
Settings::getDefaultOnNodeDeleteCB()
{
    return _defaultOnNodeDelete->getValue();
}

std::string
Settings::getOnProjectCreatedCB()
{
    return _onProjectCreated->getValue();
}

bool
Settings::isLoadFromPyPlugsEnabled() const
{
    return _loadPyPlugsFromPythonScript->getValue();
}

bool
Settings::isAutoDeclaredVariablePrintActivated() const
{
    return _echoVariableDeclarationToPython->getValue();
}

bool
Settings::isPluginIconActivatedOnNodeGraph() const
{
    return _usePluginIconsInNodeGraph->getValue();

}

void
Settings::getSunkenColor(double* r,double* g,double* b) const
{
    *r = _sunkenColor->getValue(0);
    *g = _sunkenColor->getValue(1);
    *b = _sunkenColor->getValue(2);
}

void
Settings::getBaseColor(double* r,double* g,double* b) const
{
    *r = _baseColor->getValue(0);
    *g = _baseColor->getValue(1);
    *b = _baseColor->getValue(2);
}
void
Settings::getRaisedColor(double* r,double* g,double* b) const
{
    *r = _raisedColor->getValue(0);
    *g = _raisedColor->getValue(1);
    *b = _raisedColor->getValue(2);
}
void
Settings::getSelectionColor(double* r,double* g,double* b) const
{
    *r = _selectionColor->getValue(0);
    *g = _selectionColor->getValue(1);
    *b = _selectionColor->getValue(2);
}
void
Settings::getInterpolatedColor(double* r,double* g,double* b) const
{
    *r = _interpolatedColor->getValue(0);
    *g = _interpolatedColor->getValue(1);
    *b = _interpolatedColor->getValue(2);
}
void
Settings::getKeyframeColor(double* r,double* g,double* b) const
{
    *r = _keyframeColor->getValue(0);
    *g = _keyframeColor->getValue(1);
    *b = _keyframeColor->getValue(2);
}

void
Settings::getExprColor(double* r,double* g,double* b) const
{
    *r = _exprColor->getValue(0);
    *g = _exprColor->getValue(1);
    *b = _exprColor->getValue(2);
}

void
Settings::getTextColor(double* r,double* g,double* b) const
{
    *r = _textColor->getValue(0);
    *g = _textColor->getValue(1);
    *b = _textColor->getValue(2);
}

void
Settings::getAltTextColor(double* r,double* g,double* b) const
{
    *r = _altTextColor->getValue(0);
    *g = _altTextColor->getValue(1);
    *b = _altTextColor->getValue(2);
}

void
Settings::getTimelinePlayheadColor(double* r,double* g,double* b) const
{
    *r = _timelinePlayheadColor->getValue(0);
    *g = _timelinePlayheadColor->getValue(1);
    *b = _timelinePlayheadColor->getValue(2);
}

void
Settings::getTimelineBoundsColor(double* r,double* g,double* b) const
{
    *r = _timelineBoundsColor->getValue(0);
    *g = _timelineBoundsColor->getValue(1);
    *b = _timelineBoundsColor->getValue(2);
}

void
Settings::getTimelineBGColor(double* r,double* g,double* b) const
{
    *r = _timelineBGColor->getValue(0);
    *g = _timelineBGColor->getValue(1);
    *b = _timelineBGColor->getValue(2);
}

void
Settings::getCachedFrameColor(double* r,double* g,double* b) const
{
    *r = _cachedFrameColor->getValue(0);
    *g = _cachedFrameColor->getValue(1);
    *b = _cachedFrameColor->getValue(2);
}

void
Settings::getDiskCachedColor(double* r,double* g,double* b) const
{
    *r = _diskCachedFrameColor->getValue(0);
    *g = _diskCachedFrameColor->getValue(1);
    *b = _diskCachedFrameColor->getValue(2);
}

void
Settings::getCurveEditorBGColor(double* r,double* g,double* b) const
{
    *r = _curveEditorBGColor->getValue(0);
    *g = _curveEditorBGColor->getValue(1);
    *b = _curveEditorBGColor->getValue(2);
}

void
Settings::getCurveEditorGridColor(double* r,double* g,double* b) const
{
    *r = _gridColor->getValue(0);
    *g = _gridColor->getValue(1);
    *b = _gridColor->getValue(2);
}

void
Settings::getCurveEditorScaleColor(double* r,double* g,double* b) const
{
    *r = _curveEditorScaleColor->getValue(0);
    *g = _curveEditorScaleColor->getValue(1);
    *b = _curveEditorScaleColor->getValue(2);
}

void
Settings::getDopeSheetEditorBackgroundColor(double *r, double *g, double *b) const
{
    *r = _dopeSheetEditorBackgroundColor->getValue(0);
    *g = _dopeSheetEditorBackgroundColor->getValue(1);
    *b = _dopeSheetEditorBackgroundColor->getValue(2);
}

void
Settings::getDopeSheetEditorRootRowBackgroundColor(double *r, double *g, double *b, double *a) const
{
    *r = _dopeSheetEditorRootSectionBackgroundColor->getValue(0);
    *g = _dopeSheetEditorRootSectionBackgroundColor->getValue(1);
    *b = _dopeSheetEditorRootSectionBackgroundColor->getValue(2);
    *a = _dopeSheetEditorRootSectionBackgroundColor->getValue(3);
}

void
Settings::getDopeSheetEditorKnobRowBackgroundColor(double *r, double *g, double *b, double *a) const
{
    *r = _dopeSheetEditorKnobSectionBackgroundColor->getValue(0);
    *g = _dopeSheetEditorKnobSectionBackgroundColor->getValue(1);
    *b = _dopeSheetEditorKnobSectionBackgroundColor->getValue(2);
    *a = _dopeSheetEditorKnobSectionBackgroundColor->getValue(3);
}

void
Settings::getDopeSheetEditorScaleColor(double *r, double *g, double *b) const
{
    *r = _dopeSheetEditorScaleColor->getValue(0);
    *g = _dopeSheetEditorScaleColor->getValue(1);
    *b = _dopeSheetEditorScaleColor->getValue(2);
}

void
Settings::getDopeSheetEditorGridColor(double *r, double *g, double *b) const
{
    *r = _dopeSheetEditorGridColor->getValue(0);
    *g = _dopeSheetEditorGridColor->getValue(1);
    *b = _dopeSheetEditorGridColor->getValue(2);
}

void Settings::getPluginIconFrameColor(int *r, int *g, int *b) const
{
    *r = 50;
    *g = 50;
    *b = 50;
}

int Settings::getDopeSheetEditorNodeSeparationWith() const
{
    return 4;
}

bool
Settings::isAutoProxyEnabled() const
{
    return _autoProxyWhenScrubbingTimeline->getValue();
}

unsigned int
Settings::getAutoProxyMipMapLevel() const
{
    return (unsigned int)_autoProxyLevel->getValue() + 1;
}

bool
Settings::isNaNHandlingEnabled() const
{
    return _convertNaNValues->getValue();
}


void
Settings::setOnProjectCreatedCB(const std::string& func)
{
    _onProjectCreated->setValue(func, 0);
}

void
Settings::setOnProjectLoadedCB(const std::string& func)
{
    _defaultOnProjectLoaded->setValue(func, 0);
}

bool
Settings::isInViewerProgressReportEnabled() const
{
    return _enableProgressReport->getValue();
}

bool
Settings::isDefaultAppearanceOutdated() const
{
    return _defaultAppearanceOutdated;
}

void
Settings::restoreDefaultAppearance()
{
    std::vector< boost::shared_ptr<KnobI> > children = _appearanceTab->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        KnobColor* isColorKnob = dynamic_cast<KnobColor*>(children[i].get());
        if (isColorKnob && isColorKnob->isSimplified()) {
            isColorKnob->blockValueChanges();
            for (int j = 0; j < isColorKnob->getDimension(); ++j) {
                isColorKnob->resetToDefaultValue(j);
            }
            isColorKnob->unblockValueChanges();
        }
    }
    _defaultAppearanceOutdated = false;
    appPTR->reloadStylesheets();
}

std::string
Settings::getUserStyleSheetFilePath() const
{
    return _qssFile->getValue();
}
