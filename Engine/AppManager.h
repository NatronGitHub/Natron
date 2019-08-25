/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h"

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QtGlobal> // for Q_OS_*
// /usr/include/qt5/QtCore/qgenericatomic.h:177:13: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QMap>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#endif

#include "Engine/AfterQuitProcessingI.h"
#include "Engine/Plugin.h"
#include "Engine/KnobFactory.h"
#include "Engine/LogEntry.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


/*macro to get the unique pointer to the controller*/
#define appPTR AppManager::instance()


#define TO_DPI(x, y) ( appPTR->adjustSizeToDPI(x, y) )
#define TO_DPIX(x) ( appPTR->adjustSizeToDPIX(x) )
#define TO_DPIY(y) ( appPTR->adjustSizeToDPIY(y) )


enum AppInstanceStatusEnum
{
    eAppInstanceStatusInactive = 0,     //< the app has not been loaded yet or has been closed already
    eAppInstanceStatusActive     //< the app is active and can be used
};


struct PythonUserCommand
{
    QString grouping;
    Key key;
    KeyboardModifiers modifiers;
    std::string pythonFunction;
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


    AppInstancePtr getAppInstance(int appID) const WARN_UNUSED_RETURN;

    int getNumInstances() const WARN_UNUSED_RETURN;

    void removeInstance(int appID);

    void setAsTopLevelInstance(int appID);

    const AppInstanceVec& getAppInstances() const WARN_UNUSED_RETURN;
    AppInstancePtr getTopLevelInstance () const WARN_UNUSED_RETURN;
    const PluginsMap & getPluginsList() const WARN_UNUSED_RETURN;
    PluginPtr getPluginBinary(const QString & pluginId,
                            int majorVersion,
                            int minorVersion,
                            bool caseSensitive = true) const WARN_UNUSED_RETURN;
    PluginPtr getPluginBinaryFromOldID(const QString & pluginId, int majorVersion, int minorVersion, bool caseSensitive) const WARN_UNUSED_RETURN;

    /*Find a builtin format with the same resolution and aspect ratio*/
    Format findExistingFormat(int w, int h, double par = 1.0) const WARN_UNUSED_RETURN;
    const std::vector<Format> & getFormats() const WARN_UNUSED_RETURN;

    CacheBasePtr getGeneralPurposeCache() const;

    CacheBasePtr getTileCache() const;

    void deleteCacheEntriesInSeparateThread(const std::list<ImageStorageBasePtr> & entriesToDelete);

    /**
     * @brief Notifies the StorageDeleterThread that it should check cache memory. If full, it will evict
     * (the least recent) entries from the cache until it falls under the max memory amount.
     **/
    void checkCachesMemory();

    SettingsPtr getCurrentSettings() const WARN_UNUSED_RETURN;
    const KnobFactory & getKnobFactory() const WARN_UNUSED_RETURN;

    std::string getCacheDirPath() const WARN_UNUSED_RETURN;

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
    std::string getApplicationBinaryFilePath() const;
    std::string getApplicationBinaryDirPath() const;
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

    virtual void debugImage(const ImagePtr& /*image*/,
                            const RectI& /*roi*/,
                            const QString & /*filename = QString()*/) const
    {
    }

    void registerPlugin(const PluginPtr& plugin);

    void onCheckerboardSettingsChanged() { Q_EMIT checkerboardSettingsChanged(); }

    void onOCIOConfigPathChanged(const std::string& path);
    ///Non MT-safe!
    const std::string& getOCIOConfigPath() const;


    int getHardwareIdealThreadCount();
    int getPhysicalThreadCount();
    int getMaxThreadCount(); //!<  actual number of threads in the thread pool (depends on application settings)

    void setOFXLastActionCaller_TLS(const OfxEffectInstancePtr& effect);

    OfxEffectInstancePtr getOFXCurrentEffect_TLS() const;

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


    bool isAggressiveCachingEnabled() const;

    PyObject* getMainModule();

    QStringList getAllNonOFXPluginsPaths() const;

    QDir getBundledPluginDirectory() const;

    QString getSystemNatronPluginDirectory() const;

    void launchPythonInterpreter();

    int isProjectAlreadyOpened(const std::string& projectFilePath) const;

    virtual void reloadStylesheets() {}

private:
    friend class PythonGILLocker;
#ifdef USE_NATRON_GIL
    void takeNatronGIL();

    void releaseNatronGIL();
#endif
public:

    int getGILLockedCount() const;

#ifdef __NATRON_WIN32__
    void registerUNCPath(const QString& path, const QChar& driveLetter);
    QString mapUNCPathToPathWithDriveLetter(const QString& uncPath) const;
#endif

#ifdef Q_OS_UNIX
    static QString qt_tildeExpansion(const QString &path, bool *expanded = 0);
#endif


    void setOFXHostHandle(void* handle);

    OFX::Host::ImageEffect::Descriptor* getPluginContextAndDescribe(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                                    ContextEnum* ctx);
    AppTLS* getAppTLS() const;
    const OfxHost* getOFXHost() const;
    GPUContextPool* getGPUContextPool() const;

    const MultiThread* getMultiThreadHandler() const;


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

    void addMenuCommand(const std::string& grouping,
                        const std::string& pythonFunction,
                        const KeyboardModifiers& modifiers,
                        Key key);

    const std::list<PythonUserCommand>& getUserPythonCommands() const;



    void setOnProjectLoadedCallback(const std::string& pythonFunc);
    void setOnProjectCreatedCallback(const std::string& pythonFunc);

    void requestOFXDIalogOnMainThread(const OfxEffectInstancePtr& instance, void* instanceData);


    ///Closes the application not saving any projects.
    virtual void exitApp(bool warnUserForSave);

    bool isSpawnedFromCrashReporter() const;

    virtual void reloadScriptEditorFonts() {}

    QString getBoostVersion() const;

    QString getQtVersion() const;

    QString getCairoVersion() const;

    QString getHoedownVersion() const;

    QString getCeresVersion() const;

    QString getOpenMVGVersion() const;

    QString getQHTTPServerVersion() const;

    QString getPySideVersion() const;

    int getOpenGLVersionMajor() const;

    int getOpenGLVersionMinor() const;

    bool initializeOpenGLFunctionsOnce(bool createOpenGLContext = false);

    const std::list<OpenGLRendererInfo>& getOpenGLRenderers() const;

    void refreshOpenGLRenderingFlagOnAllInstances();

    /**
     * @brief Returns true if we could correctly fetch needed OpenGL functions and extensions
     **/
    bool isOpenGLLoaded() const;

    bool isTextureFloatSupported() const;

    bool hasOpenGLForRequirements(OpenGLRequirementsTypeEnum type, QString* missingOpenGLError = 0) const;

    virtual void updateAboutWindowLibrariesVersion() {}

#ifdef __NATRON_WIN32__
    const OSGLContext_wgl_data* getWGLData() const;
#endif
#ifdef __NATRON_LINUX__
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

    virtual NodePtr createNodeForProjectLoading(const SERIALIZATION_NAMESPACE::NodeSerializationPtr& serialization, const NodeCollectionPtr& group);

    virtual void aboutToSaveProject(SERIALIZATION_NAMESPACE::ProjectSerialization* /*serialization*/) {}

    static void setApplicationLocale();

    void setLastPythonAPICaller_TLS(const EffectInstancePtr& effect);

    EffectInstancePtr getLastPythonAPICaller_TLS() const;


    /**
     * @brief A application-wide TLS struct containing all stuff needed to workaround Python expressions:
     * There's a unique main module with attributes, hence we cannot isolate each render from one another
     **/
    struct PythonTLSData
    {
        std::list<EffectInstancePtr> pythonEffectStack;

        PythonTLSData()
        : pythonEffectStack()
        {
        }
    };

    typedef boost::shared_ptr<PythonTLSData> PythonTLSDataPtr;


    TreeRenderQueueManagerPtr getTasksQueueManager() const;


public Q_SLOTS:

    void printCacheMemoryStats() const;

    void exitAppWithSaveWarning()
    {
        exitApp(true);
    }


    void toggleAutoHideGraphInputs();

    void clearPluginsLoadedCache();

    void clearAllCaches();

    void onMaxPanelsOpenedChanged(int maxPanels);

    void onQueueRendersChanged(bool queuingEnabled);

    void onCrashReporterNoLongerResponding();

    void onOFXDialogOnMainThreadReceived(const OfxEffectInstancePtr& instance, void* instanceData);

Q_SIGNALS:


    void checkerboardSettingsChanged();

    void s_requestOFXDialogOnMainThread(const OfxEffectInstancePtr& instance, void* instanceData);

protected:

    virtual bool initGui(const CLArgs& cl);
    virtual void loadBuiltinNodePlugins();
    virtual AppInstancePtr makeNewInstance(int appID) const;
    virtual void registerGuiMetaTypes() const
    {
    }

    virtual void initializeQApp(int &argc, char** argv);
    virtual void onLoadCompleted()
    {
    }

    virtual void onPluginLoaded(const PluginPtr& plugin);

    virtual void onAllPluginsLoaded();

    virtual void initBuiltinPythonModules();

    bool loadInternalAfterInitGui(const CLArgs& cl);

    /*
     * @brief Derived by NatronProjectConverter to load using boost serialization instead
     */
    virtual void loadProjectFromFileFunction(std::istream& ifile, const std::string& filename, const AppInstancePtr& app, SERIALIZATION_NAMESPACE::ProjectSerialization* obj);

    /**
     * @brief Check if the project is an older project made prior Natron 2.2.
     * If this is an older project, it converts the file to a new file.
     **/
    virtual bool checkForOlderProjectFile(const AppInstancePtr& app, const QString& filePathIn, QString* filePathOut);

private:

    void findAllScriptsRecursive(const QDir& directory,
                            QStringList& allPlugins,
                            QStringList *foundInit,
                            QStringList *foundInitGui);

    void findAllPresetsRecursive(const QDir& directory,
                                 QStringList& presetFiles);

    bool findAndRunScriptFile(const QString& path,
                         const QStringList& files,
                         const QString& script);


    virtual void afterQuitProcessingCallback(const GenericWatcherCallerArgsPtr& args) OVERRIDE FINAL;

    bool loadInternal(const CLArgs& cl);

    void loadPythonGroups();

    void loadNodesPresets();

    void registerEngineMetaTypes() const;

    void loadAllPlugins();

    void initPython();

    void tearDownPython();

    // To access loadProjectFromFileFunction
    friend class Project;

    static AppManager *_instance;
    boost::scoped_ptr<AppManagerPrivate> _imp;
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

StandardButtonEnum questionDialog(const std::string & title, const std::string & message, bool useHtml,
                                  StandardButtons buttons =
                                      StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                  StandardButtonEnum defaultButton = eStandardButtonNoButton);

StandardButtonEnum questionDialog(const std::string & title, const std::string & message, bool useHtml,
                                  StandardButtons buttons,
                                  StandardButtonEnum defaultButton,
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

// Clear Python error state and output
void clearPythonStdErr();
void clearPythonStdOut();

std::string getPythonStdOut();
std::string getPythonStdErr();

//void compilePyScript(const std::string& script,PyObject** code);

std::string PyStringToStdString(PyObject* obj);
std::string makeNameScriptFriendlyWithDots(const std::string& str);
std::string makeNameScriptFriendly(const std::string& str);

bool getGroupInfos(const std::string& pythonModule,
                   std::string* pluginID,
                   std::string* pluginLabel,
                   std::string* iconFilePath,
                   std::string* grouping,
                   std::string* description,
                   std::string* pythonScriptDirPath,
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

NATRON_NAMESPACE_EXIT


#endif // Engine_AppManager_h
