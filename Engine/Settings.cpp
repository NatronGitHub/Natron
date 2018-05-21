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

#include "Settings.h"

#include <cassert>
#include <stdexcept>
#include <sstream>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QThreadPool>
#include <QtCore/QThread>
#include <QtCore/QTextStream>

#ifdef WINDOWS
#include <tchar.h>
#endif

#include "Global/StrUtils.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/Cache.h"
#include "Global/FStreamsSupport.h"
#include "Engine/KeybindShortcut.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/MemoryInfo.h" // getSystemTotalRAM, isApplication32Bits, printAsRAM
#include "Engine/Node.h"
#include "Engine/OSGLContext.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/StandardPaths.h"
#include "Engine/Utils.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/SettingsSerialization.h"
#include "Serialization/SerializationIO.h"

#include "Gui/GuiDefines.h"

#include <SequenceParsing.h> // for SequenceParsing::removePath

#ifdef WINDOWS
#include <ofxhPluginCache.h>
#endif

#ifdef HAVE_OSMESA
#include "Engine/OSGLFunctions_mesa.h"
#endif

#define NATRON_SETTINGS_FILENAME "settings." NATRON_SETTINGS_FILE_EXT

#define NATRON_DEFAULT_OCIO_CONFIG_NAME "blender"


#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

// Increment this everytime a default value for a color in the Appearance tab is changed
// If the user was running Natron is a default appearance of an older version, it will prompt
// to update to the newer appearance.
#define NATRON_DEFAULT_APPEARANCE_VERSION 1

#define NATRON_CUSTOM_HOST_NAME_ENTRY "Custom..."

NATRON_NAMESPACE_ENTER

enum CPUDriverEnum {
    eCPUDriverSoftPipe = 0,
    eCPUDriverLLVMPipe
};

static const CPUDriverEnum defaultMesaDriver = eCPUDriverLLVMPipe;



struct SettingsPrivate
{

    Q_DECLARE_TR_FUNCTIONS(Settings)

public:

    // General
    KnobPagePtr _generalTab;
    KnobBoolPtr _checkForUpdates;
#ifdef NATRON_USE_BREAKPAD
    KnobBoolPtr _enableCrashReports;
    KnobButtonPtr _testCrashReportButton;
#endif
    KnobBoolPtr _autoSaveUnSavedProjects;
    KnobPathPtr _fileDialogSavedPaths;
    KnobIntPtr _autoSaveDelay;
    KnobBoolPtr _saveSafetyMode;
    KnobChoicePtr _hostName;
    KnobStringPtr _customHostName;

    // General/Threading
    KnobPagePtr _threadingPage;
    KnobIntPtr _numberOfThreads;
    KnobBoolPtr _renderInSeparateProcess;
    KnobBoolPtr _queueRenders;

    // General/Rendering
    KnobPagePtr _renderingPage;
    KnobBoolPtr _convertNaNValues;
    KnobBoolPtr _activateRGBSupport;
    KnobBoolPtr _activateTransformConcatenationSupport;

    // General/GPU rendering
    KnobPagePtr _gpuPage;
    KnobStringPtr _openglRendererString;
    KnobChoicePtr _availableOpenGLRenderers;
    KnobChoicePtr _osmesaRenderers;
    KnobIntPtr _nOpenGLContexts;
    KnobChoicePtr _enableOpenGL;



    // General/Projects setup
    KnobPagePtr _projectsPage;
    KnobBoolPtr _firstReadSetProjectFormat;
    KnobBoolPtr _autoPreviewEnabledForNewProjects;
    KnobBoolPtr _fixPathsOnProjectPathChanged;
    KnobBoolPtr _enableMappingFromDriveLettersToUNCShareNames;

    // General/Documentation
    KnobPagePtr _documentationPage;
    KnobIntPtr _wwwServerPort;
#ifdef NATRON_DOCUMENTATION_ONLINE
    KnobChoicePtr _documentationSource;
#endif

    // General/User Interface
    KnobPagePtr _uiPage;
    KnobBoolPtr _notifyOnFileChange;

    KnobBoolPtr _filedialogForWriters;

    KnobBoolPtr _renderOnEditingFinished;
    KnobBoolPtr _linearPickers;
    KnobIntPtr _maxPanelsOpened;
    KnobBoolPtr _useCursorPositionIncrements;
    KnobFilePtr _defaultLayoutFile;
    KnobBoolPtr _loadProjectsWorkspace;

    // Color-Management
    KnobPagePtr _ocioTab;
    KnobChoicePtr _ocioConfigKnob;
    KnobBoolPtr _ocioStartupCheck;
    KnobFilePtr _customOcioConfigFile;

    // Caching
    KnobPagePtr _cachingTab;
    KnobBoolPtr _aggressiveCaching;

    // The total disk space allowed for all Natron's caches
    KnobIntPtr _maxDiskCacheSizeGb;
    KnobPathPtr _diskCachePath;

    // Viewer
    KnobPagePtr _viewersTab;
    KnobChoicePtr _texturesMode;
    KnobIntPtr _checkerboardTileSize;
    KnobColorPtr _checkerboardColor1;
    KnobColorPtr _checkerboardColor2;
    KnobBoolPtr _autoWipe;
    KnobBoolPtr _autoProxyWhenScrubbingTimeline;
    KnobChoicePtr _autoProxyLevel;
    KnobIntPtr _maximumNodeViewerUIOpened;
    KnobBoolPtr _viewerKeys;

    // Nodegraph
    KnobPagePtr _nodegraphTab;
    KnobBoolPtr _autoScroll;
    KnobBoolPtr _autoTurbo;
    KnobBoolPtr _snapNodesToConnections;
    KnobBoolPtr _useBWIcons;
    KnobIntPtr _maxUndoRedoNodeGraph;
    KnobIntPtr _disconnectedArrowLength;
    KnobBoolPtr _hideOptionalInputsAutomatically;
    KnobBoolPtr _useInputAForMergeAutoConnect;
    KnobBoolPtr _usePluginIconsInNodeGraph;
    KnobBoolPtr _useAntiAliasing;


    // Plugins
    KnobPagePtr _pluginsTab;
    KnobPathPtr _extraPluginPaths;
    KnobBoolPtr _useStdOFXPluginsLocation;
    KnobPathPtr _templatesPluginPaths;
    KnobBoolPtr _preferBundledPlugins;
    KnobBoolPtr _loadBundledPlugins;

    // Python
    KnobPagePtr _pythonPage;
    KnobStringPtr _onProjectCreated;
    KnobStringPtr _defaultOnProjectLoaded;
    KnobStringPtr _defaultOnProjectSave;
    KnobStringPtr _defaultOnProjectClose;
    KnobStringPtr _defaultOnNodeCreated;
    KnobStringPtr _defaultOnNodeDelete;
    KnobBoolPtr _echoVariableDeclarationToPython;

    // Appearance
    KnobPagePtr _appearanceTab;
    KnobChoicePtr _systemFontChoice;
    KnobIntPtr _fontSize;
    KnobFilePtr _qssFile;
    KnobIntPtr _defaultAppearanceVersion;

    // Appearance/Main Window
    KnobPagePtr _guiColorsTab;
    KnobColorPtr _sunkenColor;
    KnobColorPtr _baseColor;
    KnobColorPtr _raisedColor;
    KnobColorPtr _selectionColor;
    KnobColorPtr _textColor;
    KnobColorPtr _altTextColor;
    KnobColorPtr _timelinePlayheadColor;
    KnobColorPtr _timelineBGColor;
    KnobColorPtr _timelineBoundsColor;
    KnobColorPtr _interpolatedColor;
    KnobColorPtr _keyframeColor;
    KnobColorPtr _trackerKeyframeColor;
    KnobColorPtr _exprColor;
    KnobColorPtr _cachedFrameColor;
    KnobColorPtr _sliderColor;

    // Appearance/Dope Sheet
    KnobPagePtr _animationColorsTab;
    KnobColorPtr _animationViewBackgroundColor;
    KnobColorPtr _dopesheetRootSectionBackgroundColor;
    KnobColorPtr _dopesheetKnobSectionBackgroundColor;
    KnobColorPtr _animationViewScaleColor;
    KnobColorPtr _animationViewGridColor;

    // Appearance/Script Editor
    KnobPagePtr _scriptEditorColorsTab;
    KnobColorPtr _curLineColor;
    KnobColorPtr _keywordColor;
    KnobColorPtr _operatorColor;
    KnobColorPtr _braceColor;
    KnobColorPtr _defClassColor;
    KnobColorPtr _stringsColor;
    KnobColorPtr _commentsColor;
    KnobColorPtr _selfColor;
    KnobColorPtr _numbersColor;
    KnobChoicePtr _scriptEditorFontChoice;
    KnobIntPtr _scriptEditorFontSize;

    // Appearance/Node Graph
    KnobPagePtr _nodegraphColorsTab;
    KnobColorPtr _cloneColor;
    KnobColorPtr _defaultNodeColor;
    KnobColorPtr _defaultBackdropColor;
    KnobColorPtr _defaultGeneratorColor;
    KnobColorPtr _defaultReaderColor;
    KnobColorPtr _defaultWriterColor;
    KnobColorPtr _defaultColorGroupColor;
    KnobColorPtr _defaultFilterGroupColor;
    KnobColorPtr _defaultTransformGroupColor;
    KnobColorPtr _defaultTimeGroupColor;
    KnobColorPtr _defaultDrawGroupColor;
    KnobColorPtr _defaultKeyerGroupColor;
    KnobColorPtr _defaultChannelGroupColor;
    KnobColorPtr _defaultMergeGroupColor;
    KnobColorPtr _defaultViewsGroupColor;
    KnobColorPtr _defaultDeepGroupColor;
    std::vector<ChoiceOption> _knownHostNames;

    Settings* _publicInterface;

    std::set<KnobIWPtr> _knobsRequiringRestart;
    std::set<KnobIWPtr> _knobsRequiringOFXCacheClear;

    // Data for each plug-in, saved in the settings file
    SERIALIZATION_NAMESPACE::SettingsSerialization::PluginDataMap pluginsData;

    ApplicationShortcutsMap shortcuts;


    // Count the number of stylesheet changes during restore defaults
    // to avoid reloading it many times
    int _nRedrawStyleSheetRequests;
    bool _restoringSettings;
    bool _defaultAppearanceOutdated;
    bool saveSettings;



    SettingsPrivate(Settings* publicInterface)
    : _publicInterface(publicInterface)
    , _knobsRequiringRestart()
    , _knobsRequiringOFXCacheClear()
    , _nRedrawStyleSheetRequests(0)
    , _restoringSettings(false)
    , _defaultAppearanceOutdated(false)
    , saveSettings(true)
    {
        
    }

    void initializeKnobsGeneral();
    void initializeKnobsThreading();
    void initializeKnobsRendering();
    void initializeKnobsGPU();
    void initializeKnobsProjectSetup();
    void initializeKnobsDocumentation();
    void initializeKnobsUserInterface();
    void initializeKnobsColorManagement();
    void initializeKnobsCaching();
    void initializeKnobsViewers();
    void initializeKnobsNodeGraph();
    void initializeKnobsPlugins();
    void initializeKnobsPython();
    void initializeKnobsAppearance();
    void initializeKnobsGuiColors();
    void initializeKnobsDopeSheetColors();
    void initializeKnobsNodeGraphColors();
    void initializeKnobsScriptEditorColors();

    bool tryLoadOpenColorIOConfig();

    std::string ensureUserDataDirectory();

    std::string getSettingsAbsoluteFilepath(const std::string& userDataDirectoryPath);

    void loadSettingsFromFileInternal(const SERIALIZATION_NAMESPACE::SettingsSerialization& serialization, int loadType);

    void setOpenGLRenderersInfo();

    void restoreNumThreads();

    void refreshCacheSize();

    void populateOpenGLRenderers(const std::list<OpenGLRendererInfo>& renderers);


};


Settings::Settings()
: KnobHolder( AppInstancePtr() ) // < Settings are process wide and do not belong to a single AppInstance
, _imp(new SettingsPrivate(this))
{
}

Settings::~Settings()
{
}


static QStringList
getDefaultOcioConfigPaths()
{
    QString binaryPath = QString::fromUtf8(appPTR->getApplicationBinaryDirPath().c_str());
    StrUtils::ensureLastPathSeparator(binaryPath);

#ifdef __NATRON_LINUX__
    QStringList ret;
    ret.push_back( QString::fromUtf8("/usr/share/OpenColorIO-Configs") );
    ret.push_back( QString( binaryPath + QString::fromUtf8("../share/OpenColorIO-Configs") ) );
    ret.push_back( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );

    return ret;
#elif defined(__NATRON_WIN32__)

    return QStringList( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );
#elif defined(__NATRON_OSX__)

    return QStringList( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );
#endif
}

bool
Settings::doesKnobChangeRequireRestart(const KnobIPtr& knob)
{
    std::set<KnobIWPtr>::iterator found = _imp->_knobsRequiringRestart.find(knob);
    if (found == _imp->_knobsRequiringRestart.end()) {
        return false;
    }
    return true;
}

bool
Settings::doesKnobChangeRequireOFXCacheClear(const KnobIPtr& knob)
{
    std::set<KnobIWPtr>::iterator found = _imp->_knobsRequiringOFXCacheClear.find(knob);
    if (found == _imp->_knobsRequiringOFXCacheClear.end()) {
        return false;
    }
    return true;
}

void
Settings::addKeybind(const std::string & grouping,
                     const std::string & id,
                     const std::string& label,
                     const std::string & description,
                     const KeyboardModifiers & modifiers,
                     Key symbol,
                     const KeyboardModifiers & modifiersMask)
{
    GroupShortcutsMap& groupMap = _imp->shortcuts[grouping];
    KeybindShortcut& kA = groupMap[id];

    kA.actionID = id;
    kA.actionLabel = label;
    kA.grouping = grouping;
    kA.description = description;
    kA.defaultModifiers = modifiers;
    kA.modifiers = modifiers;
    kA.defaultShortcut = symbol;
    kA.currentShortcut = symbol;
    kA.ignoreMask = modifiersMask;

    Q_EMIT shortcutsChanged();
    
} // addKeybind

void
Settings::removeKeybind(const std::string& grouping, const std::string& id)
{
    ApplicationShortcutsMap::iterator foundGroup = _imp->shortcuts.find(grouping);
    if (foundGroup == _imp->shortcuts.end()) {
        return;
    }
    GroupShortcutsMap::iterator foundShortcut = foundGroup->second.find(id);
    if (foundShortcut != foundGroup->second.end()) {
        foundGroup->second.erase(foundShortcut);
    }
}

bool
Settings::getShortcutKeybind(const std::string & grouping,
                             const std::string & id,
                             KeyboardModifiers* modifiers,
                             Key* symbol) const
{
    ApplicationShortcutsMap::iterator foundGroup = _imp->shortcuts.find(grouping);
    if (foundGroup == _imp->shortcuts.end()) {
        return false;
    }
    GroupShortcutsMap::iterator foundShortcut = foundGroup->second.find(id);
    if (foundShortcut == foundGroup->second.end()) {
        return false;
    }
    *modifiers = foundShortcut->second.modifiers;
    *symbol = foundShortcut->second.currentShortcut;
    return true;
}

void
Settings::setShortcutKeybind(const std::string & grouping,
                             const std::string & id,
                             const KeyboardModifiers & modifiers,
                             Key symbol)
{
    ApplicationShortcutsMap::iterator foundGroup = _imp->shortcuts.find(grouping);
    if (foundGroup == _imp->shortcuts.end()) {
        return ;
    }
    GroupShortcutsMap::iterator foundShortcut = foundGroup->second.find(id);
    if (foundShortcut == foundGroup->second.end()) {
        return ;
    }
    foundShortcut->second.modifiers = modifiers;
    foundShortcut->second.currentShortcut = symbol;
    foundShortcut->second.updateActionsShortcut();

    Q_EMIT shortcutsChanged();

}

void
Settings::addShortcutAction(const std::string & grouping, const std::string & id, KeybindListenerI* action)
{
    ApplicationShortcutsMap::iterator foundGroup = _imp->shortcuts.find(grouping);
    if (foundGroup == _imp->shortcuts.end()) {
        return;
    }
    GroupShortcutsMap::iterator foundShortcut = foundGroup->second.find(id);
    if (foundShortcut == foundGroup->second.end()) {
        return;
    }
    foundShortcut->second.actions.push_back(action);
}

void
Settings::removeShortcutAction(const std::string & grouping, const std::string & id, KeybindListenerI* action)
{
    ApplicationShortcutsMap::iterator foundGroup = _imp->shortcuts.find(grouping);
    if (foundGroup == _imp->shortcuts.end()) {
        return;
    }
    GroupShortcutsMap::iterator foundShortcut = foundGroup->second.find(id);
    if (foundShortcut == foundGroup->second.end()) {
        return;
    }
    std::list<KeybindListenerI*>::iterator found = std::find(foundShortcut->second.actions.begin(), foundShortcut->second.actions.end(), action);
    if (found != foundShortcut->second.actions.end()) {
        foundShortcut->second.actions.erase(found);
    }
}

const ApplicationShortcutsMap&
Settings::getAllShortcuts() const
{
    return _imp->shortcuts;
}

static bool
matchesModifers(const KeyboardModifiers & lhs,
                const KeyboardModifiers & rhs,
                const KeyboardModifiers& ignoreMask)
{
    bool hasMask = ignoreMask != eKeyboardModifierNone;

    return (!hasMask && lhs == rhs) ||
    (hasMask && (lhs & ~ignoreMask) == rhs);
}

static bool
matchesKey(Key lhs,
           Key rhs)
{
    if (lhs == rhs) {
        return true;
    }
    ///special case for the backspace and delete keys that mean the same thing generally
    else if ( ( (lhs == Key_BackSpace) && (rhs == Key_Delete) ) ||
             ( ( lhs == Key_Delete) && ( rhs == Key_BackSpace) ) ) {
        return true;
    }

    return false;
}


bool
Settings::matchesKeybind(const std::string & grouping,
                         const std::string & id,
                         const KeyboardModifiers & modifiers,
                         Key symbol) const
{
    ApplicationShortcutsMap::iterator foundGroup = _imp->shortcuts.find(grouping);
    if (foundGroup == _imp->shortcuts.end()) {
        return false;
    }
    GroupShortcutsMap::iterator foundShortcut = foundGroup->second.find(id);
    if (foundShortcut == foundGroup->second.end()) {
        return false;
    }


    // the following macro only tests the Control, Alt, and Shift modifiers, and discards the others
    KeyboardModifiers onlyCAS = modifiers & (eKeyboardModifierControl | eKeyboardModifierAlt | eKeyboardModifierShift);

    if ( matchesModifers(onlyCAS, foundShortcut->second.modifiers, foundShortcut->second.ignoreMask) ) {
        // modifiers are equal, now test symbol
        return matchesKey(symbol, foundShortcut->second.currentShortcut );
    }

    return false;
}

void
Settings::restoreDefaultShortcuts()
{
    for (ApplicationShortcutsMap::iterator it = _imp->shortcuts.begin(); it != _imp->shortcuts.end(); ++it) {
        for (GroupShortcutsMap::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            it2->second.modifiers = it2->second.defaultModifiers;
            it2->second.currentShortcut = it2->second.defaultShortcut;
            it2->second.updateActionsShortcut();
        }
    }
    Q_EMIT shortcutsChanged();
}

void
Settings::populateShortcuts()
{

    // General
    addKeybind(kShortcutGroupGlobal, kShortcutActionNewProject, kShortcutActionNewProjectLabel, kShortcutActionNewProjectHint, eKeyboardModifierControl, Key_N);
    addKeybind(kShortcutGroupGlobal, kShortcutActionOpenProject, kShortcutActionOpenProjectLabel, kShortcutActionOpenProjectHint, eKeyboardModifierControl, Key_O);
    addKeybind(kShortcutGroupGlobal, kShortcutActionSaveProject, kShortcutActionSaveProjectLabel, kShortcutActionSaveProjectHint, eKeyboardModifierControl, Key_S);
    addKeybind(kShortcutGroupGlobal, kShortcutActionSaveAsProject, kShortcutActionSaveAsProjectLabel, kShortcutActionSaveAsProjectHint, KeyboardModifiers(eKeyboardModifierControl | eKeyboardModifierShift), Key_S);
    addKeybind(kShortcutGroupGlobal, kShortcutActionCloseProject, kShortcutActionCloseProjectLabel, kShortcutActionCloseProjectHint, eKeyboardModifierControl, Key_W);
    addKeybind(kShortcutGroupGlobal, kShortcutActionPreferences, kShortcutActionPreferencesLabel, kShortcutActionPreferencesHint, eKeyboardModifierShift, Key_S);
    addKeybind(kShortcutGroupGlobal, kShortcutActionQuit, kShortcutActionQuitLabel, kShortcutActionQuitHint, eKeyboardModifierControl, Key_Q);

    addKeybind(kShortcutGroupGlobal, kShortcutActionSaveAndIncrVersion, kShortcutActionSaveAndIncrVersionLabel, kShortcutActionSaveAndIncrVersionHint, KeyboardModifiers(eKeyboardModifierControl | eKeyboardModifierShift |
                    eKeyboardModifierAlt), Key_S);
    addKeybind(kShortcutGroupGlobal, kShortcutActionReloadProject, kShortcutActionReloadProjectLabel, kShortcutActionReloadProjectHint, KeyboardModifiers(eKeyboardModifierControl | eKeyboardModifierShift), Key_R);
    addKeybind(kShortcutGroupGlobal, kShortcutActionShowAbout, kShortcutActionShowAboutLabel, kShortcutActionShowAboutHint, eKeyboardModifierNone, (Key)0);

    addKeybind(kShortcutGroupGlobal, kShortcutActionImportLayout, kShortcutActionImportLayoutLabel, kShortcutActionImportLayoutHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupGlobal, kShortcutActionExportLayout, kShortcutActionExportLayoutLabel, kShortcutActionExportLayoutHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupGlobal, kShortcutActionDefaultLayout, kShortcutActionDefaultLayoutLabel, kShortcutActionDefaultLayoutHint, eKeyboardModifierNone, (Key)0);

    addKeybind(kShortcutGroupGlobal, kShortcutActionProjectSettings, kShortcutActionProjectSettingsLabel, kShortcutActionProjectSettingsHint, eKeyboardModifierNone, Key_S);
    addKeybind(kShortcutGroupGlobal, kShortcutActionShowErrorLog, kShortcutActionShowErrorLogLabel, kShortcutActionShowErrorLogHint, eKeyboardModifierNone, (Key)0);

    addKeybind(kShortcutGroupGlobal, kShortcutActionNewViewer, kShortcutActionNewViewerLabel, kShortcutActionNewViewerHint, eKeyboardModifierControl, Key_I);
    addKeybind(kShortcutGroupGlobal, kShortcutActionFullscreen, kShortcutActionFullscreenLabel, kShortcutActionFullscreenHint, eKeyboardModifierAlt, Key_S); // as in Nuke
    //addKeybind(kShortcutGroupGlobal, kShortcutActionFullscreen, kShortcutActionFullscreen, eKeyboardModifierControl | eKeyboardModifierAlt, Key_F);

    addKeybind(kShortcutGroupGlobal, kShortcutActionClearPluginsLoadCache, kShortcutActionClearPluginsLoadCacheLabel, kShortcutActionClearPluginsLoadCacheHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupGlobal, kShortcutActionClearCache, kShortcutActionClearCacheLabel, kShortcutActionClearCacheHint, KeyboardModifiers(eKeyboardModifierControl | eKeyboardModifierShift), Key_K);
    addKeybind(kShortcutGroupGlobal, kShortcutActionRenderSelected, kShortcutActionRenderSelectedLabel, kShortcutActionRenderSelectedHint, eKeyboardModifierNone, Key_F7);

    addKeybind(kShortcutGroupGlobal, kShortcutActionRenderAll, kShortcutActionRenderAllLabel, kShortcutActionRenderAllHint, eKeyboardModifierNone, Key_F5);

    addKeybind(kShortcutGroupGlobal, kShortcutActionEnableRenderStats, kShortcutActionEnableRenderStatsLabel, kShortcutActionEnableRenderStatsHint, eKeyboardModifierNone, Key_F2);

    // Note: keys 0-1 are handled by Gui::handleNativeKeys(), and should thus work even on international keyboards
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput1, kShortcutActionConnectViewerToInput1Label, kShortcutActionConnectViewerToInput1Hint, eKeyboardModifierNone, Key_1);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput2, kShortcutActionConnectViewerToInput2Label, kShortcutActionConnectViewerToInput2Hint, eKeyboardModifierNone, Key_2);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput3, kShortcutActionConnectViewerToInput3Label, kShortcutActionConnectViewerToInput3Hint, eKeyboardModifierNone, Key_3);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput4, kShortcutActionConnectViewerToInput4Label, kShortcutActionConnectViewerToInput4Hint, eKeyboardModifierNone, Key_4);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput5, kShortcutActionConnectViewerToInput5Label, kShortcutActionConnectViewerToInput5Hint, eKeyboardModifierNone, Key_5);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput6, kShortcutActionConnectViewerToInput6Label, kShortcutActionConnectViewerToInput6Hint, eKeyboardModifierNone, Key_6);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput7, kShortcutActionConnectViewerToInput7Label, kShortcutActionConnectViewerToInput7Hint, eKeyboardModifierNone, Key_7);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput8, kShortcutActionConnectViewerToInput8Label, kShortcutActionConnectViewerToInput8Hint, eKeyboardModifierNone, Key_8);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput9, kShortcutActionConnectViewerToInput9Label, kShortcutActionConnectViewerToInput9Hint, eKeyboardModifierNone, Key_9);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput10, kShortcutActionConnectViewerToInput10Label, kShortcutActionConnectViewerToInput10Hint, eKeyboardModifierNone, Key_0);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput1, kShortcutActionConnectViewerBToInput1Label, kShortcutActionConnectViewerBToInput1Hint, eKeyboardModifierShift, Key_1);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput2, kShortcutActionConnectViewerBToInput2Label, kShortcutActionConnectViewerBToInput2Hint, eKeyboardModifierShift, Key_2);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput3, kShortcutActionConnectViewerBToInput3Label, kShortcutActionConnectViewerBToInput3Hint, eKeyboardModifierShift, Key_3);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput4, kShortcutActionConnectViewerBToInput4Label, kShortcutActionConnectViewerBToInput4Hint, eKeyboardModifierShift, Key_4);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput5, kShortcutActionConnectViewerBToInput5Label, kShortcutActionConnectViewerBToInput5Hint, eKeyboardModifierShift, Key_5);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput6, kShortcutActionConnectViewerBToInput6Label, kShortcutActionConnectViewerBToInput6Hint, eKeyboardModifierShift, Key_6);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput7, kShortcutActionConnectViewerBToInput7Label, kShortcutActionConnectViewerBToInput7Hint, eKeyboardModifierShift, Key_7);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput8, kShortcutActionConnectViewerBToInput8Label, kShortcutActionConnectViewerBToInput8Hint, eKeyboardModifierShift, Key_8);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput9, kShortcutActionConnectViewerBToInput9Label, kShortcutActionConnectViewerBToInput9Hint, eKeyboardModifierShift, Key_9);
    addKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput10, kShortcutActionConnectViewerBToInput10Label, kShortcutActionConnectViewerBToInput10Hint, eKeyboardModifierShift, Key_0);

    addKeybind(kShortcutGroupGlobal, kShortcutActionShowPaneFullScreen, kShortcutActionShowPaneFullScreenLabel, kShortcutActionShowPaneFullScreenHint, eKeyboardModifierNone, Key_space);
    addKeybind(kShortcutGroupGlobal, kShortcutActionNextTab, kShortcutActionNextTabLabel, kShortcutActionNextTabHint, eKeyboardModifierControl, Key_T);
    addKeybind(kShortcutGroupGlobal, kShortcutActionPrevTab, kShortcutActionPrevTabLabel, kShortcutActionPrevTabHint, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierControl), Key_T);
    addKeybind(kShortcutGroupGlobal, kShortcutActionCloseTab, kShortcutActionCloseTabLabel, kShortcutActionCloseTabHint, eKeyboardModifierShift, Key_Escape);


    ///Nodegraph

    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphRearrangeNodes, kShortcutActionGraphRearrangeNodesLabel, kShortcutActionGraphRearrangeNodesHint, eKeyboardModifierNone, Key_L);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphDisableNodes, kShortcutActionGraphDisableNodesLabel, kShortcutActionGraphDisableNodesHint, eKeyboardModifierNone, Key_D);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphRemoveNodes, kShortcutActionGraphRemoveNodesLabel, kShortcutActionGraphRemoveNodesHint, eKeyboardModifierNone, Key_BackSpace);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphShowExpressions, kShortcutActionGraphShowExpressionsLabel, kShortcutActionGraphShowExpressionsHint, eKeyboardModifierShift, Key_E);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphSelectUp, kShortcutActionGraphSelectUpLabel, kShortcutActionGraphSelectUpHint, eKeyboardModifierShift, Key_Up);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphSelectDown, kShortcutActionGraphSelectDownLabel, kShortcutActionGraphSelectDownHint, eKeyboardModifierShift, Key_Down);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphSelectAll, kShortcutActionGraphSelectAllLabel, kShortcutActionGraphSelectAllHint, eKeyboardModifierControl, Key_A);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphSelectAllVisible, kShortcutActionGraphSelectAllVisibleLabel, kShortcutActionGraphSelectAllVisibleHint, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierControl), Key_A);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphAutoHideInputs, kShortcutActionGraphAutoHideInputsLabel, kShortcutActionGraphAutoHideInputsHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphHideInputs, kShortcutActionGraphHideInputsLabel, kShortcutActionGraphHideInputsHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphSwitchInputs, kShortcutActionGraphSwitchInputsLabel, kShortcutActionGraphSwitchInputsHint, eKeyboardModifierShift, Key_X);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphCopy, kShortcutActionGraphCopyLabel, kShortcutActionGraphCopyHint, eKeyboardModifierControl, Key_C);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphPaste, kShortcutActionGraphPasteLabel, kShortcutActionGraphPasteHint, eKeyboardModifierControl, Key_V);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphCut, kShortcutActionGraphCutLabel, kShortcutActionGraphCutHint, eKeyboardModifierControl, Key_X);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphClone, kShortcutActionGraphCloneLabel, kShortcutActionGraphCloneHint, eKeyboardModifierAlt, Key_K);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphDeclone, kShortcutActionGraphDecloneLabel, kShortcutActionGraphDecloneHint, KeyboardModifiers(eKeyboardModifierAlt | eKeyboardModifierShift), Key_K);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphDuplicate, kShortcutActionGraphDuplicateLabel, kShortcutActionGraphDuplicateHint, eKeyboardModifierAlt, Key_C);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphForcePreview, kShortcutActionGraphForcePreviewLabel, kShortcutActionGraphForcePreviewHint, eKeyboardModifierShift, Key_P);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphTogglePreview, kShortcutActionGraphTogglePreviewLabel, kShortcutActionGraphToggleAutoPreviewHint, eKeyboardModifierAlt, Key_P);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphToggleAutoPreview, kShortcutActionGraphToggleAutoPreviewLabel, kShortcutActionGraphToggleAutoPreviewHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphToggleAutoTurbo, kShortcutActionGraphToggleAutoTurboLabel, kShortcutActionGraphToggleAutoTurboHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphFrameNodes, kShortcutActionGraphFrameNodesLabel, kShortcutActionGraphFrameNodesHint, eKeyboardModifierNone, Key_F);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphShowCacheSize, kShortcutActionGraphShowCacheSizeLabel, kShortcutActionGraphShowCacheSizeHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphFindNode, kShortcutActionGraphFindNodeLabel, kShortcutActionGraphFindNodeHint, eKeyboardModifierControl, Key_F);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphRenameNode, kShortcutActionGraphRenameNodeLabel, kShortcutActionGraphRenameNodeHint, eKeyboardModifierNone, Key_N);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphExtractNode, kShortcutActionGraphExtractNodeLabel, kShortcutActionGraphExtractNodeHint, KeyboardModifiers(eKeyboardModifierControl | eKeyboardModifierShift),
                    Key_X);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphMakeGroup, kShortcutActionGraphMakeGroupLabel, kShortcutActionGraphMakeGroupHint, eKeyboardModifierControl,
                    Key_G);
    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphExpandGroup, kShortcutActionGraphExpandGroupLabel, kShortcutActionGraphExpandGroupHint, KeyboardModifiers(eKeyboardModifierControl | eKeyboardModifierAlt),
                    Key_G);

    addKeybind(kShortcutGroupNodegraph, kShortcutActionGraphOpenNodePanel, kShortcutActionGraphOpenNodePanelLabel, kShortcutActionGraphOpenNodePanelHint, eKeyboardModifierNone, Key_Return);


    // Animation Module
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleRemoveKeys, kShortcutActionAnimationModuleRemoveKeysLabel, kShortcutActionAnimationModuleRemoveKeysHint, eKeyboardModifierNone, Key_BackSpace);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleConstant, kShortcutActionAnimationModuleConstantLabel, kShortcutActionAnimationModuleConstantHint, eKeyboardModifierNone, Key_K);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSmooth, kShortcutActionAnimationModuleSmoothLabel, kShortcutActionAnimationModuleSmoothHint, eKeyboardModifierNone, Key_Z);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleLinear, kShortcutActionAnimationModuleLinearLabel, kShortcutActionAnimationModuleLinearHint, eKeyboardModifierNone, Key_L);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCatmullrom, kShortcutActionAnimationModuleCatmullromLabel, kShortcutActionAnimationModuleCatmullromHint, eKeyboardModifierNone, Key_R);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCubic, kShortcutActionAnimationModuleCubicLabel, kShortcutActionAnimationModuleCubicHint, eKeyboardModifierNone, Key_C);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleHorizontal, kShortcutActionAnimationModuleHorizontalLabel, kShortcutActionAnimationModuleHorizontalHint, eKeyboardModifierNone, Key_H);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleBreak, kShortcutActionAnimationModuleBreakLabel, kShortcutActionAnimationModuleBreakHint, eKeyboardModifierNone, Key_X);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSelectAll, kShortcutActionAnimationModuleSelectAllLabel, kShortcutActionAnimationModuleSelectAllHint, eKeyboardModifierControl, Key_A);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenterAll, kShortcutActionAnimationModuleCenterAllLabel, kShortcutActionAnimationModuleCenterAllHint
                    , eKeyboardModifierNone, Key_A);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenter, kShortcutActionAnimationModuleCenterLabel, kShortcutActionAnimationModuleCenterHint
                    , eKeyboardModifierNone, Key_F);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCopy, kShortcutActionAnimationModuleCopyLabel, kShortcutActionAnimationModuleCopyHint, eKeyboardModifierControl, Key_C);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModulePasteKeyframes, kShortcutActionAnimationModulePasteKeyframesLabel, kShortcutActionAnimationModulePasteKeyframesHint, eKeyboardModifierControl, Key_V);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModulePasteKeyframesAbsolute, kShortcutActionAnimationModulePasteKeyframesAbsoluteLabel, kShortcutActionAnimationModulePasteKeyframesAbsoluteHint, KeyboardModifiers(eKeyboardModifierControl | eKeyboardModifierShift), Key_V);

    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleStackView, kShortcutActionAnimationModuleStackViewLabel, kShortcutActionAnimationModuleStackViewHint, eKeyboardModifierNone, Key_S);
    addKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleShowOnlyAnimated, kShortcutActionAnimationModuleShowOnlyAnimatedLabel, kShortcutActionAnimationModuleShowOnlyAnimatedHint, eKeyboardModifierNone, (Key)0);

    //Script editor
    addKeybind(kShortcutGroupScriptEditor, kShortcutActionScriptEditorPrevScript, kShortcutActionScriptEditorPrevScriptLabel, kShortcutActionScriptEditorPrevScriptHint, eKeyboardModifierControl, Key_bracketleft, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierAlt));
    addKeybind(kShortcutGroupScriptEditor, kShortcutActionScriptEditorNextScript, kShortcutActionScriptEditorNextScriptLabel, kShortcutActionScriptEditorNextScriptHint, eKeyboardModifierControl, Key_bracketright, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierAlt));
    addKeybind(kShortcutGroupScriptEditor, kShortcutActionScriptEditorClearHistory, kShortcutActionScriptEditorClearHistoryLabel, kShortcutActionScriptEditorClearHistoryHint, eKeyboardModifierNone, (Key)0);
    addKeybind(kShortcutGroupScriptEditor, kShortcutActionScriptExecScript, kShortcutActionScriptExecScriptLabel, kShortcutActionScriptExecScriptHint, eKeyboardModifierControl, Key_Return);
    addKeybind(kShortcutGroupScriptEditor, kShortcutActionScriptClearOutput, kShortcutActionScriptClearOutputLabel, kShortcutActionScriptClearOutputHint, eKeyboardModifierControl, Key_BackSpace);
    addKeybind(kShortcutGroupScriptEditor, kShortcutActionScriptShowOutput, kShortcutActionScriptShowOutputLabel, kShortcutActionScriptShowOutputHint, eKeyboardModifierNone, (Key)0);
} // populateShortcuts

void
Settings::initializeKnobs()
{
    _imp->initializeKnobsGeneral();
    _imp->initializeKnobsThreading();
    _imp->initializeKnobsRendering();
    _imp->initializeKnobsGPU();
    _imp->initializeKnobsProjectSetup();
    _imp->initializeKnobsDocumentation();
    _imp->initializeKnobsUserInterface();
    _imp->initializeKnobsColorManagement();
    _imp->initializeKnobsCaching();
    _imp->initializeKnobsViewers();
    _imp->initializeKnobsNodeGraph();
    _imp->initializeKnobsPlugins();
    _imp->initializeKnobsPython();
    _imp->initializeKnobsAppearance();
    _imp->initializeKnobsGuiColors();
    _imp->initializeKnobsDopeSheetColors();
    _imp->initializeKnobsNodeGraphColors();
    _imp->initializeKnobsScriptEditorColors();

    populateShortcuts();

    const std::list<OpenGLRendererInfo>& renderers = appPTR->getOpenGLRenderers();
    _imp->populateOpenGLRenderers(renderers);
}

void
SettingsPrivate::initializeKnobsGeneral()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _generalTab = _publicInterface->createKnob<KnobPage>("generalPage");
    _generalTab->setLabel(tr("General"));

    _checkForUpdates = _publicInterface->createKnob<KnobBool>("checkForUpdates");
    _checkForUpdates->setLabel(tr("Always check for updates on start-up"));
    _checkForUpdates->setHintToolTip( tr("When checked, %1 will check for new updates on start-up of the application.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) +
                                      QLatin1Char('\n') +
                                      tr("Changing this requires a restart of the application to take effect.") );
    _knobsRequiringRestart.insert(_checkForUpdates);
    _generalTab->addKnob(_checkForUpdates);

#ifdef NATRON_USE_BREAKPAD
    _enableCrashReports = _publicInterface->createKnob<KnobBool>("enableCrashReports");
    _enableCrashReports->setLabel(tr("Enable crash reporting"));
    _enableCrashReports->setHintToolTip( tr("When checked, if %1 crashes a window will pop-up asking you "
                                            "whether you want to upload the crash dump to the developers or not. "
                                            "This can help them track down the bug.\n"
                                            "If you need to turn the crash reporting system off, uncheck this.\n"
                                            "Note that when using the application in command-line mode, if crash reports are "
                                            "enabled, they will be automatically uploaded.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) +
                                          QLatin1Char('\n') +
                                          tr("Changing this requires a restart of the application to take effect.") );
    _enableCrashReports->setDefaultValue(true);
    _enableCrashReports->setAddNewLine(false);
    _knobsRequiringRestart.insert(_enableCrashReports);
    _generalTab->addKnob(_enableCrashReports);

    _testCrashReportButton = _publicInterface->createKnob<KnobButton>("testCrashReporting");
    _testCrashReportButton->setLabel(tr("Test Crash Reporting"));
    _testCrashReportButton->setHintToolTip( tr("This button is for developers only to test whether the crash reporting system "
                                               "works correctly. Do not use this.") );
    _generalTab->addKnob(_testCrashReportButton);
#endif

    _autoSaveDelay = _publicInterface->createKnob<KnobInt>("autoSaveDelay");
    _autoSaveDelay->setLabel(tr("Auto-save trigger delay"));
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setRange(0, 60);
    _autoSaveDelay->setHintToolTip( tr("The number of seconds after an event that %1 should wait before "
                                       " auto-saving. Note that if a render is in progress, %1 will "
                                       " wait until it is done to actually auto-save.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_autoSaveDelay);


    _autoSaveUnSavedProjects = _publicInterface->createKnob<KnobBool>("autoSaveUnSavedProjects");
    _autoSaveUnSavedProjects->setLabel(tr("Enable Auto-save for unsaved projects"));
    _autoSaveUnSavedProjects->setHintToolTip( tr("When activated %1 will auto-save projects that have never been "
                                                 "saved and will prompt you on startup if an auto-save of that unsaved project was found. "
                                                 "Disabling this will no longer save un-saved project.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _autoSaveUnSavedProjects->setDefaultValue(false);
    _generalTab->addKnob(_autoSaveUnSavedProjects);

    _fileDialogSavedPaths = _publicInterface->createKnob<KnobPath>("fileDialogPaths");
    _fileDialogSavedPaths->setLabel(tr("File Dialog Paths"));
    _fileDialogSavedPaths->setHintToolTip(tr("These are the paths to directories visible in the favorite view of the file dialog"));
    _fileDialogSavedPaths->setSecret(true);
    _fileDialogSavedPaths->setAsStringList(true);
    _generalTab->addKnob(_fileDialogSavedPaths);


    _saveSafetyMode = _publicInterface->createKnob<KnobBool>("fullRecoverySave");
    _saveSafetyMode->setLabel(tr("Full Recovery Save"));
    _saveSafetyMode->setHintToolTip(tr("The save safety is used when saving projects. When checked all default values of parameters will be saved in the project, even if they did not change. This is useful "
                                       "in the case a default value in a plug-in was changed by its developers, to ensure that the value is still the same when loading a project."
                                       "By default this should not be needed as default values change are very rare. In a scenario where a project cannot be recovered in a newer version because "
                                       "the default values for a node have changed, just save your project in an older version of %1 with this parameter checked so that it reloads correctly in the newer version.\n"
                                       "Note that checking this parameter can make project files significantly larger.").arg(QString::fromUtf8(NATRON_APPLICATION_NAME)));
    _generalTab->addKnob(_saveSafetyMode);


    _hostName = _publicInterface->createKnob<KnobChoice>("pluginHostName");
    _hostName->setLabel(tr("Appear to plug-ins as"));
    _knobsRequiringRestart.insert(_hostName);
    _knobsRequiringOFXCacheClear.insert(_hostName);
    _hostName->setHintToolTip( tr("%1 will appear with the name of the selected application to the OpenFX plug-ins. "
                                  "Changing it to the name of another application can help loading plugins which "
                                  "restrict their usage to specific OpenFX host(s). "
                                  "If a Host is not listed here, use the \"Custom\" entry to enter a custom host name.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) +
                               QLatin1Char('\n') +
                               tr("Changing this requires a restart of the application to take effect.")  );
    _knownHostNames.clear();
    std::vector<ChoiceOption> visibleHostEntries;
    assert(visibleHostEntries.size() == (int)eKnownHostNameNatron);
    visibleHostEntries.push_back(ChoiceOption(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME, NATRON_APPLICATION_NAME, ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameNuke);
    visibleHostEntries.push_back(ChoiceOption("uk.co.thefoundry.nuke", "Nuke", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameFusion);
    visibleHostEntries.push_back(ChoiceOption("com.eyeonline.Fusion", "Fusion", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameCatalyst);
    visibleHostEntries.push_back(ChoiceOption("com.sony.Catalyst.Edit", "Sony Catalyst Edit", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameVegas);
    visibleHostEntries.push_back(ChoiceOption("com.sonycreativesoftware.vegas", "Sony Vegas", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameToxik);
    visibleHostEntries.push_back(ChoiceOption("Autodesk Toxik", "Toxik", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameScratch);
    visibleHostEntries.push_back(ChoiceOption("Assimilator", "Scratch", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameDustBuster);
    visibleHostEntries.push_back(ChoiceOption("Dustbuster", "DustBuster", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameResolve);
    visibleHostEntries.push_back(ChoiceOption("DaVinciResolve", "Da Vinci Resolve", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameResolveLite);
    visibleHostEntries.push_back(ChoiceOption("DaVinciResolveLite", "Da Vinci Resolve Lite", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameMistika);
    visibleHostEntries.push_back(ChoiceOption("Mistika", "SGO Mistika", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNamePablo);
    visibleHostEntries.push_back(ChoiceOption("com.quantel.genq", "Quantel Pablo Rio", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameMotionStudio);
    visibleHostEntries.push_back(ChoiceOption("com.idtvision.MotionStudio", "IDT Motion Studio", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameShake);
    visibleHostEntries.push_back(ChoiceOption("com.apple.shake", "Shake", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameBaselight);
    visibleHostEntries.push_back(ChoiceOption("Baselight", "Baselight", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameFrameCycler);
    visibleHostEntries.push_back(ChoiceOption("IRIDAS Framecycler", "FrameCycler", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameNucoda);
    visibleHostEntries.push_back(ChoiceOption("Nucoda", "Nucoda Film Master", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameAvidDS);
    visibleHostEntries.push_back(ChoiceOption("DS OFX HOST", "Avid DS", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameDX);
    visibleHostEntries.push_back(ChoiceOption("com.chinadigitalvideo.dx", "China Digital Video DX", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameTitlerPro);
    visibleHostEntries.push_back(ChoiceOption("com.newblue.titlerpro", "NewBlueFX Titler Pro", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameNewBlueOFXBridge);
    visibleHostEntries.push_back(ChoiceOption("com.newblue.ofxbridge", "NewBlueFX OFX Bridge", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameRamen);
    visibleHostEntries.push_back(ChoiceOption("Ramen", "Ramen", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameTuttleOfx);
    visibleHostEntries.push_back(ChoiceOption("TuttleOfx", "TuttleOFX", ""));

    _knownHostNames = visibleHostEntries;

    visibleHostEntries.push_back(ChoiceOption(NATRON_CUSTOM_HOST_NAME_ENTRY, "Custom host name", ""));

    _hostName->populateChoices(visibleHostEntries);
    _hostName->setAddNewLine(false);
    _generalTab->addKnob(_hostName);

    _customHostName = _publicInterface->createKnob<KnobString>("customHostName");
    _customHostName->setLabel(tr("Custom Host name"));
    _knobsRequiringRestart.insert(_customHostName);
    _knobsRequiringOFXCacheClear.insert(_customHostName);
    _customHostName->setHintToolTip( tr("This is the name of the OpenFX host (application) as it appears to the OpenFX plugins. "
                                        "Changing it to the name of another application can help loading some plugins which "
                                        "restrict their usage to specific OpenFX hosts. You shoud leave "
                                        "this to its default value, unless a specific plugin refuses to load or run. "
                                        "The default host name is: \n%1").arg( QString::fromUtf8(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME) ) +
                                     QLatin1Char('\n') +
                                     tr("Changing this requires a restart of the application to take effect.")  );
    _customHostName->setSecret(true);
    _generalTab->addKnob(_customHostName);
} // Settings::initializeKnobsGeneral

void
SettingsPrivate::initializeKnobsThreading()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _threadingPage = _publicInterface->createKnob<KnobPage>("threadingPage");
    _threadingPage->setLabel(tr("Threading"));

    _numberOfThreads = _publicInterface->createKnob<KnobInt>("numRenderThreads");
    _numberOfThreads->setLabel(tr("Number of render threads (0=\"guess\")"));

    int hwThreadsCount = appPTR->getHardwareIdealThreadCount();
    QString numberOfThreadsToolTip = tr("Controls how many threads %1 should use to render. \n"
                                        "0: Guess the thread count from the number of cores. The ideal threads count for this hardware is %2.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( hwThreadsCount );
    _numberOfThreads->setHintToolTip( numberOfThreadsToolTip.toStdString() );
    _numberOfThreads->disableSlider();
#ifdef DEBUG
    // -1: Disable multithreading totally (useful for debugging)
    _numberOfThreads->setRange(-1, hwThreadsCount);
#else
    _numberOfThreads->setRange(0, hwThreadsCount);
#endif
    _numberOfThreads->setDisplayRange(0, hwThreadsCount);
    _numberOfThreads->setDefaultValue(0);
    _threadingPage->addKnob(_numberOfThreads);


    _renderInSeparateProcess = _publicInterface->createKnob<KnobBool>("renderNewProcess");
    _renderInSeparateProcess->setLabel(tr("Render in a separate process"));
    _renderInSeparateProcess->setHintToolTip( tr("If checked, %1 will render frames to disk in "
                                                 "a separate process so that if the main application crashes, the render goes on.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _threadingPage->addKnob(_renderInSeparateProcess);

    _queueRenders = _publicInterface->createKnob<KnobBool>("queueRenders");
    _queueRenders->setLabel(tr("Append new renders to queue"));
    _queueRenders->setHintToolTip( tr("When checked, renders will be queued in the Progress Panel and will start only when all "
                                      "other prior tasks are done.") );
    _threadingPage->addKnob(_queueRenders);
} // Settings::initializeKnobsThreading

void
SettingsPrivate::initializeKnobsRendering()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _renderingPage = _publicInterface->createKnob<KnobPage>("renderingPage");
    _renderingPage->setLabel(tr("Rendering"));

    _convertNaNValues = _publicInterface->createKnob<KnobBool>("convertNaNs");
    _convertNaNValues->setLabel(tr("Convert NaN values"));
    _convertNaNValues->setHintToolTip( tr("When activated, any pixel that is a Not-a-Number will be converted to 1 to avoid potential crashes from "
                                          "downstream nodes. These values can be produced by faulty plug-ins when they use wrong arithmetic such as "
                                          "division by zero. Disabling this option will keep the NaN(s) in the buffers: this may lead to an "
                                          "undefined behavior.") );
    _convertNaNValues->setDefaultValue(true);
    _renderingPage->addKnob(_convertNaNValues);

    _activateRGBSupport = _publicInterface->createKnob<KnobBool>("rgbSupport");
    _activateRGBSupport->setLabel(tr("RGB components support"));
    _activateRGBSupport->setHintToolTip( tr("When checked %1 is able to process images with only RGB components "
                                            "(support for images with RGBA and Alpha components is always enabled). "
                                            "Un-checking this option may prevent plugins that do not well support RGB components from crashing %1. "
                                            "Changing this option requires a restart of the application.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateRGBSupport->setDefaultValue(true);
    _renderingPage->addKnob(_activateRGBSupport);


    _activateTransformConcatenationSupport = _publicInterface->createKnob<KnobBool>("transformCatSupport");
    _activateTransformConcatenationSupport->setLabel(tr("Transforms concatenation support"));
    _activateTransformConcatenationSupport->setHintToolTip( tr("When checked %1 is able to concatenate transform effects "
                                                               "when they are chained in the compositing tree. This yields better results and faster "
                                                               "render times because the image is only filtered once instead of as many times as there are "
                                                               "transformations.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateTransformConcatenationSupport->setDefaultValue(true);
    _renderingPage->addKnob(_activateTransformConcatenationSupport);
}

void
SettingsPrivate::populateOpenGLRenderers(const std::list<OpenGLRendererInfo>& renderers)
{
    if ( renderers.empty() ) {
        _availableOpenGLRenderers->setSecret(true);
        _nOpenGLContexts->setSecret(true);
        _enableOpenGL->setSecret(true);
    } else {

        _nOpenGLContexts->setSecret(false);
        _enableOpenGL->setSecret(false);

        std::vector<ChoiceOption> entries( renderers.size() );
        int i = 0;
        for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
            std::string option = it->vendorName + ' ' + it->rendererName + ' ' + it->glVersionString;
            entries[i].id = option;
        }
        _availableOpenGLRenderers->populateChoices(entries);
        _availableOpenGLRenderers->setSecret(renderers.size() == 1);
    }

#ifdef HAVE_OSMESA
#ifdef OSMESA_GALLIUM_DRIVER
    std::vector<ChoiceOption> mesaDrivers;
    mesaDrivers.push_back(ChoiceOption("softpipe", "",""));
    mesaDrivers.push_back(ChoiceOption("llvmpipe", "",""));
    _osmesaRenderers->populateChoices(mesaDrivers);
    _osmesaRenderers->setSecret(false);
#else
    _osmesaRenderers->setSecret(false);
#endif
#else
    _osmesaRenderers->setSecret(true);
#endif
}

GLRendererID
Settings::getOpenGLCPUDriver() const
{
    GLRendererID ret;
    if (!_imp->_osmesaRenderers->getIsSecret()) {
        ret.renderID =  _imp->_osmesaRenderers->getValue();
    } else {
        ret.renderID = 0;

    }
    return ret;
}

bool
Settings::isOpenGLRenderingEnabled() const
{
    if (_imp->_enableOpenGL->getIsSecret()) {
        return false;
    }
    EnableOpenGLEnum enableOpenGL = (EnableOpenGLEnum)_imp->_enableOpenGL->getValue();
    return enableOpenGL == eEnableOpenGLEnabled || (enableOpenGL == eEnableOpenGLDisabledIfBackground && !appPTR->isBackground());
}

int
Settings::getMaxOpenGLContexts() const
{
    return _imp->_nOpenGLContexts->getValue();
}

GLRendererID
Settings::getActiveOpenGLRendererID() const
{
    if ( _imp->_availableOpenGLRenderers->getIsSecret() ) {
        // We were not able to detect multiple renderers, use default
        return GLRendererID();
    }
    int activeIndex = _imp->_availableOpenGLRenderers->getValue();
    const std::list<OpenGLRendererInfo>& renderers = appPTR->getOpenGLRenderers();
    if ( (activeIndex < 0) || ( activeIndex >= (int)renderers.size() ) ) {
        // Invalid index
        return GLRendererID();
    }
    int i = 0;
    for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
        if (i == activeIndex) {
            return it->rendererID;
        }
    }

    return GLRendererID();
}

void
SettingsPrivate::initializeKnobsGPU()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _gpuPage = _publicInterface->createKnob<KnobPage>("gpuPage");
    _gpuPage->setLabel(tr("GPU Rendering"));
    _openglRendererString = _publicInterface->createKnob<KnobString>("activeOpenGLRenderer");

    _openglRendererString->setLabel(tr("Active OpenGL renderer"));
    _openglRendererString->setHintToolTip( tr("The currently active OpenGL renderer") );
    _openglRendererString->setAsLabel();
    _gpuPage->addKnob(_openglRendererString);

    _availableOpenGLRenderers = _publicInterface->createKnob<KnobChoice>("chooseOpenGLRenderer");
    _availableOpenGLRenderers->setLabel(tr("OpenGL renderer"));
    _availableOpenGLRenderers->setHintToolTip( tr("The renderer used to perform OpenGL rendering.") +
                                               QLatin1Char('\n') +
                                               tr("Changing this requires a restart of the application to take effect.")  );
    _knobsRequiringRestart.insert(_availableOpenGLRenderers);
    _gpuPage->addKnob(_availableOpenGLRenderers);

    _osmesaRenderers = _publicInterface->createKnob<KnobChoice>("cpuOpenGLRenderer");
    _osmesaRenderers->setLabel(tr("CPU OpenGL renderer"));
    _knobsRequiringRestart.insert(_osmesaRenderers);
    _osmesaRenderers->setHintToolTip( tr("Internally, %1 can render OpenGL plug-ins on the CPU by using the OSMesa open-source library. "
                                         "You may select which driver OSMesa uses to perform it's CPU rendering. "
                                         "llvm-pipe is more efficient but may contain some bugs.").arg(QString::fromUtf8(NATRON_APPLICATION_NAME)) +
                                      QLatin1Char('\n') +
                                      tr("Changing this requires a restart of the application to take effect.") );
    _osmesaRenderers->setDefaultValue(defaultMesaDriver);
    _gpuPage->addKnob(_osmesaRenderers);


    _nOpenGLContexts = _publicInterface->createKnob<KnobInt>("maxOpenGLContexts");
    _nOpenGLContexts->setLabel(tr("No. of OpenGL Contexts"));
    _nOpenGLContexts->setRange(1, 8);
    _nOpenGLContexts->setDisplayRange(1, 8);;
    _nOpenGLContexts->setHintToolTip( tr("The number of OpenGL contexts created to perform OpenGL rendering. Each OpenGL context can be attached to a CPU thread, allowing for more frames to be rendered simultaneously. Increasing this value may increase performances for graphs with mixed CPU/GPU nodes but can drastically reduce performances if too many OpenGL contexts are active at once.") );
    _nOpenGLContexts->setDefaultValue(2);
    _gpuPage->addKnob(_nOpenGLContexts);


    _enableOpenGL = _publicInterface->createKnob<KnobChoice>("enableOpenGLRendering");
    _enableOpenGL->setLabel(tr("OpenGL Rendering"));
    {
        std::vector<ChoiceOption> entries;
        assert(entries.size() == (int)Settings::eEnableOpenGLEnabled);
        entries.push_back(ChoiceOption("enabled",
                                       tr("Enabled").toStdString(),
                                       tr("If a plug-in support GPU rendering, prefer rendering using the GPU if possible.").toStdString()));
        assert(entries.size() == (int)Settings::eEnableOpenGLDisabled);
        entries.push_back(ChoiceOption("disabled",
                                       tr("Disabled").toStdString(),
                                       tr("Disable GPU rendering for all plug-ins.").toStdString()));
        assert(entries.size() == (int)Settings::eEnableOpenGLDisabledIfBackground);
        entries.push_back(ChoiceOption("foreground",
                                       tr("Disabled If Background").toStdString(),
                                       tr("Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.").toStdString()));
        _enableOpenGL->populateChoices(entries);
    }
    _enableOpenGL->setHintToolTip( tr("Select whether to activate OpenGL rendering or not. If disabled, even though a Project enable GPU rendering, it will not be activated.") );
#if NATRON_VERSION_MAJOR < 2 || (NATRON_VERSION_MAJOR == 2 && NATRON_VERSION_MINOR < 2)
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabled);
#else
#  if NATRON_VERSION_MAJOR >= 3
    _enableOpenGL->setDefaultValue((int)Settings::eEnableOpenGLEnabled);
#  else
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabledIfBackground);
#  endif
#endif
    _gpuPage->addKnob(_enableOpenGL);

}

void
SettingsPrivate::initializeKnobsProjectSetup()
{

    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _projectsPage = _publicInterface->createKnob<KnobPage>("projectSetupPage");
    _projectsPage->setLabel(tr("Project Setup"));

    _firstReadSetProjectFormat = _publicInterface->createKnob<KnobBool>("autoProjectFormat");
    _firstReadSetProjectFormat->setLabel(tr("First image read set project format"));
    _firstReadSetProjectFormat->setHintToolTip( tr("If checked, the project size is set to this of the first image or video read within the project.") );
    _firstReadSetProjectFormat->setDefaultValue(true);

    _projectsPage->addKnob(_firstReadSetProjectFormat);


    _autoPreviewEnabledForNewProjects = _publicInterface->createKnob<KnobBool>("enableAutoPreviewNewProjects");
    _autoPreviewEnabledForNewProjects->setLabel(tr("Auto-preview enabled by default for new projects"));
    _autoPreviewEnabledForNewProjects->setHintToolTip( tr("If checked, then when creating a new project, the Auto-preview option"
                                                          " is enabled.") );
    _autoPreviewEnabledForNewProjects->setDefaultValue(true);

    _projectsPage->addKnob(_autoPreviewEnabledForNewProjects);


    _fixPathsOnProjectPathChanged = _publicInterface->createKnob<KnobBool>("autoFixRelativePaths");
    _fixPathsOnProjectPathChanged->setHintToolTip( tr("If checked, when a project-path changes (either the name or the value pointed to), %1 checks all file-path parameters in the project and tries to fix them.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _fixPathsOnProjectPathChanged->setLabel(tr("Auto fix relative file-paths"));
    _fixPathsOnProjectPathChanged->setDefaultValue(true);

    _projectsPage->addKnob(_fixPathsOnProjectPathChanged);

    _enableMappingFromDriveLettersToUNCShareNames = _publicInterface->createKnob<KnobBool>("useDriverLetters");
    _enableMappingFromDriveLettersToUNCShareNames->setHintToolTip( tr("This is only relevant for Windows: If checked, %1 will not convert a path starting with a drive letter from the file dialog to a network share name. You may use this if for example you want to share a same project with several users across facilities with different servers but where users have all the same drive attached to a server.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _enableMappingFromDriveLettersToUNCShareNames->setLabel(tr("Use drive letters instead of server names (Windows only)"));
#ifndef __NATRON_WIN32__
    _enableMappingFromDriveLettersToUNCShareNames->setEnabled(false);
#endif
    _projectsPage->addKnob(_enableMappingFromDriveLettersToUNCShareNames);
}

void
SettingsPrivate::initializeKnobsDocumentation()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _documentationPage = _publicInterface->createKnob<KnobPage>("documentationPage");
    _documentationPage->setLabel(tr("Documentation"));

#ifdef NATRON_DOCUMENTATION_ONLINE
    _documentationSource = _publicInterface->createKnob<KnobChoice>("documentationSource");
    _documentationSource->setLabel( tr("Documentation Source"));
    _documentationSource->setHintToolTip( tr("Documentation source.") );
    _documentationSource->appendChoice(ChoiceOption("local",
                                                    tr("Local").toStdString(),
                                                    tr("Use the documentation distributed with the software.").toStdString()));
    _documentationSource->appendChoice(ChoiceOption("online",
                                                    tr("Online").toStdString(),
                                                    tr("Use the online version of the documentation (requires an internet connection).").toStdString()));
    _documentationSource->appendChoice(ChoiceOption("none",
                                                    tr("None").toStdString(),
                                                    tr("Disable documentation").toStdString()));
    _documentationSource->setDefaultValue(0);
    _documentationPage->addKnob(_documentationSource);
#endif

    /// used to store temp port for local webserver
    _wwwServerPort = _publicInterface->createKnob<KnobInt>("webserverPort");
    _wwwServerPort->setLabel(tr("Documentation local port (0=auto)"));
    _wwwServerPort->setHintToolTip( tr("The port onto which the documentation server will listen to. A value of 0 indicate that the documentation should automatically find a port by itself.") );
    _wwwServerPort->setDefaultValue(0);
    _documentationPage->addKnob(_wwwServerPort);
}

void
SettingsPrivate::initializeKnobsUserInterface()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _uiPage = _publicInterface->createKnob<KnobPage>("uiPage");
    _uiPage->setLabel(tr("User Interface"));

    _notifyOnFileChange = _publicInterface->createKnob<KnobBool>("warnOnExternalChange");
    _notifyOnFileChange->setLabel(tr("Warn when a file changes externally"));
    _notifyOnFileChange->setHintToolTip( tr("When checked, if a file read from a file parameter changes externally, a warning will be displayed "
                                            "on the viewer. Turning this off will suspend the notification system.") );
    _notifyOnFileChange->setDefaultValue(true);
    _uiPage->addKnob(_notifyOnFileChange);

    _filedialogForWriters = _publicInterface->createKnob<KnobBool>("writeUseDialog");
    _filedialogForWriters->setLabel(tr("Prompt with file dialog when creating Write node"));
    _filedialogForWriters->setDefaultValue(true);
    _filedialogForWriters->setHintToolTip( tr("When checked, opens-up a file dialog when creating a Write node") );
    _uiPage->addKnob(_filedialogForWriters);


    _renderOnEditingFinished = _publicInterface->createKnob<KnobBool>("renderOnEditingFinished");
    _renderOnEditingFinished->setLabel(tr("Refresh viewer only when editing is finished"));
    _renderOnEditingFinished->setHintToolTip( tr("When checked, the viewer triggers a new render only when mouse is released when editing parameters, curves "
                                                 " or the timeline. This setting doesn't apply to roto splines editing.") );
    _uiPage->addKnob(_renderOnEditingFinished);


    _linearPickers = _publicInterface->createKnob<KnobBool>("linearPickers");
    _linearPickers->setLabel(tr("Linear color pickers"));
    _linearPickers->setHintToolTip( tr("When activated, all colors picked from the color parameters are linearized "
                                       "before being fetched. Otherwise they are in the same colorspace "
                                       "as the viewer they were picked from.") );
    _linearPickers->setDefaultValue(true);
    _uiPage->addKnob(_linearPickers);

    _maxPanelsOpened = _publicInterface->createKnob<KnobInt>("maxPanels");
    _maxPanelsOpened->setLabel(tr("Maximum number of open settings panels (0=\"unlimited\")"));
    _maxPanelsOpened->setHintToolTip( tr("This property holds the maximum number of settings panels that can be "
                                         "held by the properties dock at the same time."
                                         "The special value of 0 indicates there can be an unlimited number of panels opened.") );
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setRange(1, 100);
    _maxPanelsOpened->setDefaultValue(10);
    _uiPage->addKnob(_maxPanelsOpened);

    _useCursorPositionIncrements = _publicInterface->createKnob<KnobBool>("cursorPositionAwareFields");
    _useCursorPositionIncrements->setLabel(tr("Value increments based on cursor position"));
    _useCursorPositionIncrements->setHintToolTip( tr("When enabled, incrementing the value fields of parameters with the "
                                                     "mouse wheel or with arrow keys will increment the digits on the right "
                                                     "of the cursor. \n"
                                                     "When disabled, the value fields are incremented given what the plug-in "
                                                     "decided it should be. You can alter this increment by holding "
                                                     "Shift (x10) or Control (/10) while incrementing.") );
    _useCursorPositionIncrements->setDefaultValue(true);
    _uiPage->addKnob(_useCursorPositionIncrements);

    _defaultLayoutFile = _publicInterface->createKnob<KnobFile>("defaultLayout");
    _defaultLayoutFile->setLabel(tr("Default layout file"));
    _defaultLayoutFile->setHintToolTip( tr("When set, %1 uses the given layout file "
                                           "as default layout for new projects. You can export/import a layout to/from a file "
                                           "from the Layout menu. If empty, the default application layout is used.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _uiPage->addKnob(_defaultLayoutFile);

    _loadProjectsWorkspace = _publicInterface->createKnob<KnobBool>("loadProjectWorkspace");
    _loadProjectsWorkspace->setLabel(tr("Load workspace embedded within projects"));
    _loadProjectsWorkspace->setHintToolTip( tr("When checked, when loading a project, the workspace (windows layout) will also be loaded, otherwise it "
                                               "will use your current layout.") );
    _uiPage->addKnob(_loadProjectsWorkspace);
} // Settings::initializeKnobsUserInterface

void
SettingsPrivate::initializeKnobsColorManagement()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _ocioTab = _publicInterface->createKnob<KnobPage>("ocioPage");
    _ocioTab->setLabel(tr("Color Management"));
    _ocioConfigKnob = _publicInterface->createKnob<KnobChoice>("ocioConfig");
    _knobsRequiringRestart.insert(_ocioConfigKnob);
    _ocioConfigKnob->setLabel(tr("OpenColorIO configuration"));

    std::vector<ChoiceOption> configs;
    int defaultIndex = 0;
    QStringList defaultOcioConfigsPaths = getDefaultOcioConfigPaths();
    Q_FOREACH(const QString &defaultOcioConfigsDir, defaultOcioConfigsPaths) {
        QDir ocioConfigsDir(defaultOcioConfigsDir);

        if ( ocioConfigsDir.exists() ) {
            QStringList entries = ocioConfigsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
            for (int j = 0; j < entries.size(); ++j) {
                if ( entries[j] == QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ) {
                    defaultIndex = j;
                }
                configs.push_back(ChoiceOption( entries[j].toStdString(), "",""));

            }

            break; //if we found 1 OpenColorIO-Configs directory, skip the next
        }
    }
    configs.push_back(ChoiceOption(NATRON_CUSTOM_OCIO_CONFIG_NAME, "", ""));

    _ocioConfigKnob->populateChoices(configs);
    _ocioConfigKnob->setDefaultValue(defaultIndex);
    _ocioConfigKnob->setHintToolTip( tr("Select the OpenColorIO configuration you would like to use globally for all "
                                        "operators and plugins that use OpenColorIO, by setting the \"OCIO\" "
                                        "environment variable. Only nodes created after changing this parameter will take "
                                        "it into account. "
                                        "When \"%1\" is selected, the "
                                        "\"Custom OpenColorIO config file\" parameter is used.").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) +
                                     QLatin1Char('\n') +
                                     tr("Changing this requires a restart of the application to take effect.")  );

    _ocioTab->addKnob(_ocioConfigKnob);

    _customOcioConfigFile = _publicInterface->createKnob<KnobFile>("ocioCustomConfigFile");

    _customOcioConfigFile->setLabel( tr("Custom OpenColorIO configuration file"));
    _knobsRequiringRestart.insert(_customOcioConfigFile);

    if (_ocioConfigKnob->getNumEntries() == 1) {
        _customOcioConfigFile->setEnabled(true);
    } else {
        _customOcioConfigFile->setEnabled(false);
    }

    _customOcioConfigFile->setHintToolTip( tr("OpenColorIO configuration file (config.ocio) to use when \"%1\" "
                                              "is selected as the OpenColorIO config.").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) +
                                           QLatin1Char('\n') +
                                           tr("Changing this requires a restart of the application to take effect.") );
    _ocioTab->addKnob(_customOcioConfigFile);


    _ocioStartupCheck = _publicInterface->createKnob<KnobBool>("sartupCheckOCIO");
    _ocioStartupCheck->setLabel(tr("Warn on startup if OpenColorIO config is not the default"));
    _ocioStartupCheck->setDefaultValue(true);

    _ocioTab->addKnob(_ocioStartupCheck);
} // Settings::initializeKnobsColorManagement

void
SettingsPrivate::initializeKnobsAppearance()
{

    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _appearanceTab = _publicInterface->createKnob<KnobPage>("appearancePage");
    _appearanceTab->setLabel(tr("Appearance"));

    _defaultAppearanceVersion = _publicInterface->createKnob<KnobInt>("appearanceVersion");
    _defaultAppearanceVersion->setLabel(tr("Appearance version"));
    _defaultAppearanceVersion->setDefaultValue(NATRON_DEFAULT_APPEARANCE_VERSION);
    _defaultAppearanceVersion->setSecret(true);
    _appearanceTab->addKnob(_defaultAppearanceVersion);

    _systemFontChoice = _publicInterface->createKnob<KnobChoice>("systemFont");
    _systemFontChoice->setHintToolTip( tr("List of all fonts available on the system.") +
                                      QLatin1Char('\n') +
                                      tr("Changing this requires a restart of the application to take effect.")  );
    _systemFontChoice->setLabel(tr("Font"));
    _systemFontChoice->setAddNewLine(false);
    _systemFontChoice->setDefaultValueFromID(NATRON_FONT);
    _knobsRequiringRestart.insert(_systemFontChoice);
    _appearanceTab->addKnob(_systemFontChoice);

    _fontSize = _publicInterface->createKnob<KnobInt>("fontSize");
    _fontSize->setLabel(tr("Font size"));
    _fontSize->setHintToolTip( tr("The application font size") +
                               QLatin1Char('\n') +
                              tr("Changing this requires a restart of the application to take effect.") );
    _fontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);
    _knobsRequiringRestart.insert(_fontSize);
    _appearanceTab->addKnob(_fontSize);

    _qssFile = _publicInterface->createKnob<KnobFile>("stylesheetFile");
    _qssFile->setLabel(tr("Stylesheet file (.qss)"));
    _knobsRequiringRestart.insert(_qssFile);
    _qssFile->setHintToolTip( tr("When pointing to a valid .qss file, the stylesheet of the application will be set according to this file instead of the default "
                                 "stylesheet. You can adapt the default stylesheet that can be found in your distribution of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) +
                              QLatin1Char('\n') +
                              tr("Changing this requires a restart of the application to take effect.") );
    _appearanceTab->addKnob(_qssFile);
} // Settings::initializeKnobsAppearance

void
SettingsPrivate::initializeKnobsGuiColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _guiColorsTab =_publicInterface-> createKnob<KnobPage>("mainWindowAppearancePage");
    _guiColorsTab->setLabel(tr("Main Window"));
    _appearanceTab->addKnob(_guiColorsTab);

    _useBWIcons = _publicInterface->createKnob<KnobBool>("useBwIcons");
    _useBWIcons->setLabel(tr("Use black & white toolbutton icons"));
    _useBWIcons->setHintToolTip( tr("When checked, the tools icons in the left toolbar are greyscale. Changing this takes "
                                    "effect upon the next launch of the application.") );
    _useBWIcons->setDefaultValue(false);
    _guiColorsTab->addKnob(_useBWIcons);


    _sunkenColor =  _publicInterface->createKnob<KnobColor>("sunken", 3);
    _sunkenColor->setLabel(tr("Sunken"));
    _sunkenColor->setSimplified(true);
    _sunkenColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _sunkenColor->setDefaultValue(0.12, DimIdx(0));
    _sunkenColor->setDefaultValue(0.12, DimIdx(1));
    _sunkenColor->setDefaultValue(0.12, DimIdx(2));

    _guiColorsTab->addKnob(_sunkenColor);

    _baseColor =  _publicInterface->createKnob<KnobColor>("base", 3);
    _baseColor->setLabel(tr("Base"));
    _baseColor->setSimplified(true);
    _baseColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _baseColor->setDefaultValue(0.19, DimIdx(0));
    _baseColor->setDefaultValue(0.19, DimIdx(1));
    _baseColor->setDefaultValue(0.19, DimIdx(2));

    _guiColorsTab->addKnob(_baseColor);

    _raisedColor =  _publicInterface->createKnob<KnobColor>("raised", 3);
    _raisedColor->setLabel(tr("Raised"));
    _raisedColor->setSimplified(true);
    _raisedColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _raisedColor->setDefaultValue(0.28, DimIdx(0));
    _raisedColor->setDefaultValue(0.28, DimIdx(1));
    _raisedColor->setDefaultValue(0.28, DimIdx(2));

    _guiColorsTab->addKnob(_raisedColor);

    _selectionColor =  _publicInterface->createKnob<KnobColor>("selection", 3);
    _selectionColor->setLabel(tr("Selection"));
    _selectionColor->setSimplified(true);
    _selectionColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _selectionColor->setDefaultValue(0.95, DimIdx(0));
    _selectionColor->setDefaultValue(0.54, DimIdx(1));
    _selectionColor->setDefaultValue(0., DimIdx(2));

    _guiColorsTab->addKnob(_selectionColor);

    _textColor =  _publicInterface->createKnob<KnobColor>("text", 3);
    _textColor->setLabel(tr("Text"));
    _textColor->setSimplified(true);
    _textColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _textColor->setDefaultValue(0.78, DimIdx(0));
    _textColor->setDefaultValue(0.78, DimIdx(1));
    _textColor->setDefaultValue(0.78, DimIdx(2));

    _guiColorsTab->addKnob(_textColor);

    _altTextColor =  _publicInterface->createKnob<KnobColor>("unmodifiedText", 3);
    _altTextColor->setLabel(tr("Unmodified text"));
    _altTextColor->setSimplified(true);
    _altTextColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _altTextColor->setDefaultValue(0.6, DimIdx(0));
    _altTextColor->setDefaultValue(0.6, DimIdx(1));
    _altTextColor->setDefaultValue(0.6, DimIdx(2));

    _guiColorsTab->addKnob(_altTextColor);

    _timelinePlayheadColor =  _publicInterface->createKnob<KnobColor>("timelinePlayhead", 3);
    _timelinePlayheadColor->setLabel(tr("Timeline playhead"));
    _timelinePlayheadColor->setSimplified(true);
    _timelinePlayheadColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _timelinePlayheadColor->setDefaultValue(0.95, DimIdx(0));
    _timelinePlayheadColor->setDefaultValue(0.54, DimIdx(1));
    _timelinePlayheadColor->setDefaultValue(0., DimIdx(2));

    _guiColorsTab->addKnob(_timelinePlayheadColor);


    _timelineBGColor =  _publicInterface->createKnob<KnobColor>("timelineBG", 3);
    _timelineBGColor->setLabel(tr("Timeline background"));
    _timelineBGColor->setSimplified(true);
    _timelineBGColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _timelineBGColor->setDefaultValue(0, DimIdx(0));
    _timelineBGColor->setDefaultValue(0, DimIdx(1));
    _timelineBGColor->setDefaultValue(0., DimIdx(2));

    _guiColorsTab->addKnob(_timelineBGColor);

    _timelineBoundsColor =  _publicInterface->createKnob<KnobColor>("timelineBound", 3);
    _timelineBoundsColor->setLabel(tr("Timeline bounds"));
    _timelineBoundsColor->setSimplified(true);
    _timelineBoundsColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _timelineBoundsColor->setDefaultValue(0.81, DimIdx(0));
    _timelineBoundsColor->setDefaultValue(0.27, DimIdx(1));
    _timelineBoundsColor->setDefaultValue(0.02, DimIdx(2));

    _guiColorsTab->addKnob(_timelineBoundsColor);

    _cachedFrameColor =  _publicInterface->createKnob<KnobColor>("cachedFrame", 3);
    _cachedFrameColor->setLabel(tr("Cached frame"));
    _cachedFrameColor->setSimplified(true);
    _cachedFrameColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _cachedFrameColor->setDefaultValue(0.56, DimIdx(0));
    _cachedFrameColor->setDefaultValue(0.79, DimIdx(1));
    _cachedFrameColor->setDefaultValue(0.4, DimIdx(2));

    _guiColorsTab->addKnob(_cachedFrameColor);


    _interpolatedColor =  _publicInterface->createKnob<KnobColor>("interpValue", 3);
    _interpolatedColor->setLabel(tr("Interpolated value"));
    _interpolatedColor->setSimplified(true);
    _interpolatedColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _interpolatedColor->setDefaultValue(0.34, DimIdx(0));
    _interpolatedColor->setDefaultValue(0.46, DimIdx(1));
    _interpolatedColor->setDefaultValue(0.6, DimIdx(2));

    _guiColorsTab->addKnob(_interpolatedColor);

    _keyframeColor =  _publicInterface->createKnob<KnobColor>("keyframe", 3);
    _keyframeColor->setLabel(tr("Keyframe"));
    _keyframeColor->setSimplified(true);
    _keyframeColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _keyframeColor->setDefaultValue(0.08, DimIdx(0));
    _keyframeColor->setDefaultValue(0.38, DimIdx(1));
    _keyframeColor->setDefaultValue(0.97, DimIdx(2));

    _guiColorsTab->addKnob(_keyframeColor);

    _trackerKeyframeColor =  _publicInterface->createKnob<KnobColor>("trackUserKeyframe", 3);
    _trackerKeyframeColor->setLabel(tr("Track User Keyframes"));
    _trackerKeyframeColor->setSimplified(true);
    _trackerKeyframeColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _trackerKeyframeColor->setDefaultValue(0.7, DimIdx(0));
    _trackerKeyframeColor->setDefaultValue(0.78, DimIdx(1));
    _trackerKeyframeColor->setDefaultValue(0.39, DimIdx(2));

    _guiColorsTab->addKnob(_trackerKeyframeColor);

    _exprColor =  _publicInterface->createKnob<KnobColor>("exprCoor", 3);
    _exprColor->setLabel(tr("Expression"));
    _exprColor->setSimplified(true);
    _exprColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _exprColor->setDefaultValue(0.7, DimIdx(0));
    _exprColor->setDefaultValue(0.78, DimIdx(1));
    _exprColor->setDefaultValue(0.39, DimIdx(2));

    _guiColorsTab->addKnob(_exprColor);

    _sliderColor =  _publicInterface->createKnob<KnobColor>("slider", 3);
    _sliderColor->setLabel(tr("Slider"));
    _sliderColor->setSimplified(true);
    _sliderColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _sliderColor->setDefaultValue(0.33, DimIdx(0));
    _sliderColor->setDefaultValue(0.45, DimIdx(1));
    _sliderColor->setDefaultValue(0.44, DimIdx(2));

    _guiColorsTab->addKnob(_sliderColor);

} // Settings::initializeKnobsGuiColors


void
SettingsPrivate::initializeKnobsDopeSheetColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _animationColorsTab = _publicInterface->createKnob<KnobPage>("animationModuleColorPage");
    _animationColorsTab->setLabel(tr("Animation Module"));
    _appearanceTab->addKnob(_animationColorsTab);

    _animationViewBackgroundColor = _publicInterface->createKnob<KnobColor>("animationViewBGColor", 3);
    _animationViewBackgroundColor->setLabel(tr("Background Color"));
    _animationViewBackgroundColor->setSimplified(true);
    _animationViewBackgroundColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _animationViewBackgroundColor->setDefaultValue(0.19, DimIdx(0));
    _animationViewBackgroundColor->setDefaultValue(0.19, DimIdx(1));
    _animationViewBackgroundColor->setDefaultValue(0.19, DimIdx(2));

    _animationColorsTab->addKnob(_animationViewBackgroundColor);

    _dopesheetRootSectionBackgroundColor = _publicInterface->createKnob<KnobColor>("dopesheetRootSectionBackground", 4);
    _dopesheetRootSectionBackgroundColor->setLabel(tr("Root section background color"));
    _dopesheetRootSectionBackgroundColor->setSimplified(true);
    _dopesheetRootSectionBackgroundColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.204, DimIdx(0));
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.204, DimIdx(1));
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.204, DimIdx(2));
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.2, DimIdx(3));

    _animationColorsTab->addKnob(_dopesheetRootSectionBackgroundColor);

    _dopesheetKnobSectionBackgroundColor = _publicInterface->createKnob<KnobColor>("dopesheetParamSectionBackground", 4);
    _dopesheetKnobSectionBackgroundColor->setLabel(tr("Parameter background color"));
    _dopesheetKnobSectionBackgroundColor->setSimplified(true);
    _dopesheetKnobSectionBackgroundColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.443, DimIdx(0));
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.443, DimIdx(1));
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.443, DimIdx(2));
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.2, DimIdx(2));

    _animationColorsTab->addKnob(_dopesheetKnobSectionBackgroundColor);

    _animationViewScaleColor = _publicInterface->createKnob<KnobColor>("animationViewScaleColor", 3);
    _animationViewScaleColor->setLabel(tr("Scale Text Color"));
    _animationViewScaleColor->setSimplified(true);
    _animationViewScaleColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _animationViewScaleColor->setDefaultValue(0.714, DimIdx(0));
    _animationViewScaleColor->setDefaultValue(0.718, DimIdx(1));
    _animationViewScaleColor->setDefaultValue(0.714, DimIdx(2));

    _animationColorsTab->addKnob(_animationViewScaleColor);

    _animationViewGridColor = _publicInterface->createKnob<KnobColor>("animationViewGridColor", 3);
    _animationViewGridColor->setLabel(tr("Grid Color"));
    _animationViewGridColor->setSimplified(true);
    _animationViewGridColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _animationViewGridColor->setDefaultValue(0.35, DimIdx(0));
    _animationViewGridColor->setDefaultValue(0.35, DimIdx(1));
    _animationViewGridColor->setDefaultValue(0.35, DimIdx(2));
    _animationColorsTab->addKnob(_animationViewGridColor);

} // initializeKnobsDopeSheetColors

void
SettingsPrivate::initializeKnobsNodeGraphColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _nodegraphColorsTab = _publicInterface->createKnob<KnobPage>("nodeGraphColorsPage");
    _nodegraphColorsTab->setLabel(tr("Node Graph"));
    _appearanceTab->addKnob(_nodegraphColorsTab);

    _usePluginIconsInNodeGraph = _publicInterface->createKnob<KnobBool>("usePluginIcons");
    _usePluginIconsInNodeGraph->setLabel(tr("Display plug-in icon on node-graph"));
    _usePluginIconsInNodeGraph->setHintToolTip( tr("When checked, each node that has a plug-in icon will display it in the node-graph."
                                                   "Changing this option will not affect already existing nodes, unless a restart of Natron is made.") );
    _usePluginIconsInNodeGraph->setAddNewLine(false);
    _usePluginIconsInNodeGraph->setDefaultValue(true);

    _nodegraphColorsTab->addKnob(_usePluginIconsInNodeGraph);

    _useAntiAliasing = _publicInterface->createKnob<KnobBool>("antiAliasing");
    _useAntiAliasing->setLabel(tr("Anti-Aliasing"));
    _useAntiAliasing->setHintToolTip( tr("When checked, the node graph will be painted using anti-aliasing. Unchecking it may increase performances."
                                         " Changing this requires a restart of Natron") );
    _useAntiAliasing->setDefaultValue(true);

    _nodegraphColorsTab->addKnob(_useAntiAliasing);


    _defaultNodeColor = _publicInterface->createKnob<KnobColor>("defaultNodeColor", 3);
    _defaultNodeColor->setLabel(tr("Default node color"));
    _defaultNodeColor->setSimplified(true);
    _defaultNodeColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultNodeColor->setHintToolTip( tr("The default color used for newly created nodes.") );
    _defaultNodeColor->setDefaultValue(0.7, DimIdx(0));
    _defaultNodeColor->setDefaultValue(0.7, DimIdx(1));
    _defaultNodeColor->setDefaultValue(0.7, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultNodeColor);


    _cloneColor = _publicInterface->createKnob<KnobColor>("cloneColor", 3);
    _cloneColor->setLabel(tr("Clone Color"));
    _cloneColor->setSimplified(true);
    _cloneColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _cloneColor->setHintToolTip( tr("The default color used for clone nodes.") );
    _cloneColor->setDefaultValue(0.78, DimIdx(0));
    _cloneColor->setDefaultValue(0.27, DimIdx(1));
    _cloneColor->setDefaultValue(0.39, DimIdx(2));

    _nodegraphColorsTab->addKnob(_cloneColor);



    _defaultBackdropColor =  _publicInterface->createKnob<KnobColor>("backdropColor", 3);
    _defaultBackdropColor->setLabel(tr("Default backdrop color"));
    _defaultBackdropColor->setSimplified(true);
    _defaultBackdropColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultBackdropColor->setHintToolTip( tr("The default color used for newly created backdrop nodes.") );
    _defaultBackdropColor->setDefaultValue(0.45, DimIdx(0));
    _defaultBackdropColor->setDefaultValue(0.45, DimIdx(1));
    _defaultBackdropColor->setDefaultValue(0.45, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultBackdropColor);

    _defaultReaderColor =  _publicInterface->createKnob<KnobColor>("readerColor", 3);
    _defaultReaderColor->setLabel(tr(PLUGIN_GROUP_IMAGE_READERS));
    _defaultReaderColor->setSimplified(true);
    _defaultReaderColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultReaderColor->setHintToolTip( tr("The color used for newly created Reader nodes.") );
    _defaultReaderColor->setDefaultValue(0.7, DimIdx(0));
    _defaultReaderColor->setDefaultValue(0.7, DimIdx(1));
    _defaultReaderColor->setDefaultValue(0.7, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultReaderColor);

    _defaultWriterColor =  _publicInterface->createKnob<KnobColor>("writerColor", 3);
    _defaultWriterColor->setLabel(tr(PLUGIN_GROUP_IMAGE_WRITERS));
    _defaultWriterColor->setSimplified(true);
    _defaultWriterColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultWriterColor->setHintToolTip( tr("The color used for newly created Writer nodes.") );
    _defaultWriterColor->setDefaultValue(0.75, DimIdx(0));
    _defaultWriterColor->setDefaultValue(0.75, DimIdx(1));
    _defaultWriterColor->setDefaultValue(0., DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultWriterColor);

    _defaultGeneratorColor =  _publicInterface->createKnob<KnobColor>("generatorColor", 3);
    _defaultGeneratorColor->setLabel(tr("Generators"));
    _defaultGeneratorColor->setSimplified(true);
    _defaultGeneratorColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultGeneratorColor->setHintToolTip( tr("The color used for newly created Generator nodes.") );
    _defaultGeneratorColor->setDefaultValue(0.3, DimIdx(0));
    _defaultGeneratorColor->setDefaultValue(0.5, DimIdx(1));
    _defaultGeneratorColor->setDefaultValue(0.2, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultGeneratorColor);

    _defaultColorGroupColor =  _publicInterface->createKnob<KnobColor>("colorNodesColor", 3);
    _defaultColorGroupColor->setLabel(tr("Color group"));
    _defaultColorGroupColor->setSimplified(true);
    _defaultColorGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultColorGroupColor->setHintToolTip( tr("The color used for newly created Color nodes.") );
    _defaultColorGroupColor->setDefaultValue(0.48, DimIdx(0));
    _defaultColorGroupColor->setDefaultValue(0.66, DimIdx(1));
    _defaultColorGroupColor->setDefaultValue(1., DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultColorGroupColor);

    _defaultFilterGroupColor =  _publicInterface->createKnob<KnobColor>("filterNodesColor", 3);
    _defaultFilterGroupColor->setLabel(tr("Filter group"));
    _defaultFilterGroupColor->setSimplified(true);
    _defaultFilterGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultFilterGroupColor->setHintToolTip( tr("The color used for newly created Filter nodes.") );
    _defaultFilterGroupColor->setDefaultValue(0.8, DimIdx(0));
    _defaultFilterGroupColor->setDefaultValue(0.5, DimIdx(1));
    _defaultFilterGroupColor->setDefaultValue(0.3, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultFilterGroupColor);

    _defaultTransformGroupColor =  _publicInterface->createKnob<KnobColor>("transformNodesColor", 3);
    _defaultTransformGroupColor->setLabel(tr("Transform group"));
    _defaultTransformGroupColor->setSimplified(true);
    _defaultTransformGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultTransformGroupColor->setHintToolTip( tr("The color used for newly created Transform nodes.") );
    _defaultTransformGroupColor->setDefaultValue(0.7, DimIdx(0));
    _defaultTransformGroupColor->setDefaultValue(0.3, DimIdx(1));
    _defaultTransformGroupColor->setDefaultValue(0.1, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultTransformGroupColor);

    _defaultTimeGroupColor =  _publicInterface->createKnob<KnobColor>("timeNodesColor", 3);
    _defaultTimeGroupColor->setLabel(tr("Time group"));
    _defaultTimeGroupColor->setSimplified(true);
    _defaultTimeGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultTimeGroupColor->setHintToolTip( tr("The color used for newly created Time nodes.") );
    _defaultTimeGroupColor->setDefaultValue(0.7, DimIdx(0));
    _defaultTimeGroupColor->setDefaultValue(0.65, DimIdx(1));
    _defaultTimeGroupColor->setDefaultValue(0.35, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultTimeGroupColor);

    _defaultDrawGroupColor =  _publicInterface->createKnob<KnobColor>("drawNodesColor", 3);
    _defaultDrawGroupColor->setLabel(tr("Draw group"));
    _defaultDrawGroupColor->setSimplified(true);
    _defaultDrawGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultDrawGroupColor->setHintToolTip( tr("The color used for newly created Draw nodes.") );
    _defaultDrawGroupColor->setDefaultValue(0.75, DimIdx(0));
    _defaultDrawGroupColor->setDefaultValue(0.75, DimIdx(1));
    _defaultDrawGroupColor->setDefaultValue(0.75, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultDrawGroupColor);

    _defaultKeyerGroupColor =  _publicInterface->createKnob<KnobColor>("keyerNodesColor", 3);
    _defaultKeyerGroupColor->setLabel( tr("Keyer group"));
    _defaultKeyerGroupColor->setSimplified(true);
    _defaultKeyerGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultKeyerGroupColor->setHintToolTip( tr("The color used for newly created Keyer nodes.") );
    _defaultKeyerGroupColor->setDefaultValue(0., DimIdx(0));
    _defaultKeyerGroupColor->setDefaultValue(1, DimIdx(1));
    _defaultKeyerGroupColor->setDefaultValue(0., DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultKeyerGroupColor);

    _defaultChannelGroupColor =  _publicInterface->createKnob<KnobColor>("channelNodesColor", 3);
    _defaultChannelGroupColor->setLabel(tr("Channel group"));
    _defaultChannelGroupColor->setSimplified(true);
    _defaultChannelGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultChannelGroupColor->setHintToolTip( tr("The color used for newly created Channel nodes.") );
    _defaultChannelGroupColor->setDefaultValue(0.6, DimIdx(0));
    _defaultChannelGroupColor->setDefaultValue(0.24, DimIdx(1));
    _defaultChannelGroupColor->setDefaultValue(0.39, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultChannelGroupColor);

    _defaultMergeGroupColor =  _publicInterface->createKnob<KnobColor>("defaultMergeColor", 3);
    _defaultMergeGroupColor->setLabel(tr("Merge group"));
    _defaultMergeGroupColor->setSimplified(true);
    _defaultMergeGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultMergeGroupColor->setHintToolTip( tr("The color used for newly created Merge nodes.") );
    _defaultMergeGroupColor->setDefaultValue(0.3, DimIdx(0));
    _defaultMergeGroupColor->setDefaultValue(0.37, DimIdx(1));
    _defaultMergeGroupColor->setDefaultValue(0.776, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultMergeGroupColor);

    _defaultViewsGroupColor =  _publicInterface->createKnob<KnobColor>("defaultViewsColor", 3);
    _defaultViewsGroupColor->setLabel(tr("Views group"));
    _defaultViewsGroupColor->setSimplified(true);
    _defaultViewsGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultViewsGroupColor->setHintToolTip( tr("The color used for newly created Views nodes.") );
    _defaultViewsGroupColor->setDefaultValue(0.5, DimIdx(0));
    _defaultViewsGroupColor->setDefaultValue(0.9, DimIdx(1));
    _defaultViewsGroupColor->setDefaultValue(0.7, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultViewsGroupColor);

    _defaultDeepGroupColor =  _publicInterface->createKnob<KnobColor>("defaultDeepColor", 3);
    _defaultDeepGroupColor->setLabel(tr("Deep group"));
    _defaultDeepGroupColor->setSimplified(true);
    _defaultDeepGroupColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defaultDeepGroupColor->setHintToolTip( tr("The color used for newly created Deep nodes.") );
    _defaultDeepGroupColor->setDefaultValue(0., DimIdx(0));
    _defaultDeepGroupColor->setDefaultValue(0., DimIdx(1));
    _defaultDeepGroupColor->setDefaultValue(0.38, DimIdx(2));

    _nodegraphColorsTab->addKnob(_defaultDeepGroupColor);

} // Settings::initializeKnobsNodeGraphColors

void
SettingsPrivate::initializeKnobsScriptEditorColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _scriptEditorColorsTab = _publicInterface->createKnob<KnobPage>("scriptEditorAppearancePage");
    _scriptEditorColorsTab->setLabel(tr("Script Editor"));
    _scriptEditorColorsTab->setParentKnob(_appearanceTab);

    _scriptEditorFontChoice = _publicInterface->createKnob<KnobChoice>("scriptEditorFont");
    _scriptEditorFontChoice->setHintToolTip( tr("List of all fonts available on the system") );
    _scriptEditorFontChoice->setDefaultValueFromID(NATRON_SCRIPT_FONT);
    _scriptEditorFontChoice->setLabel(tr("Font"));

    _scriptEditorColorsTab->addKnob(_scriptEditorFontChoice);

    _scriptEditorFontSize = _publicInterface->createKnob<KnobInt>("scriptEditorFontSize");
    _scriptEditorFontSize->setHintToolTip( tr("The font size") );
    _scriptEditorFontSize->setLabel(tr("Font Size"));
    _scriptEditorFontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);

    _scriptEditorColorsTab->addKnob(_scriptEditorFontSize);

    _curLineColor = _publicInterface->createKnob<KnobColor>("currentLineColor", 3);
    _curLineColor->setLabel(tr("Current Line Color"));
    _curLineColor->setSimplified(true);
    _curLineColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _curLineColor->setDefaultValue(0.35, DimIdx(0));
    _curLineColor->setDefaultValue(0.35, DimIdx(1));
    _curLineColor->setDefaultValue(0.35, DimIdx(2));

    _scriptEditorColorsTab->addKnob(_curLineColor);

    _keywordColor = _publicInterface->createKnob<KnobColor>("keywordColor", 3);
    _keywordColor->setLabel(tr("Keyword Color"));
    _keywordColor->setSimplified(true);
    _keywordColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _keywordColor->setDefaultValue(0.7, DimIdx(0));
    _keywordColor->setDefaultValue(0.7, DimIdx(1));
    _keywordColor->setDefaultValue(0., DimIdx(2));

    _scriptEditorColorsTab->addKnob(_keywordColor);

    _operatorColor = _publicInterface->createKnob<KnobColor>("operatorColor", 3);
    _operatorColor->setLabel(tr("Operator Color"));
    _operatorColor->setSimplified(true);
    _operatorColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _operatorColor->setDefaultValue(0.78, DimIdx(0));
    _operatorColor->setDefaultValue(0.78, DimIdx(1));
    _operatorColor->setDefaultValue(0.78, DimIdx(2));

    _scriptEditorColorsTab->addKnob(_operatorColor);

    _braceColor = _publicInterface->createKnob<KnobColor>("braceColor", 3);
    _braceColor->setLabel(tr("Brace Color"));
    _braceColor->setSimplified(true);
    _braceColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _braceColor->setDefaultValue(0.85, DimIdx(0));
    _braceColor->setDefaultValue(0.85, DimIdx(1));
    _braceColor->setDefaultValue(0.85, DimIdx(2));

    _scriptEditorColorsTab->addKnob(_braceColor);

    _defClassColor = _publicInterface->createKnob<KnobColor>("classDefColor", 3);
    _defClassColor->setLabel(tr("Class Def Color"));
    _defClassColor->setSimplified(true);
    _defClassColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _defClassColor->setDefaultValue(0.7, DimIdx(0));
    _defClassColor->setDefaultValue(0.7, DimIdx(1));
    _defClassColor->setDefaultValue(0., DimIdx(2));

    _scriptEditorColorsTab->addKnob(_defClassColor);

    _stringsColor = _publicInterface->createKnob<KnobColor>("stringsColor", 3);
    _stringsColor->setLabel( tr("Strings Color"));
    _stringsColor->setSimplified(true);
    _stringsColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _stringsColor->setDefaultValue(0.8, DimIdx(0));
    _stringsColor->setDefaultValue(0.2, DimIdx(1));
    _stringsColor->setDefaultValue(0., DimIdx(2));

    _scriptEditorColorsTab->addKnob(_stringsColor);

    _commentsColor = _publicInterface->createKnob<KnobColor>("commentsColor", 3);
    _commentsColor->setLabel(tr("Comments Color"));
    _commentsColor->setSimplified(true);
    _commentsColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _commentsColor->setDefaultValue(0.25, DimIdx(0));
    _commentsColor->setDefaultValue(0.6, DimIdx(1));
    _commentsColor->setDefaultValue(0.25, DimIdx(2));

    _scriptEditorColorsTab->addKnob(_commentsColor);

    _selfColor = _publicInterface->createKnob<KnobColor>("selfColor", 3);
    _selfColor->setLabel(tr("Self Color"));
    _selfColor->setSimplified(true);
    _selfColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _selfColor->setDefaultValue(0.7, DimIdx(0));
    _selfColor->setDefaultValue(0.7, DimIdx(1));
    _selfColor->setDefaultValue(0., DimIdx(2));

    _scriptEditorColorsTab->addKnob(_selfColor);

    _numbersColor = _publicInterface->createKnob<KnobColor>("numbersColor", 3);
    _numbersColor->setLabel(tr("Numbers Color"));
    _numbersColor->setSimplified(true);
    _numbersColor->setInternalColorspaceName(kColorKnobDefaultUIColorspaceName);
    _numbersColor->setDefaultValue(0.25, DimIdx(0));
    _numbersColor->setDefaultValue(0.8, DimIdx(1));
    _numbersColor->setDefaultValue(0.9, DimIdx(2));

    _scriptEditorColorsTab->addKnob(_numbersColor);


} // Settings::initializeKnobsScriptEditorColors

void
SettingsPrivate::initializeKnobsViewers()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _viewersTab = _publicInterface->createKnob<KnobPage>("viewerPage");
    _viewersTab->setLabel(tr("Viewer"));

    _texturesMode = _publicInterface->createKnob<KnobChoice>("texturesBitDepth");
    _texturesMode->setLabel(tr("Viewer textures bit depth"));
    std::vector<ChoiceOption> textureModes;
    textureModes.push_back(ChoiceOption("8u",
                                        tr("8-bit").toStdString(),
                                        tr("Post-processing done by the viewer (such as colorspace conversion) is done "
                                           "by the CPU. The size of cached textures is thus smaller.").toStdString() ));

    //textureModes.push_back("16bits half-float");
    //helpStringsTextureModes.push_back("Not available yet. Similar to 32bits fp.");
    textureModes.push_back(ChoiceOption("32f",
                                        tr("32-bit floating-point").toStdString(),
                                        tr("Post-processing done by the viewer (such as colorspace conversion) is done "
                                           "by the GPU, using GLSL. The size of cached textures is thus larger.").toStdString()));
    _texturesMode->populateChoices(textureModes);
    _texturesMode->setHintToolTip( tr("Bit depth of the viewer textures used for rendering."
                                      " Hover each option with the mouse for a detailed description.") );
    _texturesMode->setDefaultValue(0);
    _viewersTab->addKnob(_texturesMode);

    _checkerboardTileSize = _publicInterface->createKnob<KnobInt>("checkerboardTileSize");
    _checkerboardTileSize->setLabel(tr("Checkerboard tile size (pixels)"));
    _checkerboardTileSize->setRange(1, INT_MAX);
    _checkerboardTileSize->setHintToolTip( tr("The size (in screen pixels) of one tile of the checkerboard.") );
    _checkerboardTileSize->setDefaultValue(5);
    _viewersTab->addKnob(_checkerboardTileSize);

    _checkerboardColor1 = _publicInterface->createKnob<KnobColor>("checkerboardColor1", 4);
    _checkerboardColor1->setLabel(tr("Checkerboard color 1"));
    _checkerboardColor1->setHintToolTip( tr("The first color used by the checkerboard.") );
    _checkerboardColor1->setDefaultValue(0.5);
    _checkerboardColor1->setDefaultValue(0.5, DimIdx(1));
    _checkerboardColor1->setDefaultValue(0.5, DimIdx(2));
    _checkerboardColor1->setDefaultValue(0.5, DimIdx(3));

    _viewersTab->addKnob(_checkerboardColor1);

    _checkerboardColor2 = _publicInterface->createKnob<KnobColor>("checkerboardColor2", 4);
    _checkerboardColor2->setLabel(tr("Checkerboard color 2"));
    _checkerboardColor2->setHintToolTip( tr("The second color used by the checkerboard.") );
    _checkerboardColor2->setDefaultValue(0.);
    _checkerboardColor2->setDefaultValue(0., DimIdx(1));
    _checkerboardColor2->setDefaultValue(0., DimIdx(2));
    _checkerboardColor2->setDefaultValue(0., DimIdx(3));

    _viewersTab->addKnob(_checkerboardColor2);

    _autoWipe = _publicInterface->createKnob<KnobBool>("autoWipeForViewer");
    _autoWipe->setLabel(tr("Automatically enable wipe"));
    _autoWipe->setHintToolTip( tr("When checked, the wipe tool of the viewer will be automatically enabled "
                                  "when the mouse is hovering the viewer and changing an input of a viewer." ) );
    _autoWipe->setDefaultValue(true);

    _viewersTab->addKnob(_autoWipe);


    _autoProxyWhenScrubbingTimeline = _publicInterface->createKnob<KnobBool>("autoProxyScrubbing");
    _autoProxyWhenScrubbingTimeline->setLabel(tr("Automatically enable proxy when scrubbing the timeline"));
    _autoProxyWhenScrubbingTimeline->setHintToolTip( tr("When checked, the proxy mode will be at least at the level "
                                                        "indicated by the auto-proxy parameter.") );
    _autoProxyWhenScrubbingTimeline->setAddNewLine(false);
    _autoProxyWhenScrubbingTimeline->setDefaultValue(true);

    _viewersTab->addKnob(_autoProxyWhenScrubbingTimeline);


    _autoProxyLevel = _publicInterface->createKnob<KnobChoice>("autoProxyLevel");
    _autoProxyLevel->setLabel(tr("Auto-proxy level"));
    std::vector<ChoiceOption> autoProxyChoices;
    autoProxyChoices.push_back(ChoiceOption("2", "",""));
    autoProxyChoices.push_back(ChoiceOption("4", "",""));
    autoProxyChoices.push_back(ChoiceOption("8", "",""));
    autoProxyChoices.push_back(ChoiceOption("16", "",""));
    autoProxyChoices.push_back(ChoiceOption("32", "",""));
    _autoProxyLevel->populateChoices(autoProxyChoices);
    _autoProxyLevel->setDefaultValue(1);

    _viewersTab->addKnob(_autoProxyLevel);

    _maximumNodeViewerUIOpened = _publicInterface->createKnob<KnobInt>("maxNodeUiOpened");
    _maximumNodeViewerUIOpened->setLabel(tr("Max. opened node viewer interface"));
    _maximumNodeViewerUIOpened->setRange(1, INT_MAX);
    _maximumNodeViewerUIOpened->disableSlider();
    _maximumNodeViewerUIOpened->setHintToolTip( tr("Controls the maximum amount of nodes that can have their interface showing up at the same time in the viewer") );
    _maximumNodeViewerUIOpened->setDefaultValue(2);
    _viewersTab->addKnob(_maximumNodeViewerUIOpened);

    _viewerKeys = _publicInterface->createKnob<KnobBool>("viewerNumberKeys");
    _viewerKeys->setLabel(tr("Use number keys for the viewer"));
    _viewerKeys->setHintToolTip( tr("When enabled, the row of number keys on the keyboard "
                                    "is used for switching input (<key> connects input to A side, "
                                    "<shift-key> connects input to B side), even if the corresponding "
                                    "character in the current keyboard layout is not a number.\n"
                                    "This may have to be disabled when using a remote display connection "
                                    "to Linux from a different OS.") );
    _viewerKeys->setDefaultValue(true);
    _viewersTab->addKnob(_viewerKeys);




} // Settings::initializeKnobsViewers

void
SettingsPrivate::initializeKnobsNodeGraph()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    /////////// Nodegraph tab
    _nodegraphTab = _publicInterface->createKnob<KnobPage>("nodegraphTab");
    _nodegraphTab->setLabel(tr("Nodegraph"));

    _autoScroll = _publicInterface->createKnob<KnobBool>("autoScroll");
    _autoScroll->setLabel(tr("Auto Scroll"));
    _autoScroll->setHintToolTip( tr("When checked the node graph will auto scroll if you move a node outside the current graph view.") );
    _autoScroll->setDefaultValue(false);
    _nodegraphTab->addKnob(_autoScroll);

    _autoTurbo = _publicInterface->createKnob<KnobBool>("autoTurbo");
    _autoTurbo->setLabel( tr("Auto-turbo"));
    _autoTurbo->setHintToolTip( tr("When checked the Turbo-mode will be enabled automatically when playback is started and disabled "
                                   "when finished.") );
    _autoTurbo->setDefaultValue(false);
    _nodegraphTab->addKnob(_autoTurbo);

    _snapNodesToConnections = _publicInterface->createKnob<KnobBool>("enableSnapToNode");
    _snapNodesToConnections->setLabel(tr("Snap to node"));
    _snapNodesToConnections->setHintToolTip( tr("When moving nodes on the node graph, snap to positions where they are lined up "
                                                "with the inputs and output nodes.") );
    _snapNodesToConnections->setDefaultValue(true);
    _nodegraphTab->addKnob(_snapNodesToConnections);


    _maxUndoRedoNodeGraph = _publicInterface->createKnob<KnobInt>("maxUndoRedo");
    _maxUndoRedoNodeGraph->setLabel(tr("Maximum undo/redo for the node graph"));
    _maxUndoRedoNodeGraph->disableSlider();
    _maxUndoRedoNodeGraph->setRange(0, INT_MAX);
    _maxUndoRedoNodeGraph->setHintToolTip( tr("Set the maximum of events related to the node graph %1 "
                                              "remembers. Past this limit, older events will be deleted forever, "
                                              "allowing to re-use the RAM for other purposes. \n"
                                              "Changing this value will clear the undo/redo stack.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _maxUndoRedoNodeGraph->setDefaultValue(20);
    _nodegraphTab->addKnob(_maxUndoRedoNodeGraph);


    _disconnectedArrowLength = _publicInterface->createKnob<KnobInt>("disconnectedArrowLength");
    _disconnectedArrowLength->setLabel(tr("Disconnected arrow length"));
    _disconnectedArrowLength->setHintToolTip( tr("The size of a disconnected node input arrow in pixels.") );
    _disconnectedArrowLength->disableSlider();
    _disconnectedArrowLength->setDefaultValue(30);

    _nodegraphTab->addKnob(_disconnectedArrowLength);

    _hideOptionalInputsAutomatically = _publicInterface->createKnob<KnobBool>("autoHideInputs");
    _hideOptionalInputsAutomatically->setLabel(tr("Auto hide masks inputs"));
    _hideOptionalInputsAutomatically->setHintToolTip( tr("When checked, any diconnected mask input of a node in the nodegraph "
                                                         "will be visible only when the mouse is hovering the node or when it is "
                                                         "selected.") );
    _hideOptionalInputsAutomatically->setDefaultValue(true);
    _nodegraphTab->addKnob(_hideOptionalInputsAutomatically);

    _useInputAForMergeAutoConnect = _publicInterface->createKnob<KnobBool>("mergeConnectToA");
    _useInputAForMergeAutoConnect->setLabel(tr("Merge node connect to A input"));
    _useInputAForMergeAutoConnect->setHintToolTip( tr("If checked, upon creation of a new Merge node, or any other node with inputs named "
                                                      "A and B, input A is be preferred "
                                                      "for auto-connection. When the node is disabled, B is always output, whether this is checked or not.") );
    _useInputAForMergeAutoConnect->setDefaultValue(true);
    _nodegraphTab->addKnob(_useInputAForMergeAutoConnect);
} // Settings::initializeKnobsNodeGraph

void
SettingsPrivate::initializeKnobsCaching()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    /////////// Caching tab
    _cachingTab = _publicInterface->createKnob<KnobPage>("cachingPage");
    _cachingTab->setLabel(tr("Caching"));

    _aggressiveCaching = _publicInterface->createKnob<KnobBool>("aggressiveCaching");
    _aggressiveCaching->setLabel(tr("Aggressive Caching"));
    _aggressiveCaching->setHintToolTip( tr("When checked, %1 will cache the output of all images "
                                           "rendered by all nodes, regardless of their \"Force caching\" parameter. When enabling this option "
                                           "you need to have at least 8GiB of RAM, and 16GiB is recommended.\n"
                                           "If not checked, %1 will only cache the nodes when needed").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _aggressiveCaching->setDefaultValue(false);

    _cachingTab->addKnob(_aggressiveCaching);


    _maxDiskCacheSizeGb = _publicInterface->createKnob<KnobInt>("maxDiskCacheGb");
    _maxDiskCacheSizeGb->setLabel(tr("Maximum Disk Cache Size (GiB)"));
    _maxDiskCacheSizeGb->disableSlider();

    // The disk should at least allow storage of 1000 tiles accross each bucket
    std::size_t cacheMinSize = NATRON_TILE_SIZE_BYTES;
    cacheMinSize = (cacheMinSize * 1024 * 256) / (1024 * 1024 * 1024);
    _maxDiskCacheSizeGb->setRange(cacheMinSize, INT_MAX);
    _maxDiskCacheSizeGb->setHintToolTip( tr("The maximum Disk size that may be used by the Cache (in GiB)") );
    _maxDiskCacheSizeGb->setDefaultValue(8);

    _cachingTab->addKnob(_maxDiskCacheSizeGb);


    _diskCachePath = _publicInterface->createKnob<KnobPath>("diskCachePath");
    _diskCachePath->setLabel(tr("Disk Cache Path (empty = default)"));
    _diskCachePath->setMultiPath(false);

    QString defaultLocation = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache);
    QString diskCacheTt( tr("This is the location where the disk cache is. "
                            "This variable should point to your fastest disk. "
                            "You may override this setting by setting the NATRON_DISK_CACHE_PATH system environment variable when before launching Natron. "
                            "If the location is not specified or does not exist on the filesystem, "
                            "the default location will be used.\nThe default location is: %1\n").arg(defaultLocation) +
                         QLatin1Char('\n') +
                         tr("Changing this requires a restart of the application to take effect.") );

    _diskCachePath->setHintToolTip(diskCacheTt);
    _knobsRequiringRestart.insert(_diskCachePath);

    _cachingTab->addKnob(_diskCachePath);


} // Settings::initializeKnobsCaching

void
SettingsPrivate::initializeKnobsPlugins()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _pluginsTab = _publicInterface->createKnob<KnobPage>("pluginsPage");
    _pluginsTab->setLabel(tr("Plug-ins"));

#if defined(__linux__) || defined(__FreeBSD__)
    std::string searchPath("/usr/OFX/Plugins");
#elif defined(__APPLE__)
    std::string searchPath("/Library/OFX/Plugins");
#elif defined(WINDOWS)

    std::wstring basePath = std::wstring( OFX::Host::PluginCache::getStdOFXPluginPath() );
    basePath.append( std::wstring(L" and C:\\Program Files\\Common Files\\OFX\\Plugins") );
    std::string searchPath = StrUtils::utf16_to_utf8(basePath);

#endif

    _loadBundledPlugins = _publicInterface->createKnob<KnobBool>("useBundledPlugins");
    _knobsRequiringRestart.insert(_loadBundledPlugins);
    _knobsRequiringOFXCacheClear.insert(_loadBundledPlugins);
    _loadBundledPlugins->setLabel(tr("Use bundled plug-ins"));
    _loadBundledPlugins->setHintToolTip( tr("When checked, %1 also uses the plug-ins bundled "
                                            "with the binary distribution.\n"
                                            "When unchecked, only system-wide plug-ins found in are loaded (more information can be "
                                            "found in the help for the \"Extra plug-ins search paths\" setting).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) +
                                         QLatin1Char('\n') +
                                         tr("Changing this requires a restart of the application to take effect.") );
    _loadBundledPlugins->setDefaultValue(true);
    _pluginsTab->addKnob(_loadBundledPlugins);

    _preferBundledPlugins = _publicInterface->createKnob<KnobBool>("preferBundledPlugins");
    _knobsRequiringRestart.insert(_preferBundledPlugins);
    _knobsRequiringOFXCacheClear.insert(_preferBundledPlugins);
    _preferBundledPlugins->setLabel(tr("Prefer bundled plug-ins over system-wide plug-ins"));
    _preferBundledPlugins->setHintToolTip( tr("When checked, and if \"Use bundled plug-ins\" is also checked, plug-ins bundled with "
                                              "the %1 binary distribution will take precedence over system-wide plug-ins "
                                              "if they have the same internal ID.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) +
                                           QLatin1Char('\n') +
                                           tr("Changing this requires a restart of the application to take effect.") );
    _preferBundledPlugins->setDefaultValue(true);
    _pluginsTab->addKnob(_preferBundledPlugins);

    _useStdOFXPluginsLocation = _publicInterface->createKnob<KnobBool>("useStdOFXPluginsLocation");
    _knobsRequiringRestart.insert(_useStdOFXPluginsLocation);
    _knobsRequiringOFXCacheClear.insert(_useStdOFXPluginsLocation);
    _useStdOFXPluginsLocation->setLabel(tr("Enable default OpenFX plugins location"));
    _useStdOFXPluginsLocation->setHintToolTip( tr("When checked, %1 also uses the OpenFX plug-ins found in the default location (%2).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) +
                                               QLatin1Char('\n') +
                                               tr("Changing this requires a restart of the application to take effect.") );
    _useStdOFXPluginsLocation->setDefaultValue(true);
    _pluginsTab->addKnob(_useStdOFXPluginsLocation);

    _extraPluginPaths = _publicInterface->createKnob<KnobPath>("extraPluginsSearchPaths");
    _knobsRequiringRestart.insert(_extraPluginPaths);
    _knobsRequiringOFXCacheClear.insert(_extraPluginPaths);
    _extraPluginPaths->setLabel(tr("OpenFX plug-ins search path"));
    _extraPluginPaths->setHintToolTip( tr("Extra search paths where %1 should scan for OpenFX plug-ins. "
                                          "Extra plug-ins search paths can also be specified using the OFX_PLUGIN_PATH environment variable.\n"
                                          "The priority order for system-wide plug-ins, from high to low, is:\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is checked)\n"
                                          "- plug-ins found in OFX_PLUGIN_PATH\n"
                                          "- plug-ins found in %2 (if \"Enable default OpenFX plug-ins location\" is checked)\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is not checked)").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) +
                                       QLatin1Char('\n') +
                                       tr("Changing this requires a restart of the application to take effect.") );
    _extraPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_extraPluginPaths);

    _templatesPluginPaths = _publicInterface->createKnob<KnobPath>("groupPluginsSearchPath");
    _templatesPluginPaths->setLabel(tr("PyPlugs search path"));
    _knobsRequiringRestart.insert(_templatesPluginPaths);
    _templatesPluginPaths->setHintToolTip( tr("Search path where %1 should scan for Python group scripts (PyPlugs). "
                                              "The search paths for groups can also be specified using the "
                                              "NATRON_PLUGIN_PATH environment variable.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) +
                                           QLatin1Char('\n') +
                                           tr("Changing this requires a restart of the application to take effect.") );
    _templatesPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_templatesPluginPaths);


} // Settings::initializeKnobsPlugins

void
SettingsPrivate::initializeKnobsPython()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _pythonPage = _publicInterface->createKnob<KnobPage>("pythonPage");
    _pythonPage->setLabel(tr("Python"));


    _onProjectCreated = _publicInterface->createKnob<KnobString>("afterProjectCreated");
    _onProjectCreated->setLabel(tr("After project created"));
    _onProjectCreated->setHintToolTip( tr("Callback called once a new project is created (this is never called "
                                          "when \"After project loaded\" is called.)\n"
                                          "The signature of the callback is : callback(app) where:\n"
                                          "- app: points to the current application instance\n") );
    _pythonPage->addKnob(_onProjectCreated);


    _defaultOnProjectLoaded = _publicInterface->createKnob<KnobString>("defOnProjectLoaded");
    _defaultOnProjectLoaded->setLabel(tr("Default after project loaded"));
    _defaultOnProjectLoaded->setHintToolTip( tr("The default afterProjectLoad callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectLoaded);

    _defaultOnProjectSave = _publicInterface->createKnob<KnobString>("defOnProjectSave");
    _defaultOnProjectSave->setLabel(tr("Default before project save") );
    _defaultOnProjectSave->setHintToolTip( tr("The default beforeProjectSave callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectSave);


    _defaultOnProjectClose = _publicInterface->createKnob<KnobString>("defOnProjectClose" );
    _defaultOnProjectClose->setLabel(tr("Default before project close"));
    _defaultOnProjectClose->setHintToolTip( tr("The default beforeProjectClose callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectClose);


    _defaultOnNodeCreated = _publicInterface->createKnob<KnobString>("defOnNodeCreated");
    _defaultOnNodeCreated->setLabel(tr("Default after node created"));
    _defaultOnNodeCreated->setHintToolTip( tr("The default afterNodeCreated callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnNodeCreated);


    _defaultOnNodeDelete = _publicInterface->createKnob<KnobString>("defOnNodeDelete");
    _defaultOnNodeDelete->setLabel(tr("Default before node removal"));
    _defaultOnNodeDelete->setHintToolTip( tr("The default beforeNodeRemoval callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnNodeDelete);


    _echoVariableDeclarationToPython = _publicInterface->createKnob<KnobBool>("printAutoDeclaredVars");
    _echoVariableDeclarationToPython->setLabel(tr("Print auto-declared variables in the Script Editor"));
    _echoVariableDeclarationToPython->setHintToolTip( tr("When checked, %1 will print in the Script Editor all variables that are "
                                                         "automatically declared, such as the app variable or node attributes.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _echoVariableDeclarationToPython->setDefaultValue(false);
    _pythonPage->addKnob(_echoVariableDeclarationToPython);
} // initializeKnobs

std::string
SettingsPrivate::getSettingsAbsoluteFilepath(const std::string& userDataDirectoryPath)
{
    std::stringstream ss;
    ss << userDataDirectoryPath << NATRON_SETTINGS_FILENAME;
    return ss.str();
}

std::string
Settings::getSettingsAbsoluteFilePath() const
{
    std::string userDataDirPath = _imp->ensureUserDataDirectory();
    return _imp->getSettingsAbsoluteFilepath(userDataDirPath);
}

void
Settings::setSaveSettings(bool enable)
{
    _imp->saveSettings = enable;
}

bool
Settings::getSaveSettings() const
{
    return _imp->saveSettings;
}

void
Settings::saveSettingsToFile()
{
    if (!_imp->saveSettings) {
        return;
    }

    std::string userDataDirPath = _imp->ensureUserDataDirectory();
    std::string settingsFilePath = _imp->getSettingsAbsoluteFilepath(userDataDirPath);

    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, settingsFilePath);

    if (!ofile) {
        std::string message = tr("Failed to save settings to %1").arg(QString::fromUtf8(settingsFilePath.c_str())).toStdString();
        Dialogs::errorDialog(tr("Settings").toStdString(), message);
        return;
    }

    bool hasWrittenSomething = false;


    SERIALIZATION_NAMESPACE::SettingsSerialization serialization;
    serialization.pluginsData = _imp->pluginsData;

    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (!(*it)->getIsPersistent()) {
            continue;
        }
        if (!(*it)->hasModifications()) {
            continue;
        }
        SERIALIZATION_NAMESPACE::KnobSerializationPtr k(new SERIALIZATION_NAMESPACE::KnobSerialization);
        (*it)->toSerialization(k.get());
        if (!k->_mustSerialize) {
            continue;
        }
        serialization.knobs.push_back(k);
        hasWrittenSomething = true;

    } // for all knobs

    const PluginsMap& plugins = appPTR->getPluginsList();
    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        assert(it->second.size() > 0);

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            const PluginPtr& plugin = *itver;

            SERIALIZATION_NAMESPACE::SettingsSerialization::PluginData data;
            data.enabled = plugin->isEnabled();
            data.renderScaleEnabled = plugin->isRenderScaleEnabled();
            data.multiThreadingEnabled = plugin->isMultiThreadingEnabled();
            data.openGLEnabled = plugin->isOpenGLEnabled();

            if (!data.enabled || !data.renderScaleEnabled || !data.multiThreadingEnabled || !data.openGLEnabled) {
                SERIALIZATION_NAMESPACE::SettingsSerialization::PluginID id;
                id.identifier = plugin->getPluginID();
                id.majorVersion = plugin->getMajorVersion();
                id.minorVersion = plugin->getMinorVersion();
                serialization.pluginsData[id] = data;
                hasWrittenSomething = true;
            }
        }
    }

    for (ApplicationShortcutsMap::const_iterator it = _imp->shortcuts.begin(); it != _imp->shortcuts.end(); ++it) {
        for (GroupShortcutsMap::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {


            if (it2->second.modifiers == it2->second.defaultModifiers && it2->second.currentShortcut == it2->second.defaultShortcut) {
                continue;
            }
            
            SERIALIZATION_NAMESPACE::SettingsSerialization::KeybindShortcut shortcut;
            SERIALIZATION_NAMESPACE::SettingsSerialization::KeybindShortcutKey key;
            key.grouping = it->first;
            key.actionID = it2->first;

            shortcut.modifiers = KeybindShortcut::modifiersToStringList(it2->second.modifiers);
            shortcut.symbol = KeybindShortcut::keySymbolToString(it2->second.currentShortcut);
            hasWrittenSomething = true;

        }
    }

    if (!hasWrittenSomething) {
        ofile.close();
        QFile::remove(QString::fromUtf8(settingsFilePath.c_str()));
    } else {
        SERIALIZATION_NAMESPACE::write(ofile, serialization, NATRON_SETTINGS_FILE_HEADER);
    }


} // saveSettingsToFile


void
SettingsPrivate::loadSettingsFromFileInternal(const SERIALIZATION_NAMESPACE::SettingsSerialization& serialization, int loadType)
{
    // Restore all knobs with a serialization
    const KnobsVec& knobs = _publicInterface->getKnobs();

    if (loadType & Settings::eLoadSettingsTypeKnobs) {
        for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = serialization.knobs.begin(); it != serialization.knobs.end(); ++it) {
            KnobIPtr knob;
            for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
                if ((*it2)->getName() == (*it)->_scriptName) {
                    knob = *it2;
                    break;
                }
            }
            if (!knob) {
                continue;
            }
            knob->fromSerialization(**it);
        } // for all serialized knobs
    }

    pluginsData = serialization.pluginsData;

    if (loadType & Settings::eLoadSettingsTypePlugins) {
        for (SERIALIZATION_NAMESPACE::SettingsSerialization::PluginDataMap::const_iterator it = pluginsData.begin(); it != pluginsData.end(); ++it) {

            PluginPtr plugin;
            try {
                plugin = appPTR->getPluginBinary(QString::fromUtf8(it->first.identifier.c_str()), it->first.majorVersion, it->first.minorVersion);
            } catch (...) {
                continue;
            }
            if (!plugin) {
                continue;
            }

            plugin->setEnabled(it->second.enabled);
            plugin->setRenderScaleEnabled(it->second.renderScaleEnabled);
            plugin->setMultiThreadingEnabled(it->second.multiThreadingEnabled);
            plugin->setOpenGLEnabled(it->second.openGLEnabled);
            
            
        } // for all plug-ins data
    }

    if (loadType & Settings::eLoadSettingsTypeShortcuts) {
         for (SERIALIZATION_NAMESPACE::SettingsSerialization::KeybindsShortcutMap::const_iterator it = serialization.shortcuts.begin(); it != serialization.shortcuts.end(); ++it) {
             KeyboardModifiers modifiers = KeybindShortcut::modifiersFromStringList(it->second.modifiers);
             Key symbol = KeybindShortcut::keySymbolFromString(it->second.symbol);
             _publicInterface->setShortcutKeybind(it->first.grouping, it->first.actionID, modifiers, symbol);
         }
    }
} // loadSettingsFromFileInternal



void
Settings::loadSettingsFromFile(LoadSettingsType loadType)
{
    _imp->_restoringSettings = true;



    std::string userDataDirPath = _imp->ensureUserDataDirectory();
    std::string settingsFilePath = _imp->getSettingsAbsoluteFilepath(userDataDirPath);

    if (QFile::exists(QString::fromUtf8(settingsFilePath.c_str()))) {
        FStreamsSupport::ifstream ifile;
        FStreamsSupport::open(&ifile, settingsFilePath);

        SERIALIZATION_NAMESPACE::SettingsSerialization serialization;
        try {
            SERIALIZATION_NAMESPACE::read(NATRON_SETTINGS_FILE_HEADER, ifile, &serialization);
        } catch (SERIALIZATION_NAMESPACE::InvalidSerializationFileException& e) {
            Dialogs::errorDialog(tr("Settings").toStdString(), tr("Failed to open %1: This file does not appear to be a settings file").arg(QString::fromUtf8(settingsFilePath.c_str())).toStdString());
            return;
        } catch (...) {
            Dialogs::errorDialog(tr("Settings").toStdString(), tr("Failed to open %1: File could not be decoded").arg(QString::fromUtf8(settingsFilePath.c_str())).toStdString());
            return;
        }
        _imp->loadSettingsFromFileInternal(serialization, loadType);
    } // file exists


    if (loadType & Settings::eLoadSettingsTypeKnobs) {


        // Restore number of threads
        _imp->restoreNumThreads();


        // If the appearance changed, flag it
        try {
            int appearanceVersion = _imp->_defaultAppearanceVersion->getValue();
            if (appearanceVersion < NATRON_DEFAULT_APPEARANCE_VERSION) {
                _imp->_defaultAppearanceOutdated = true;
                _imp->_defaultAppearanceVersion->setValue(NATRON_DEFAULT_APPEARANCE_VERSION);
            }
        } catch (std::logic_error) {
            // ignore
        }

        // Ensure the texture mode of the Viewer cannot be selected if the user gfx card
        // does not support 32 bit fp textures.
        if (!appPTR->isTextureFloatSupported()) {
            if (_imp->_texturesMode) {
                _imp->_texturesMode->setSecret(true);
            }
        }

    }

    if ((loadType & Settings::eLoadSettingsTypeKnobs) || loadType == Settings::eLoadSettingsNone) {

        // Load OCIO config even if there's no serialization
        _imp->tryLoadOpenColorIOConfig();

    }

    _imp->_restoringSettings = false;


} // loadSettingsFromFile

void
SettingsPrivate::setOpenGLRenderersInfo()
{
    std::vector<ChoiceOption> availableRenderers = _availableOpenGLRenderers->getEntries();
    QString missingGLError;
    bool hasGL = appPTR->hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering, &missingGLError);

    if ( availableRenderers.empty() || !hasGL) {
        if (missingGLError.isEmpty()) {
            _openglRendererString->setValue( tr("OpenGL rendering disabled: No device meeting %1 requirements could be found.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        } else {
            _openglRendererString->setValue(missingGLError.toStdString());
        }
    }
    int curIndex = _availableOpenGLRenderers->getValue();
    if ( (curIndex < 0) || ( curIndex >= (int)availableRenderers.size() ) ) {
        return;
    }
    const std::list<OpenGLRendererInfo>& renderers = appPTR->getOpenGLRenderers();
    int i = 0;
    for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
        if (i == curIndex) {

            QString maxMemoryString = it->maxMemBytes == 0 ? tr("Unknown") : printAsRAM(it->maxMemBytes);
            QString curRenderer = (QString::fromUtf8("<p><h2>") +
                                   tr("OpenGL Renderer Infos:") +
                                   QString::fromUtf8("</h2></p><p><b>") +
                                   tr("Vendor:") +
                                   QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->vendorName.c_str() ) ) +
                                   tr("Renderer:") +
                                   QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->rendererName.c_str() ) ) +
                                   tr("OpenGL Version:") +
                                   QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->glVersionString.c_str() ) ) +
                                   tr("GLSL Version:") +
                                   QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->glslVersionString.c_str() ) ) +
                                   tr("Max. Memory:") +
                                   QString::fromUtf8("</b> %1</p><p><b>").arg(maxMemoryString) +
                                   tr("Max. Texture Size (px):") +
                                   QString::fromUtf8("</b> %5</p<").arg(it->maxTextureSize));
            _openglRendererString->setValue( curRenderer.toStdString() );
            break;
        }
    }

} // setOpenGLRenderersInfo

std::string
SettingsPrivate::ensureUserDataDirectory()
{
    // Create the ~/NatronUserData directory if it doesn't exist.
    QDir homeDir = QDir::home();
    QString userDataDirPath = QString::fromUtf8("%1UserData").arg(QString::fromUtf8(NATRON_APPLICATION_NAME));
    if (!homeDir.exists(userDataDirPath)) {
        homeDir.mkdir(userDataDirPath);
    }
    QString sep = QString::fromUtf8("/");
    return QString(homeDir.absolutePath() + sep + userDataDirPath + sep).toStdString();
}

bool
SettingsPrivate::tryLoadOpenColorIOConfig()
{
    // the default value is the environment variable "OCIO"
    QString configFile = QFile::decodeName( qgetenv(NATRON_OCIO_ENV_VAR_NAME) );

    // OCIO environment variable overrides everything, then try the custom config...
    if ( configFile.isEmpty() && _customOcioConfigFile->isEnabled() ) {
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
        configFile = QString::fromUtf8( file.c_str() );
    }
    if ( !configFile.isEmpty() ) {
        if ( !QFile::exists(configFile) )  {
            Dialogs::errorDialog( "OpenColorIO", tr("%1: No such file.").arg(configFile).toStdString() );

            return false;
        }
    } else {
        // ... and finally try the setting from the choice menu.
        try {
            ///try to load from the combobox
            QString activeEntryText  = QString::fromUtf8( _ocioConfigKnob->getCurrentEntry().id.c_str() );
            QString configFileName = QString( activeEntryText + QString::fromUtf8(".ocio") );
            QStringList defaultConfigsPaths = getDefaultOcioConfigPaths();
            Q_FOREACH(const QString &defaultConfigsDirStr, defaultConfigsPaths) {
                QDir defaultConfigsDir(defaultConfigsDirStr);

                if ( !defaultConfigsDir.exists() ) {
                    qDebug() << "Attempt to read an OpenColorIO configuration but the configuration directory"
                             << defaultConfigsDirStr << "does not exist.";
                    continue;
                }
                ///try to open the .ocio config file first in the defaultConfigsDir
                ///if we can't find it, try to look in a subdirectory with the name of the config for the file config.ocio
                if ( !defaultConfigsDir.exists(configFileName) ) {
                    QDir subDir(defaultConfigsDirStr + QDir::separator() + activeEntryText);
                    if ( !subDir.exists() ) {
                        Dialogs::errorDialog( "OpenColorIO", tr("%1: No such file or directory.").arg( subDir.absoluteFilePath( QString::fromUtf8("config.ocio") ) ).toStdString() );

                        return false;
                    }
                    if ( !subDir.exists( QString::fromUtf8("config.ocio") ) ) {
                        Dialogs::errorDialog( "OpenColorIO", tr("%1: No such file or directory.").arg( subDir.absoluteFilePath( QString::fromUtf8("config.ocio") ) ).toStdString() );

                        return false;
                    }
                    configFile = subDir.absoluteFilePath( QString::fromUtf8("config.ocio") );
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
#ifdef DEBUG
    qDebug() << "setting OCIO=" << configFile;
#endif
    std::string stdConfigFile = configFile.toStdString();
#if 0 //def __NATRON_WIN32__
    _wputenv_s(L"OCIO", StrUtils::utf8_to_utf16(stdConfigFile).c_str());
#else
    qputenv( NATRON_OCIO_ENV_VAR_NAME, stdConfigFile.c_str() );
#endif

    std::string configPath = SequenceParsing::removePath(stdConfigFile);
    if ( !configPath.empty() && (configPath[configPath.size() - 1] == '/') ) {
        configPath.erase(configPath.size() - 1, 1);
    }
    appPTR->onOCIOConfigPathChanged(configPath);

    return true;
} // tryLoadOpenColorIOConfig

inline
void
crash_application()
{
    std::cerr << "CRASHING APPLICATION NOW UPON USER REQUEST!" << std::endl;
    volatile int* a = (int*)(NULL);

    // coverity[var_deref_op]
    *a = 1;
}

void
SettingsPrivate::restoreNumThreads()
{
    int nbThreads = _numberOfThreads->getValue();
#ifdef DEBUG
    if (nbThreads == -1) {
        nbThreads = 1;
    }
#endif
    assert(nbThreads >= 0);
    if (nbThreads == 0) {
        int idealCount = appPTR->getHardwareIdealThreadCount();
        assert(idealCount > 0);
        idealCount = std::max(idealCount, 1);
        QThreadPool::globalInstance()->setMaxThreadCount(idealCount);
    } else {
        QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
    }
}

void
SettingsPrivate::refreshCacheSize()
{
    CacheBasePtr tileCache = appPTR->getTileCache();
    if (tileCache) {
        tileCache->setMaximumCacheSize(_publicInterface->getTileCacheSize());
    }

    CacheBasePtr cache = appPTR->getGeneralPurposeCache();
    if (cache) {
        cache->setMaximumCacheSize(_publicInterface->getGeneralPurposeCacheSize());
    }
}

std::size_t
Settings::getGeneralPurposeCacheSize() const
{
    std::size_t kb = 1024;
    std::size_t mb = kb * kb;
    std::size_t maxGeneralPurposeCache = 128 * mb;
    return maxGeneralPurposeCache;
}

std::size_t
Settings::getTileCacheSize() const
{
    std::size_t kb = 1024;
    std::size_t mb = kb * kb;
    std::size_t gb = mb * kb;
    std::size_t maxDiskBytes = (std::size_t)_imp->_maxDiskCacheSizeGb->getValue() * gb;
    return maxDiskBytes;
}

bool
Settings::onKnobValueChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             TimeValue /*time*/,
                             ViewSetSpec /*view*/)
{

    Q_EMIT settingChanged(k, reason);
    bool ret = true;

    if ( k == _imp->_maxDiskCacheSizeGb ) {
        _imp->refreshCacheSize();
    }  else if ( k == _imp->_numberOfThreads ) {
        _imp->restoreNumThreads();
    } else if ( k == _imp->_ocioConfigKnob ) {
        if (_imp->_ocioConfigKnob->getCurrentEntry().id == NATRON_CUSTOM_OCIO_CONFIG_NAME) {
            _imp->_customOcioConfigFile->setEnabled(true);
        } else {
            _imp->_customOcioConfigFile->setEnabled(false);
        }
        _imp->tryLoadOpenColorIOConfig();
    } else if ( k == _imp->_customOcioConfigFile ) {
        if ( _imp->_customOcioConfigFile->isEnabled() ) {
            _imp->tryLoadOpenColorIOConfig();
        }
    } else if ( k == _imp->_maxUndoRedoNodeGraph ) {
        appPTR->setUndoRedoStackLimit( _imp->_maxUndoRedoNodeGraph->getValue() );
    } else if ( k == _imp->_maxPanelsOpened ) {
        appPTR->onMaxPanelsOpenedChanged( _imp->_maxPanelsOpened->getValue() );
    } else if ( k == _imp->_queueRenders ) {
        appPTR->onQueueRendersChanged( _imp->_queueRenders->getValue() );
    } else if ( ( k == _imp->_checkerboardTileSize ) || ( k == _imp->_checkerboardColor1 ) || ( k == _imp->_checkerboardColor2 ) ) {
        appPTR->onCheckerboardSettingsChanged();
    } else if ( k == _imp->_texturesMode &&  !_imp->_restoringSettings) {
        appPTR->clearAllCaches();
    } else if ( ( k == _imp->_hideOptionalInputsAutomatically ) && !_imp->_restoringSettings && (reason == eValueChangedReasonUserEdited) ) {
        appPTR->toggleAutoHideGraphInputs();
    } else if ( k == _imp->_autoProxyWhenScrubbingTimeline ) {
        _imp->_autoProxyLevel->setSecret( !_imp->_autoProxyWhenScrubbingTimeline->getValue() );
    }  else if ( k == _imp->_hostName ) {
        ChoiceOption hostName = _imp->_hostName->getCurrentEntry();
        bool isCustom = hostName.id == NATRON_CUSTOM_HOST_NAME_ENTRY;
        _imp->_customHostName->setSecret(!isCustom);
#ifdef NATRON_USE_BREAKPAD
    } else if ( ( k == _imp->_testCrashReportButton ) && (reason == eValueChangedReasonUserEdited) ) {
        StandardButtonEnum reply = Dialogs::questionDialog( tr("Crash Test").toStdString(),
                                                           tr("You are about to make %1 crash to test the reporting system.\n"
                                                              "Do you really want to crash?").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), false,
                                                           StandardButtons(eStandardButtonYes | eStandardButtonNo) );
        if (reply == eStandardButtonYes) {
            crash_application();
        }
#endif
    } else if ( ( k == _imp->_scriptEditorFontChoice ) || ( k == _imp->_scriptEditorFontSize ) ) {
        appPTR->reloadScriptEditorFonts();
    } else if ( k == _imp->_enableOpenGL ) {
        appPTR->refreshOpenGLRenderingFlagOnAllInstances();
        if (!_imp->_restoringSettings) {
            appPTR->clearPluginsLoadedCache();
        }
    } else {
        ret = false;
    }

    // Check if the knob is part of the appearance tab. If so
    // refresh stylesheet
    KnobPagePtr isPage = toKnobPage(k);
    if (!isPage) {
        KnobIPtr parent = k->getParentKnob();
        while (parent) {
            if (parent == _imp->_appearanceTab) {
                if (reason == eValueChangedReasonRestoreDefault) {
                    ++_imp->_nRedrawStyleSheetRequests;
                } else {
                    appPTR->reloadStylesheets();
                }
            }
            parent = parent->getParentKnob();
        }
    }
    return ret;
} // onKnobValueChanged

////////////////////////////////////////////////////////
// "Viewers" pane
KnobChoicePtr
Settings::getViewerBitDepthKnob() const
{
    return _imp->_texturesMode;
}

ImageBitDepthEnum
Settings::getViewersBitDepth() const
{
    if (!appPTR->isTextureFloatSupported()) {
        return eImageBitDepthByte;
    }
    int v = _imp->_texturesMode->getValue();

    if (v == 0) {
        return eImageBitDepthByte;
    } else if (v == 1) {
        return eImageBitDepthFloat;
    } else {
        return eImageBitDepthByte;
    }
}

int
Settings::getCheckerboardTileSize() const
{
    return _imp->_checkerboardTileSize->getValue();
}

void
Settings::getCheckerboardColor1(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _imp->_checkerboardColor1->getValue(DimIdx(0));
    *g = _imp->_checkerboardColor1->getValue(DimIdx(1));
    *b = _imp->_checkerboardColor1->getValue(DimIdx(2));
    *a = _imp->_checkerboardColor1->getValue(DimIdx(3));
}

void
Settings::getCheckerboardColor2(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _imp->_checkerboardColor2->getValue(DimIdx(0));
    *g = _imp->_checkerboardColor2->getValue(DimIdx(1));
    *b = _imp->_checkerboardColor2->getValue(DimIdx(2));
    *a = _imp->_checkerboardColor2->getValue(DimIdx(3));
}

bool
Settings::isAutoWipeEnabled() const
{
    return _imp->_autoWipe->getValue();
}

bool
Settings::isAutoProxyEnabled() const
{
    return _imp->_autoProxyWhenScrubbingTimeline->getValue();
}

unsigned int
Settings::getAutoProxyMipMapLevel() const
{
    return (unsigned int)_imp->_autoProxyLevel->getValue() + 1;
}

int
Settings::getMaxOpenedNodesViewerContext() const
{
    return _imp->_maximumNodeViewerUIOpened->getValue();
}

bool
Settings::isViewerKeysEnabled() const
{
    return _imp->_viewerKeys->getValue();
}

///////////////////////////////////////////////////////
// "Caching" pane

std::string
Settings::getDiskCachePath() const
{
    return _imp->_diskCachePath->getValue();
}

bool
Settings::isAggressiveCachingEnabled() const
{
    return _imp->_aggressiveCaching->getValue();
}

bool
Settings::getColorPickerLinear() const
{
    return _imp->_linearPickers->getValue();
}

int
Settings::getNumberOfThreads() const
{
    return _imp->_numberOfThreads->getValue();
}

void
Settings::setNumberOfThreads(int threadsNb)
{
    _imp->_numberOfThreads->setValue(threadsNb);
}

bool
Settings::isAutoPreviewOnForNewProjects() const
{
    return _imp->_autoPreviewEnabledForNewProjects->getValue();
}

#ifdef NATRON_DOCUMENTATION_ONLINE
int
Settings::getDocumentationSource() const
{
    return _imp->_documentationSource->getValue();
}
#endif

int
Settings::getServerPort() const
{
    return _imp->_wwwServerPort->getValue();
}

void
Settings::setServerPort(int port) const
{
    _imp->_wwwServerPort->setValue(port);
}

bool
Settings::isAutoScrollEnabled() const
{
    return _imp->_autoScroll->getValue();
}

QString
Settings::makeHTMLDocumentation(bool genHTML) const
{
    QString ret;
    QString markdown;
    QTextStream ts(&ret);
    QTextStream ms(&markdown);

    ms << tr("Preferences") << "\n==========\n\n";

    const KnobsVec& knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
#pragma message WARN("TODO: restore getDefaultIsSecret from RB-2.2")
        //if ( (*it)->getDefaultIsSecret() ) {
        if ( (*it)->getIsSecret() ) {
            continue;
        }
        //QString knobScriptName = QString::fromUtf8( (*it)->getName().c_str() );
        QString knobLabel = convertFromPlainTextToMarkdown( QString::fromStdString( (*it)->getLabel() ), genHTML, false );
        QString knobHint = convertFromPlainTextToMarkdown( QString::fromStdString( (*it)->getHintToolTip() ), genHTML, false );
        KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( it->get() );
        if (isPage) {
            if (isPage->getParentKnob()) {
                ms << "### " << knobLabel << "\n\n";
            } else {
                ms << knobLabel << "\n----------\n\n";
            }
        } else if (isSep) {
            ms << "**" << knobLabel << "**\n\n";
        } else if ( !knobLabel.isEmpty() && !knobHint.isEmpty() ) {
            if ( ( knobLabel != QString::fromUtf8("Enabled") ) && ( knobLabel != QString::fromUtf8("Zoom support") ) ) {
                ms << "**" << knobLabel << "**\n\n";
                ms << knobHint << "\n\n";
            }
        }
    }

    if (genHTML) {
        ts << ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
               "  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
               "\n"
               "\n"
               "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
               "  <head>\n"
               "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
               "    \n"
               "    <title>") << tr("Preferences") << " &#8212; NATRON_DOCUMENTATION</title>\n";
        ts << ("    \n"
               "    <link rel=\"stylesheet\" href=\"_static/markdown.css\" type=\"text/css\" />\n"
               "    \n"
               "    <script type=\"text/javascript\" src=\"_static/jquery.js\"></script>\n"
               "    <script type=\"text/javascript\" src=\"_static/dropdown.js\"></script>\n"
               "    <link rel=\"index\" title=\"Index\" href=\"genindex.html\" />\n"
               "    <link rel=\"search\" title=\"Search\" href=\"search.html\" />\n"
               "  </head>\n"
               "  <body role=\"document\">\n"
               "    <div class=\"related\" role=\"navigation\" aria-label=\"related navigation\">\n"
               "      <h3>") << tr("Navigation") << "</h3>\n";
        ts << ("      <ul>\n"
               "        <li class=\"right\" style=\"margin-right: 10px\">\n"
               "          <a href=\"genindex.html\" title=\"General Index\"\n"
               "             accesskey=\"I\">") << tr("index") << "</a></li>\n";
        ts << ("        <li class=\"right\" >\n"
               "          <a href=\"py-modindex.html\" title=\"Python Module Index\"\n"
               "             >") << tr("modules") << "</a> |</li>\n";
        ts << ("        <li class=\"nav-item nav-item-0\"><a href=\"index.html\">NATRON_DOCUMENTATION</a> &#187;</li>\n"
               "          <li class=\"nav-item nav-item-1\"><a href=\"_group.html\" >") << tr("Reference Guide") << "</a> &#187;</li>\n";
        ts << ("      </ul>\n"
               "    </div>  \n"
               "\n"
               "    <div class=\"document\">\n"
               "      <div class=\"documentwrapper\">\n"
               "          <div class=\"body\" role=\"main\">\n"
               "            \n"
               "  <div class=\"section\">\n");
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixSettingsHTML(html);
        ts << ("</div>\n"
               "\n"
               "\n"
               "          </div>\n"
               "      </div>\n"
               "      <div class=\"clearer\"></div>\n"
               "    </div>\n"
               "\n"
               "    <div class=\"footer\" role=\"contentinfo\">\n"
               "        &#169; Copyright 2013-2018 The Natron documentation authors, licensed under CC BY-SA 4.0.\n"
               "    </div>\n"
               "  </body>\n"
               "</html>");
    } else {
        ts << markdown;
    }

    return ret;
} // Settings::makeHTMLDocumentation

void
Settings::populateSystemFonts(const std::vector<std::string>& fonts)
{
    std::vector<ChoiceOption> options(fonts.size());
    for (std::size_t i = 0; i < fonts.size(); ++i) {
        options[i].id = fonts[i];
    }
    _imp->_systemFontChoice->populateChoices(options);
    _imp->_scriptEditorFontChoice->populateChoices(options);
}

void
Settings::getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const
{
    assert(paths);
    try {
        _imp->_extraPluginPaths->getPaths(paths);
    } catch (std::logic_error) {
        paths->clear();
    }
}

bool
Settings::getUseStdOFXPluginsLocation() const
{
    return _imp->_useStdOFXPluginsLocation->getValue();
}

void
Settings::restoreAllSettingsToDefaults()
{
    restoreDefaultAppearance();
    restoreDefaultShortcuts();
    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr isPage = toKnobPage(*it);
        if (!isPage) {
            continue;
        }
        restorePageToDefaults(isPage);
    }
}

void
Settings::restorePageToDefaults(const KnobPagePtr& tab)
{

    _imp->_nRedrawStyleSheetRequests = 0;
    {
        ScopedChanges_RAII changes(this);
        const KnobsVec & knobs = tab->getChildren();
        for (U32 i = 0; i < knobs.size(); ++i) {
            knobs[i]->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
        }
    }

    if (tab == _imp->_pluginsTab) {
        const PluginsMap& allPlugins = appPTR->getPluginsList();
        for (PluginsMap::const_iterator it = allPlugins.begin(); it != allPlugins.end(); ++it) {
            for (PluginVersionsOrdered::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                (*it2)->setEnabled(true);
                (*it2)->setRenderScaleEnabled(true);
                (*it2)->setMultiThreadingEnabled(true);
                (*it2)->setOpenGLEnabled(true);
            }
        }
        _imp->pluginsData.clear();
    }

    if (_imp->_nRedrawStyleSheetRequests > 0) {
        appPTR->reloadStylesheets();
        _imp->_nRedrawStyleSheetRequests = 0;
    }
} // restorePageToDefaults

bool
Settings::isRenderInSeparatedProcessEnabled() const
{
    return _imp->_renderInSeparateProcess->getValue();
}

int
Settings::getMaximumUndoRedoNodeGraph() const
{
    return _imp->_maxUndoRedoNodeGraph->getValue();
}

int
Settings::getAutoSaveDelayMS() const
{
    return _imp->_autoSaveDelay->getValue() * 1000;
}

bool
Settings::isAutoSaveEnabledForUnsavedProjects() const
{
    return _imp->_autoSaveUnSavedProjects->getValue();
}

bool
Settings::isSnapToNodeEnabled() const
{
    return _imp->_snapNodesToConnections->getValue();
}

bool
Settings::isCheckForUpdatesEnabled() const
{
    return _imp->_checkForUpdates->getValue();
}

void
Settings::setCheckUpdatesEnabled(bool enabled)
{
    _imp->_checkForUpdates->setValue(enabled);
}

#ifdef NATRON_USE_BREAKPAD
bool
Settings::isCrashReportingEnabled() const
{
    return _imp->_enableCrashReports->getValue();
}
#endif

int
Settings::getMaxPanelsOpened() const
{
    return _imp->_maxPanelsOpened->getValue();
}

void
Settings::setMaxPanelsOpened(int maxPanels)
{
    _imp->_maxPanelsOpened->setValue(maxPanels);
}

bool
Settings::loadBundledPlugins() const
{
    return _imp->_loadBundledPlugins->getValue();
}

bool
Settings::preferBundledPlugins() const
{
    return _imp->_preferBundledPlugins->getValue();
}

void
Settings::getDefaultNodeColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _imp->_defaultNodeColor->getValue(DimIdx(0));
    *g = _imp->_defaultNodeColor->getValue(DimIdx(1));
    *b = _imp->_defaultNodeColor->getValue(DimIdx(2));
}

void
Settings::getDefaultBackdropColor(float *r,
                                  float *g,
                                  float *b) const
{
    *r = _imp->_defaultBackdropColor->getValue(DimIdx(0));
    *g = _imp->_defaultBackdropColor->getValue(DimIdx(1));
    *b = _imp->_defaultBackdropColor->getValue(DimIdx(2));
}

void
Settings::getGeneratorColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultGeneratorColor->getValue(DimIdx(0));
    *g = _imp->_defaultGeneratorColor->getValue(DimIdx(1));
    *b = _imp->_defaultGeneratorColor->getValue(DimIdx(2));
}

void
Settings::getReaderColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _imp->_defaultReaderColor->getValue(DimIdx(0));
    *g = _imp->_defaultReaderColor->getValue(DimIdx(1));
    *b = _imp->_defaultReaderColor->getValue(DimIdx(2));
}

void
Settings::getWriterColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _imp->_defaultWriterColor->getValue(DimIdx(0));
    *g = _imp->_defaultWriterColor->getValue(DimIdx(1));
    *b = _imp->_defaultWriterColor->getValue(DimIdx(2));
}

void
Settings::getColorGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultColorGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultColorGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultColorGroupColor->getValue(DimIdx(2));
}

void
Settings::getFilterGroupColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _imp->_defaultFilterGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultFilterGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultFilterGroupColor->getValue(DimIdx(2));
}

void
Settings::getTransformGroupColor(float *r,
                                 float *g,
                                 float *b) const
{
    *r = _imp->_defaultTransformGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultTransformGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultTransformGroupColor->getValue(DimIdx(2));
}

void
Settings::getTimeGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultTimeGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultTimeGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultTimeGroupColor->getValue(DimIdx(2));
}

void
Settings::getDrawGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultDrawGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultDrawGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultDrawGroupColor->getValue(DimIdx(2));
}

void
Settings::getKeyerGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultKeyerGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultKeyerGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultKeyerGroupColor->getValue(DimIdx(2));
}

void
Settings::getChannelGroupColor(float *r,
                               float *g,
                               float *b) const
{
    *r = _imp->_defaultChannelGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultChannelGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultChannelGroupColor->getValue(DimIdx(2));
}

void
Settings::getMergeGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultMergeGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultMergeGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultMergeGroupColor->getValue(DimIdx(2));
}

void
Settings::getViewsGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultViewsGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultViewsGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultViewsGroupColor->getValue(DimIdx(2));
}

void
Settings::getDeepGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultDeepGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultDeepGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultDeepGroupColor->getValue(DimIdx(2));
}

int
Settings::getDisconnectedArrowLength() const
{
    return _imp->_disconnectedArrowLength->getValue();
}

std::string
Settings::getHostName() const
{
    int entry_i =  _imp->_hostName->getValue();
    std::vector<ChoiceOption> entries = _imp->_hostName->getEntries();

    if ( (entry_i >= 0) && ( entry_i < (int)entries.size() ) && (entries[entry_i].id == NATRON_CUSTOM_HOST_NAME_ENTRY) ) {
        return _imp->_customHostName->getValue();
    } else {
        if ( (entry_i >= 0) && ( entry_i < (int)_imp->_knownHostNames.size() ) ) {
            return _imp->_knownHostNames[entry_i].id;

        }

        return std::string();
    }
}

const std::string&
Settings::getKnownHostName(KnownHostNameEnum e) const
{
    return _imp->_knownHostNames[(int)e].id;
}

bool
Settings::getRenderOnEditingFinishedOnly() const
{
    return _imp->_renderOnEditingFinished->getValue();
}

void
Settings::setRenderOnEditingFinishedOnly(bool render)
{
    _imp->_renderOnEditingFinished->setValue(render);
}

bool
Settings::getIconsBlackAndWhite() const
{
    return _imp->_useBWIcons->getValue();
}

std::string
Settings::getDefaultLayoutFile() const
{
    return _imp->_defaultLayoutFile->getValue();
}

bool
Settings::getLoadProjectWorkspce() const
{
    return _imp->_loadProjectsWorkspace->getValue();
}

bool
Settings::useCursorPositionIncrements() const
{
    return _imp->_useCursorPositionIncrements->getValue();
}

bool
Settings::isAutoProjectFormatEnabled() const
{
    return _imp->_firstReadSetProjectFormat->getValue();
}

bool
Settings::isAutoFixRelativeFilePathEnabled() const
{
    return _imp->_fixPathsOnProjectPathChanged->getValue();
}

bool
Settings::areRGBPixelComponentsSupported() const
{
    return _imp->_activateRGBSupport->getValue();
}

bool
Settings::isTransformConcatenationEnabled() const
{
    return _imp->_activateTransformConcatenationSupport->getValue();
}

bool
Settings::useInputAForMergeAutoConnect() const
{
    return _imp->_useInputAForMergeAutoConnect->getValue();
}

void
Settings::doOCIOStartupCheckIfNeeded()
{
    bool docheck = _imp->_ocioStartupCheck->getValue();
    AppInstancePtr mainInstance = appPTR->getTopLevelInstance();

    if (!mainInstance) {
        qDebug() << "WARNING: doOCIOStartupCheckIfNeeded() called without a AppInstance";

        return;
    }

    if (docheck && mainInstance) {
        int entry_i = _imp->_ocioConfigKnob->getValue();
        std::vector<ChoiceOption> entries = _imp->_ocioConfigKnob->getEntries();
        std::string warnText;
        if ( (entry_i < 0) || ( entry_i >= (int)entries.size() ) ) {
            warnText = tr("The current OCIO config selected in the preferences is invalid, would you like to set it to the default config (%1)?").arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString();
        } else if (entries[entry_i].id != NATRON_DEFAULT_OCIO_CONFIG_NAME) {
            warnText = tr("The current OCIO config selected in the preferences is not the default one (%1), would you like to set it to the default config?").arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString();
        } else {
            return;
        }

        bool stopAsking = false;
        StandardButtonEnum reply = mainInstance->questionDialog("OCIO config", warnText, false,
                                                                StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                                                eStandardButtonYes,
                                                                &stopAsking);
        if (stopAsking != !docheck) {
            _imp->_ocioStartupCheck->setValue(!stopAsking);
        }

        if (reply == eStandardButtonYes) {
            int defaultIndex = -1;
            for (unsigned i = 0; i < entries.size(); ++i) {
                if (entries[i].id.find(NATRON_DEFAULT_OCIO_CONFIG_NAME) != std::string::npos) {
                    defaultIndex = i;
                    break;
                }
            }
            if (defaultIndex != -1) {
                _imp->_ocioConfigKnob->setValue(defaultIndex);
            } else {
                Dialogs::warningDialog( "OCIO config", tr("The %2 OCIO config could not be found.\n"
                                                          "This is probably because you're not using the OpenColorIO-Configs folder that should "
                                                          "be bundled with your %1 installation.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString() );
            }
        }
    }
} // Settings::doOCIOStartupCheckIfNeeded

bool
Settings::notifyOnFileChange() const
{
    return _imp->_notifyOnFileChange->getValue();
}

bool
Settings::isAutoTurboEnabled() const
{
    return _imp->_autoTurbo->getValue();
}

void
Settings::setAutoTurboModeEnabled(bool e)
{
    _imp->_autoTurbo->setValue(e);
}

void
Settings::setOptionalInputsAutoHidden(bool hidden)
{
    _imp->_hideOptionalInputsAutomatically->setValue(hidden);
}

bool
Settings::areOptionalInputsAutoHidden() const
{
    return _imp->_hideOptionalInputsAutomatically->getValue();
}

void
Settings::getPythonGroupsSearchPaths(std::list<std::string>* templates) const
{
    _imp->_templatesPluginPaths->getPaths(templates);
}

void
Settings::appendPythonGroupsPath(const std::string& path)
{
    _imp->_templatesPluginPaths->appendPath(path);
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.setValue( QString::fromUtf8( _imp->_templatesPluginPaths->getName().c_str() ), QVariant( QString::fromUtf8( _imp->_templatesPluginPaths->getValue(DimIdx(0)).c_str() ) ) );
}

std::string
Settings::getDefaultOnProjectLoadedCB()
{
    return _imp->_defaultOnProjectLoaded->getValue();
}

std::string
Settings::getDefaultOnProjectSaveCB()
{
    return _imp->_defaultOnProjectSave->getValue();
}

std::string
Settings::getDefaultOnProjectCloseCB()
{
    return _imp->_defaultOnProjectClose->getValue();
}

std::string
Settings::getDefaultOnNodeCreatedCB()
{
    return _imp->_defaultOnNodeCreated->getValue();
}

std::string
Settings::getDefaultOnNodeDeleteCB()
{
    return _imp->_defaultOnNodeDelete->getValue();
}

std::string
Settings::getOnProjectCreatedCB()
{
    return _imp->_onProjectCreated->getValue();
}

bool
Settings::isAutoDeclaredVariablePrintActivated() const
{
    return _imp->_echoVariableDeclarationToPython->getValue();
}

void
Settings::setAutoDeclaredVariablePrintEnabled(bool enabled)
{
    _imp->_echoVariableDeclarationToPython->setValue(enabled);
}

bool
Settings::isPluginIconActivatedOnNodeGraph() const
{
    return _imp->_usePluginIconsInNodeGraph->getValue();
}

bool
Settings::isNodeGraphAntiAliasingEnabled() const
{
    return _imp->_useAntiAliasing->getValue();
}

void
Settings::getSunkenColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _imp->_sunkenColor->getValue(DimIdx(0));
    *g = _imp->_sunkenColor->getValue(DimIdx(1));
    *b = _imp->_sunkenColor->getValue(DimIdx(2));
}

void
Settings::getBaseColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_baseColor->getValue(DimIdx(0));
    *g = _imp->_baseColor->getValue(DimIdx(1));
    *b = _imp->_baseColor->getValue(DimIdx(2));
}

void
Settings::getRaisedColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _imp->_raisedColor->getValue(DimIdx(0));
    *g = _imp->_raisedColor->getValue(DimIdx(1));
    *b = _imp->_raisedColor->getValue(DimIdx(2));
}

void
Settings::getSelectionColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_selectionColor->getValue(DimIdx(0));
    *g = _imp->_selectionColor->getValue(DimIdx(1));
    *b = _imp->_selectionColor->getValue(DimIdx(2));
}

void
Settings::getInterpolatedColor(double* r,
                               double* g,
                               double* b) const
{
    *r = _imp->_interpolatedColor->getValue(DimIdx(0));
    *g = _imp->_interpolatedColor->getValue(DimIdx(1));
    *b = _imp->_interpolatedColor->getValue(DimIdx(2));
}

void
Settings::getKeyframeColor(double* r,
                           double* g,
                           double* b) const
{
    *r = _imp->_keyframeColor->getValue(DimIdx(0));
    *g = _imp->_keyframeColor->getValue(DimIdx(1));
    *b = _imp->_keyframeColor->getValue(DimIdx(2));
}

void
Settings::getTrackerKeyframeColor(double* r,
                                  double* g,
                                  double* b) const
{
    *r = _imp->_trackerKeyframeColor->getValue(DimIdx(0));
    *g = _imp->_trackerKeyframeColor->getValue(DimIdx(1));
    *b = _imp->_trackerKeyframeColor->getValue(DimIdx(2));
}

void
Settings::getExprColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_exprColor->getValue(DimIdx(0));
    *g = _imp->_exprColor->getValue(DimIdx(1));
    *b = _imp->_exprColor->getValue(DimIdx(2));
}


void
Settings::getCloneColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_cloneColor->getValue(DimIdx(0));
    *g = _imp->_cloneColor->getValue(DimIdx(1));
    *b = _imp->_cloneColor->getValue(DimIdx(2));
}

void
Settings::getTextColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_textColor->getValue(DimIdx(0));
    *g = _imp->_textColor->getValue(DimIdx(1));
    *b = _imp->_textColor->getValue(DimIdx(2));
}

void
Settings::getAltTextColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _imp->_altTextColor->getValue(DimIdx(0));
    *g = _imp->_altTextColor->getValue(DimIdx(1));
    *b = _imp->_altTextColor->getValue(DimIdx(2));
}

void
Settings::getTimelinePlayheadColor(double* r,
                                   double* g,
                                   double* b) const
{
    *r = _imp->_timelinePlayheadColor->getValue(DimIdx(0));
    *g = _imp->_timelinePlayheadColor->getValue(DimIdx(1));
    *b = _imp->_timelinePlayheadColor->getValue(DimIdx(2));
}

void
Settings::getTimelineBoundsColor(double* r,
                                 double* g,
                                 double* b) const
{
    *r = _imp->_timelineBoundsColor->getValue(DimIdx(0));
    *g = _imp->_timelineBoundsColor->getValue(DimIdx(1));
    *b = _imp->_timelineBoundsColor->getValue(DimIdx(2));
}

void
Settings::getTimelineBGColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_timelineBGColor->getValue(DimIdx(0));
    *g = _imp->_timelineBGColor->getValue(DimIdx(1));
    *b = _imp->_timelineBGColor->getValue(DimIdx(2));
}

void
Settings::getCachedFrameColor(double* r,
                              double* g,
                              double* b) const
{
    *r = _imp->_cachedFrameColor->getValue(DimIdx(0));
    *g = _imp->_cachedFrameColor->getValue(DimIdx(1));
    *b = _imp->_cachedFrameColor->getValue(DimIdx(2));
}


void
Settings::getAnimationModuleEditorBackgroundColor(double *r,
                                            double *g,
                                            double *b) const
{
    *r = _imp->_animationViewBackgroundColor->getValue(DimIdx(0));
    *g = _imp->_animationViewBackgroundColor->getValue(DimIdx(1));
    *b = _imp->_animationViewBackgroundColor->getValue(DimIdx(2));
}

void
Settings::getAnimationModuleEditorRootRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(0));
    *g = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(1));
    *b = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(2));
    *a = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(3));
}

void
Settings::getAnimationModuleEditorKnobRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(0));
    *g = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(1));
    *b = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(2));
    *a = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(3));
}

void
Settings::getAnimationModuleEditorScaleColor(double *r,
                                       double *g,
                                       double *b) const
{
    *r = _imp->_animationViewScaleColor->getValue(DimIdx(0));
    *g = _imp->_animationViewScaleColor->getValue(DimIdx(1));
    *b = _imp->_animationViewScaleColor->getValue(DimIdx(2));
}

void
Settings::getAnimationModuleEditorGridColor(double *r,
                                      double *g,
                                      double *b) const
{
    *r = _imp->_animationViewGridColor->getValue(DimIdx(0));
    *g = _imp->_animationViewGridColor->getValue(DimIdx(1));
    *b = _imp->_animationViewGridColor->getValue(DimIdx(2));
}

void
Settings::getSEKeywordColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_keywordColor->getValue(DimIdx(0));
    *g = _imp->_keywordColor->getValue(DimIdx(1));
    *b = _imp->_keywordColor->getValue(DimIdx(2));
}

void
Settings::getSEOperatorColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_operatorColor->getValue(DimIdx(0));
    *g = _imp->_operatorColor->getValue(DimIdx(1));
    *b = _imp->_operatorColor->getValue(DimIdx(2));
}

void
Settings::getSEBraceColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _imp->_braceColor->getValue(DimIdx(0));
    *g = _imp->_braceColor->getValue(DimIdx(1));
    *b = _imp->_braceColor->getValue(DimIdx(2));
}

void
Settings::getSEDefClassColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_defClassColor->getValue(DimIdx(0));
    *g = _imp->_defClassColor->getValue(DimIdx(1));
    *b = _imp->_defClassColor->getValue(DimIdx(2));
}

void
Settings::getSEStringsColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_stringsColor->getValue(DimIdx(0));
    *g = _imp->_stringsColor->getValue(DimIdx(1));
    *b = _imp->_stringsColor->getValue(DimIdx(2));
}

void
Settings::getSECommentsColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_commentsColor->getValue(DimIdx(0));
    *g = _imp->_commentsColor->getValue(DimIdx(1));
    *b = _imp->_commentsColor->getValue(DimIdx(2));
}

void
Settings::getSESelfColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _imp->_selfColor->getValue(DimIdx(0));
    *g = _imp->_selfColor->getValue(DimIdx(1));
    *b = _imp->_selfColor->getValue(DimIdx(2));
}

void
Settings::getSENumbersColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_numbersColor->getValue(DimIdx(0));
    *g = _imp->_numbersColor->getValue(DimIdx(1));
    *b = _imp->_numbersColor->getValue(DimIdx(2));
}

void
Settings::getSECurLineColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_curLineColor->getValue(DimIdx(0));
    *g = _imp->_curLineColor->getValue(DimIdx(1));
    *b = _imp->_curLineColor->getValue(DimIdx(2));
}

void
Settings::getSliderColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_sliderColor->getValue(DimIdx(0));
    *g = _imp->_sliderColor->getValue(DimIdx(1));
    *b = _imp->_sliderColor->getValue(DimIdx(2));
}

int
Settings::getSEFontSize() const
{
    return _imp->_scriptEditorFontSize->getValue();
}

std::string
Settings::getSEFontFamily() const
{
    return _imp->_scriptEditorFontChoice->getCurrentEntry().id;
}

std::string
Settings::getApplicationFontFamily() const
{
    return _imp->_systemFontChoice->getCurrentEntry().id;
}

int
Settings::getApplicationFontSize() const
{
    return _imp->_fontSize->getValue();
}

void
Settings::getPluginIconFrameColor(int *r,
                                  int *g,
                                  int *b) const
{
    *r = 50;
    *g = 50;
    *b = 50;
}

bool
Settings::isNaNHandlingEnabled() const
{
    return _imp->_convertNaNValues->getValue();
}

void
Settings::setOnProjectCreatedCB(const std::string& func)
{
    _imp->_onProjectCreated->setValue(func);
}

void
Settings::setOnProjectLoadedCB(const std::string& func)
{
    _imp->_defaultOnProjectLoaded->setValue(func);
}

bool
Settings::isDefaultAppearanceOutdated() const
{
    return _imp->_defaultAppearanceOutdated;
}

void
Settings::restoreDefaultAppearance()
{
    restorePageToDefaults(_imp->_appearanceTab);
}

std::string
Settings::getUserStyleSheetFilePath() const
{
    return _imp->_qssFile->getValue();
}

void
Settings::setRenderQueuingEnabled(bool enabled)
{
    _imp->_queueRenders->setValue(enabled);
}

bool
Settings::isRenderQueuingEnabled() const
{
    return _imp->_queueRenders->getValue();
}

bool
Settings::isFileDialogEnabledForNewWriters() const
{
    return _imp->_filedialogForWriters->getValue();
}

bool
Settings::isDriveLetterToUNCPathConversionEnabled() const
{
    return !_imp->_enableMappingFromDriveLettersToUNCShareNames->getValue();
}


bool
Settings::getIsFullRecoverySaveModeEnabled() const
{
    return _imp->_saveSafetyMode->getValue();
}

KnobPathPtr
Settings::getFileDialogFavoritePathsKnob() const
{
    return _imp->_fileDialogSavedPaths;
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_Settings.cpp"
