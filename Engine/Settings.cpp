/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "Settings.h"

#include <cassert>
#include <stdexcept>

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

#include "Gui/GuiDefines.h"

#include <SequenceParsing.h> // for removePath

#ifdef WINDOWS
#include <ofxhPluginCache.h>
#endif

#define NATRON_DEFAULT_OCIO_CONFIG_NAME "blender"


#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

#define NATRON_DEFAULT_APPEARANCE_VERSION 1

#define NATRON_CUSTOM_HOST_NAME_ENTRY "Custom..."

NATRON_NAMESPACE_ENTER

Settings::Settings()
    : KnobHolder( AppInstancePtr() ) // < Settings are process wide and do not belong to a single AppInstance
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

void
Settings::initializeKnobs()
{
    initializeKnobsGeneral();
    initializeKnobsThreading();
    initializeKnobsRendering();
    initializeKnobsGPU();
    initializeKnobsProjectSetup();
    initializeKnobsDocumentation();
    initializeKnobsUserInterface();
    initializeKnobsColorManagement();
    initializeKnobsCaching();
    initializeKnobsViewers();
    initializeKnobsNodeGraph();
    initializeKnobsPlugins();
    initializeKnobsPython();
    initializeKnobsAppearance();
    initializeKnobsGuiColors();
    initializeKnobsCurveEditorColors();
    initializeKnobsDopeSheetColors();
    initializeKnobsNodeGraphColors();
    initializeKnobsScriptEditorColors();

    setDefaultValues();
}

void
Settings::initializeKnobsGeneral()
{
    _generalTab = AppManager::createKnob<KnobPage>( this, tr("General") );

    _natronSettingsExist = AppManager::createKnob<KnobBool>( this, tr("Existing settings") );
    _natronSettingsExist->setName("existingSettings");
    _natronSettingsExist->setSecretByDefault(true);
    _generalTab->addKnob(_natronSettingsExist);

    _saveSettings = AppManager::createKnob<KnobBool>( this, tr("Save settings on change") );
    _saveSettings->setName("saveSettings");
    _saveSettings->setDefaultValue(true);
    _saveSettings->setSecretByDefault(true);
    _generalTab->addKnob(_saveSettings);

    _checkForUpdates = AppManager::createKnob<KnobBool>( this, tr("Always check for updates on start-up") );
    _checkForUpdates->setName("checkForUpdates");
    _checkForUpdates->setHintToolTip( tr("When checked, %1 will check for new updates on start-up of the application.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_checkForUpdates);

#ifdef NATRON_USE_BREAKPAD
    _enableCrashReports = AppManager::createKnob<KnobBool>( this, tr("Enable crash reporting") );
    _enableCrashReports->setName("enableCrashReports");
    _enableCrashReports->setHintToolTip( tr("When checked, if %1 crashes a window will pop-up asking you "
                                            "whether you want to upload the crash dump to the developers or not. "
                                            "This can help them track down the bug.\n"
                                            "If you need to turn the crash reporting system off, uncheck this.\n"
                                            "Note that when using the application in command-line mode, if crash reports are "
                                            "enabled, they will be automatically uploaded.\n"
                                            "Changing this requires a restart of the application to take effect.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _enableCrashReports->setAddNewLine(false);
    _generalTab->addKnob(_enableCrashReports);

    _testCrashReportButton = AppManager::createKnob<KnobButton>( this, tr("Test Crash Reporting") );
    _testCrashReportButton->setName("testCrashReporting");
    _testCrashReportButton->setHintToolTip( tr("This button is for developers only to test whether the crash reporting system "
                                               "works correctly. Do not use this.") );
    _generalTab->addKnob(_testCrashReportButton);
#endif

    _autoSaveDelay = AppManager::createKnob<KnobInt>( this, tr("Auto-save trigger delay") );
    _autoSaveDelay->setName("autoSaveDelay");
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setMinimum(0);
    _autoSaveDelay->setMaximum(60);
    _autoSaveDelay->setHintToolTip( tr("The number of seconds after an event that %1 should wait before "
                                       " auto-saving. Note that if a render is in progress, %1 will "
                                       " wait until it is done to actually auto-save.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_autoSaveDelay);


    _autoSaveUnSavedProjects = AppManager::createKnob<KnobBool>( this, tr("Enable Auto-save for unsaved projects") );
    _autoSaveUnSavedProjects->setName("autoSaveUnSavedProjects");
    _autoSaveUnSavedProjects->setHintToolTip( tr("When activated %1 will auto-save projects that have never been "
                                                 "saved and will prompt you on startup if an auto-save of that unsaved project was found. "
                                                 "Disabling this will no longer save un-saved project.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_autoSaveUnSavedProjects);


    _hostName = AppManager::createKnob<KnobChoice>( this, tr("Appear to plug-ins as") );
    _hostName->setName("pluginHostName");
    _hostName->setHintToolTip( tr("%1 will appear with the name of the selected application to the OpenFX plug-ins. "
                                  "Changing it to the name of another application can help loading plugins which "
                                  "restrict their usage to specific OpenFX host(s). "
                                  "If a Host is not listed here, use the \"Custom\" entry to enter a custom host name. Changing this requires "
                                  "a restart of the application and requires clearing "
                                  "the OpenFX plugins cache from the Cache menu.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _knownHostNames.clear();
    std::vector<ChoiceOption> visibleHostEntries;
    assert(visibleHostEntries.size() == (int)eKnownHostNameNatron);
    visibleHostEntries.push_back(ChoiceOption(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME, NATRON_APPLICATION_NAME, ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameNuke);
    visibleHostEntries.push_back(ChoiceOption("uk.co.thefoundry.nuke", "Nuke", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameFusion);
    visibleHostEntries.push_back(ChoiceOption("com.eyeonline.Fusion", "Fusion", "")); // or com.blackmagicdesign.Fusion
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

    _customHostName = AppManager::createKnob<KnobString>( this, tr("Custom Host name") );
    _customHostName->setName("customHostName");
    _customHostName->setHintToolTip( tr("This is the name of the OpenFX host (application) as it appears to the OpenFX plugins. "
                                        "Changing it to the name of another application can help loading some plugins which "
                                        "restrict their usage to specific OpenFX hosts. You should leave "
                                        "this to its default value, unless a specific plugin refuses to load or run. "
                                        "Changing this takes effect upon the next application launch, and requires clearing "
                                        "the OpenFX plugins cache from the Cache menu. "
                                        "The default host name is: \n%1").arg( QString::fromUtf8(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME) ) );
    _customHostName->setSecretByDefault(true);
    _generalTab->addKnob(_customHostName);
} // Settings::initializeKnobsGeneral

void
Settings::initializeKnobsThreading()
{
    _threadingPage = AppManager::createKnob<KnobPage>( this, tr("Threading") );

    _numberOfThreads = AppManager::createKnob<KnobInt>( this, tr("Number of render threads (0=\"guess\")") );
    _numberOfThreads->setName("noRenderThreads");

    QString numberOfThreadsToolTip = tr("Controls how many threads %1 should use to render. \n"
                                        "-1: Disable multithreading totally (useful for debugging) \n"
                                        "0: Guess the thread count from the number of cores and the available memory (min(num_cores,memory/3.5Gb)). The ideal threads count for this hardware is %2.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QThread::idealThreadCount() );
    _numberOfThreads->setHintToolTip( numberOfThreadsToolTip.toStdString() );
    _numberOfThreads->disableSlider();
    _numberOfThreads->setMinimum(-1);
    _numberOfThreads->setDisplayMinimum(-1);
    _threadingPage->addKnob(_numberOfThreads);

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders = AppManager::createKnob<KnobInt>( this, tr("Number of parallel renders (0=\"guess\")") );
    _numberOfParallelRenders->setHintToolTip( tr("Controls the number of parallel frame that will be rendered at the same time by the renderer. "
                                                 "A value of 0 indicate that %1 should automatically determine "
                                                 "the best number of parallel renders to launch given your CPU activity. "
                                                 "Setting a value different than 0 should be done only if you know what you're doing and can lead "
                                                 "in some situations to worse performances. Overall to get the best performances you should have your "
                                                 "CPU at 100% activity without idle times.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _numberOfParallelRenders->setName("nParallelRenders");
    _numberOfParallelRenders->setMinimum(0);
    _numberOfParallelRenders->disableSlider();
    _threadingPage->addKnob(_numberOfParallelRenders);
#endif

    _useThreadPool = AppManager::createKnob<KnobBool>( this, tr("Effects use the thread-pool") );
    _useThreadPool->setName("useThreadPool");
    _useThreadPool->setHintToolTip( tr("When checked, all effects will use a global thread-pool to do their processing instead of launching "
                                       "their own threads. "
                                       "This suppresses the overhead created by the operating system creating new threads on demand for "
                                       "each rendering of a special effect. As a result of this, the rendering might be faster on systems "
                                       "with a lot of cores (>= 8). \n"
                                       "WARNING: This is known not to work when using The Foundry's Furnace plug-ins (and potentially "
                                       "some other plug-ins that the dev team hasn't not tested against it). When using these plug-ins, "
                                       "make sure to uncheck this option first otherwise it will crash %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _threadingPage->addKnob(_useThreadPool);

    _nThreadsPerEffect = AppManager::createKnob<KnobInt>( this, tr("Max threads usable per effect (0=\"guess\")") );
    _nThreadsPerEffect->setName("nThreadsPerEffect");
    _nThreadsPerEffect->setHintToolTip( tr("Controls how many threads a specific effect can use at most to do its processing. "
                                           "A high value will allow 1 effect to spawn lots of thread and might not be efficient because "
                                           "the time spent to launch all the threads might exceed the time spent actually processing. "
                                           "By default (0) the renderer applies an heuristic to determine what's the best number of threads "
                                           "for an effect.") );

    _nThreadsPerEffect->setMinimum(0);
    _nThreadsPerEffect->disableSlider();
    _threadingPage->addKnob(_nThreadsPerEffect);

    _renderInSeparateProcess = AppManager::createKnob<KnobBool>( this, tr("Render in a separate process") );
    _renderInSeparateProcess->setName("renderNewProcess");
    _renderInSeparateProcess->setHintToolTip( tr("If true, %1 will render frames to disk in "
                                                 "a separate process so that if the main application crashes, the render goes on.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _threadingPage->addKnob(_renderInSeparateProcess);

    _queueRenders = AppManager::createKnob<KnobBool>( this, tr("Append new renders to queue") );
    _queueRenders->setHintToolTip( tr("When checked, renders will be queued in the Progress Panel and will start only when all "
                                      "other prior tasks are done.") );
    _queueRenders->setName("queueRenders");
    _threadingPage->addKnob(_queueRenders);
} // Settings::initializeKnobsThreading

void
Settings::initializeKnobsRendering()
{
    _renderingPage = AppManager::createKnob<KnobPage>( this, tr("Rendering") );

    _convertNaNValues = AppManager::createKnob<KnobBool>( this, tr("Convert NaN values") );
    _convertNaNValues->setName("convertNaNs");
    _convertNaNValues->setHintToolTip( tr("When activated, any pixel that is a Not-a-Number will be converted to 1 to avoid potential crashes from "
                                          "downstream nodes. These values can be produced by faulty plug-ins when they use wrong arithmetic such as "
                                          "division by zero. Disabling this option will keep the NaN(s) in the buffers: this may lead to an "
                                          "undefined behavior.") );
    _renderingPage->addKnob(_convertNaNValues);

    _pluginUseImageCopyForSource = AppManager::createKnob<KnobBool>( this, tr("Copy input image before rendering any plug-in") );
    _pluginUseImageCopyForSource->setName("copyInputImage");
    _pluginUseImageCopyForSource->setHintToolTip( tr("If checked, when before rendering any node, %1 will copy "
                                                     "the input image to a local temporary image. This is to work-around some plug-ins "
                                                     "that write to the source image, thus modifying the output of the node upstream in "
                                                     "the cache. This is a known bug of an old version of RevisionFX REMap for instance. "
                                                     "By default, this parameter should be leaved unchecked, as this will require an extra "
                                                     "image allocation and copy before rendering any plug-in.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _renderingPage->addKnob(_pluginUseImageCopyForSource);

    _activateRGBSupport = AppManager::createKnob<KnobBool>( this, tr("RGB components support") );
    _activateRGBSupport->setHintToolTip( tr("When checked %1 is able to process images with only RGB components "
                                            "(support for images with RGBA and Alpha components is always enabled). "
                                            "Un-checking this option may prevent plugins that do not well support RGB components from crashing %1. "
                                            "Changing this option requires a restart of the application.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateRGBSupport->setName("rgbSupport");
    _renderingPage->addKnob(_activateRGBSupport);


    _activateTransformConcatenationSupport = AppManager::createKnob<KnobBool>( this, tr("Transforms concatenation support") );
    _activateTransformConcatenationSupport->setHintToolTip( tr("When checked %1 is able to concatenate transform effects "
                                                               "when they are chained in the compositing tree. This yields better results and faster "
                                                               "render times because the image is only filtered once instead of as many times as there are "
                                                               "transformations.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateTransformConcatenationSupport->setName("transformCatSupport");
    _renderingPage->addKnob(_activateTransformConcatenationSupport);
}

void
Settings::populateOpenGLRenderers(const std::list<OpenGLRendererInfo>& renderers)
{
    if ( renderers.empty() ) {
        _availableOpenGLRenderers->setSecret(true);
        _nOpenGLContexts->setSecret(true);
        _enableOpenGL->setSecret(true);
        return;
    }

    _nOpenGLContexts->setSecret(false);
    _enableOpenGL->setSecret(false);

    std::vector<ChoiceOption> entries( renderers.size() );
    int i = 0;
    for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
        std::string option = it->vendorName + ' ' + it->rendererName + ' ' + it->glVersionString;
        entries[i] = ChoiceOption(option);
    }
    _availableOpenGLRenderers->populateChoices(entries);
    _availableOpenGLRenderers->setSecret(renderers.size() == 1);
}

bool
Settings::isOpenGLRenderingEnabled() const
{
    if (_enableOpenGL->getIsSecret()) {
        return false;
    }
    EnableOpenGLEnum enableOpenGL = (EnableOpenGLEnum)_enableOpenGL->getValue();
    return enableOpenGL == eEnableOpenGLEnabled || (enableOpenGL == eEnableOpenGLDisabledIfBackground && !appPTR->isBackground());
}

int
Settings::getMaxOpenGLContexts() const
{
    return _nOpenGLContexts->getValue();
}

GLRendererID
Settings::getActiveOpenGLRendererID() const
{
    if ( _availableOpenGLRenderers->getIsSecret() ) {
        // We were not able to detect multiple renderers, use default
        return GLRendererID();
    }
    int activeIndex = _availableOpenGLRenderers->getValue();
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
Settings::initializeKnobsGPU()
{
    _gpuPage = AppManager::createKnob<KnobPage>( this, tr("GPU Rendering") );
    _openglRendererString = AppManager::createKnob<KnobString>( this, tr("Active OpenGL renderer") );
    _openglRendererString->setName("activeOpenGLRenderer");
    _openglRendererString->setHintToolTip( tr("The currently active OpenGL renderer.") );
    _openglRendererString->setAsLabel();
    _gpuPage->addKnob(_openglRendererString);

    _availableOpenGLRenderers = AppManager::createKnob<KnobChoice>( this, tr("OpenGL renderer") );
    _availableOpenGLRenderers->setName("chooseOpenGLRenderer");
    _availableOpenGLRenderers->setHintToolTip( tr("The renderer used to perform OpenGL rendering. Changing the OpenGL renderer requires a restart of the application.") );
    _gpuPage->addKnob(_availableOpenGLRenderers);

    _nOpenGLContexts = AppManager::createKnob<KnobInt>( this, tr("No. of OpenGL Contexts") );
    _nOpenGLContexts->setName("maxOpenGLContexts");
    _nOpenGLContexts->setMinimum(1);
    _nOpenGLContexts->setDisplayMinimum(1);
    _nOpenGLContexts->setDisplayMaximum(8);
    _nOpenGLContexts->setMaximum(8);
    _nOpenGLContexts->setHintToolTip( tr("The number of OpenGL contexts created to perform OpenGL rendering. Each OpenGL context can be attached to a CPU thread, allowing for more frames to be rendered simultaneously. Increasing this value may increase performances for graphs with mixed CPU/GPU nodes but can drastically reduce performances if too many OpenGL contexts are active at once.") );
    _gpuPage->addKnob(_nOpenGLContexts);


    _enableOpenGL = AppManager::createKnob<KnobChoice>( this, tr("OpenGL Rendering") );
    _enableOpenGL->setName("enableOpenGLRendering");
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
    _gpuPage->addKnob(_enableOpenGL);
}

void
Settings::initializeKnobsProjectSetup()
{
    _projectsPage = AppManager::createKnob<KnobPage>( this, tr("Project Setup") );

    _firstReadSetProjectFormat = AppManager::createKnob<KnobBool>( this, tr("First image read set project format") );
    _firstReadSetProjectFormat->setName("autoProjectFormat");
    _firstReadSetProjectFormat->setHintToolTip( tr("If checked, the project size is set to this of the first image or video read within the project.") );
    _projectsPage->addKnob(_firstReadSetProjectFormat);


    _autoPreviewEnabledForNewProjects = AppManager::createKnob<KnobBool>( this, tr("Auto-preview enabled by default for new projects") );
    _autoPreviewEnabledForNewProjects->setName("enableAutoPreviewNewProjects");
    _autoPreviewEnabledForNewProjects->setHintToolTip( tr("If checked, then when creating a new project, the Auto-preview option"
                                                          " is enabled.") );
    _projectsPage->addKnob(_autoPreviewEnabledForNewProjects);


    _fixPathsOnProjectPathChanged = AppManager::createKnob<KnobBool>( this, tr("Auto fix relative file-paths") );
    _fixPathsOnProjectPathChanged->setHintToolTip( tr("If checked, when a project-path changes (either the name or the value pointed to), %1 checks all file-path parameters in the project and tries to fix them.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _fixPathsOnProjectPathChanged->setName("autoFixRelativePaths");

    _projectsPage->addKnob(_fixPathsOnProjectPathChanged);

    _enableMappingFromDriveLettersToUNCShareNames = AppManager::createKnob<KnobBool>( this, tr("Use drive letters instead of server names (Windows only)") );
    _enableMappingFromDriveLettersToUNCShareNames->setHintToolTip( tr("This is only relevant for Windows: If checked, %1 will not convert a path starting with a drive letter from the file dialog to a network share name. You may use this if for example you want to share a same project with several users across facilities with different servers but where users have all the same drive attached to a server.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _enableMappingFromDriveLettersToUNCShareNames->setName("useDriveLetters");
#ifndef __NATRON_WIN32__
    _enableMappingFromDriveLettersToUNCShareNames->setAllDimensionsEnabled(false);
#endif
    _projectsPage->addKnob(_enableMappingFromDriveLettersToUNCShareNames);
}

void
Settings::initializeKnobsDocumentation()
{
    _documentationPage = AppManager::createKnob<KnobPage>( this, tr("Documentation") );

#ifdef NATRON_DOCUMENTATION_ONLINE
    _documentationSource = AppManager::createKnob<KnobChoice>( this, tr("Documentation Source") );
    _documentationSource->setName("documentationSource");
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
    _documentationPage->addKnob(_documentationSource);
#endif

    /// used to store temp port for local webserver
    _wwwServerPort = AppManager::createKnob<KnobInt>( this, tr("Documentation local port (0=auto)") );
    _wwwServerPort->setName("webserverPort");
    _wwwServerPort->setHintToolTip( tr("The port onto which the documentation server will listen to. A value of 0 indicate that the documentation should automatically find a port by itself.") );
    _documentationPage->addKnob(_wwwServerPort);
}

void
Settings::initializeKnobsUserInterface()
{
    _uiPage = AppManager::createKnob<KnobPage>( this, tr("User Interface") );
    _uiPage->setName("userInterfacePage");

    _notifyOnFileChange = AppManager::createKnob<KnobBool>( this, tr("Warn when a file changes externally") );
    _notifyOnFileChange->setName("warnOnExternalChange");
    _notifyOnFileChange->setHintToolTip( tr("When checked, if a file read from a file parameter changes externally, a warning will be displayed "
                                            "on the viewer. Turning this off will suspend the notification system.") );
    _uiPage->addKnob(_notifyOnFileChange);

#ifdef NATRON_ENABLE_IO_META_NODES
    _filedialogForWriters = AppManager::createKnob<KnobBool>( this, tr("Prompt with file dialog when creating Write node") );
    _filedialogForWriters->setName("writeUseDialog");
    _filedialogForWriters->setDefaultValue(true);
    _filedialogForWriters->setHintToolTip( tr("When checked, opens-up a file dialog when creating a Write node") );
    _uiPage->addKnob(_filedialogForWriters);
#endif


    _renderOnEditingFinished = AppManager::createKnob<KnobBool>( this, tr("Refresh viewer only when editing is finished") );
    _renderOnEditingFinished->setName("renderOnEditingFinished");
    _renderOnEditingFinished->setHintToolTip( tr("When checked, the viewer triggers a new render only when mouse is released when editing parameters, curves "
                                                 " or the timeline. This setting doesn't apply to roto splines editing.") );
    _uiPage->addKnob(_renderOnEditingFinished);


    _linearPickers = AppManager::createKnob<KnobBool>( this, tr("Linear color pickers") );
    _linearPickers->setName("linearPickers");
    _linearPickers->setHintToolTip( tr("When activated, all colors picked from the color parameters are linearized "
                                       "before being fetched. Otherwise they are in the same colorspace "
                                       "as the viewer they were picked from.") );
    _uiPage->addKnob(_linearPickers);

    _maxPanelsOpened = AppManager::createKnob<KnobInt>( this, tr("Maximum number of open settings panels (0=\"unlimited\")") );
    _maxPanelsOpened->setName("maxPanels");
    _maxPanelsOpened->setHintToolTip( tr("This property holds the maximum number of settings panels that can be "
                                         "held by the properties dock at the same time. "
                                         "The special value of 0 indicates there can be an unlimited number of panels opened.") );
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setMinimum(0);
    _maxPanelsOpened->setMaximum(99);
    _uiPage->addKnob(_maxPanelsOpened);

    _useCursorPositionIncrements = AppManager::createKnob<KnobBool>( this, tr("Value increments based on cursor position") );
    _useCursorPositionIncrements->setName("cursorPositionAwareFields");
    _useCursorPositionIncrements->setHintToolTip( tr("When enabled, incrementing the value fields of parameters with the "
                                                     "mouse wheel or with arrow keys will increment the digits on the right "
                                                     "of the cursor. \n"
                                                     "When disabled, the value fields are incremented given what the plug-in "
                                                     "decided it should be. You can alter this increment by holding "
                                                     "Shift (x10) or Control (/10) while incrementing.") );
    _uiPage->addKnob(_useCursorPositionIncrements);

    _defaultLayoutFile = AppManager::createKnob<KnobFile>( this, tr("Default layout file") );
    _defaultLayoutFile->setName("defaultLayout");
    _defaultLayoutFile->setHintToolTip( tr("When set, %1 uses the given layout file "
                                           "as default layout for new projects. You can export/import a layout to/from a file "
                                           "from the Layout menu. If empty, the default application layout is used.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _uiPage->addKnob(_defaultLayoutFile);

    _loadProjectsWorkspace = AppManager::createKnob<KnobBool>( this, tr("Load workspace embedded within projects") );
    _loadProjectsWorkspace->setName("loadProjectWorkspace");
    _loadProjectsWorkspace->setHintToolTip( tr("When checked, when loading a project, the workspace (windows layout) will also be loaded, otherwise it "
                                               "will use your current layout.") );
    _uiPage->addKnob(_loadProjectsWorkspace);

#ifdef Q_OS_WIN
    _enableConsoleWindow = AppManager::createKnob<KnobBool>( this, tr("Enable console window") );
    _enableConsoleWindow->setName("enableConsoleWindow");
    _enableConsoleWindow->setHintToolTip( tr("When checked show console window on Windows.") );
    _uiPage->addKnob(_enableConsoleWindow);
#endif
} // Settings::initializeKnobsUserInterface

void
Settings::initializeKnobsColorManagement()
{
    _ocioTab = AppManager::createKnob<KnobPage>( this, tr("Color Management") );
    _ocioConfigKnob = AppManager::createKnob<KnobChoice>( this, tr("OpenColorIO configuration") );
    _ocioConfigKnob->setName("ocioConfig");

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
                configs.push_back(ChoiceOption( entries[j].toStdString() ));
            }

            break; //if we found 1 OpenColorIO-Configs directory, skip the next
        }
    }
    configs.push_back(ChoiceOption(NATRON_CUSTOM_OCIO_CONFIG_NAME));
    _ocioConfigKnob->populateChoices(configs);
    _ocioConfigKnob->setDefaultValue(defaultIndex, 0);
    _ocioConfigKnob->setHintToolTip( tr("Select the OpenColorIO configuration you would like to use globally for all "
                                        "operators and plugins that use OpenColorIO, by setting the \"OCIO\" "
                                        "environment variable. Only nodes created after changing this parameter will take "
                                        "it into account, and it is better to restart the application after changing it. "
                                        "When \"%1\" is selected, the "
                                        "\"Custom OpenColorIO config file\" parameter is used.").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) );

    _ocioTab->addKnob(_ocioConfigKnob);

    _customOcioConfigFile = AppManager::createKnob<KnobFile>( this, tr("Custom OpenColorIO configuration file") );
    _customOcioConfigFile->setName("ocioCustomConfigFile");

    if (_ocioConfigKnob->getNumEntries() == 1) {
        _customOcioConfigFile->setDefaultAllDimensionsEnabled(true);
    } else {
        _customOcioConfigFile->setDefaultAllDimensionsEnabled(false);
    }

    _customOcioConfigFile->setHintToolTip( tr("OpenColorIO configuration file (config.ocio) to use when \"%1\" "
                                              "is selected as the OpenColorIO config.").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) );
    _ocioTab->addKnob(_customOcioConfigFile);

    _warnOcioConfigKnobChanged = AppManager::createKnob<KnobBool>( this, tr("Warn on OpenColorIO config change") );
    _warnOcioConfigKnobChanged->setName("warnOCIOChanged");
    _warnOcioConfigKnobChanged->setHintToolTip( tr("Show a warning dialog when changing the OpenColorIO config to remember that a restart is required.") );
    _ocioTab->addKnob(_warnOcioConfigKnobChanged);

    _ocioStartupCheck = AppManager::createKnob<KnobBool>( this, tr("Warn on startup if OpenColorIO config is not the default") );
    _ocioStartupCheck->setName("startupCheckOCIO");
    _ocioTab->addKnob(_ocioStartupCheck);
} // Settings::initializeKnobsColorManagement

void
Settings::initializeKnobsAppearance()
{
    //////////////APPEARANCE TAB/////////////////
    _appearanceTab = AppManager::createKnob<KnobPage>( this, tr("Appearance") );

    _defaultAppearanceVersion = AppManager::createKnob<KnobInt>( this, tr("Appearance version") );
    _defaultAppearanceVersion->setName("appearanceVersion");
    _defaultAppearanceVersion->setSecretByDefault(true);
    _appearanceTab->addKnob(_defaultAppearanceVersion);

    _systemFontChoice = AppManager::createKnob<KnobChoice>( this, tr("Font") );
    _systemFontChoice->setHintToolTip( tr("List of all fonts available on your system") );
    _systemFontChoice->setName("systemFont");
    _systemFontChoice->setAddNewLine(false);
    _appearanceTab->addKnob(_systemFontChoice);

    _fontSize = AppManager::createKnob<KnobInt>( this, tr("Font size") );
    _fontSize->setName("fontSize");
    _appearanceTab->addKnob(_fontSize);

    _qssFile = AppManager::createKnob<KnobFile>( this, tr("Stylesheet file (.qss)") );
    _qssFile->setName("stylesheetFile");
    _qssFile->setHintToolTip( tr("When pointing to a valid .qss file, the stylesheet of the application will be set according to this file instead of the default "
                                 "stylesheet. You can adapt the default stylesheet that can be found in your distribution of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _appearanceTab->addKnob(_qssFile);
} // Settings::initializeKnobsAppearance

void
Settings::initializeKnobsGuiColors()
{
    _guiColorsTab = AppManager::createKnob<KnobPage>( this, tr("Main Window") );
    _appearanceTab->addKnob(_guiColorsTab);

    _useBWIcons = AppManager::createKnob<KnobBool>( this, tr("Use black & white toolbutton icons") );
    _useBWIcons->setName("useBwIcons");
    _useBWIcons->setHintToolTip( tr("When checked, the tools icons in the left toolbar are greyscale. Changing this takes "
                                    "effect upon the next launch of the application.") );
    _guiColorsTab->addKnob(_useBWIcons);


    _sunkenColor =  AppManager::createKnob<KnobColor>(this, tr("Sunken"), 3);
    _sunkenColor->setName("sunken");
    _sunkenColor->setSimplified(true);
    _guiColorsTab->addKnob(_sunkenColor);

    _baseColor =  AppManager::createKnob<KnobColor>(this, tr("Base"), 3);
    _baseColor->setName("base");
    _baseColor->setSimplified(true);
    _guiColorsTab->addKnob(_baseColor);

    _raisedColor =  AppManager::createKnob<KnobColor>(this, tr("Raised"), 3);
    _raisedColor->setName("raised");
    _raisedColor->setSimplified(true);
    _guiColorsTab->addKnob(_raisedColor);

    _selectionColor =  AppManager::createKnob<KnobColor>(this, tr("Selection"), 3);
    _selectionColor->setName("selection");
    _selectionColor->setSimplified(true);
    _guiColorsTab->addKnob(_selectionColor);

    _textColor =  AppManager::createKnob<KnobColor>(this, tr("Text"), 3);
    _textColor->setName("text");
    _textColor->setSimplified(true);
    _guiColorsTab->addKnob(_textColor);

    _altTextColor =  AppManager::createKnob<KnobColor>(this, tr("Unmodified text"), 3);
    _altTextColor->setName("unmodifiedText");
    _altTextColor->setSimplified(true);
    _guiColorsTab->addKnob(_altTextColor);

    _timelinePlayheadColor =  AppManager::createKnob<KnobColor>(this, tr("Timeline playhead"), 3);
    _timelinePlayheadColor->setName("timelinePlayhead");
    _timelinePlayheadColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelinePlayheadColor);


    _timelineBGColor =  AppManager::createKnob<KnobColor>(this, tr("Timeline background"), 3);
    _timelineBGColor->setName("timelineBG");
    _timelineBGColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelineBGColor);

    _timelineBoundsColor =  AppManager::createKnob<KnobColor>(this, tr("Timeline bounds"), 3);
    _timelineBoundsColor->setName("timelineBound");
    _timelineBoundsColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelineBoundsColor);

    _cachedFrameColor =  AppManager::createKnob<KnobColor>(this, tr("Cached frame"), 3);
    _cachedFrameColor->setName("cachedFrame");
    _cachedFrameColor->setSimplified(true);
    _guiColorsTab->addKnob(_cachedFrameColor);

    _diskCachedFrameColor =  AppManager::createKnob<KnobColor>(this, tr("Disk cached frame"), 3);
    _diskCachedFrameColor->setName("diskCachedFrame");
    _diskCachedFrameColor->setSimplified(true);
    _guiColorsTab->addKnob(_diskCachedFrameColor);

    _interpolatedColor =  AppManager::createKnob<KnobColor>(this, tr("Interpolated value"), 3);
    _interpolatedColor->setName("interpValue");
    _interpolatedColor->setSimplified(true);
    _guiColorsTab->addKnob(_interpolatedColor);

    _keyframeColor =  AppManager::createKnob<KnobColor>(this, tr("Keyframe"), 3);
    _keyframeColor->setName("keyframe");
    _keyframeColor->setSimplified(true);
    _guiColorsTab->addKnob(_keyframeColor);

    _trackerKeyframeColor =  AppManager::createKnob<KnobColor>(this, tr("Track User Keyframes"), 3);
    _trackerKeyframeColor->setName("trackUserKeyframe");
    _trackerKeyframeColor->setSimplified(true);
    _guiColorsTab->addKnob(_trackerKeyframeColor);

    _exprColor =  AppManager::createKnob<KnobColor>(this, tr("Expression"), 3);
    _exprColor->setName("exprColor");
    _exprColor->setSimplified(true);
    _guiColorsTab->addKnob(_exprColor);

    _sliderColor =  AppManager::createKnob<KnobColor>(this, tr("Slider"), 3);
    _sliderColor->setName("slider");
    _sliderColor->setSimplified(true);
    _guiColorsTab->addKnob(_sliderColor);
} // Settings::initializeKnobsGuiColors

void
Settings::initializeKnobsCurveEditorColors()
{
    _curveEditorColorsTab = AppManager::createKnob<KnobPage>( this, tr("Curve Editor") );
    _appearanceTab->addKnob(_curveEditorColorsTab);

    _curveEditorBGColor =  AppManager::createKnob<KnobColor>(this, tr("Background color"), 3);
    _curveEditorBGColor->setName("curveEditorBG");
    _curveEditorBGColor->setSimplified(true);
    _curveEditorColorsTab->addKnob(_curveEditorBGColor);

    _gridColor =  AppManager::createKnob<KnobColor>(this, tr("Grid color"), 3);
    _gridColor->setName("curveditorGrid");
    _gridColor->setSimplified(true);
    _curveEditorColorsTab->addKnob(_gridColor);

    _curveEditorScaleColor =  AppManager::createKnob<KnobColor>(this, tr("Scale color"), 3);
    _curveEditorScaleColor->setName("curveeditorScale");
    _curveEditorScaleColor->setSimplified(true);
    _curveEditorColorsTab->addKnob(_curveEditorScaleColor);
}

void
Settings::initializeKnobsDopeSheetColors()
{
    _dopeSheetEditorColorsTab = AppManager::createKnob<KnobPage>( this, tr("Dope Sheet") );
    _appearanceTab->addKnob(_dopeSheetEditorColorsTab);

    _dopeSheetEditorBackgroundColor = AppManager::createKnob<KnobColor>(this, tr("Sheet background color"), 3);
    _dopeSheetEditorBackgroundColor->setName("dopesheetBackground");
    _dopeSheetEditorBackgroundColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorBackgroundColor);

    _dopeSheetEditorRootSectionBackgroundColor = AppManager::createKnob<KnobColor>(this, tr("Root section background color"), 4);
    _dopeSheetEditorRootSectionBackgroundColor->setName("dopesheetRootSectionBackground");
    _dopeSheetEditorRootSectionBackgroundColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorRootSectionBackgroundColor);

    _dopeSheetEditorKnobSectionBackgroundColor = AppManager::createKnob<KnobColor>(this, tr("Knob section background color"), 4);
    _dopeSheetEditorKnobSectionBackgroundColor->setName("dopesheetKnobSectionBackground");
    _dopeSheetEditorKnobSectionBackgroundColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorKnobSectionBackgroundColor);

    _dopeSheetEditorScaleColor = AppManager::createKnob<KnobColor>(this, tr("Sheet scale color"), 3);
    _dopeSheetEditorScaleColor->setName("dopesheetScale");
    _dopeSheetEditorScaleColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorScaleColor);

    _dopeSheetEditorGridColor = AppManager::createKnob<KnobColor>(this, tr("Sheet grid color"), 3);
    _dopeSheetEditorGridColor->setName("dopesheetGrid");
    _dopeSheetEditorGridColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorGridColor);
}

void
Settings::initializeKnobsNodeGraphColors()
{
    _nodegraphColorsTab = AppManager::createKnob<KnobPage>( this, tr("Node Graph") );
    _appearanceTab->addKnob(_nodegraphColorsTab);

    _usePluginIconsInNodeGraph = AppManager::createKnob<KnobBool>( this, tr("Display plug-in icon on node-graph") );
    _usePluginIconsInNodeGraph->setName("usePluginIcons");
    _usePluginIconsInNodeGraph->setHintToolTip( tr("When checked, each node that has a plug-in icon will display it in the node-graph. "
                                                   "Changing this option will not affect already existing nodes, unless a restart of Natron is made.") );
    _usePluginIconsInNodeGraph->setAddNewLine(false);
    _nodegraphColorsTab->addKnob(_usePluginIconsInNodeGraph);

    _useAntiAliasing = AppManager::createKnob<KnobBool>( this, tr("Anti-Aliasing") );
    _useAntiAliasing->setName("antiAliasing");
    _useAntiAliasing->setHintToolTip( tr("When checked, the node graph will be painted using anti-aliasing. Unchecking it may increase performance. "
                                         " Changing this requires a restart of Natron") );
    _nodegraphColorsTab->addKnob(_useAntiAliasing);


    _defaultNodeColor = AppManager::createKnob<KnobColor>(this, tr("Default node color"), 3);
    _defaultNodeColor->setName("defaultNodeColor");
    _defaultNodeColor->setSimplified(true);
    _defaultNodeColor->setHintToolTip( tr("The default color used for newly created nodes.") );

    _nodegraphColorsTab->addKnob(_defaultNodeColor);


    _defaultBackdropColor =  AppManager::createKnob<KnobColor>(this, tr("Default backdrop color"), 3);
    _defaultBackdropColor->setName("backdropColor");
    _defaultBackdropColor->setSimplified(true);
    _defaultBackdropColor->setHintToolTip( tr("The default color used for newly created backdrop nodes.") );
    _nodegraphColorsTab->addKnob(_defaultBackdropColor);

    _defaultReaderColor =  AppManager::createKnob<KnobColor>(this, tr(PLUGIN_GROUP_IMAGE_READERS), 3);
    _defaultReaderColor->setName("readerColor");
    _defaultReaderColor->setSimplified(true);
    _defaultReaderColor->setHintToolTip( tr("The color used for newly created Reader nodes.") );
    _nodegraphColorsTab->addKnob(_defaultReaderColor);

    _defaultWriterColor =  AppManager::createKnob<KnobColor>(this, tr(PLUGIN_GROUP_IMAGE_WRITERS), 3);
    _defaultWriterColor->setName("writerColor");
    _defaultWriterColor->setSimplified(true);
    _defaultWriterColor->setHintToolTip( tr("The color used for newly created Writer nodes.") );
    _nodegraphColorsTab->addKnob(_defaultWriterColor);

    _defaultGeneratorColor =  AppManager::createKnob<KnobColor>(this, tr("Generators"), 3);
    _defaultGeneratorColor->setName("generatorColor");
    _defaultGeneratorColor->setSimplified(true);
    _defaultGeneratorColor->setHintToolTip( tr("The color used for newly created Generator nodes.") );
    _nodegraphColorsTab->addKnob(_defaultGeneratorColor);

    _defaultColorGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Color group"), 3);
    _defaultColorGroupColor->setName("colorNodesColor");
    _defaultColorGroupColor->setSimplified(true);
    _defaultColorGroupColor->setHintToolTip( tr("The color used for newly created Color nodes.") );
    _nodegraphColorsTab->addKnob(_defaultColorGroupColor);

    _defaultFilterGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Filter group"), 3);
    _defaultFilterGroupColor->setName("filterNodesColor");
    _defaultFilterGroupColor->setSimplified(true);
    _defaultFilterGroupColor->setHintToolTip( tr("The color used for newly created Filter nodes.") );
    _nodegraphColorsTab->addKnob(_defaultFilterGroupColor);

    _defaultTransformGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Transform group"), 3);
    _defaultTransformGroupColor->setName("transformNodesColor");
    _defaultTransformGroupColor->setSimplified(true);
    _defaultTransformGroupColor->setHintToolTip( tr("The color used for newly created Transform nodes.") );
    _nodegraphColorsTab->addKnob(_defaultTransformGroupColor);

    _defaultTimeGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Time group"), 3);
    _defaultTimeGroupColor->setName("timeNodesColor");
    _defaultTimeGroupColor->setSimplified(true);
    _defaultTimeGroupColor->setHintToolTip( tr("The color used for newly created Time nodes.") );
    _nodegraphColorsTab->addKnob(_defaultTimeGroupColor);

    _defaultDrawGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Draw group"), 3);
    _defaultDrawGroupColor->setName("drawNodesColor");
    _defaultDrawGroupColor->setSimplified(true);
    _defaultDrawGroupColor->setHintToolTip( tr("The color used for newly created Draw nodes.") );
    _nodegraphColorsTab->addKnob(_defaultDrawGroupColor);

    _defaultKeyerGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Keyer group"), 3);
    _defaultKeyerGroupColor->setName("keyerNodesColor");
    _defaultKeyerGroupColor->setSimplified(true);
    _defaultKeyerGroupColor->setHintToolTip( tr("The color used for newly created Keyer nodes.") );
    _nodegraphColorsTab->addKnob(_defaultKeyerGroupColor);

    _defaultChannelGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Channel group"), 3);
    _defaultChannelGroupColor->setName("channelNodesColor");
    _defaultChannelGroupColor->setSimplified(true);
    _defaultChannelGroupColor->setHintToolTip( tr("The color used for newly created Channel nodes.") );
    _nodegraphColorsTab->addKnob(_defaultChannelGroupColor);

    _defaultMergeGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Merge group"), 3);
    _defaultMergeGroupColor->setName("defaultMergeColor");
    _defaultMergeGroupColor->setSimplified(true);
    _defaultMergeGroupColor->setHintToolTip( tr("The color used for newly created Merge nodes.") );
    _nodegraphColorsTab->addKnob(_defaultMergeGroupColor);

    _defaultViewsGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Views group"), 3);
    _defaultViewsGroupColor->setName("defaultViewsColor");
    _defaultViewsGroupColor->setSimplified(true);
    _defaultViewsGroupColor->setHintToolTip( tr("The color used for newly created Views nodes.") );
    _nodegraphColorsTab->addKnob(_defaultViewsGroupColor);

    _defaultDeepGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Deep group"), 3);
    _defaultDeepGroupColor->setName("defaultDeepColor");
    _defaultDeepGroupColor->setSimplified(true);
    _defaultDeepGroupColor->setHintToolTip( tr("The color used for newly created Deep nodes.") );
    _nodegraphColorsTab->addKnob(_defaultDeepGroupColor);
} // Settings::initializeKnobsNodeGraphColors

void
Settings::initializeKnobsScriptEditorColors()
{
    _scriptEditorColorsTab = AppManager::createKnob<KnobPage>( this, tr("Script Editor") );
    _scriptEditorColorsTab->setParentKnob(_appearanceTab);

    _scriptEditorFontChoice = AppManager::createKnob<KnobChoice>( this, tr("Font") );
    _scriptEditorFontChoice->setHintToolTip( tr("List of all fonts available on your system") );
    _scriptEditorFontChoice->setName("scriptEditorFont");
    _scriptEditorColorsTab->addKnob(_scriptEditorFontChoice);

    _scriptEditorFontSize = AppManager::createKnob<KnobInt>( this, tr("Font Size") );
    _scriptEditorFontSize->setHintToolTip( tr("The font size") );
    _scriptEditorFontSize->setName("scriptEditorFontSize");
    _scriptEditorColorsTab->addKnob(_scriptEditorFontSize);

    _curLineColor = AppManager::createKnob<KnobColor>(this, tr("Current Line Color"), 3);
    _curLineColor->setName("currentLineColor");
    _curLineColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_curLineColor);

    _keywordColor = AppManager::createKnob<KnobColor>(this, tr("Keyword Color"), 3);
    _keywordColor->setName("keywordColor");
    _keywordColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_keywordColor);

    _operatorColor = AppManager::createKnob<KnobColor>(this, tr("Operator Color"), 3);
    _operatorColor->setName("operatorColor");
    _operatorColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_operatorColor);

    _braceColor = AppManager::createKnob<KnobColor>(this, tr("Brace Color"), 3);
    _braceColor->setName("braceColor");
    _braceColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_braceColor);

    _defClassColor = AppManager::createKnob<KnobColor>(this, tr("Class Def Color"), 3);
    _defClassColor->setName("classDefColor");
    _defClassColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_defClassColor);

    _stringsColor = AppManager::createKnob<KnobColor>(this, tr("Strings Color"), 3);
    _stringsColor->setName("stringsColor");
    _stringsColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_stringsColor);

    _commentsColor = AppManager::createKnob<KnobColor>(this, tr("Comments Color"), 3);
    _commentsColor->setName("commentsColor");
    _commentsColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_commentsColor);

    _selfColor = AppManager::createKnob<KnobColor>(this, tr("Self Color"), 3);
    _selfColor->setName("selfColor");
    _selfColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_selfColor);

    _numbersColor = AppManager::createKnob<KnobColor>(this, tr("Numbers Color"), 3);
    _numbersColor->setName("numbersColor");
    _numbersColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_numbersColor);
} // Settings::initializeKnobsScriptEditorColors

void
Settings::initializeKnobsViewers()
{
    _viewersTab = AppManager::createKnob<KnobPage>( this, tr("Viewer") );

    _texturesMode = AppManager::createKnob<KnobChoice>( this, tr("Viewer textures bit depth") );
    _texturesMode->setName("texturesBitDepth");
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


    _texturesMode->setHintToolTip( tr("Bit depth of the viewer textures used for rendering. "
                                      "Hover each option with the mouse for a detailed description.") );
    _viewersTab->addKnob(_texturesMode);

    _powerOf2Tiling = AppManager::createKnob<KnobInt>( this, tr("Viewer tile size is 2 to the power of...") );
    _powerOf2Tiling->setName("viewerTiling");
    _powerOf2Tiling->setHintToolTip( tr("The dimension of the viewer tiles is 2^n by 2^n (i.e. 256 by 256 pixels for n=8). "
                                        "A high value means that the viewer renders large tiles, so that "
                                        "rendering is done less often, but on larger areas.") );
    _powerOf2Tiling->disableSlider();
    _powerOf2Tiling->setMinimum(4);
    _powerOf2Tiling->setDisplayMinimum(4);
    _powerOf2Tiling->setMaximum(9);
    _powerOf2Tiling->setDisplayMaximum(9);

    _viewersTab->addKnob(_powerOf2Tiling);

    _checkerboardTileSize = AppManager::createKnob<KnobInt>( this, tr("Checkerboard tile size (pixels)") );
    _checkerboardTileSize->setName("checkerboardTileSize");
    _checkerboardTileSize->setMinimum(1);
    _checkerboardTileSize->setHintToolTip( tr("The size (in screen pixels) of one tile of the checkerboard.") );
    _viewersTab->addKnob(_checkerboardTileSize);

    _checkerboardColor1 = AppManager::createKnob<KnobColor>(this, tr("Checkerboard color 1"), 4);
    _checkerboardColor1->setName("checkerboardColor1");
    _checkerboardColor1->setHintToolTip( tr("The first color used by the checkerboard.") );
    _viewersTab->addKnob(_checkerboardColor1);

    _checkerboardColor2 = AppManager::createKnob<KnobColor>(this, tr("Checkerboard color 2"), 4);
    _checkerboardColor2->setName("checkerboardColor2");
    _checkerboardColor2->setHintToolTip( tr("The second color used by the checkerboard.") );
    _viewersTab->addKnob(_checkerboardColor2);

    _autoWipe = AppManager::createKnob<KnobBool>( this, tr("Automatically enable wipe") );
    _autoWipe->setName("autoWipeForViewer");
    _autoWipe->setHintToolTip( tr("When checked, the wipe tool of the viewer will be automatically enabled "
                                  "when the mouse is hovering the viewer and changing an input of a viewer." ) );
    _viewersTab->addKnob(_autoWipe);


    _autoProxyWhenScrubbingTimeline = AppManager::createKnob<KnobBool>( this, tr("Automatically enable proxy when scrubbing the timeline") );
    _autoProxyWhenScrubbingTimeline->setName("autoProxyScrubbing");
    _autoProxyWhenScrubbingTimeline->setHintToolTip( tr("When checked, the proxy mode will be at least at the level "
                                                        "indicated by the auto-proxy parameter.") );
    _autoProxyWhenScrubbingTimeline->setAddNewLine(false);
    _viewersTab->addKnob(_autoProxyWhenScrubbingTimeline);


    _autoProxyLevel = AppManager::createKnob<KnobChoice>( this, tr("Auto-proxy level") );
    _autoProxyLevel->setName("autoProxyLevel");
    std::vector<ChoiceOption> autoProxyChoices;
    autoProxyChoices.push_back(ChoiceOption("2", "",""));
    autoProxyChoices.push_back(ChoiceOption("4", "",""));
    autoProxyChoices.push_back(ChoiceOption("8", "",""));
    autoProxyChoices.push_back(ChoiceOption("16", "",""));
    autoProxyChoices.push_back(ChoiceOption("32", "",""));
    _autoProxyLevel->populateChoices(autoProxyChoices);

    _viewersTab->addKnob(_autoProxyLevel);

    _maximumNodeViewerUIOpened = AppManager::createKnob<KnobInt>( this, tr("Max. opened node viewer interface") );
    _maximumNodeViewerUIOpened->setName("maxNodeUiOpened");
    _maximumNodeViewerUIOpened->setMinimum(1);
    _maximumNodeViewerUIOpened->disableSlider();
    _maximumNodeViewerUIOpened->setHintToolTip( tr("Controls the maximum amount of nodes that can have their interface showing up at the same time in the viewer") );
    _viewersTab->addKnob(_maximumNodeViewerUIOpened);

    _viewerKeys = AppManager::createKnob<KnobBool>( this, tr("Use number keys for the viewer") );
    _viewerKeys->setName("viewerNumberKeys");
    _viewerKeys->setHintToolTip( tr("When enabled, the row of number keys on the keyboard "
                                    "is used for switching input (<key> connects input to A side, "
                                    "<shift-key> connects input to B side), even if the corresponding "
                                    "character in the current keyboard layout is not a number.\n"
                                    "This may have to be disabled when using a remote display connection "
                                    "to Linux from a different OS.") );
    _viewersTab->addKnob(_viewerKeys);
} // Settings::initializeKnobsViewers

void
Settings::initializeKnobsNodeGraph()
{
    /////////// Nodegraph tab
    _nodegraphTab = AppManager::createKnob<KnobPage>( this, tr("Nodegraph") );

    _autoScroll = AppManager::createKnob<KnobBool>( this, tr("Auto Scroll") );
    _autoScroll->setName("autoScroll");
    _autoScroll->setHintToolTip( tr("When checked the node graph will auto scroll if you move a node outside the current graph view.") );
    _nodegraphTab->addKnob(_autoScroll);

    _autoTurbo = AppManager::createKnob<KnobBool>( this, tr("Auto-turbo") );
    _autoTurbo->setName("autoTurbo");
    _autoTurbo->setHintToolTip( tr("When checked the Turbo-mode will be enabled automatically when playback is started and disabled "
                                   "when finished.") );
    _nodegraphTab->addKnob(_autoTurbo);

    _snapNodesToConnections = AppManager::createKnob<KnobBool>( this, tr("Snap to node") );
    _snapNodesToConnections->setName("enableSnapToNode");
    _snapNodesToConnections->setHintToolTip( tr("When moving nodes on the node graph, snap to positions where they are lined up "
                                                "with the inputs and output nodes.") );
    _nodegraphTab->addKnob(_snapNodesToConnections);


    _maxUndoRedoNodeGraph = AppManager::createKnob<KnobInt>( this, tr("Maximum undo/redo for the node graph") );
    _maxUndoRedoNodeGraph->setName("maxUndoRedo");
    _maxUndoRedoNodeGraph->disableSlider();
    _maxUndoRedoNodeGraph->setMinimum(0);
    _maxUndoRedoNodeGraph->setHintToolTip( tr("Set the maximum of events related to the node graph %1 "
                                              "remembers. Past this limit, older events will be deleted forever, "
                                              "allowing to re-use the RAM for other purposes. \n"
                                              "Changing this value will clear the undo/redo stack.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _nodegraphTab->addKnob(_maxUndoRedoNodeGraph);


    _disconnectedArrowLength = AppManager::createKnob<KnobInt>( this, tr("Disconnected arrow length") );
    _disconnectedArrowLength->setName("disconnectedArrowLength");
    _disconnectedArrowLength->setHintToolTip( tr("The size of a disconnected node input arrow in pixels.") );
    _disconnectedArrowLength->disableSlider();

    _nodegraphTab->addKnob(_disconnectedArrowLength);

    _hideOptionalInputsAutomatically = AppManager::createKnob<KnobBool>( this, tr("Auto hide masks inputs") );
    _hideOptionalInputsAutomatically->setName("autoHideInputs");
    _hideOptionalInputsAutomatically->setHintToolTip( tr("When checked, any diconnected mask input of a node in the nodegraph "
                                                         "will be visible only when the mouse is hovering the node or when it is "
                                                         "selected.") );
    _nodegraphTab->addKnob(_hideOptionalInputsAutomatically);

    _useInputAForMergeAutoConnect = AppManager::createKnob<KnobBool>( this, tr("Merge node connect to A input") );
    _useInputAForMergeAutoConnect->setName("mergeConnectToA");
    _useInputAForMergeAutoConnect->setHintToolTip( tr("If checked, upon creation of a new Merge node, or any other node with inputs named "
                                                      "A and B, input A is be preferred "
                                                      "for auto-connection. When the node is disabled, B is always output, whether this is checked or not.") );
    _nodegraphTab->addKnob(_useInputAForMergeAutoConnect);
} // Settings::initializeKnobsNodeGraph

void
Settings::initializeKnobsCaching()
{
    /////////// Caching tab
    _cachingTab = AppManager::createKnob<KnobPage>( this, tr("Caching") );

    _aggressiveCaching = AppManager::createKnob<KnobBool>( this, tr("Aggressive caching") );
    _aggressiveCaching->setName("aggressiveCaching");
    _aggressiveCaching->setHintToolTip( tr("When checked, %1 will cache the output of all images "
                                           "rendered by all nodes, regardless of their \"Force caching\" parameter. When enabling this option "
                                           "you need to have at least 8GiB of RAM, and 16GiB is recommended.\n"
                                           "If not checked, %1 will only cache the  nodes "
                                           "which have multiple outputs, or their parameter \"Force caching\" checked or if one of its "
                                           "output has its settings panel opened.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _cachingTab->addKnob(_aggressiveCaching);

    _maxRAMPercent = AppManager::createKnob<KnobInt>( this, tr("Maximum amount of RAM memory used for caching (% of total RAM)") );
    _maxRAMPercent->setName("maxRAMPercent");
    _maxRAMPercent->disableSlider();
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    QString ramHint( tr("This setting indicates the percentage of the total RAM which can be used by the memory caches. "
                        "This system has %1 of RAM.").arg( printAsRAM( getSystemTotalRAM() ) ) );
    if ( isApplication32Bits() && (getSystemTotalRAM() > 4ULL * 1024ULL * 1024ULL * 1024ULL) ) {
        ramHint.append( QString::fromUtf8("\n") );
        ramHint.append( tr("The version of %1 you are running is 32 bits, which means the available RAM "
                           "is limited to 4GiB. The amount of RAM used for caching is 4GiB * MaxRamPercent.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    }

    _maxRAMPercent->setHintToolTip(ramHint);
    _maxRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_maxRAMPercent);

    _maxRAMLabel = AppManager::createKnob<KnobString>( this, std::string() );
    _maxRAMLabel->setName("maxRamLabel");
    _maxRAMLabel->setIsPersistent(false);
    _maxRAMLabel->setAsLabel();
    _cachingTab->addKnob(_maxRAMLabel);


    _unreachableRAMPercent = AppManager::createKnob<KnobInt>( this, tr("System RAM to keep free (% of total RAM)") );
    _unreachableRAMPercent->setName("unreachableRAMPercent");
    _unreachableRAMPercent->disableSlider();
    _unreachableRAMPercent->setMinimum(0);
    _unreachableRAMPercent->setMaximum(90);
    _unreachableRAMPercent->setHintToolTip(tr("This determines how much RAM should be kept free for other applications "
                                              "running on the same system. "
                                              "When this limit is reached, the caches start recycling memory instead of growing. "
                                              //"A reasonable value should be set for it allowing the caches to stay in physical RAM " // users don't understand what swap is
                                              //"and avoid being swapped-out on disk. "
                                              "This value should reflect the amount of memory "
                                              "you want to keep available on your computer for other usage. "
                                              "A low value may result in a massive slowdown and high disk usage.")
                                           );
    _unreachableRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_unreachableRAMPercent);
    _unreachableRAMLabel = AppManager::createKnob<KnobString>( this, std::string() );
    _unreachableRAMLabel->setName("unreachableRAMLabel");
    _unreachableRAMLabel->setIsPersistent(false);
    _unreachableRAMLabel->setAsLabel();
    _cachingTab->addKnob(_unreachableRAMLabel);

    _maxViewerDiskCacheGB = AppManager::createKnob<KnobInt>( this, tr("Maximum playback disk cache size (GiB)") );
    _maxViewerDiskCacheGB->setName("maxViewerDiskCache");
    _maxViewerDiskCacheGB->disableSlider();
    _maxViewerDiskCacheGB->setMinimum(0);
    _maxViewerDiskCacheGB->setMaximum(100);
    _maxViewerDiskCacheGB->setHintToolTip( tr("The maximum size that may be used by the playback cache on disk (in GiB)") );
    _cachingTab->addKnob(_maxViewerDiskCacheGB);

    _maxDiskCacheNodeGB = AppManager::createKnob<KnobInt>( this, tr("Maximum DiskCache node disk usage (GiB)") );
    _maxDiskCacheNodeGB->setName("maxDiskCacheNode");
    _maxDiskCacheNodeGB->disableSlider();
    _maxDiskCacheNodeGB->setMinimum(0);
    _maxDiskCacheNodeGB->setMaximum(100);
    _maxDiskCacheNodeGB->setHintToolTip( tr("The maximum size that may be used by the DiskCache node on disk (in GiB)") );
    _cachingTab->addKnob(_maxDiskCacheNodeGB);


    _diskCachePath = AppManager::createKnob<KnobPath>( this, tr("Disk cache path") );
    _diskCachePath->setName("diskCachePath");
    _diskCachePath->setMultiPath(false);

    QString defaultLocation = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache);
    QString diskCacheTt( tr("WARNING: Changing this parameter requires a restart of the application. \n"
                            "This points to the location where %1 on-disk caches will be. "
                            "This variable should point to your fastest disk. This parameter can be "
                            "overriden by the value of the environment variable %2.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_DISK_CACHE_PATH_ENV_VAR) ) );

    QString diskCacheTt2( tr("If the parameter is left empty or the location set is invalid, "
                             "the default location will be used. The default location is: %1").arg(defaultLocation) );

    _diskCachePath->setHintToolTip( diskCacheTt + QLatin1Char('\n') + diskCacheTt2 );
    _cachingTab->addKnob(_diskCachePath);

    _wipeDiskCache = AppManager::createKnob<KnobButton>( this, tr("Wipe Disk Cache") );
    _wipeDiskCache->setHintToolTip( tr("Cleans-up all caches, deleting all folders that may contain cached data. "
                                       "This is provided in case %1 lost track of cached images "
                                       "for some reason.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _cachingTab->addKnob(_wipeDiskCache);
} // Settings::initializeKnobsCaching

void
Settings::initializeKnobsPlugins()
{
    _pluginsTab = AppManager::createKnob<KnobPage>( this, tr("Plug-ins") );
    _pluginsTab->setName("plugins");

#if defined(__linux__) || defined(__FreeBSD__)
    std::string searchPath("/usr/OFX/Plugins");
#elif defined(__APPLE__)
    std::string searchPath("/Library/OFX/Plugins");
#elif defined(WINDOWS)

    std::wstring basePath = std::wstring( OFX::Host::PluginCache::getStdOFXPluginPath() );
    basePath.append( std::wstring(L" and C:\\Program Files\\Common Files\\OFX\\Plugins") );
    std::string searchPath = StrUtils::utf16_to_utf8(basePath);

#endif

    _loadBundledPlugins = AppManager::createKnob<KnobBool>( this, tr("Use bundled plug-ins") );
    _loadBundledPlugins->setName("useBundledPlugins");
    _loadBundledPlugins->setHintToolTip( tr("When checked, %1 also uses the plug-ins bundled "
                                            "with the binary distribution.\n"
                                            "When unchecked, only system-wide plug-ins found in are loaded (more information can be "
                                            "found in the help for the \"Extra plug-ins search paths\" setting).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pluginsTab->addKnob(_loadBundledPlugins);

    _preferBundledPlugins = AppManager::createKnob<KnobBool>( this, tr("Prefer bundled plug-ins over system-wide plug-ins") );
    _preferBundledPlugins->setName("preferBundledPlugins");
    _preferBundledPlugins->setHintToolTip( tr("When checked, and if \"Use bundled plug-ins\" is also checked, plug-ins bundled with the %1 binary distribution will take precedence over system-wide plug-ins "
                                              "if they have the same internal ID.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pluginsTab->addKnob(_preferBundledPlugins);

    _useStdOFXPluginsLocation = AppManager::createKnob<KnobBool>( this, tr("Enable default OpenFX plugins location") );
    _useStdOFXPluginsLocation->setName("useStdOFXPluginsLocation");
    _useStdOFXPluginsLocation->setHintToolTip( tr("When checked, %1 also uses the OpenFX plug-ins found in the default location (%2).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) );
    _pluginsTab->addKnob(_useStdOFXPluginsLocation);

    _extraPluginPaths = AppManager::createKnob<KnobPath>( this, tr("OpenFX plug-ins search path") );
    _extraPluginPaths->setName("extraPluginsSearchPaths");
    _extraPluginPaths->setHintToolTip( tr("Extra search paths where %1 should scan for OpenFX plug-ins. "
                                          "Extra plug-ins search paths can also be specified using the OFX_PLUGIN_PATH environment variable.\n"
                                          "The priority order for system-wide plug-ins, from high to low, is:\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is checked)\n"
                                          "- plug-ins found in OFX_PLUGIN_PATH\n"
                                          "- plug-ins found in %2 (if \"Enable default OpenFX plug-ins location\" is checked)\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is not checked)\n"
                                          "Any change will take effect on the next launch of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) );
    _extraPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_extraPluginPaths);


    _templatesPluginPaths = AppManager::createKnob<KnobPath>( this, tr("PyPlugs search path") );
    _templatesPluginPaths->setName("groupPluginsSearchPath");
    _templatesPluginPaths->setHintToolTip( tr("Search path where %1 should scan for Python group scripts (PyPlugs). "
                                              "The search paths for groups can also be specified using the "
                                              "NATRON_PLUGIN_PATH environment variable.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _templatesPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_templatesPluginPaths);

} // Settings::initializeKnobsPlugins

void
Settings::initializeKnobsPython()
{
    _pythonPage = AppManager::createKnob<KnobPage>( this, tr("Python") );


    _onProjectCreated = AppManager::createKnob<KnobString>( this, tr("After project created") );
    _onProjectCreated->setName("afterProjectCreated");
    _onProjectCreated->setHintToolTip( tr("Callback called once a new project is created (this is never called "
                                          "when \"After project loaded\" is called.)\n"
                                          "The signature of the callback is: callback(app) where:\n"
                                          "- app: points to the current application instance\n") );
    _pythonPage->addKnob(_onProjectCreated);


    _defaultOnProjectLoaded = AppManager::createKnob<KnobString>( this, tr("Default after project loaded") );
    _defaultOnProjectLoaded->setName("defOnProjectLoaded");
    _defaultOnProjectLoaded->setHintToolTip( tr("The default afterProjectLoad callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectLoaded);

    _defaultOnProjectSave = AppManager::createKnob<KnobString>( this, tr("Default before project save") );
    _defaultOnProjectSave->setName("defOnProjectSave");
    _defaultOnProjectSave->setHintToolTip( tr("The default beforeProjectSave callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectSave);


    _defaultOnProjectClose = AppManager::createKnob<KnobString>( this, tr("Default before project close") );
    _defaultOnProjectClose->setName("defOnProjectClose");
    _defaultOnProjectClose->setHintToolTip( tr("The default beforeProjectClose callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectClose);


    _defaultOnNodeCreated = AppManager::createKnob<KnobString>( this, tr("Default after node created") );
    _defaultOnNodeCreated->setName("defOnNodeCreated");
    _defaultOnNodeCreated->setHintToolTip( tr("The default afterNodeCreated callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnNodeCreated);


    _defaultOnNodeDelete = AppManager::createKnob<KnobString>( this, tr("Default before node removal") );
    _defaultOnNodeDelete->setName("defOnNodeDelete");
    _defaultOnNodeDelete->setHintToolTip( tr("The default beforeNodeRemoval callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnNodeDelete);

    _loadPyPlugsFromPythonScript = AppManager::createKnob<KnobBool>( this, tr("Load PyPlugs in projects from .py if possible") );
    _loadPyPlugsFromPythonScript->setName("loadFromPyFile");
    _loadPyPlugsFromPythonScript->setHintToolTip( tr("When checked, if a project contains a PyPlug, it will try to first load the PyPlug "
                                                     "from the .py file. If the version of the PyPlug has changed Natron will ask you "
                                                     "whether you want to upgrade to the new version of the PyPlug in your project. "
                                                     "If the .py file is not found, it will fallback to the same behavior "
                                                     "as when this option is unchecked. When unchecked the PyPlug will load as a regular group "
                                                     "with the information embedded in the project file.") );
    _loadPyPlugsFromPythonScript->setDefaultValue(true);
    _pythonPage->addKnob(_loadPyPlugsFromPythonScript);

    _echoVariableDeclarationToPython = AppManager::createKnob<KnobBool>( this, tr("Print auto-declared variables in the Script Editor") );
    _echoVariableDeclarationToPython->setName("printAutoDeclaredVars");
    _echoVariableDeclarationToPython->setHintToolTip( tr("When checked, %1 will print in the Script Editor all variables that are "
                                                         "automatically declared, such as the app variable or node attributes.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pythonPage->addKnob(_echoVariableDeclarationToPython);
} // initializeKnobs

void
Settings::setCachingLabels()
{
    int maxTotalRam = _maxRAMPercent->getValue();
    U64 systemTotalRam = getSystemTotalRAM();
    U64 maxRAM = (U64)( ( (double)maxTotalRam / 100. ) * systemTotalRam );

    _maxRAMLabel->setValue( printAsRAM(maxRAM).toStdString() );
    _unreachableRAMLabel->setValue( printAsRAM( (double)systemTotalRam * ( (double)_unreachableRAMPercent->getValue() / 100. ) ).toStdString() );
}

void
Settings::setDefaultValues()
{
    beginChanges();

    _natronSettingsExist->setDefaultValue(false);

    // General
    _checkForUpdates->setDefaultValue(false);
#ifdef NATRON_USE_BREAKPAD
    _enableCrashReports->setDefaultValue(true);
#endif
    _autoSaveUnSavedProjects->setDefaultValue(true);
    _autoSaveDelay->setDefaultValue(5, 0);
    _hostName->setDefaultValue(0);
    _customHostName->setDefaultValue(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);

    // General/Threading
    _numberOfThreads->setDefaultValue(0, 0);
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders->setDefaultValue(0, 0);
#endif
    _useThreadPool->setDefaultValue(true);
    _nThreadsPerEffect->setDefaultValue(0);
    _renderInSeparateProcess->setDefaultValue(false, 0);
    _queueRenders->setDefaultValue(false);

    // General/Rendering
    _convertNaNValues->setDefaultValue(true);
    _pluginUseImageCopyForSource->setDefaultValue(false);
    _activateRGBSupport->setDefaultValue(true);
    _activateTransformConcatenationSupport->setDefaultValue(true);

    // General/GPU rendering
    //_openglRendererString
    _nOpenGLContexts->setDefaultValue(2);
#if NATRON_VERSION_MAJOR < 2 || (NATRON_VERSION_MAJOR == 2 && NATRON_VERSION_MINOR < 2)
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabled);
#else
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabledIfBackground);
#endif

    // General/Projects setup
    _firstReadSetProjectFormat->setDefaultValue(true);
    _autoPreviewEnabledForNewProjects->setDefaultValue(true, 0);
    _fixPathsOnProjectPathChanged->setDefaultValue(true);
    //_enableMappingFromDriveLettersToUNCShareNames

    // General/Documentation
    _wwwServerPort->setDefaultValue(0);
#ifdef NATRON_DOCUMENTATION_ONLINE
    _documentationSource->setDefaultValue(0);
#endif

    // General/User Interface
    _notifyOnFileChange->setDefaultValue(true);
#ifdef NATRON_ENABLE_IO_META_NODES
    //_filedialogForWriters
#endif
    _renderOnEditingFinished->setDefaultValue(false);
    _linearPickers->setDefaultValue(true, 0);
    _maxPanelsOpened->setDefaultValue(10, 0);
    _useCursorPositionIncrements->setDefaultValue(true);
    //_defaultLayoutFile
    _loadProjectsWorkspace->setDefaultValue(false);
#ifdef Q_OS_WIN
    _enableConsoleWindow->setDefaultValue(false);
#endif

    // Color-Management
    //_ocioConfigKnob
    _warnOcioConfigKnobChanged->setDefaultValue(true);
    _ocioStartupCheck->setDefaultValue(true);
    //_customOcioConfigFile

    // Caching
    _aggressiveCaching->setDefaultValue(false);
    _maxRAMPercent->setDefaultValue(50, 0);
    _unreachableRAMPercent->setDefaultValue(20); // see https://github.com/NatronGitHub/Natron/issues/486
    _maxViewerDiskCacheGB->setDefaultValue(5, 0);
    _maxDiskCacheNodeGB->setDefaultValue(10, 0);
    //_diskCachePath
    setCachingLabels();

    // Viewer
    _texturesMode->setDefaultValue(0, 0);
    _powerOf2Tiling->setDefaultValue(8, 0);
    _checkerboardTileSize->setDefaultValue(5);
    _checkerboardColor1->setDefaultValue(0.5, 0);
    _checkerboardColor1->setDefaultValue(0.5, 1);
    _checkerboardColor1->setDefaultValue(0.5, 2);
    _checkerboardColor1->setDefaultValue(0.5, 3);
    _checkerboardColor2->setDefaultValue(0., 0);
    _checkerboardColor2->setDefaultValue(0., 1);
    _checkerboardColor2->setDefaultValue(0., 2);
    _checkerboardColor2->setDefaultValue(0., 3);
    _autoWipe->setDefaultValue(true);
    _autoProxyWhenScrubbingTimeline->setDefaultValue(true);
    _autoProxyLevel->setDefaultValue(1);
    _maximumNodeViewerUIOpened->setDefaultValue(2);
    _viewerKeys->setDefaultValue(true);

    // Nodegraph
    _autoScroll->setDefaultValue(false);
    _autoTurbo->setDefaultValue(false);
    _snapNodesToConnections->setDefaultValue(true);
    _useBWIcons->setDefaultValue(false);
    _maxUndoRedoNodeGraph->setDefaultValue(20, 0);
    _disconnectedArrowLength->setDefaultValue(30);
    _hideOptionalInputsAutomatically->setDefaultValue(true);
    _useInputAForMergeAutoConnect->setDefaultValue(true);
    _usePluginIconsInNodeGraph->setDefaultValue(true);
    _useAntiAliasing->setDefaultValue(true);

    // Plugins
    _extraPluginPaths->setDefaultValue("", 0);
    _useStdOFXPluginsLocation->setDefaultValue(true);
    //_templatesPluginPaths
    _preferBundledPlugins->setDefaultValue(true);
    _loadBundledPlugins->setDefaultValue(true);

    // Python
    //_onProjectCreated;
    //_defaultOnProjectLoaded;
    //_defaultOnProjectSave;
    //_defaultOnProjectClose;
    //_defaultOnNodeCreated;
    //_defaultOnNodeDelete;
    //_loadPyPlugsFromPythonScript;
    _echoVariableDeclarationToPython->setDefaultValue(false);

    // Appearance
    _systemFontChoice->setDefaultValue(0);
    _fontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);
    //_qssFile
    //_defaultAppearanceVersion

    // Appearance/Main Window
    _sunkenColor->setDefaultValue(0.12, 0);
    _sunkenColor->setDefaultValue(0.12, 1);
    _sunkenColor->setDefaultValue(0.12, 2);
    _baseColor->setDefaultValue(0.19, 0);
    _baseColor->setDefaultValue(0.19, 1);
    _baseColor->setDefaultValue(0.19, 2);
    _raisedColor->setDefaultValue(0.28, 0);
    _raisedColor->setDefaultValue(0.28, 1);
    _raisedColor->setDefaultValue(0.28, 2);
    _selectionColor->setDefaultValue(0.95, 0);
    _selectionColor->setDefaultValue(0.54, 1);
    _selectionColor->setDefaultValue(0., 2);
    _textColor->setDefaultValue(0.78, 0);
    _textColor->setDefaultValue(0.78, 1);
    _textColor->setDefaultValue(0.78, 2);
    _altTextColor->setDefaultValue(0.6, 0);
    _altTextColor->setDefaultValue(0.6, 1);
    _altTextColor->setDefaultValue(0.6, 2);
    _timelinePlayheadColor->setDefaultValue(0.95, 0);
    _timelinePlayheadColor->setDefaultValue(0.54, 1);
    _timelinePlayheadColor->setDefaultValue(0., 2);
    _timelineBGColor->setDefaultValue(0, 0);
    _timelineBGColor->setDefaultValue(0, 1);
    _timelineBGColor->setDefaultValue(0., 2);
    _timelineBoundsColor->setDefaultValue(0.81, 0);
    _timelineBoundsColor->setDefaultValue(0.27, 1);
    _timelineBoundsColor->setDefaultValue(0.02, 2);
    _interpolatedColor->setDefaultValue(0.34, 0);
    _interpolatedColor->setDefaultValue(0.46, 1);
    _interpolatedColor->setDefaultValue(0.6, 2);
    _keyframeColor->setDefaultValue(0.08, 0);
    _keyframeColor->setDefaultValue(0.38, 1);
    _keyframeColor->setDefaultValue(0.97, 2);
    _trackerKeyframeColor->setDefaultValue(0.7, 0);
    _trackerKeyframeColor->setDefaultValue(0.78, 1);
    _trackerKeyframeColor->setDefaultValue(0.39, 2);
    _exprColor->setDefaultValue(0.7, 0);
    _exprColor->setDefaultValue(0.78, 1);
    _exprColor->setDefaultValue(0.39, 2);
    _cachedFrameColor->setDefaultValue(0.56, 0);
    _cachedFrameColor->setDefaultValue(0.79, 1);
    _cachedFrameColor->setDefaultValue(0.4, 2);
    _diskCachedFrameColor->setDefaultValue(0.27, 0);
    _diskCachedFrameColor->setDefaultValue(0.38, 1);
    _diskCachedFrameColor->setDefaultValue(0.25, 2);
    _sliderColor->setDefaultValue(0.33, 0);
    _sliderColor->setDefaultValue(0.45, 1);
    _sliderColor->setDefaultValue(0.44, 2);

    // Apprance/Curve Editor
    _curveEditorBGColor->setDefaultValue(0., 0);
    _curveEditorBGColor->setDefaultValue(0., 1);
    _curveEditorBGColor->setDefaultValue(0., 2);
    _gridColor->setDefaultValue(0.46, 0);
    _gridColor->setDefaultValue(0.84, 1);
    _gridColor->setDefaultValue(0.35, 2);
    _curveEditorScaleColor->setDefaultValue(0.26, 0);
    _curveEditorScaleColor->setDefaultValue(0.48, 1);
    _curveEditorScaleColor->setDefaultValue(0.2, 2);

    // Appearance/Dope Sheet
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

    // Appearance/Script Editor
    _curLineColor->setDefaultValue(0.35, 0);
    _curLineColor->setDefaultValue(0.35, 1);
    _curLineColor->setDefaultValue(0.35, 2);
    _keywordColor->setDefaultValue(0.7, 0);
    _keywordColor->setDefaultValue(0.7, 1);
    _keywordColor->setDefaultValue(0., 2);
    _operatorColor->setDefaultValue(0.78, 0);
    _operatorColor->setDefaultValue(0.78, 1);
    _operatorColor->setDefaultValue(0.78, 2);
    _braceColor->setDefaultValue(0.85, 0);
    _braceColor->setDefaultValue(0.85, 1);
    _braceColor->setDefaultValue(0.85, 2);
    _defClassColor->setDefaultValue(0.7, 0);
    _defClassColor->setDefaultValue(0.7, 1);
    _defClassColor->setDefaultValue(0., 2);
    _stringsColor->setDefaultValue(0.8, 0);
    _stringsColor->setDefaultValue(0.2, 1);
    _stringsColor->setDefaultValue(0., 2);
    _commentsColor->setDefaultValue(0.25, 0);
    _commentsColor->setDefaultValue(0.6, 1);
    _commentsColor->setDefaultValue(0.25, 2);
    _selfColor->setDefaultValue(0.7, 0);
    _selfColor->setDefaultValue(0.7, 1);
    _selfColor->setDefaultValue(0., 2);
    _numbersColor->setDefaultValue(0.25, 0);
    _numbersColor->setDefaultValue(0.8, 1);
    _numbersColor->setDefaultValue(0.9, 2);
    _scriptEditorFontChoice->setDefaultValue(0);
    _scriptEditorFontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);


    // Appearance/Node Graph
    _defaultNodeColor->setDefaultValue(0.7, 0);
    _defaultNodeColor->setDefaultValue(0.7, 1);
    _defaultNodeColor->setDefaultValue(0.7, 2);
    _defaultBackdropColor->setDefaultValue(0.45, 0);
    _defaultBackdropColor->setDefaultValue(0.45, 1);
    _defaultBackdropColor->setDefaultValue(0.45, 2);
    _defaultGeneratorColor->setDefaultValue(0.3, 0);
    _defaultGeneratorColor->setDefaultValue(0.5, 1);
    _defaultGeneratorColor->setDefaultValue(0.2, 2);
    _defaultReaderColor->setDefaultValue(0.7, 0);
    _defaultReaderColor->setDefaultValue(0.7, 1);
    _defaultReaderColor->setDefaultValue(0.7, 2);
    _defaultWriterColor->setDefaultValue(0.75, 0);
    _defaultWriterColor->setDefaultValue(0.75, 1);
    _defaultWriterColor->setDefaultValue(0., 2);
    _defaultColorGroupColor->setDefaultValue(0.48, 0);
    _defaultColorGroupColor->setDefaultValue(0.66, 1);
    _defaultColorGroupColor->setDefaultValue(1., 2);
    _defaultFilterGroupColor->setDefaultValue(0.8, 0);
    _defaultFilterGroupColor->setDefaultValue(0.5, 1);
    _defaultFilterGroupColor->setDefaultValue(0.3, 2);
    _defaultTransformGroupColor->setDefaultValue(0.7, 0);
    _defaultTransformGroupColor->setDefaultValue(0.3, 1);
    _defaultTransformGroupColor->setDefaultValue(0.1, 2);
    _defaultTimeGroupColor->setDefaultValue(0.7, 0);
    _defaultTimeGroupColor->setDefaultValue(0.65, 1);
    _defaultTimeGroupColor->setDefaultValue(0.35, 2);
    _defaultDrawGroupColor->setDefaultValue(0.75, 0);
    _defaultDrawGroupColor->setDefaultValue(0.75, 1);
    _defaultDrawGroupColor->setDefaultValue(0.75, 2);
    _defaultKeyerGroupColor->setDefaultValue(0., 0);
    _defaultKeyerGroupColor->setDefaultValue(1, 1);
    _defaultKeyerGroupColor->setDefaultValue(0., 2);
    _defaultChannelGroupColor->setDefaultValue(0.6, 0);
    _defaultChannelGroupColor->setDefaultValue(0.24, 1);
    _defaultChannelGroupColor->setDefaultValue(0.39, 2);
    _defaultMergeGroupColor->setDefaultValue(0.3, 0);
    _defaultMergeGroupColor->setDefaultValue(0.37, 1);
    _defaultMergeGroupColor->setDefaultValue(0.776, 2);
    _defaultViewsGroupColor->setDefaultValue(0.5, 0);
    _defaultViewsGroupColor->setDefaultValue(0.9, 1);
    _defaultViewsGroupColor->setDefaultValue(0.7, 2);
    _defaultDeepGroupColor->setDefaultValue(0., 0);
    _defaultDeepGroupColor->setDefaultValue(0., 1);
    _defaultDeepGroupColor->setDefaultValue(0.38, 2);


    endChanges();
} // setDefaultValues

void
Settings::warnChangedKnobs(const std::vector<KnobI*>& knobs)
{
    bool didFontWarn = false;
    bool didOCIOWarn = false;
    bool didOFXCacheWarn = false;

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( ( ( knobs[i] == _fontSize.get() ) ||
               ( knobs[i] == _systemFontChoice.get() ) )
             && !didFontWarn ) {
            didFontWarn = true;
            Dialogs::warningDialog( tr("Font change").toStdString(),
                                    tr("Changing the font requires a restart of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        } else if ( ( ( knobs[i] == _ocioConfigKnob.get() ) ||
                      ( knobs[i] == _customOcioConfigFile.get() ) )
                    && !didOCIOWarn ) {
            didOCIOWarn = true;
            bool warnOcioChanged = _warnOcioConfigKnobChanged->getValue();
            if (warnOcioChanged) {
                bool stopAsking = false;
                Dialogs::warningDialog(tr("OCIO config changed").toStdString(),
                                       tr("The OpenColorIO config change requires a restart of %1 to be effective.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), &stopAsking);
                if (stopAsking) {
                    _warnOcioConfigKnobChanged->setValue(false);
                    saveSetting( _warnOcioConfigKnobChanged.get() );
                }
            }
        } else if ( knobs[i] == _texturesMode.get() ) {
            AppInstanceVec apps = appPTR->getAppInstances();
            for (AppInstanceVec::iterator it = apps.begin(); it != apps.end(); ++it) {
                std::list<ViewerInstance*> allViewers;
                (*it)->getProject()->getViewers(&allViewers);
                for (std::list<ViewerInstance*>::iterator it = allViewers.begin(); it != allViewers.end(); ++it) {
                    (*it)->renderCurrentFrame(true);
                }
            }
        } else if ( ( ( knobs[i] == _loadBundledPlugins.get() ) ||
                      ( knobs[i] == _preferBundledPlugins.get() ) ||
                      ( knobs[i] == _useStdOFXPluginsLocation.get() ) ||
                      ( knobs[i] == _extraPluginPaths.get() ) )
                    && !didOFXCacheWarn ) {
            didOFXCacheWarn = true;
            appPTR->clearPluginsLoadedCache(); // clear the cache for next restart
            Dialogs::warningDialog( tr("OpenFX plugins path changed").toStdString(),
                                    tr("The OpenFX plugins path change requires a restart of %1 to be effective.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        }
    }
} // Settings::warnChangedKnobs

void
Settings::saveAllSettings()
{
    if ( !_saveSettings->getValue() ) {
        return;
    }
    const KnobsVec &knobs = getKnobs();
    std::vector<KnobI*> k( knobs.size() );

    for (U32 i = 0; i < knobs.size(); ++i) {
        k[i] = knobs[i].get();
    }
    saveSettings(k, false, true);
}

void
Settings::restorePluginSettings()
{
    const PluginsMap& plugins = appPTR->getPluginsList();
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        if ( it->first.empty() ) {
            continue;
        }
        assert(it->second.size() > 0);

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            Plugin* plugin  = *itver;
            assert(plugin);

            if ( plugin->getIsForInternalUseOnly() ) {
                continue;
            }


            {
                QString pluginIDKey = plugin->getPluginID() + QString::fromUtf8("_") + QString::number( plugin->getMajorVersion() ) + QString::fromUtf8("_") + QString::number( plugin->getMinorVersion() );
                QString enabledKey = pluginIDKey + QString::fromUtf8("_enabled");
                if ( settings.contains(enabledKey) ) {
                    bool enabled = settings.value(enabledKey).toBool();
                    plugin->setActivated(enabled);
                } else {
                    settings.setValue( enabledKey, plugin->isActivated() );
                }

                QString rsKey = pluginIDKey + QString::fromUtf8("_rs");
                if ( settings.contains(rsKey) ) {
                    bool renderScaleEnabled = settings.value(rsKey).toBool();
                    plugin->setRenderScaleEnabled(renderScaleEnabled);
                } else {
                    settings.setValue( rsKey, plugin->isRenderScaleEnabled() );
                }

                QString mtKey = pluginIDKey + QString::fromUtf8("_mt");
                if ( settings.contains(mtKey) ) {
                    bool multiThreadingEnabled = settings.value(mtKey).toBool();
                    plugin->setMultiThreadingEnabled(multiThreadingEnabled);
                } else {
                    settings.setValue( mtKey, plugin->isMultiThreadingEnabled() );
                }

                QString glKey = pluginIDKey + QString::fromUtf8("_gl");
                if (settings.contains(glKey)) {
                    bool openglEnabled = settings.value(glKey).toBool();
                    plugin->setOpenGLEnabled(openglEnabled);
                } else {
                    settings.setValue(glKey, plugin->isOpenGLEnabled());
                }

            }
        }
    }
} // Settings::restorePluginSettings

void
Settings::savePluginsSettings()
{
    const PluginsMap& plugins = appPTR->getPluginsList();
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        assert(it->second.size() > 0);

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            Plugin* plugin  = *itver;
            assert(plugin);

            QString pluginID = plugin->getPluginID() + QString::fromUtf8("_") + QString::number( plugin->getMajorVersion() ) + QString::fromUtf8("_") + QString::number( plugin->getMinorVersion() );
            QString enabledKey = pluginID + QString::fromUtf8("_enabled");
            settings.setValue( enabledKey, plugin->isActivated() );

            QString rsKey = pluginID + QString::fromUtf8("_rs");
            settings.setValue( rsKey, plugin->isRenderScaleEnabled() );

            QString mtKey = pluginID + QString::fromUtf8("_mt");
            settings.setValue(mtKey, plugin->isMultiThreadingEnabled());

            QString glKey = pluginID + QString::fromUtf8("_gl");
            settings.setValue(glKey, plugin->isOpenGLEnabled());

        }
    }
}

void
Settings::setSaveSettings(bool v)
{
    _saveSettings->setValue(v);
}

bool
Settings::getSaveSettings() const
{
    return _saveSettings->getValue();
}

void
Settings::saveSettings(const std::vector<KnobI*>& knobs,
                       bool doWarnings,
                       bool pluginSettings)
{
    if ( !_saveSettings->getValue() ) {
        return;
    }
    if (pluginSettings) {
        savePluginsSettings();
    }
    std::vector<KnobI*> changedKnobs;
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    settings.setValue(QString::fromUtf8(kQSettingsSoftwareMajorVersionSettingName), NATRON_VERSION_MAJOR);
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>(knobs[i]);
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(knobs[i]);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knobs[i]);
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(knobs[i]);
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(knobs[i]);

        const std::string& name = knobs[i]->getName();
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            QString dimensionName;
            if (knobs[i]->getDimension() > 1) {
                dimensionName =  QString::fromUtf8( name.c_str() ) + QLatin1Char('.') + QString::fromUtf8( knobs[i]->getDimensionName(j).c_str() );
            } else {
                dimensionName = QString::fromUtf8( name.c_str() );
            }
            try {
                if (isString) {
                    QString old = settings.value(dimensionName).toString();
                    QString newValue = QString::fromUtf8( isString->getValue(j).c_str() );
                    if (old != newValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        int newIndex = isChoice->getValue(j);
                        const std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
                        if ( newIndex < (int)entries.size() ) {
                            QString oldValue = settings.value(dimensionName).toString();
                            QString newValue = QString::fromUtf8( entries[newIndex].id.c_str());
                            if (oldValue != newValue) {
                                changedKnobs.push_back(knobs[i]);
                            }
                            settings.setValue( dimensionName, QVariant(newValue) );
                        }
                    } else {
                        int newValue = isInt->getValue(j);
                        int oldValue = settings.value( dimensionName, QVariant(INT_MIN) ).toInt();
                        if (newValue != oldValue) {
                            changedKnobs.push_back(knobs[i]);
                        }
                        settings.setValue( dimensionName, QVariant(newValue) );
                    }
                } else if (isDouble) {
                    double newValue = isDouble->getValue(j);
                    double oldValue = settings.value( dimensionName, QVariant(INT_MIN) ).toDouble();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else if (isBool) {
                    bool newValue = isBool->getValue(j);
                    bool oldValue = settings.value(dimensionName).toBool();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else {
                    assert(false);
                }
            } catch (std::logic_error&) {
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
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>(knobs[i]);
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(knobs[i]);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knobs[i]);
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(knobs[i]);
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(knobs[i]);

        const std::string& name = knobs[i]->getName();

        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            std::string dimensionName = knobs[i]->getDimension() > 1 ? name + '.' + knobs[i]->getDimensionName(j) : name;
            QString qDimName = QString::fromUtf8( dimensionName.c_str() );

            if ( settings.contains(qDimName) ) {
                if (isString) {
                    isString->setValue(settings.value(qDimName).toString().toStdString(), ViewSpec::all(), j);
                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        std::string value = settings.value(qDimName).toString().toStdString();
                        const std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
                        int found = -1;

                        for (U32 k = 0; k < entries.size(); ++k) {
                            if (entries[k].id == value) {
                                found = (int)k;
                                break;
                            }
                        }

                        if (found >= 0) {
                            isChoice->setValue(found, ViewSpec::all(), j);
                        }
                    } else {
                        isInt->setValue(settings.value(qDimName).toInt(), ViewSpec::all(), j);
                    }
                } else if (isDouble) {
                    isDouble->setValue(settings.value(qDimName).toDouble(), ViewSpec::all(), j);
                } else if (isBool) {
                    isBool->setValue(settings.value(qDimName).toBool(), ViewSpec::all(), j);
                } else {
                    assert(false);
                }
            }
        }
    }
} // Settings::restoreKnobsFromSettings

void
Settings::restoreKnobsFromSettings(const KnobsVec& knobs)
{
    std::vector<KnobI*> k( knobs.size() );

    for (U32 i = 0; i < knobs.size(); ++i) {
        k[i] = knobs[i].get();
    }
    restoreKnobsFromSettings(k);
}

void
Settings::restoreSettings(bool useDefault)
{
    _restoringSettings = true;

    // call restoreKnobsFromSettings only if --no-settings is not part of the command-line arguments
    if (!useDefault) {
        const KnobsVec& knobs = getKnobs();
        restoreKnobsFromSettings(knobs);
    }

    if (!_ocioRestored) {
        ///Load even though there's no settings!
        tryLoadOpenColorIOConfig();
    }

    // Restore opengl renderer
    {
        std::vector<ChoiceOption> availableRenderers = _availableOpenGLRenderers->getEntries_mt_safe();
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
        if ( (curIndex >= 0) && ( curIndex < (int)availableRenderers.size() ) ) {
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
        }
    }

    if (!appPTR->isTextureFloatSupported()) {
        if (_texturesMode) {
            _texturesMode->setSecret(true);
        }
    }

    _settingsExisted = false;
    try {
        _settingsExisted = _natronSettingsExist->getValue();

        if (!_settingsExisted && !useDefault) {
            _natronSettingsExist->setValue(true);
            saveSetting( _natronSettingsExist.get() );
        }

        int appearanceVersion = _defaultAppearanceVersion->getValue();
        if ( _settingsExisted && (appearanceVersion < NATRON_DEFAULT_APPEARANCE_VERSION) ) {
            _defaultAppearanceOutdated = true;
            _defaultAppearanceVersion->setValue(NATRON_DEFAULT_APPEARANCE_VERSION);
            saveSetting( _defaultAppearanceVersion.get() );
        }

        appPTR->setNThreadsPerEffect( getNumberOfThreadsPerEffect() );
        appPTR->setNThreadsToRender( getNumberOfThreads() );
        appPTR->setUseThreadPool( _useThreadPool->getValue() );
        appPTR->setPluginsUseInputImageCopyToRender( _pluginUseImageCopyForSource->getValue() );
    } catch (std::logic_error&) {
        // ignore
    }

    _restoringSettings = false;
} // restoreSettings

bool
Settings::tryLoadOpenColorIOConfig()
{
    // the default value is the environment variable "OCIO"
    QString configFile = QFile::decodeName( qgetenv(NATRON_OCIO_ENV_VAR_NAME) );

    // OCIO environment variable overrides everything, then try the custom config...
    if ( configFile.isEmpty() && _customOcioConfigFile->isEnabled(0) ) {
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
            QString activeEntryText  = QString::fromUtf8( _ocioConfigKnob->getActiveEntry().id.c_str() );
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
    _ocioRestored = true;
#ifdef DEBUG
    qDebug() << "setting OCIO=" << configFile;
#endif
    std::string stdConfigFile = configFile.toStdString();
#if 0 //def __NATRON_WIN32__ // commented out in https://github.com/NatronGitHub/Natron/commit/3445d671f15fbd97bca164b53ceb41cef47c61c3
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

#ifdef NATRON_USE_BREAKPAD
inline
void
crash_application()
{
    std::cerr << "CRASHING APPLICATION NOW UPON USER REQUEST!" << std::endl;
    volatile int* a = (int*)(NULL);

    // coverity[var_deref_op]
    *a = 1;
}
#endif

bool
Settings::onKnobValueChanged(KnobI* k,
                             ValueChangedReasonEnum reason,
                             double /*time*/,
                             ViewSpec /*view*/,
                             bool /*originatedFromMainThread*/)
{
    Q_EMIT settingChanged(k);
    bool ret = true;

    if ( k == _maxViewerDiskCacheGB.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumViewerDiskSpace( getMaximumViewerDiskCacheSize() );
        }
    } else if ( k == _maxDiskCacheNodeGB.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumDiskSpace( getMaximumDiskCacheNodeSize() );
        }
    } else if ( k == _maxRAMPercent.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumMemoryPercent( getRamMaximumPercent() );
        }
        setCachingLabels();
    } else if ( k == _diskCachePath.get() ) {
        QString path = QString::fromUtf8(_diskCachePath->getValue().c_str());
        qputenv(NATRON_DISK_CACHE_PATH_ENV_VAR, path.toUtf8());
        appPTR->refreshDiskCacheLocation();
    } else if ( k == _wipeDiskCache.get() ) {
        appPTR->wipeAndCreateDiskCacheStructure();
    } else if ( k == _numberOfThreads.get() ) {
        int nbThreads = getNumberOfThreads();
        appPTR->setNThreadsToRender(nbThreads);
        if (nbThreads == -1) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else if (nbThreads == 0) {
            // See https://github.com/NatronGitHub/Natron/issues/554
            // min(num_cores, RAM/3.5Gb)
            int maxThread = getSystemTotalRAM() / ( (3ULL * 1024ULL + 512ULL) * 1024ULL * 1024ULL );
            maxThread = std::min( std::max(1, maxThread), QThread::idealThreadCount() );
            QThreadPool::globalInstance()->setMaxThreadCount(maxThread);
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
        }
    } else if ( k == _nThreadsPerEffect.get() ) {
        appPTR->setNThreadsPerEffect( getNumberOfThreadsPerEffect() );
    } else if ( k == _ocioConfigKnob.get() ) {
        if (_ocioConfigKnob->getActiveEntry().id == NATRON_CUSTOM_OCIO_CONFIG_NAME) {
            _customOcioConfigFile->setAllDimensionsEnabled(true);
        } else {
            _customOcioConfigFile->setAllDimensionsEnabled(false);
        }
        tryLoadOpenColorIOConfig();
    } else if ( k == _useThreadPool.get() ) {
        bool useTP = _useThreadPool->getValue();
        appPTR->setUseThreadPool(useTP);
    } else if ( k == _customOcioConfigFile.get() ) {
        if ( _customOcioConfigFile->isEnabled(0) ) {
            tryLoadOpenColorIOConfig();
            bool warnOcioChanged = _warnOcioConfigKnobChanged->getValue();
            if ( warnOcioChanged && appPTR->getTopLevelInstance() ) {
                bool stopAsking = false;
                Dialogs::warningDialog(tr("OCIO config changed").toStdString(),
                                       tr("The OpenColorIO config change requires a restart of %1 to be effective.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), &stopAsking);
                if (stopAsking) {
                    _warnOcioConfigKnobChanged->setValue(false);
                }
            }
        }
    } else if ( k == _maxUndoRedoNodeGraph.get() ) {
        appPTR->setUndoRedoStackLimit( _maxUndoRedoNodeGraph->getValue() );
    } else if ( k == _maxPanelsOpened.get() ) {
        appPTR->onMaxPanelsOpenedChanged( _maxPanelsOpened->getValue() );
    } else if ( k == _queueRenders.get() ) {
        appPTR->onQueueRendersChanged( _queueRenders->getValue() );
    } else if ( ( k == _checkerboardTileSize.get() ) || ( k == _checkerboardColor1.get() ) || ( k == _checkerboardColor2.get() ) ) {
        appPTR->onCheckerboardSettingsChanged();
    } else if ( k == _powerOf2Tiling.get() && !_restoringSettings) {
        appPTR->onViewerTileCacheSizeChanged();
    } else if ( k == _texturesMode.get() &&  !_restoringSettings) {
         appPTR->onViewerTileCacheSizeChanged();
    } else if ( ( k == _hideOptionalInputsAutomatically.get() ) && !_restoringSettings && (reason == eValueChangedReasonUserEdited) ) {
        appPTR->toggleAutoHideGraphInputs();
    } else if ( k == _autoProxyWhenScrubbingTimeline.get() ) {
        _autoProxyLevel->setSecret( !_autoProxyWhenScrubbingTimeline->getValue() );
    } else if ( !_restoringSettings &&
                ( ( k == _sunkenColor.get() ) ||
                  ( k == _baseColor.get() ) ||
                  ( k == _raisedColor.get() ) ||
                  ( k == _selectionColor.get() ) ||
                  ( k == _textColor.get() ) ||
                  ( k == _altTextColor.get() ) ||
                  ( k == _timelinePlayheadColor.get() ) ||
                  ( k == _timelineBoundsColor.get() ) ||
                  ( k == _timelineBGColor.get() ) ||
                  ( k == _interpolatedColor.get() ) ||
                  ( k == _keyframeColor.get() ) ||
                  ( k == _trackerKeyframeColor.get() ) ||
                  ( k == _cachedFrameColor.get() ) ||
                  ( k == _diskCachedFrameColor.get() ) ||
                  ( k == _curveEditorBGColor.get() ) ||
                  ( k == _gridColor.get() ) ||
                  ( k == _curveEditorScaleColor.get() ) ||
                  ( k == _dopeSheetEditorBackgroundColor.get() ) ||
                  ( k == _dopeSheetEditorRootSectionBackgroundColor.get() ) ||
                  ( k == _dopeSheetEditorKnobSectionBackgroundColor.get() ) ||
                  ( k == _dopeSheetEditorScaleColor.get() ) ||
                  ( k == _dopeSheetEditorGridColor.get() ) ||
                  ( k == _keywordColor.get() ) ||
                  ( k == _operatorColor.get() ) ||
                  ( k == _curLineColor.get() ) ||
                  ( k == _braceColor.get() ) ||
                  ( k == _defClassColor.get() ) ||
                  ( k == _stringsColor.get() ) ||
                  ( k == _commentsColor.get() ) ||
                  ( k == _selfColor.get() ) ||
                  ( k == _sliderColor.get() ) ||
                  ( k == _numbersColor.get() ) ) ) {
        appPTR->reloadStylesheets();
    } else if ( k == _qssFile.get() ) {
        appPTR->reloadStylesheets();
    } else if ( k == _hostName.get() ) {
        std::string hostName = _hostName->getActiveEntry().id;
        bool isCustom = hostName == NATRON_CUSTOM_HOST_NAME_ENTRY;
        _customHostName->setSecret(!isCustom);
#ifdef NATRON_USE_BREAKPAD
    } else if ( ( k == _testCrashReportButton.get() ) && (reason == eValueChangedReasonUserEdited) ) {
        StandardButtonEnum reply = Dialogs::questionDialog( tr("Crash Test").toStdString(),
                                                            tr("You are about to make %1 crash to test the reporting system.\n"
                                                               "Do you really want to crash?").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), false,
                                                            StandardButtons(eStandardButtonYes | eStandardButtonNo) );
        if (reply == eStandardButtonYes) {
            crash_application();
        }
#endif
    } else if ( ( k == _scriptEditorFontChoice.get() ) || ( k == _scriptEditorFontSize.get() ) ) {
        appPTR->reloadScriptEditorFonts();
    } else if ( k == _pluginUseImageCopyForSource.get() ) {
        appPTR->setPluginsUseInputImageCopyToRender( _pluginUseImageCopyForSource->getValue() );
    } else if ( k == _enableOpenGL.get() ) {
        appPTR->refreshOpenGLRenderingFlagOnAllInstances();
        if (!_restoringSettings) {
            appPTR->clearPluginsLoadedCache();
        }
    } else {
        ret = false;
    }
    if (ret) {
        if ( ( ( k == _hostName.get() ) || ( k == _customHostName.get() ) ) && !_restoringSettings ) {
            Dialogs::warningDialog( tr("Host-name change").toStdString(), tr("Changing this requires a restart of %1 and clearing the OpenFX plug-ins load cache from the Cache menu.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        }
    }

    return ret;
} // onKnobValueChanged

////////////////////////////////////////////////////////
// "Viewers" pane

ImageBitDepthEnum
Settings::getViewersBitDepth() const
{
    if (!appPTR->isTextureFloatSupported()) {
        return eImageBitDepthByte;
    }
    int v = _texturesMode->getValue();

    if (v == 0) {
        return eImageBitDepthByte;
    } else if (v == 1) {
        return eImageBitDepthFloat;
    } else {
        return eImageBitDepthByte;
    }
}

int
Settings::getViewerTilesPowerOf2() const
{
    return _powerOf2Tiling->getValue();
}

int
Settings::getCheckerboardTileSize() const
{
    return _checkerboardTileSize->getValue();
}

void
Settings::getCheckerboardColor1(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _checkerboardColor1->getValue(0);
    *g = _checkerboardColor1->getValue(1);
    *b = _checkerboardColor1->getValue(2);
    *a = _checkerboardColor1->getValue(3);
}

void
Settings::getCheckerboardColor2(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _checkerboardColor2->getValue(0);
    *g = _checkerboardColor2->getValue(1);
    *b = _checkerboardColor2->getValue(2);
    *a = _checkerboardColor2->getValue(3);
}

bool
Settings::isAutoWipeEnabled() const
{
    return _autoWipe->getValue();
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

int
Settings::getMaxOpenedNodesViewerContext() const
{
    return _maximumNodeViewerUIOpened->getValue();
}

bool
Settings::isViewerKeysEnabled() const
{
    return _viewerKeys->getValue();
}

///////////////////////////////////////////////////////
// "Caching" pane

bool
Settings::isAggressiveCachingEnabled() const
{
    return _aggressiveCaching->getValue();
}

double
Settings::getRamMaximumPercent() const
{
    return (double)_maxRAMPercent->getValue() / 100.;
}

U64
Settings::getMaximumViewerDiskCacheSize() const
{
    return (U64)( _maxViewerDiskCacheGB->getValue() ) * 1024 * 1024 * 1024;
}

U64
Settings::getMaximumDiskCacheNodeSize() const
{
    return (U64)( _maxDiskCacheNodeGB->getValue() ) * 1024 * 1024 * 1024;
}

///////////////////////////////////////////////////

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
    _numberOfThreads->setValue(threadsNb);
}

bool
Settings::isAutoPreviewOnForNewProjects() const
{
    return _autoPreviewEnabledForNewProjects->getValue();
}

#ifdef NATRON_DOCUMENTATION_ONLINE
int
Settings::getDocumentationSource() const
{
    return _documentationSource->getValue();
}
#endif

int
Settings::getServerPort() const
{
    return _wwwServerPort->getValue();
}

void
Settings::setServerPort(int port) const
{
    _wwwServerPort->setValue(port);
}

bool
Settings::isAutoScrollEnabled() const
{
    return _autoScroll->getValue();
}

QString
Settings::makeHTMLDocumentation(bool genHTML) const
{
    QString ret;
    QString markdown;
    QTextStream ts(&ret);
    QTextStream ms(&markdown);

    ms << ( QString::fromUtf8("<!--") + tr("Do not edit this file! It is generated automatically by %1 itself.").arg ( QString::fromUtf8( NATRON_APPLICATION_NAME) ) + QString::fromUtf8(" -->\n\n") );
    ms << tr("Preferences") << "\n==========\n\n";

    const KnobsVec& knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->getDefaultIsSecret() ) {
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
        QString prefs_head =
        QString::fromUtf8(
        "<!DOCTYPE html>\n"
        "<!--[if IE 8]><html class=\"no-js lt-ie9\" lang=\"en\" > <![endif]-->\n"
        "<!--[if gt IE 8]><!--> <html class=\"no-js\" lang=\"en\" > <!--<![endif]-->\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "<title>Preferences &mdash; NATRON_DOCUMENTATION</title>\n"
        "<script type=\"text/javascript\" src=\"_static/js/modernizr.min.js\"></script>\n"
        "<script type=\"text/javascript\" id=\"documentation_options\" data-url_root=\"./\" src=\"_static/documentation_options.js\"></script>\n"
        "<script src=\"_static/jquery.js\"></script>\n"
        "<script src=\"_static/underscore.js\"></script>\n"
        "<script src=\"_static/doctools.js\"></script>\n"
        "<script src=\"_static/language_data.js\"></script>\n"
        "<script async=\"async\" src=\"https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/latest.js?config=TeX-AMS-MML_HTMLorMML\"></script>\n"
        "<script type=\"text/javascript\" src=\"_static/js/theme.js\"></script>\n"
        "<link rel=\"stylesheet\" href=\"_static/css/theme.css\" type=\"text/css\" />\n"
        "<link rel=\"stylesheet\" href=\"_static/pygments.css\" type=\"text/css\" />\n"
        "<link rel=\"stylesheet\" href=\"_static/theme_overrides.css\" type=\"text/css\" />\n"
        "<link rel=\"index\" title=\"Index\" href=\"genindex.html\" />\n"
        "<link rel=\"search\" title=\"Search\" href=\"search.html\" />\n"
        "<link rel=\"next\" title=\"Environment Variables\" href=\"_environment.html\" />\n"
        "<link rel=\"prev\" title=\"Reference Guide\" href=\"_group.html\" />\n"
        "</head>\n"
        "<body class=\"wy-body-for-nav\">\n"
        "<div class=\"wy-grid-for-nav\">\n"
        "<nav data-toggle=\"wy-nav-shift\" class=\"wy-nav-side\">\n"
        "<div class=\"wy-side-scroll\">\n"
        "<div class=\"wy-side-nav-search\" >\n"
        "<a href=\"index.html\" class=\"icon icon-home\"> Natron\n"
        "<img src=\"_static/logo.png\" class=\"logo\" alt=\"Logo\"/>\n"
        "</a>\n"
        "<div class=\"version\">\n"
        "2.3\n"
        "</div>\n"
        "<div role=\"search\">\n"
        "<form id=\"rtd-search-form\" class=\"wy-form\" action=\"search.html\" method=\"get\">\n"
        "<input type=\"text\" name=\"q\" placeholder=\"Search docs\" />\n"
        "<input type=\"hidden\" name=\"check_keywords\" value=\"yes\" />\n"
        "<input type=\"hidden\" name=\"area\" value=\"default\" />\n"
        "</form>\n"
        "</div>\n"
        "</div>\n"
        "<div class=\"wy-menu wy-menu-vertical\" data-spy=\"affix\" role=\"navigation\" aria-label=\"main navigation\">\n"
        "<ul class=\"current\">\n"
        "<li class=\"toctree-l1\"><a class=\"reference internal\" href=\"guide/index.html\">User Guide</a></li>\n"
        "<li class=\"toctree-l1 current\"><a class=\"reference internal\" href=\"_group.html\">Reference Guide</a><ul class=\"current\">\n"
        "<li class=\"toctree-l2 current\"><a class=\"current reference internal\" href=\"#\">Preferences</a><ul>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#general\">General</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#threading\">Threading</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#rendering\">Rendering</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#gpu-rendering\">GPU Rendering</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#project-setup\">Project Setup</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#documentation\">Documentation</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#user-interface\">User Interface</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#color-management\">Color Management</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#caching\">Caching</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#viewer\">Viewer</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#nodegraph\">Nodegraph</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#plug-ins\">Plug-ins</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#python\">Python</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#appearance\">Appearance</a><ul>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#main-window\">Main Window</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#curve-editor\">Curve Editor</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#dope-sheet\">Dope Sheet</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#node-graph\">Node Graph</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#script-editor\">Script Editor</a></li>\n"
        "</ul>\n"
        "</li>\n"
        "</ul>\n"
        "</li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_environment.html\">Environment Variables</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupImage.html\">Image nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupDraw.html\">Draw nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupTime.html\">Time nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupChannel.html\">Channel nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupColor.html\">Color nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupFilter.html\">Filter nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupKeyer.html\">Keyer nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupMerge.html\">Merge nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupTransform.html\">Transform nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupViews.html\">Views nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupOther.html\">Other nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupGMIC.html\">GMIC nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupExtra.html\">Extra nodes</a></li>\n"
        "</ul>\n"
        "</li>\n"
        "<li class=\"toctree-l1\"><a class=\"reference internal\" href=\"devel/index.html\">Developers Guide</a></li>\n"
        "</ul>\n"
        "</div>\n"
        "</div>\n"
        "</nav>\n"
        "<section data-toggle=\"wy-nav-shift\" class=\"wy-nav-content-wrap\">\n"
        "<nav class=\"wy-nav-top\" aria-label=\"top navigation\">\n"
        "<i data-toggle=\"wy-nav-top\" class=\"fa fa-bars\"></i>\n"
        "<a href=\"index.html\">Natron</a>\n"
        "</nav>\n"
        "<div class=\"wy-nav-content\">\n"
        "<div class=\"rst-content\">\n"
        "<div role=\"navigation\" aria-label=\"breadcrumbs navigation\">\n"
        "<ul class=\"wy-breadcrumbs\">\n"
        "<li><a href=\"index.html\">Docs</a> &raquo;</li>\n"
        "<li><a href=\"_group.html\">Reference Guide</a> &raquo;</li>\n"
        "<li>Preferences</li>\n"
        "<li class=\"wy-breadcrumbs-aside\">\n"
        "<a href=\"_sources/_prefs.rst.txt\" rel=\"nofollow\"> View page source</a>\n"
        "</li>\n"
        "</ul>\n"
        "<hr/>\n"
        "</div>\n"
        "<div role=\"main\" class=\"document\" itemscope=\"itemscope\" itemtype=\"http://schema.org/Article\">\n"
        "<div itemprop=\"articleBody\">\n"
        "<div class=\"section\" id=\"preferences\">\n");

        QString prefs_foot =
        QString::fromUtf8(
        "</div>\n"
        "</div>\n"
        "</div>\n"
        "<footer>\n"
        "<div class=\"rst-footer-buttons\" role=\"navigation\" aria-label=\"footer navigation\">\n"
        "<a href=\"_environment.html\" class=\"btn btn-neutral float-right\" title=\"Environment Variables\" accesskey=\"n\" rel=\"next\">Next <span class=\"fa fa-arrow-circle-right\"></span></a>\n"
        "<a href=\"_group.html\" class=\"btn btn-neutral float-left\" title=\"Reference Guide\" accesskey=\"p\" rel=\"prev\"><span class=\"fa fa-arrow-circle-left\"></span> Previous</a>\n"
        "</div>\n"
        "<hr/>\n"
        "<div role=\"contentinfo\">\n"
        "<p>\n"
        "&copy; Copyright 2013-2021 The Natron documentation authors, licensed under CC BY-SA 4.0\n"
        "</p>\n"
        "</div>\n"
        "Built with <a href=\"http://sphinx-doc.org/\">Sphinx</a> using a <a href=\"https://github.com/rtfd/sphinx_rtd_theme\">theme</a> provided by <a href=\"https://readthedocs.org\">Read the Docs</a>.\n"
        "</footer>\n"
        "</div>\n"
        "</div>\n"
        "</section>\n"
        "</div>\n"
        "<script type=\"text/javascript\">\n"
        "jQuery(function () {\n"
        "SphinxRtdTheme.Navigation.enable(true);\n"
        "});\n"
        "</script>\n"
        "</body>\n"
        "</html>\n");

        ts << prefs_head;
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixSettingsHTML(html);
        ts << prefs_foot;
    } else {
        ts << markdown;
    }

    return ret;
} // Settings::makeHTMLDocumentation

void
Settings::populateSystemFonts(const QSettings& settings,
                              const std::vector<std::string>& fonts)
{
    std::vector<ChoiceOption> options(fonts.size());
    for (std::size_t i = 0; i < fonts.size(); ++i) {
        options[i].id = fonts[i];
    }
    _systemFontChoice->populateChoices(options);
    _scriptEditorFontChoice->populateChoices(options);

    for (U32 i = 0; i < fonts.size(); ++i) {
        if (fonts[i] == NATRON_FONT) {
            _systemFontChoice->setDefaultValue(i);
        }
        if (fonts[i] == NATRON_SCRIPT_FONT) {
            _scriptEditorFontChoice->setDefaultValue(i);
        }
    }
    ///Now restore properly the system font choice
    {
        QString name = QString::fromUtf8( _systemFontChoice->getName().c_str() );
        if ( settings.contains(name) ) {
            std::string value = settings.value(name).toString().toStdString();
            for (U32 i = 0; i < fonts.size(); ++i) {
                if (fonts[i] == value) {
                    _systemFontChoice->setValue(i);
                    break;
                }
            }
        }
    }
    {
        QString name = QString::fromUtf8( _scriptEditorFontChoice->getName().c_str() );
        if ( settings.contains(name) ) {
            std::string value = settings.value(name).toString().toStdString();
            for (U32 i = 0; i < fonts.size(); ++i) {
                if (fonts[i] == value) {
                    _scriptEditorFontChoice->setValue(i);
                    break;
                }
            }
        }
    }
}

void
Settings::getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const
{
    assert(paths);
    try {
        _extraPluginPaths->getPaths(paths);
    } catch (std::logic_error&) {
        paths->clear();
    }
}

bool
Settings::getUseStdOFXPluginsLocation() const
{
    return _useStdOFXPluginsLocation->getValue();
}

void
Settings::restoreDefault()
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    if ( !QFile::remove( settings.fileName() ) ) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }

    beginChanges();
    const KnobsVec & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            knobs[i]->resetToDefaultValue(j);
        }
    }
    setCachingLabels();
    endChanges();
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
Settings::isAutoSaveEnabledForUnsavedProjects() const
{
    return _autoSaveUnSavedProjects->getValue();
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
    _checkForUpdates->setValue(enabled);
    saveSetting( _checkForUpdates.get() );
}

#ifdef NATRON_USE_BREAKPAD
bool
Settings::isCrashReportingEnabled() const
{
    return _enableCrashReports->getValue();
}
#endif

int
Settings::getMaxPanelsOpened() const
{
    return _maxPanelsOpened->getValue();
}

void
Settings::setMaxPanelsOpened(int maxPanels)
{
    _maxPanelsOpened->setValue(maxPanels);
    saveSetting( _maxPanelsOpened.get() );
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
Settings::getDefaultBackdropColor(float *r,
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
    int entry_i =  _hostName->getValue();
    std::vector<ChoiceOption> entries = _hostName->getEntries_mt_safe();

    if ( (entry_i >= 0) && ( entry_i < (int)entries.size() ) && (entries[entry_i].id == NATRON_CUSTOM_HOST_NAME_ENTRY) ) {
        return _customHostName->getValue();
    } else {
        if ( (entry_i >= 0) && ( entry_i < (int)_knownHostNames.size() ) ) {
            return _knownHostNames[entry_i].id;
        }

        return std::string();
    }
}

bool
Settings::getRenderOnEditingFinishedOnly() const
{
    return _renderOnEditingFinished->getValue();
}

void
Settings::setRenderOnEditingFinishedOnly(bool render)
{
    _renderOnEditingFinished->setValue(render);
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
Settings::getLoadProjectWorkspce() const
{
    return _loadProjectsWorkspace->getValue();
}

#ifdef Q_OS_WIN
bool
Settings::getEnableConsoleWindow() const
{
    return _enableConsoleWindow->getValue();
}
#endif

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
Settings::getNumberOfParallelRenders() const
{
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL

    return _numberOfParallelRenders->getValue();
#else

    return 1;
#endif
}

void
Settings::setNumberOfParallelRenders(int nb)
{
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders->setValue(nb);
#endif
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
    _useThreadPool->setValue(use);
}

bool
Settings::useInputAForMergeAutoConnect() const
{
    return _useInputAForMergeAutoConnect->getValue();
}

void
Settings::doOCIOStartupCheckIfNeeded()
{
    bool docheck = _ocioStartupCheck->getValue();
    AppInstancePtr mainInstance = appPTR->getTopLevelInstance();

    if (!mainInstance) {
        qDebug() << "WARNING: doOCIOStartupCheckIfNeeded() called without a AppInstance";

        return;
    }

    if (docheck && mainInstance) {
        int entry_i = _ocioConfigKnob->getValue();
        std::vector<ChoiceOption> entries = _ocioConfigKnob->getEntries_mt_safe();
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
            _ocioStartupCheck->setValue(!stopAsking);
            saveSetting( _ocioStartupCheck.get() );
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
                _ocioConfigKnob->setValue(defaultIndex);
                saveSetting( _ocioConfigKnob.get() );
            } else {
                Dialogs::warningDialog( "OCIO config", tr("The %2 OCIO config could not be found.\n"
                                                          "This is probably because you're not using the OpenColorIO-Configs folder that should "
                                                          "be bundled with your %1 installation.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString() );
            }
        }
    }
} // Settings::doOCIOStartupCheckIfNeeded

bool
Settings::didSettingsExistOnStartup() const
{
    return _settingsExisted;
}

bool
Settings::notifyOnFileChange() const
{
    return _notifyOnFileChange->getValue();
}

bool
Settings::isAutoTurboEnabled() const
{
    return _autoTurbo->getValue();
}

void
Settings::setAutoTurboModeEnabled(bool e)
{
    _autoTurbo->setValue(e);
}

void
Settings::setOptionalInputsAutoHidden(bool hidden)
{
    _hideOptionalInputsAutomatically->setValue(hidden);
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
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.setValue( QString::fromUtf8( _templatesPluginPaths->getName().c_str() ), QVariant( QString::fromUtf8( _templatesPluginPaths->getValue(0).c_str() ) ) );
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

void
Settings::setAutoDeclaredVariablePrintEnabled(bool enabled)
{
    _echoVariableDeclarationToPython->setValue(enabled);
    saveSetting( _echoVariableDeclarationToPython.get() );
}

bool
Settings::isPluginIconActivatedOnNodeGraph() const
{
    return _usePluginIconsInNodeGraph->getValue();
}

bool
Settings::isNodeGraphAntiAliasingEnabled() const
{
    return _useAntiAliasing->getValue();
}

void
Settings::getSunkenColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _sunkenColor->getValue(0);
    *g = _sunkenColor->getValue(1);
    *b = _sunkenColor->getValue(2);
}

void
Settings::getBaseColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _baseColor->getValue(0);
    *g = _baseColor->getValue(1);
    *b = _baseColor->getValue(2);
}

void
Settings::getRaisedColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _raisedColor->getValue(0);
    *g = _raisedColor->getValue(1);
    *b = _raisedColor->getValue(2);
}

void
Settings::getSelectionColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _selectionColor->getValue(0);
    *g = _selectionColor->getValue(1);
    *b = _selectionColor->getValue(2);
}

void
Settings::getInterpolatedColor(double* r,
                               double* g,
                               double* b) const
{
    *r = _interpolatedColor->getValue(0);
    *g = _interpolatedColor->getValue(1);
    *b = _interpolatedColor->getValue(2);
}

void
Settings::getKeyframeColor(double* r,
                           double* g,
                           double* b) const
{
    *r = _keyframeColor->getValue(0);
    *g = _keyframeColor->getValue(1);
    *b = _keyframeColor->getValue(2);
}

void
Settings::getTrackerKeyframeColor(double* r,
                                  double* g,
                                  double* b) const
{
    *r = _trackerKeyframeColor->getValue(0);
    *g = _trackerKeyframeColor->getValue(1);
    *b = _trackerKeyframeColor->getValue(2);
}

void
Settings::getExprColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _exprColor->getValue(0);
    *g = _exprColor->getValue(1);
    *b = _exprColor->getValue(2);
}

void
Settings::getTextColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _textColor->getValue(0);
    *g = _textColor->getValue(1);
    *b = _textColor->getValue(2);
}

void
Settings::getAltTextColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _altTextColor->getValue(0);
    *g = _altTextColor->getValue(1);
    *b = _altTextColor->getValue(2);
}

void
Settings::getTimelinePlayheadColor(double* r,
                                   double* g,
                                   double* b) const
{
    *r = _timelinePlayheadColor->getValue(0);
    *g = _timelinePlayheadColor->getValue(1);
    *b = _timelinePlayheadColor->getValue(2);
}

void
Settings::getTimelineBoundsColor(double* r,
                                 double* g,
                                 double* b) const
{
    *r = _timelineBoundsColor->getValue(0);
    *g = _timelineBoundsColor->getValue(1);
    *b = _timelineBoundsColor->getValue(2);
}

void
Settings::getTimelineBGColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _timelineBGColor->getValue(0);
    *g = _timelineBGColor->getValue(1);
    *b = _timelineBGColor->getValue(2);
}

void
Settings::getCachedFrameColor(double* r,
                              double* g,
                              double* b) const
{
    *r = _cachedFrameColor->getValue(0);
    *g = _cachedFrameColor->getValue(1);
    *b = _cachedFrameColor->getValue(2);
}

void
Settings::getDiskCachedColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _diskCachedFrameColor->getValue(0);
    *g = _diskCachedFrameColor->getValue(1);
    *b = _diskCachedFrameColor->getValue(2);
}

void
Settings::getCurveEditorBGColor(double* r,
                                double* g,
                                double* b) const
{
    *r = _curveEditorBGColor->getValue(0);
    *g = _curveEditorBGColor->getValue(1);
    *b = _curveEditorBGColor->getValue(2);
}

void
Settings::getCurveEditorGridColor(double* r,
                                  double* g,
                                  double* b) const
{
    *r = _gridColor->getValue(0);
    *g = _gridColor->getValue(1);
    *b = _gridColor->getValue(2);
}

void
Settings::getCurveEditorScaleColor(double* r,
                                   double* g,
                                   double* b) const
{
    *r = _curveEditorScaleColor->getValue(0);
    *g = _curveEditorScaleColor->getValue(1);
    *b = _curveEditorScaleColor->getValue(2);
}

void
Settings::getDopeSheetEditorBackgroundColor(double *r,
                                            double *g,
                                            double *b) const
{
    *r = _dopeSheetEditorBackgroundColor->getValue(0);
    *g = _dopeSheetEditorBackgroundColor->getValue(1);
    *b = _dopeSheetEditorBackgroundColor->getValue(2);
}

void
Settings::getDopeSheetEditorRootRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _dopeSheetEditorRootSectionBackgroundColor->getValue(0);
    *g = _dopeSheetEditorRootSectionBackgroundColor->getValue(1);
    *b = _dopeSheetEditorRootSectionBackgroundColor->getValue(2);
    *a = _dopeSheetEditorRootSectionBackgroundColor->getValue(3);
}

void
Settings::getDopeSheetEditorKnobRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _dopeSheetEditorKnobSectionBackgroundColor->getValue(0);
    *g = _dopeSheetEditorKnobSectionBackgroundColor->getValue(1);
    *b = _dopeSheetEditorKnobSectionBackgroundColor->getValue(2);
    *a = _dopeSheetEditorKnobSectionBackgroundColor->getValue(3);
}

void
Settings::getDopeSheetEditorScaleColor(double *r,
                                       double *g,
                                       double *b) const
{
    *r = _dopeSheetEditorScaleColor->getValue(0);
    *g = _dopeSheetEditorScaleColor->getValue(1);
    *b = _dopeSheetEditorScaleColor->getValue(2);
}

void
Settings::getDopeSheetEditorGridColor(double *r,
                                      double *g,
                                      double *b) const
{
    *r = _dopeSheetEditorGridColor->getValue(0);
    *g = _dopeSheetEditorGridColor->getValue(1);
    *b = _dopeSheetEditorGridColor->getValue(2);
}

void
Settings::getSEKeywordColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _keywordColor->getValue(0);
    *g = _keywordColor->getValue(1);
    *b = _keywordColor->getValue(2);
}

void
Settings::getSEOperatorColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _operatorColor->getValue(0);
    *g = _operatorColor->getValue(1);
    *b = _operatorColor->getValue(2);
}

void
Settings::getSEBraceColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _braceColor->getValue(0);
    *g = _braceColor->getValue(1);
    *b = _braceColor->getValue(2);
}

void
Settings::getSEDefClassColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _defClassColor->getValue(0);
    *g = _defClassColor->getValue(1);
    *b = _defClassColor->getValue(2);
}

void
Settings::getSEStringsColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _stringsColor->getValue(0);
    *g = _stringsColor->getValue(1);
    *b = _stringsColor->getValue(2);
}

void
Settings::getSECommentsColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _commentsColor->getValue(0);
    *g = _commentsColor->getValue(1);
    *b = _commentsColor->getValue(2);
}

void
Settings::getSESelfColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _selfColor->getValue(0);
    *g = _selfColor->getValue(1);
    *b = _selfColor->getValue(2);
}

void
Settings::getSENumbersColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _numbersColor->getValue(0);
    *g = _numbersColor->getValue(1);
    *b = _numbersColor->getValue(2);
}

void
Settings::getSECurLineColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _curLineColor->getValue(0);
    *g = _curLineColor->getValue(1);
    *b = _curLineColor->getValue(2);
}

void
Settings::getSliderColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _sliderColor->getValue(0);
    *g = _sliderColor->getValue(1);
    *b = _sliderColor->getValue(2);
}

int
Settings::getSEFontSize() const
{
    return _scriptEditorFontSize->getValue();
}

std::string
Settings::getSEFontFamily() const
{
    return _scriptEditorFontChoice->getActiveEntry().id;
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

int
Settings::getDopeSheetEditorNodeSeparationWith() const
{
    return 4;
}

bool
Settings::isNaNHandlingEnabled() const
{
    return _convertNaNValues->getValue();
}

bool
Settings::isCopyInputImageForPluginRenderEnabled() const
{
    return _pluginUseImageCopyForSource->getValue();
}

void
Settings::setOnProjectCreatedCB(const std::string& func)
{
    _onProjectCreated->setValue(func);
}

void
Settings::setOnProjectLoadedCB(const std::string& func)
{
    _defaultOnProjectLoaded->setValue(func);
}

bool
Settings::isDefaultAppearanceOutdated() const
{
    return _defaultAppearanceOutdated;
}

void
Settings::restoreDefaultAppearance()
{
    std::vector<KnobIPtr> children = _appearanceTab->getChildren();

    for (std::size_t i = 0; i < children.size(); ++i) {
        KnobColor* isColorKnob = dynamic_cast<KnobColor*>( children[i].get() );
        if ( isColorKnob && isColorKnob->isSimplified() ) {
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

void
Settings::setRenderQueuingEnabled(bool enabled)
{
    _queueRenders->setValue(enabled);
    saveSetting( _queueRenders.get() );
}

bool
Settings::isRenderQueuingEnabled() const
{
    return _queueRenders->getValue();
}

bool
Settings::isFileDialogEnabledForNewWriters() const
{
#ifdef NATRON_ENABLE_IO_META_NODES

    return _filedialogForWriters->getValue();
#else

    return true;
#endif
}

bool
Settings::isDriveLetterToUNCPathConversionEnabled() const
{
    return !_enableMappingFromDriveLettersToUNCShareNames->getValue();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Settings.cpp"
