/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef Engine_AppManager_h
#define Engine_AppManager_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <string>
#include <vector>

#include "Global/GlobalDefines.h"

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QtGlobal> // for Q_OS_*
// /usr/include/qt5/QtCore/qgenericatomic.h:177:13: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QtCore/QProcess>
#include <QtCore/QMap>

#if (!defined(Q_MOC_RUN) && !defined(SBK_RUN)) || defined(SBK2_RUN)
#include <boost/noncopyable.hpp>
#endif

#include "Engine/AfterQuitProcessingI.h"
#include "Engine/Plugin.h"
#include "Engine/KnobFactory.h"
#include "Engine/ImageLocker.h"
#include "Engine/LogEntry.h"
#include "Engine/EngineFwd.h"

/*macro to get the unique pointer to the controller*/
#define appPTR AppManager::instance()


#define TO_DPI(x, y) ( appPTR->adjustSizeToDPI(x, y) )
#define TO_DPIX(x) ( appPTR->adjustSizeToDPIX(x) )
#define TO_DPIY(y) ( appPTR->adjustSizeToDPIY(y) )

class QDir;

NATRON_NAMESPACE_ENTER

enum AppInstanceStatusEnum
{
    eAppInstanceStatusInactive = 0,     //< the app has not been loaded yet or has been closed already
    eAppInstanceStatusActive     //< the app is active and can be used
};

class GlobalOFXTLS
{
public:
    OfxImageEffectInstance* lastEffectCallingMainEntry;

    ///Stored as int, because we need -1; list because we need it recursive for the multiThread func
    std::list<int> threadIndexes;

    GlobalOFXTLS()
        : lastEffectCallingMainEntry(0)
        , threadIndexes()
    {
    }
};

typedef std::vector<AppInstancePtr> AppInstanceVec;

struct AppManagerPrivate;
class AppManager
    : public QObject, public AfterQuitProcessingI, public boost::noncopyable
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum AppTypeEnum
    {
        eAppTypeBackground = 0, //< a background AppInstance which will not do anything but instantiate the class making it ready for use.
        //this is used by the unit tests

        eAppTypeBackgroundAutoRun, //< a background AppInstance that will launch a project and render it. If projectName is empty or
                                   //writers is empty, it doesn't make sense to call AppInstance with this parameter.

        eAppTypeBackgroundAutoRunLaunchedFromGui, //same as eAppTypeBackgroundAutoRun but a bg process launched by GUI of a main process

        eAppTypeInterpreter, //< running in Python interpreter mode

        eAppTypeGui //< a GUI AppInstance, the end-user can interact with it.
    };


    AppManager();

    /**
     * @brief This function initializes the QCoreApplication (or QApplication) object and the AppManager object (load plugins etc...)
     * It must be called right away after the constructor. It cannot be put in the constructor as this function relies on RTTI info.
     * @param argc The number of arguments passed to the main function.
     * @param argv An array of strings passed to the main function.
     * @param cl The parsed arguments passed to the command line
     * main process.
     **/
    bool load( int argc, char **argv, const CLArgs& cl);
    bool loadW( int argc, wchar_t **argv, const CLArgs& cl);

private:

    bool loadFromArgs(const CLArgs& cl);

public:

    virtual ~AppManager();

    static AppManager* instance()
    {
        return _instance;
    }

    virtual bool isBackground() const
    {
        return true;
    }

    AppManager::AppTypeEnum getAppType() const;

    bool isLoaded() const;

    AppInstancePtr newAppInstance(const CLArgs& cl, bool makeEmptyInstance);
    AppInstancePtr newBackgroundInstance(const CLArgs& cl, bool makeEmptyInstance);

private:

    AppInstancePtr newAppInstanceInternal(const CLArgs& cl, bool alwaysBackground, bool makeEmptyInstance);

public:


    virtual void hideSplashScreen()
    {
    }

    EffectInstancePtr createOFXEffect(NodePtr node,
                                  const CreateNodeArgs& args
#ifndef NATRON_ENABLE_IO_META_NODES
                                  , bool allowFileDialogs,
                                  bool *hasUsedFileDialog
#endif
                                  ) const;

    AppInstancePtr getAppInstance(int appID) const WARN_UNUSED_RETURN;

    int getNumInstances() const WARN_UNUSED_RETURN;

    void removeInstance(int appID);

    void setAsTopLevelInstance(int appID);

    const AppInstanceVec& getAppInstances() const WARN_UNUSED_RETURN;
    AppInstancePtr getTopLevelInstance () const WARN_UNUSED_RETURN;
    const PluginsMap & getPluginsList() const WARN_UNUSED_RETURN;
    Plugin* getPluginBinary(const QString & pluginId,
                            int majorVersion,
                            int minorVersion,
                            bool convertToLowerCase) const WARN_UNUSED_RETURN;
    Plugin* getPluginBinaryFromOldID(const QString & pluginId, int majorVersion, int minorVersion) const WARN_UNUSED_RETURN;

    /*Find a builtin format with the same resolution and aspect ratio*/
    Format findExistingFormat(int w, int h, double par = 1.0) const WARN_UNUSED_RETURN;
    const std::vector<Format> & getFormats() const WARN_UNUSED_RETURN;

    /**
     * @brief Attempts to load an image from cache, returns true if it could find a matching image, false otherwise.
     **/
    bool getImage(const ImageKey & key, std::list<ImagePtr>* returnValue) const;

    /**
     * @brief Same as getImage, but if it couldn't find a matching image in the cache, it will create one with the given parameters.
     **/
    bool getImageOrCreate(const ImageKey & key, const ImageParamsPtr& params,
                          ImagePtr* returnValue) const;

    bool getImage_diskCache(const ImageKey & key, std::list<ImagePtr>* returnValue) const;

    bool getImageOrCreate_diskCache(const ImageKey & key, const ImageParamsPtr& params,
                                    ImagePtr* returnValue) const;

    bool getTexture(const FrameKey & key,
                    std::list<FrameEntryPtr>* returnValue) const;

    bool getTextureOrCreate(const FrameKey & key, const FrameParamsPtr& params,
                            FrameEntryLocker* locker,
                            FrameEntryPtr* returnValue) const;


    U64 getCachesTotalMemorySize() const;
    U64 getCachesTotalDiskSize() const;
    CacheSignalEmitterPtr getOrActivateViewerCacheSignalEmitter() const;

    void setApplicationsCachesMaximumMemoryPercent(double p);

    void setApplicationsCachesMaximumViewerDiskSpace(unsigned long long size);

    void setApplicationsCachesMaximumDiskSpace(unsigned long long size);

    void removeFromNodeCache(const ImagePtr & image);
    void removeFromViewerCache(const FrameEntryPtr & texture);

    void removeFromNodeCache(U64 hash);
    void removeFromViewerCache(U64 hash);
    /**
     * @brief Given the following tree version, removes all images from the node cache with a matching
     * tree version. This is useful to wipe the cache for one particular node.
     **/
    void  removeAllImagesFromCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder, U64 treeVersion);
    void  removeAllImagesFromDiskCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder, U64 treeVersion);
    void  removeAllTexturesFromCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder, U64 treeVersion);

    void removeAllCacheEntriesForHolder(const CacheEntryHolder* holder, bool blocking);

    SettingsPtr getCurrentSettings() const WARN_UNUSED_RETURN;
    const KnobFactory & getKnobFactory() const WARN_UNUSED_RETURN;

    template <class K>
    static
    std::shared_ptr<K> createKnob(KnobHolder*  holder,
                                    const std::string &label,
                                    int dimension = 1,
                                    bool declaredByPlugin = true)
    {
        return appPTR->getKnobFactory().createKnob<K>(holder, label, dimension, declaredByPlugin);
    }

    template <class K>
    static
    std::shared_ptr<K> createKnob(KnobHolder*  holder,
                                    const QString &label,
                                    int dimension = 1,
                                    bool declaredByPlugin = true)
    {
        return createKnob<K>(holder, label.toStdString(), dimension, declaredByPlugin);
    }

    /**
     * @brief If the current process is a background process, then it will right the output pipe the
     * short message. Otherwise the longMessage is printed to stdout
     **/
    bool writeToOutputPipe(const QString & longMessage, const QString & shortMessage, bool printIfNoChannel);

    /**
     * @brief Abort any processing on all AppInstance. It is called in some very rare cases
     * such as when changing the number of threads used by the application or when a background render
     * receives a message from the GUI application.
     **/
    void abortAnyProcessing();

    virtual void setLoadingStatus(const QString & str);
    const QString & getApplicationBinaryPath() const;
    static bool parseCmdLineArgs(int argc, char* argv[],
                                 bool* isBackground,
                                 QString & projectFilename,
                                 QStringList & writers,
                                 std::list<std::pair<int, int> >& frameRanges,
                                 QString & mainProcessServerName);

    /**
     * @brief Called when the instance is exited
     **/
    void quit(const AppInstancePtr& instance);

    /**
     * @brief Same as quit except that it blocks until all processing is done instead of doing it in a separate thread.
     **/
    void quitNow(const AppInstancePtr& instance);

    /*
       @brief Calls quit() on all AppInstance's
     */
    void quitApplication();

    /**
     * @brief Starts the event loop and doesn't return until
     * quit() is called on all AppInstance's
     **/
    int exec();

    virtual void setUndoRedoStackLimit(int /*limit*/)
    {
    }

    void getErrorLog_mt_safe(std::list<LogEntry>* entries) const;

    void writeToErrorLog_mt_safe(const QString& context, const QDateTime& date, const QString & str, bool isHtml = false, const LogEntry::LogEntryColor& color = LogEntry::LogEntryColor());

    void clearErrorLog_mt_safe();

    virtual void showErrorLog();

    virtual void debugImage(const Image* /*image*/,
                            const RectI& /*roi*/,
                            const QString & /*filename = QString()*/) const
    {
    }

    Plugin* registerPlugin(const QString& resourcesPath,
                           const QStringList & groups,
                           const QString & pluginID,
                           const QString & pluginLabel,
                           const QString & pluginIconPath,
                           const QStringList & groupIconPath,
                           bool isReader,
                           bool isWriter,
                           LibraryBinary* binary,
                           bool mustCreateMutex,
                           int major,
                           int minor,
                           bool isDeprecated);

    bool isNCacheFilesOpenedCapped() const;
    size_t getNCacheFilesOpened() const;
    void increaseNCacheFilesOpened();
    void decreaseNCacheFilesOpened();

    /**
     * @brief Called by the caches to check that there's enough free memory on the computer to perform the allocation.
     * WARNING: This function may remove some entries from the caches.
     **/
    void checkCacheFreeMemoryIsGoodEnough();

    void onCheckerboardSettingsChanged() { Q_EMIT checkerboardSettingsChanged(); }

    void onOCIOConfigPathChanged(const std::string& path);
    ///Non MT-safe!
    const std::string& getOCIOConfigPath() const;


    int getHardwareIdealThreadCount();
    int getMaxThreadCount(); //!<  actual number of threads in the thread pool (depends on application settings)


    /**
     * @brief Toggle on/off multi-threading globally in Natron
     **/
    void setNumberOfThreads(int threadsNb);

    /**
     * @brief The value held by the Number of render threads settings.
     * It is stored it for faster access (1 mutex instead of 3 read/write locks)
     *
     * WARNING: This has nothing to do with the setNumberOfThreads function!
     * This function is just called by the Settings so that the getNThreadsSettings
     * function is cheap (no need to pass by the Knob API), whereas the
     * setNumberOfThreads function actually set the value of the Knob in the settings!
     **/
    void setNThreadsToRender(int nThreads);
    void setNThreadsPerEffect(int nThreadsPerEffect);
    void setUseThreadPool(bool useThreadPool);

    void getNThreadsSettings(int* nThreadsToRender, int* nThreadsPerEffect) const;
    bool getUseThreadPool() const;

    /**
     * @brief Updates the global runningThreadsCount maintained across the whole application
     **/
    void fetchAndAddNRunningThreads(int nThreads);

    /**
     * @brief Returns the number of threads that were launched by Natron for rendering.
     * This sums up threads launched by the multi-thread suite and threads launched for
     * parallel rendering.
     **/
    int getNRunningThreads() const;

    void setThreadAsActionCaller(OfxImageEffectInstance* instance, bool actionCaller);

    /**
     * @brief Returns a list of IDs of all the plug-ins currently loaded.
     * Each ID can be passed to the AppInstance::createNode function to instantiate a node
     * with a plug-in.
     **/
    std::list<std::string> getPluginIDs() const;

    /**
     * @brief Returns all plugin-ids containing the given filter string. This function compares without case sensitivity.
     **/
    std::list<std::string> getPluginIDs(const std::string& filter);
    virtual QString getAppFont() const { return QString(); }

    virtual int getAppFontSize() const { return 11; }

    virtual void setCurrentLogicalDPI(double /*dpiX*/,
                                      double /*dpiY*/) {}

    virtual double getLogicalDPIXRATIO() const
    {
        return 72;
    }

    virtual double getLogicalDPIYRATIO() const
    {
        return 72;
    }

    template <typename T>
    void adjustSizeToDPI(T &x,
                         T &y) const
    {
        x *= getLogicalDPIXRATIO();
        y *= getLogicalDPIYRATIO();
    }

    template <typename T>
    T adjustSizeToDPIX(T x) const
    {
        return x * getLogicalDPIXRATIO();
    }

    template <typename T>
    T adjustSizeToDPIY(T y) const
    {
        return y * getLogicalDPIYRATIO();
    }

    void setProjectCreatedDuringRC2Or3(bool b);

    //To by-pass a bug introduced in RC3 with the serialization of bezier curves
    bool wasProjectCreatedDuringRC2Or3() const;

    bool isNodeCacheAlmostFull() const;

    bool isAggressiveCachingEnabled() const;

    void refreshDiskCacheLocation();
    const QString& getDiskCacheLocation() const;

    void saveCaches() const;

    PyObject* getMainModule();

    QStringList getAllNonOFXPluginsPaths() const;

    QString getPyPlugsGlobalPath() const;

    void launchPythonInterpreter();

    int isProjectAlreadyOpened(const std::string& projectFilePath) const;

    virtual void reloadStylesheets() {}

#ifdef USE_NATRON_GIL
    void takeNatronGIL();

    void releaseNatronGIL();
#endif

#ifdef __NATRON_WIN32__
    void registerUNCPath(const QString& path, const QChar& driveLetter);
    QString mapUNCPathToPathWithDriveLetter(const QString& uncPath) const;
#endif

#ifdef Q_OS_UNIX
    static QString qt_tildeExpansion(const QString &path, bool *expanded = 0);
#endif

    void getMemoryStatsForCacheEntryHolder(const CacheEntryHolder* holder,
                                           std::size_t* ramOccupied,
                                           std::size_t* diskOccupied) const;

    void setOFXHostHandle(void* handle);

    OFX::Host::ImageEffect::Descriptor* getPluginContextAndDescribe(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                                    NATRON_ENUM::ContextEnum* ctx);
    AppTLS* getAppTLS() const;
    const OfxHost* getOFXHost() const;
    GPUContextPool* getGPUContextPool() const;


    /**
     * @brief Return the concatenation of all search paths of Natron, i.e:
       - The bundled plug-ins path: ../Plugin relative to the binary
       - The system wide data for Natron (architecture dependent), this is the same location as autosaves
       - The content of the NATRON_PLUGIN_PATH environment variable
       - The content of the search paths defined in the Preferences-->Plugins--> Group plugins search path
     *
     * This does not apply for OpenFX plug-ins which have their own search path.
     **/
    std::list<std::string> getNatronPath();

    /**
     * @brief Add a new path to the Natron search path
     **/
    void appendToNatronPath(const std::string& path);

    virtual void addCommand(const QString& /*grouping*/,
                            const std::string& /*pythonFunction*/,
                            Qt::Key /*key*/,
                            const Qt::KeyboardModifiers& /*modifiers*/) {}


    void setOnProjectLoadedCallback(const std::string& pythonFunc);
    void setOnProjectCreatedCallback(const std::string& pythonFunc);

    void requestOFXDIalogOnMainThread(OfxImageEffectInstance* instance, void* instanceData);


    ///Closes the application not saving any projects.
    virtual void exitApp(bool warnUserForSave);

    bool isSpawnedFromCrashReporter() const;

    virtual void reloadScriptEditorFonts() {}

    void setPluginsUseInputImageCopyToRender(bool b);
    bool isCopyInputImageForPluginRenderEnabled() const;

    QString getBoostVersion() const;

    QString getQtVersion() const;

    QString getCairoVersion() const;

    QString getHoedownVersion() const;

    QString getCeresVersion() const;

    QString getOpenMVGVersion() const;

    QString getQHTTPServerVersion() const;

    QString getPySideVersion() const;

    bool initializeOpenGLFunctionsOnce(bool createOpenGLContext = false);

    const std::list<OpenGLRendererInfo>& getOpenGLRenderers() const;

    void refreshOpenGLRenderingFlagOnAllInstances();

    /**
     * @brief Returns true if we could correctly fetch needed OpenGL functions and extensions
     **/
    bool isOpenGLLoaded() const;

    bool isTextureFloatSupported() const;

    bool hasOpenGLForRequirements(NATRON_ENUM::OpenGLRequirementsTypeEnum type, QString* missingOpenGLError = 0) const;

    virtual void updateAboutWindowLibrariesVersion() {}

    bool isOnWayland() const;

#ifdef __NATRON_WIN32__
    const OSGLContext_wgl_data* getWGLData() const;
#endif
#ifdef __NATRON_LINUX__
    const OSGLContext_egl_data* getEGLData() const;

    const OSGLContext_glx_data* getGLXData() const;
#endif

    const IOPluginsMap& getFileFormatsForReadingAndReader() const;
    const IOPluginsMap& getFileFormatsForWritingAndWriter() const;

    void getSupportedReaderFileFormats(std::vector<std::string>* formats) const;

    void getSupportedWriterFileFormats(std::vector<std::string>* formats) const;

    void getReadersForFormat(const std::string& format, IOPluginSetForFormat* decoders) const;

    void getWritersForFormat(const std::string& format, IOPluginSetForFormat* encoders) const;

    std::string getReaderPluginIDForFileType(const std::string & extension) const;
    std::string getWriterPluginIDForFileType(const std::string & extension) const;

    static void setApplicationLocale();

public Q_SLOTS:

    void exitAppWithSaveWarning()
    {
        exitApp(true);
    }

    void onViewerTileCacheSizeChanged();

    void toggleAutoHideGraphInputs();

    void clearPlaybackCache();

    void clearViewerCache();

    void clearDiskCache();

    void clearNodeCache();

    void clearExceedingEntriesFromNodeCache();

    void clearPluginsLoadedCache();

    void clearAllCaches();

    void wipeAndCreateDiskCacheStructure();

    void onNodeMemoryRegistered(qint64 mem);

    qint64 getTotalNodesMemoryRegistered() const;

    void onMaxPanelsOpenedChanged(int maxPanels);

    void onQueueRendersChanged(bool queuingEnabled);

    void onCrashReporterNoLongerResponding();

    void onOFXDialogOnMainThreadReceived(OfxImageEffectInstance* instance, void* instanceData);

Q_SIGNALS:


    void checkerboardSettingsChanged();

    void s_requestOFXDialogOnMainThread(OfxImageEffectInstance* instance, void* instanceData);

protected:

    virtual bool initGui(const CLArgs& cl);
    virtual void loadBuiltinNodePlugins(IOPluginsMap* readersMap,
                                        IOPluginsMap* writersMap);

    template <typename PLUGIN>
    void registerBuiltInPlugin(const QString& iconPath, bool isDeprecated, bool internalUseOnly);

    virtual AppInstancePtr makeNewInstance(int appID) const;
    virtual void registerGuiMetaTypes() const
    {
    }

    virtual void initializeQApp(int &argc, char** argv);
    virtual void onLoadCompleted()
    {
    }

    virtual void ignorePlugin(Plugin* /*plugin*/)
    {
    }

    virtual void onPluginLoaded(Plugin* /*plugin*/) {}

    virtual void onAllPluginsLoaded();
    virtual void clearLastRenderedTextures() {}

    virtual void initBuiltinPythonModules();

    bool loadInternalAfterInitGui(const CLArgs& cl);

private:

    void findAllScriptsRecursive(const QDir& directory,
                            QStringList& allPlugins,
                            QStringList *foundInit,
                            QStringList *foundInitGui);

    bool findAndRunScriptFile(const QString& path,
                         const QStringList& files,
                         const QString& script);


    virtual void afterQuitProcessingCallback(const GenericWatcherCallerArgsPtr& args) OVERRIDE FINAL;

    bool loadInternal(const CLArgs& cl);

    void loadPythonGroups();

    void registerEngineMetaTypes() const;

    void loadAllPlugins();

    void initPython();

    void tearDownPython();

    static AppManager *_instance;
    std::unique_ptr<AppManagerPrivate> _imp;
};

// AppManager

namespace StrUtils {

    // Ensure that path ends with a '/' character
    void ensureLastPathSeparator(QString& path);

}

struct PyCallback
{
    std::string expression; //< the one modified by Natron
    std::string originalExpression; //< the one input by the user
    PyObject* code;

    PyCallback()
        : expression(), originalExpression(),  code(0) {}
};

// put global functions in a namespace, see https://google.github.io/styleguide/cppguide.html#Nonmember,_Static_Member,_and_Global_Functions
namespace Dialogs {
void errorDialog(const std::string & title, const std::string & message, bool useHtml = false);
void errorDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml = false);

void warningDialog(const std::string & title, const std::string & message, bool useHtml = false);
void warningDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml = false);

void informationDialog(const std::string & title, const std::string & message, bool useHtml = false);
void informationDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml = false);

NATRON_ENUM::StandardButtonEnum questionDialog(const std::string & title, const std::string & message, bool useHtml,
                                  NATRON_ENUM::StandardButtons buttons =
                                      NATRON_ENUM::StandardButtons(NATRON_ENUM::eStandardButtonYes | NATRON_ENUM::eStandardButtonNo),
                                  NATRON_ENUM::StandardButtonEnum defaultButton = NATRON_ENUM::eStandardButtonNoButton);

NATRON_ENUM::StandardButtonEnum questionDialog(const std::string & title, const std::string & message, bool useHtml,
                                  NATRON_ENUM::StandardButtons buttons,
                                  NATRON_ENUM::StandardButtonEnum defaultButton,
                                  bool* stopAsking);
} // namespace Dialogs

// put global functions in a namespace, see https://google.github.io/styleguide/cppguide.html#Nonmember,_Static_Member,_and_Global_Functions
namespace NATRON_PYTHON_NAMESPACE {
/**
 * @brief Ensures that the given Python script as imported the given module
 * and returns the position of the start of the next line after the imports. Note that this position
 * can be the first character after the last one in the script.
 **/
//std::size_t ensureScriptHasModuleImport(const std::string& moduleName,std::string& script);

/**
 * @brief If the script contains import modules commands, this will return the position of the first character
 * of the next line after the last import call, if there's any, otherwise it returns 0.
 **/
//std::size_t findNewLineStartAfterImports(std::string& script);

/**
 * @brief Return a handle to the __main__ Python module, containing all global definitions.
 **/
PyObject* getMainModule();

/**
 * @brief Evaluates the given python script*
 * @param error[out] If an error occurs, this will be set to the error printed by the Python interpreter. This argument may be passed NULL,
 * however in Gui sessions, the error will not be printed in the console so it will just ignore any Python error.
 * @param output[out] The string will contain any result printed by the script on stdout. This argument may be passed NULL
 * @returns True on success, false on failure.
 **/
bool interpretPythonScript(const std::string& script, std::string* error, std::string* output);


//void compilePyScript(const std::string& script,PyObject** code);

std::string makeNameScriptFriendlyWithDots(const std::string& str);
std::string makeNameScriptFriendly(const std::string& str);

bool isKeyword(const std::string& str);

bool getGroupInfos(const std::string& modulePath,
                   const std::string& pythonModule,
                   std::string* pluginID,
                   std::string* pluginLabel,
                   std::string* iconFilePath,
                   std::string* grouping,
                   std::string* description,
                   bool* isToolset,
                   unsigned int* version);

// Does not work for functions with var args
void getFunctionArguments(const std::string& pyFunc, std::string* error, std::vector<std::string>* args);

/**
 * @brief Given a fullyQualifiedName, e.g: app1.Group1.Blur1
 * this function returns the PyObject attribute of Blur1 if it is defined, or Group1 otherwise
 * If app1 or Group1 does not exist at this point, this is a failure.
 **/
PyObject* getAttrRecursive(const std::string& fullyQualifiedName, PyObject* parentObj, bool* isDefined);
} // namespace NATRON_PYTHON_NAMESPACE

// #define DEBUG_PYTHON_GIL // to debug Python GIL issues

/**
 * @brief Small helper class to use as RAII to hold the GIL (Global Interpreter Lock) before calling ANY Python code.
 **/
class PythonGILLocker
{
    // Follow https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    PyGILState_STATE state;
#ifdef DEBUG_PYTHON_GIL
    static QMap<QString, int> pythonCount;
#ifdef USE_NATRON_GIL
    static QMap<QString, int> natronCount;
#endif
#endif
public:
    PythonGILLocker();

    ~PythonGILLocker();
};

/**
 * @brief Small helper class to use as RAII to temporarily release the GIL (Global Interpreter Lock)
 **/
class PythonGILUnlocker
{
public:
    PythonGILUnlocker();

    ~PythonGILUnlocker();
};

NATRON_NAMESPACE_EXIT


#endif // Engine_AppManager_h

