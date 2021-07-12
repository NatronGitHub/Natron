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

#include "CLArgs.h"

#include <cstddef>
#include <iostream>
#include <cassert>
#include <stdexcept>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include "Global/GlobalDefines.h"
#include "Global/GitVersion.h"
#include "Global/QtCompat.h"
#include "Global/StrUtils.h"

#include "Engine/AppManager.h"

NATRON_NAMESPACE_ENTER


struct CLArgsPrivate
{
    Q_DECLARE_TR_FUNCTIONS(CLArgs)

public:
    QString executable;
    QStringList args;
    QString filename;
    bool isPythonScript;
    QString defaultOnProjectLoadedScript;
    std::list<CLArgs::WriterArg> writers;
    std::list<CLArgs::ReaderArg> readers;
    std::list<std::string> pythonCommands;
    std::list<std::string> settingCommands; //!< executed after loading the settings
    bool isBackground;
    bool useDefaultSettings;
    bool clearCacheOnLaunch;
    bool clearOpenFXCacheOnLaunch;
    QString ipcPipe;
    int error;
    bool isInterpreterMode;
    std::list<std::pair<int, std::pair<int, int> > > frameRanges;
    bool rangeSet;
    bool enableRenderStats;
    bool isEmpty;
    mutable QString imageFilename;
#ifdef NATRON_USE_BREAKPAD
    QString breakpadPipeFilePath;
    QString breakpadComPipeFilePath;
    int breakpadPipeClientID;
    QString breakpadProcessFilePath;
    qint64 breakpadProcessPID;
#endif
    QString exportDocsPath;

    CLArgsPrivate()
        : args()
        , filename()
        , isPythonScript(false)
        , defaultOnProjectLoadedScript()
        , writers()
        , readers()
        , pythonCommands()
        , settingCommands()
        , isBackground(false)
        , useDefaultSettings(false)
        , clearCacheOnLaunch(false)
        , clearOpenFXCacheOnLaunch(false)
        , ipcPipe()
        , error(0)
        , isInterpreterMode(false)
        , frameRanges()
        , rangeSet(false)
        , enableRenderStats(false)
        , isEmpty(true)
        , imageFilename()
#ifdef NATRON_USE_BREAKPAD
        , breakpadPipeFilePath()
        , breakpadComPipeFilePath()
        , breakpadPipeClientID(-1)
        , breakpadProcessFilePath()
        , breakpadProcessPID(-1)
#endif
        , exportDocsPath()
    {
    }

    void parse();

    QStringList::iterator hasToken(const QString& longName, const QString& shortName);
    QStringList::iterator hasOutputToken(QString& indexStr);
    QStringList::iterator findFileNameWithExtension(const QString& extension);
};

CLArgs::CLArgs()
    : _imp( new CLArgsPrivate() )
{
}

// ctor
// Be careful that there are 3 constructors with very similar code, see below
CLArgs::CLArgs(int& argc,
               char* argv[],
               bool forceBackground)
    : _imp( new CLArgsPrivate() )
{
    _imp->isEmpty = false;
    if (forceBackground) {
        _imp->isBackground = true;
    }

    // Ensure application has correct locale before doing anything
    AppManager::setApplicationLocale();

    std::vector<std::string> utf8Args;
    ensureCommandLineArgsUtf8(argc, argv, &utf8Args);

    // save the program name (argv[0])
    _imp->executable = QString::fromUtf8(utf8Args[0].c_str());

    for (std::size_t i = 1; i < utf8Args.size(); ++i) {
        QString str = QString::fromUtf8(utf8Args[i].c_str());
        if ( (str.size() >= 2) && ( str[0] == QChar::fromLatin1('"') ) && ( str[str.size() - 1] == QChar::fromLatin1('"') ) ) {
            str.remove(0, 1);
            str.remove(str.size() - 1, 1);
        }
        _imp->args.push_back(str);
    }

    _imp->parse();
}

// ctor
// Be careful that there are 3 constructors with very similar code, see above and below
CLArgs::CLArgs(int& argc,
       wchar_t* argv[],
       bool forceBackground)
: _imp( new CLArgsPrivate() )
{
    _imp->isEmpty = false;
    if (forceBackground) {
        _imp->isBackground = true;
    }

    // Ensure application has correct locale before doing anything
    AppManager::setApplicationLocale();

    std::vector<std::string> utf8Args;
    for (int i = 0; i < argc; ++i) {
        std::wstring ws(argv[i]);
        std::string utf8Str = StrUtils::utf16_to_utf8(ws);
        assert(StrUtils::is_utf8(utf8Str.c_str()));
        QString str = QString::fromUtf8(utf8Str.c_str());
        if (i == 0) {
            // save the program name (argv[0])
            _imp->executable = str;
            continue;
        }
        if ( (str.size() >= 2) && ( str[0] == QChar::fromLatin1('"') ) && ( str[str.size() - 1] == QChar::fromLatin1('"') ) ) {
            str.remove(0, 1);
            str.remove(str.size() - 1, 1);
        }
        _imp->args.push_back(str);

    }
    _imp->parse();

}

// ctor
// Be careful that there are 3 constructors with very similar code, see above
CLArgs::CLArgs(const QStringList &arguments,
                bool forceBackground)
    : _imp( new CLArgsPrivate() )
{
    _imp->isEmpty = false;
    if (forceBackground) {
        _imp->isBackground = true;
    }

    // Ensure application has correct locale before doing anything
    AppManager::setApplicationLocale();

    bool first = true;
    Q_FOREACH(const QString &arg, arguments) {
        if (first) {
            // save the program name (argv[0])
            _imp->executable = arg;
            first = false;
            continue;
        }
        QString str = arg;

        // Remove surrounding quotes
        if ( (str.size() >= 2) && ( str[0] == QChar::fromLatin1('"') ) && ( str[str.size() - 1] == QChar::fromLatin1('"') ) ) {
            str.remove(0, 1);
            str.remove(str.size() - 1, 1);
        }
        _imp->args.push_back(str);
    }
    _imp->parse();
}

// GCC 4.2 requires the copy constructor
CLArgs::CLArgs(const CLArgs& other)
    : _imp( new CLArgsPrivate() )
{
    *this = other;
}

CLArgs::~CLArgs()
{
}

void
CLArgs::operator=(const CLArgs& other)
{
    _imp->executable = other._imp->executable;
    _imp->args = other._imp->args;
    _imp->filename = other._imp->filename;
    _imp->isPythonScript = other._imp->isPythonScript;
    _imp->defaultOnProjectLoadedScript = other._imp->defaultOnProjectLoadedScript;
    _imp->clearCacheOnLaunch = other._imp->clearCacheOnLaunch;
    _imp->clearOpenFXCacheOnLaunch = other._imp->clearOpenFXCacheOnLaunch;
    _imp->writers = other._imp->writers;
    _imp->readers = other._imp->readers;
    _imp->pythonCommands = other._imp->pythonCommands;
    _imp->settingCommands = other._imp->settingCommands;
    _imp->isBackground = other._imp->isBackground;
    _imp->ipcPipe = other._imp->ipcPipe;
    _imp->error = other._imp->error;
    _imp->isInterpreterMode = other._imp->isInterpreterMode;
    _imp->frameRanges = other._imp->frameRanges;
    _imp->rangeSet = other._imp->rangeSet;
    _imp->enableRenderStats = other._imp->enableRenderStats;
    _imp->isEmpty = other._imp->isEmpty;
    _imp->imageFilename = other._imp->imageFilename;
    _imp->exportDocsPath = other._imp->exportDocsPath;
}

bool
CLArgs::isEmpty() const
{
    return _imp->isEmpty;
}

void
CLArgs::printBackGroundWelcomeMessage()
{
    QString msg = tr("%1 Version %2\n"
                     "(C) 2018-2021 The Natron developers\n"
                     "(C) 2013-2018 INRIA and Alexandre Gauthier-Foichat\n"
                     ">>>Use the --help or -h option to print usage.<<<").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_VERSION_STRING) );
    std::cout << msg.toStdString() << std::endl;
}

void
CLArgs::printUsage(const std::string& programName)
{
    QString msg = tr( /* Text must hold in 80 columns ************************************************/
        "%3 usage:\n"
        "Three distinct execution modes exist in background mode:\n"
        "- The execution of %1 projects (.%2)\n"
        "- The execution of Python scripts that contain commands for %1\n"
        "- An interpreter mode where commands can be given directly to the Python\n"
        "  interpreter\n"
        "\n"
        "General options:\n"
        "  -h [ --help ]\n"
        "    Produce help message.\n"
        "  -v [ --version ]\n"
        "    Print information about %1 version.\n"
        "  -b [ --background ]\n"
        "    Enable background rendering mode. No graphical interface is shown.\n"
        "    When using %1Renderer or the -t option, this argument is implicit\n"
        "    and does not have to be given.\n"
        "    If using %1 and this option is not specified, the project is loaded\n"
        "    as if opened from the file menu.\n"
        "  -t [ --interpreter ] <python script file path>\n"
        "    Enable Python interpreter mode.\n"
        "    Python commands can be given to the interpreter and executed on the fly.\n"
        "    An optional Python script filename can be specified to source a script\n"
        "    before the interpreter is made accessible.\n"
        "    Note that %1 will not start rendering any Write node of the sourced\n"
        "    script: it must be started explicitly.\n"
        "    %1Renderer and %1 do the same thing in this mode, only the\n"
        "    init.py script is loaded.\n"
        "  --clear-cache\n"
        "    Clears the image cache on startup.\n"
        "  --clear-openfx-cache\n"
        "    Clears the OpenFX plugins cache on startup.\n"
        "  --no-settings\n"
        "    When passed on the command-line, the %1 settings will not be restored\n"
        "    from the preferences file on disk so that %1 uses the default ones.\n"
        "  --settings name=value\n"
        "    Sets the named %1 setting to the given value. This is done after loading\n"
        "    the settings and prior to executing Python commands or loading the project.\n"
        "  -c [ --cmd ] \"PythonCommand\"\n"
        "    Execute custom Python code passed as a script prior to executing the Python\n"
        "    script or loading the project passed as parameter. This option may be used\n"
        "    multiple times and each python command is executed in the order given on\n"
        "    the command-line.\n\n"
        "\n"
        /* Text must hold in 80 columns ************************************************/
        "Usage for the execution of %1 projects:\n"
        "    %3 [options] [--] <project file path> [<frameRanges>]\n"
        "Options/arguments:\n"
        "  -w [ --writer ] <Writer node script name> [<filename>]\n"
        "    Specify a Write node to render.\n"
        "    When in background mode, the renderer only renders the node script name\n"
        "    following this argument. If no such node exists in the project file, the\n"
        "    process will abort.\n"
        "    Note that if there is no --writer option, it renders all the writers in\n"
        "    the project.\n"
        "    After the writer node script name you can pass an optional output\n"
        "    filename.\n"
        "    Note that several -w options can be set to specify multiple Write nodes\n"
        "    to render.\n"
        "    You may only specify absolute file paths to the writer optional filename.\n"
        "  -i [ --reader ] <reader node script name> <filename>\n"
        "     Specify the input file/sequence/video to load for the given Reader node.\n"
        "     If the specified reader node cannot be found, the process will abort."
        "     You may only specify absolute file paths to the reader filename.\n"
        "  -l [ --onload ] <python script file path>\n"
        "    Specify a Python script to be executed after a project is created or\n"
        "    loaded.\n"
        "    Note that this is executed in GUI mode or with NatronRenderer, after\n"
        "    executing the callbacks onProjectLoaded and onProjectCreated.\n"
        "    The rules on the execution of Python scripts (see below) also apply to\n"
        "    this script.\n"
        "  -s [ --render-stats]\n"
        "     Enable render statistics that will be produced for\n"
        "     each frame in form of a file located next to the image produced by\n"
        "     the Writer node, with the same name and a -stats.txt extension. The\n"
        "     breakdown contains information about each nodes, render times etc...\n"
        "     This option is useful for debugging purposes or to control that a render\n"
        "     is working correctly.\n"
        "     **Please note** that it does not work when writing video files."
        "  <frameRanges>\n"
        "      One or more frame ranges, separated by commas.\n"
        "      Each frame range must be one of the following:\n"
        "      - <frame> (a single frame number, e.g. 57)\n"
        "      - <firstFrame>-<lastFrame> (e.g. 10-40)\n"
        "      - <firstFrame>-<lastFrame>:<frameStep> (e.g. 1-10:2 would render 1,3,5,7,9)\n"
        "      Examples:\n"
        "      1-10:1,20-30:2,40-50\n"
        "      1329,2450,123,1-10:2\n"
        "Sample uses:\n"
        "  %1 /Users/Me/MyNatronProjects/MyProject.ntp\n"
        "  %1 -b -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp\n"
        "  %1Renderer -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp\n"
        "  %1Renderer -w MyWriter /FastDisk/Pictures/sequence'###'.exr 1-100 /Users/Me/MyNatronProjects/MyProject.ntp\n"
        "  %1Renderer -w MyWriter -w MySecondWriter 1-10 /Users/Me/MyNatronProjects/MyProject.ntp\n"
        "  %1Renderer -w MyWriter 1-10 -l /Users/Me/Scripts/onProjectLoaded.py /Users/Me/MyNatronProjects/MyProject.ntp\n"
        "\n"
        /* Text must hold in 80 columns ************************************************/
        "Options for the execution of Python scripts:\n"
        "  %3 [options] [--] <Python script path>\n"
        "  [Note that the following does not apply if the -t option was given.]\n"
        "  The script argument can either be the script of a Group that was exported\n"
        "  from the graphical user interface, or an exported project, or a script\n"
        "  written by hand.\n"
        "  When executing a script, %1 first looks for a function with the\n"
        "  following signature:\n"
        "    def createInstance(app,group):\n"
        "  If this function is found, it is executed, otherwise the whole content of\n"
        "  the script is interpreted as though it were given to Python natively.\n"
        "  In either case, the \"app\" variable is always defined and points to the\n"
        "  correct application instance.\n"
        "  Note that the GUI version of the program (%1) sources the script before\n"
        "  creating the graphical user interface and does not start rendering.\n"
        "  If in background mode, the nodes to render have to be given using the -w\n"
        "  option (as described above) or with the following option:\n"
        "  -o [ --output ] <filename> <frameRange>\n"
        "    Specify an Output node in the script that should be replaced with a\n"
        "    Write node.\n"
        "    The option looks for a node named Output1 in the script and replaces it\n"
        "    by a Write node (like when creating a Write node in interactive GUI mode).\n"
        "    <filename> is a pattern for the output file names.\n"
        "    <frameRange> must be specified if it was not specified earlier on the\n"
        "    command line.\n"
        "    This option can also be used to render out multiple Output nodes, in\n"
        "    which case it has to be used like this:\n"
        "    -o1 [ --output1 ] : look for a node named Output1.\n"
        "    -o2 [ --output2 ] : look for a node named Output2 \n"
        "    etc...\n"
        "Sample uses:\n"
        "  %1 /Users/Me/MyNatronScripts/MyScript.py\n"
        "  %1 -b -w MyWriter /Users/Me/MyNatronScripts/MyScript.py\n"
        "  %1Renderer -w MyWriter /Users/Me/MyNatronScripts/MyScript.py\n"
        "  %1Renderer -o /FastDisk/Pictures/sequence'###'.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py\n"
        "  %1Renderer -o1 /FastDisk/Pictures/sequence'###'.exr -o2 /FastDisk/Pictures/test'###'.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py\n"
        "  %1Renderer -w MyWriter -o /FastDisk/Pictures/sequence'###'.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py\n"
        "\n"
        /* Text must hold in 80 columns ************************************************/
        "Options for the execution of the interpreter mode:\n"
        "  %3 -t [--] [<Python script path>]\n"
        "  %1 sources the optional script given as argument, if any, and then reads\n"
        "  Python commands from the standard input, which are interpreted by Python.\n"
        "Sample uses:\n"
        "  %1 -t\n"
        "  %1Renderer -t\n"
        "  %1Renderer -t /Users/Me/MyNatronScripts/MyScript.py\n")
                  .arg( /*%1=*/ QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( /*%2=*/ QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ).arg( /*%3=*/ QString::fromUtf8( programName.c_str() ) );
    std::cout << msg.toStdString() << std::endl;
} // CLArgs::printUsage

int
CLArgs::getError() const
{
    return _imp->error;
}

const std::list<CLArgs::WriterArg>&
CLArgs::getWriterArgs() const
{
    return _imp->writers;
}

const std::list<CLArgs::ReaderArg>&
CLArgs::getReaderArgs() const
{
    return _imp->readers;
}

const std::list<std::string>&
CLArgs::getPythonCommands() const
{
    return _imp->pythonCommands;
}

const std::list<std::string>&
CLArgs::getSettingCommands() const
{
    return _imp->settingCommands;
}

bool
CLArgs::hasFrameRange() const
{
    return _imp->rangeSet;
}

const std::list<std::pair<int, std::pair<int, int> > >&
CLArgs::getFrameRanges() const
{
    return _imp->frameRanges;
}

bool
CLArgs::isLoadedUsingDefaultSettings() const
{
    return _imp->useDefaultSettings;
}

bool
CLArgs::isCacheClearRequestedOnLaunch() const
{
    return _imp->clearCacheOnLaunch;
}

bool
CLArgs::isOpenFXCacheClearRequestedOnLaunch() const
{
    return _imp->clearOpenFXCacheOnLaunch;
}


bool
CLArgs::isBackgroundMode() const
{
    return _imp->isBackground;
}

bool
CLArgs::isInterpreterMode() const
{
    return _imp->isInterpreterMode;
}

const QString&
CLArgs::getScriptFilename() const
{
    return _imp->filename;
}

const QString&
CLArgs::getImageFilename() const
{
    if ( _imp->imageFilename.isEmpty() && !_imp->args.empty() ) {
        ///Check for image file passed to command line
        QStringList::iterator it = _imp->args.begin();
        for (; it != _imp->args.end(); ++it) {
            if ( !it->startsWith( QChar::fromLatin1('-') ) ) {
                QString fileCopy = *it;
                QString ext = QtCompat::removeFileExtension(fileCopy);
                if ( !ext.isEmpty() ) {
                    std::string readerId = appPTR->getReaderPluginIDForFileType( ext.toStdString() );
                    if ( !readerId.empty() ) {
                        _imp->imageFilename = *it;
                        _imp->args.erase(it);
                        break;
                    }
                }
            }
        }
    }

    return _imp->imageFilename;
}

const QString&
CLArgs::getDefaultOnProjectLoadedScript() const
{
    return _imp->defaultOnProjectLoadedScript;
}

const QString&
CLArgs::getIPCPipeName() const
{
    return _imp->ipcPipe;
}

bool
CLArgs::areRenderStatsEnabled() const
{
    return _imp->enableRenderStats;
}

bool
CLArgs::isPythonScript() const
{
    return _imp->isPythonScript;
}

#ifdef NATRON_USE_BREAKPAD
const QString&
CLArgs::getBreakpadProcessExecutableFilePath() const
{
    return _imp->breakpadProcessFilePath;
}

qint64
CLArgs::getBreakpadProcessPID() const
{
    return _imp->breakpadProcessPID;
}

int
CLArgs::getBreakpadClientFD() const
{
    return _imp->breakpadPipeClientID;
}

const QString&
CLArgs::getBreakpadPipeFilePath() const
{
    return _imp->breakpadPipeFilePath;
}

const QString&
CLArgs::getBreakpadComPipeFilePath() const
{
    return _imp->breakpadComPipeFilePath;
}
#endif // NATRON_USE_BREAKPAD

const QString &
CLArgs::getExportDocsPath() const
{
    return _imp->exportDocsPath;
}

QStringList::iterator
CLArgsPrivate::findFileNameWithExtension(const QString& extension)
{
    for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
        if ( it->endsWith(QChar::fromLatin1('.') + extension, Qt::CaseInsensitive) ) {
            return it;
        }
    }

    return args.end();
}

QStringList::iterator
CLArgsPrivate::hasToken(const QString& longName,
                        const QString& shortName)
{
    const QString longToken( QString::fromUtf8("--") + longName );
    const QString shortToken =  !shortName.isEmpty() ? QChar::fromLatin1('-') + shortName : QString();
    const QString endOfOptions( QString::fromUtf8("--") );

    for (QStringList::iterator it = args.begin(); it != args.end() && *it != endOfOptions; ++it) {
        if ( (*it == longToken) || ( !shortToken.isEmpty() && (*it == shortToken) ) ) {
            return it;
        }
    }

    return args.end();
}

QStringList::iterator
CLArgsPrivate::hasOutputToken(QString& indexStr)
{
    const QString outputLong( QString::fromUtf8("--output") );
    const QString outputShort( QString::fromUtf8("-o") );
    const QString endOfOptions( QString::fromUtf8("--") );

    for (QStringList::iterator it = args.begin(); it != args.end() && *it != endOfOptions; ++it) {
        int indexOf = it->indexOf(outputLong);
        if (indexOf != -1) {
            indexOf += outputLong.size();
            if ( indexOf < it->size() ) {
                indexStr = it->mid(indexOf);
                bool ok;
                indexStr.toInt(&ok);
                if (!ok) {
                    error = 1;
                    std::cout << tr("Wrong formatting for the -o option").toStdString() << std::endl;

                    return args.end();
                }
            } else {
                indexStr = QChar::fromLatin1('1');
            }

            return it;
        } else {
            indexOf = it->indexOf(outputShort);
            if (indexOf != -1) {
                if ( (it->size() > 2) && !it->at(2).isDigit() ) {
                    //This is probably the --onload option
                    return args.end();
                }
                indexOf += outputShort.size();
                if ( indexOf < it->size() ) {
                    indexStr = it->mid(indexOf);
                    bool ok;
                    indexStr.toInt(&ok);
                    if (!ok) {
                        error = 1;
                        std::cout << tr("Wrong formatting for the -o option").toStdString() << std::endl;

                        return args.end();
                    }
                } else {
                    indexStr = QChar::fromLatin1('1');
                }

                return it;
            }
        }
    }

    return args.end();
} // CLArgsPrivate::hasOutputToken

static bool
tryParseFrameRange(const QString& arg,
                   std::pair<int, int>& range,
                   int& frameStep)
{
    bool ok;
    int singleNumber = arg.toInt(&ok);

    if (ok) {
        //this is a single frame
        range.first = range.second = singleNumber;
        frameStep = INT_MIN;

        return true;
    }
    QStringList strRange = arg.split( QChar::fromLatin1('-') );
    if (strRange.size() != 2) {
        return false;
    }
    for (int i = 0; i < strRange.size(); ++i) {
        //whitespace removed from the start and the end.
        strRange[i] = strRange.at(i).trimmed();
    }
    range.first = strRange.at(0).toInt(&ok);
    if (!ok) {
        return false;
    }

    int foundColon = strRange.at(1).indexOf( QChar::fromLatin1(':') );
    if (foundColon != -1) {
        ///A frame-step has been specified
        QString lastFrameStr = strRange.at(1).mid(0, foundColon);
        QString frameStepStr = strRange.at(1).mid(foundColon + 1);
        range.second = lastFrameStr.toInt(&ok);
        if (!ok) {
            return false;
        }
        frameStep = frameStepStr.toInt(&ok);
        if (!ok) {
            return false;
        }
    } else {
        frameStep = INT_MIN;
        ///Frame range without frame-step specified
        range.second = strRange.at(1).toInt(&ok);
        if (!ok) {
            return false;
        }
    }

    return true;
}

static bool
tryParseMultipleFrameRanges(const QString& args,
                            std::list<std::pair<int, std::pair<int, int> > >& frameRanges)
{
    QStringList splits = args.split( QChar::fromLatin1(',') );
    std::list<std::pair<int, std::pair<int, int> > > newFrameRanges;

    Q_FOREACH(const QString &split, splits) {
        std::pair<int, int> frameRange;
        int frameStep = 0;

        if ( !tryParseFrameRange(split, frameRange, frameStep) ) {
            // not a frame range.
            return false;
        } else {
            newFrameRanges.push_back( std::make_pair(frameStep, frameRange) );
        }
    }

    if ( newFrameRanges.empty() ) {
        return false;
    }
    frameRanges.merge(newFrameRanges); // empties newFrameRanges

    return true;
}

void
CLArgsPrivate::parse()
{
#ifdef DEBUG
    qDebug() << "args:";
    for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
        qDebug() << *it;
    }
#endif
    {
        QStringList::iterator it = hasToken( QString::fromUtf8("version"), QString::fromUtf8("v") );
        if ( it != args.end() ) {
            it = args.erase(it);

            QString msg = tr("%1 version %2 at commit %3 on branch %4").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_VERSION_STRING) ).arg( QString::fromUtf8(GIT_COMMIT) ).arg( QString::fromUtf8(GIT_BRANCH) );
#         if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
            msg += tr(" built on %1").arg( QString::fromUtf8(__DATE__) );
#         endif
            std::cout << msg.toStdString() << std::endl;
            error = 1;

            return;
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("help"), QString::fromUtf8("h") );
        if ( it != args.end() ) {
            it = args.erase(it);

            CLArgs::printUsage( executable.toStdString() );
            error = 1;

            return;
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("clear-cache"), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);

            clearCacheOnLaunch = true;
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("clear-openfx-cache"), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);

            clearOpenFXCacheOnLaunch = true;
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("no-settings"), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);

            useDefaultSettings = true;
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("background"), QString::fromUtf8("b") );
        if ( it != args.end() ) {
            it = args.erase(it);

            isBackground = true;
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("interpreter"), QString::fromUtf8("t") );
        if ( it != args.end() ) {
            it = args.erase(it);

            isInterpreterMode = true;
            isBackground = true;
            std::cout << tr("Note: -t argument given, loading in command-line interpreter mode, only Python commands / scripts are accepted").toStdString()
                      << std::endl;
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("render-stats"), QString::fromUtf8("s") );
        if ( it != args.end() ) {
            it = args.erase(it);

            enableRenderStats = true;
        }
    }

#ifdef NATRON_USE_BREAKPAD
    {
        QStringList::iterator it = hasToken( QString::fromUtf8(NATRON_BREAKPAD_PROCESS_PID), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);

            if ( it == args.end() ) {
                std::cout << tr("You must specify the breakpad process executable file path").toStdString() << std::endl;
                error = 1;

                return;
            }
            breakpadProcessPID = it->toLongLong();
            it = args.erase(it);
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8(NATRON_BREAKPAD_PROCESS_EXEC), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);
            if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
                std::cout << tr("You must specify the breakpad process executable file path").toStdString() << std::endl;
                error = 1;

                return;
            }
            breakpadProcessFilePath = *it;
            it = args.erase(it);
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8(NATRON_BREAKPAD_CLIENT_FD_ARG), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);

            if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
                std::cout << tr("You must specify the breakpad pipe client FD").toStdString() << std::endl;
                error = 1;

                return;
            }
            breakpadPipeClientID = it->toInt();
            it = args.erase(it);
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8(NATRON_BREAKPAD_PIPE_ARG), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);

            if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
                std::cout << tr("You must specify the breakpad pipe path").toStdString() << std::endl;
                error = 1;

                return;
            }
            breakpadPipeFilePath = *it;
            args.erase(it);
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8(NATRON_BREAKPAD_COM_PIPE_ARG), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);
            if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
                std::cout << tr("You must specify the breakpad communication pipe path").toStdString() << std::endl;
                error = 1;

                return;
            }
            breakpadComPipeFilePath = *it;
            it = args.erase(it);
        }
    }
#endif // NATRON_USE_BREAKPAD

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("export-docs"), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);

            if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
                std::cout << tr("You must specify the doc dir path").toStdString() << std::endl;
                error = 1;

                return;
            }

            exportDocsPath = *it;
            it = args.erase(it);
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("IPCpipe"), QString() );
        if ( it != args.end() ) {
            it = args.erase(it);
            if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
                std::cout << tr("You must specify the IPC pipe filename").toStdString() << std::endl;
                error = 1;

                return;
            }
            ipcPipe = *it;
            it = args.erase(it);
        }
    }

    {
        QStringList::iterator it = hasToken( QString::fromUtf8("onload"), QString::fromUtf8("l") );
        if ( it != args.end() ) {
            it = args.erase(it);

            if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
                std::cout << tr("--onload or -l specified, you must enter a script filename afterwards.").toStdString() << std::endl;
                error = 1;

                return;
            }
            defaultOnProjectLoadedScript = *it;
            it = args.erase(it);

#ifdef __NATRON_UNIX__
            defaultOnProjectLoadedScript = AppManager::qt_tildeExpansion(defaultOnProjectLoadedScript);
#endif
            QFileInfo fi(defaultOnProjectLoadedScript);
            if ( !fi.exists() ) {
                std::cout << tr("WARNING: --onload %1 ignored because the file does not exist.").arg(defaultOnProjectLoadedScript).toStdString() << std::endl;
                defaultOnProjectLoadedScript.clear();
            } else {
                defaultOnProjectLoadedScript = fi.canonicalFilePath();
            }

            if ( !defaultOnProjectLoadedScript.endsWith( QString::fromUtf8(".py") ) ) {
                std::cout << tr("The optional on project load script must be a Python script (filename ending with .py).").toStdString() << std::endl;
                error = 1;

                return;
            }
        }
    }
    //Parse settings
    for (;;) {
        QStringList::iterator it = hasToken( QString::fromUtf8("setting"), QString() );
        if ( it == args.end() ) {
            break;
        }

        it = args.erase(it);
        
        if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
            std::cout << tr("You must specify a setting name and value as \"name=value\" when using the --setting option").toStdString() << std::endl;
            error = 1;

            return;
        }
        int pos = it->indexOf( QLatin1Char('=') );
        if (pos == -1) {
            std::cout << tr("You must specify a setting name and value as \"name=value\" when using the --setting option").toStdString() << std::endl;
            error = 1;

            return;
        }
        QString name = it->left(pos);
        QString value = it->mid(pos+1);
        it = args.erase(it);

        // note: if there is any --setting option, settingCommands is non-empty, and
        // AppManager::loadInternal() sets _saveSettings Knob in Settings class to false,
        // so that settings are not saved.

        settingCommands.push_back( "NatronEngine.natron.getSettings().getParam(\"" + name.toStdString() + "\").setValue(" + value.toStdString() + ")");
    } // for (;;)

    //Parse python commands
    for (;; ) {
        QStringList::iterator it = hasToken( QString::fromUtf8("cmd"), QString::fromUtf8("c") );
        if ( it == args.end() ) {
            break;
        }

        it = args.erase(it);

        if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
            std::cout << tr("You must specify a command when using the -c option").toStdString() << std::endl;
            error = 1;

            return;
        }

        pythonCommands.push_back( it->toStdString() );
        it = args.erase(it);
    } // for (;;)

    //Parse writers
    for (;; ) {
        QStringList::iterator it = hasToken( QString::fromUtf8("writer"), QString::fromUtf8("w") );
        if ( it == args.end() ) {
            break;
        }

        if (!isBackground || isInterpreterMode) {
            std::cout << tr("You cannot use the -w option in interactive or interpreter mode").toStdString() << std::endl;
            error = 1;

            return;
        }

        it = args.erase(it);

        if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
            std::cout << tr("You must specify the name of a Write node when using the -w option").toStdString() << std::endl;
            error = 1;

            return;
        }


        //Check that the name is conform to a Python acceptable script name
        std::string pythonConform = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly( it->toStdString() );
        if (it->toStdString() != pythonConform) {
            std::cout << tr("The name of the Write node specified is not valid: it cannot contain non alpha-numerical "
                            "characters and must not start with a digit.").toStdString() << std::endl;
            error = 1;

            return;
        }

        CLArgs::WriterArg w;
        w.name = *it;
        it = args.erase(it);

        //Check for an optional filename
        // THIS IS DANGEROUS AND NOT STANDARD: options can not have optional arguments,
        // else it becomes impossible to distinguish positional arguments from options.
        // We're keeping this for backward compatibility, but this is very hacky!
        // A clean solution would be to separate the scriptName and the fileName with a comma.
        if ( it != args.end() && !it->startsWith( QChar::fromLatin1('-') ) ) {
            // Check that it's neither a python script, a natron project, nor a frame range.
            QRegExp re( QString::fromUtf8("[0-9\\-,]*") ); // Matches frame ranges.
            if (!it->endsWith(QString::fromUtf8(".py"), Qt::CaseInsensitive) &&
                !it->endsWith(QString::fromUtf8(".ntp"), Qt::CaseInsensitive) &&
                !re.exactMatch(*it)) {
                w.filename = *it;
#ifdef __NATRON_UNIX__
                w.filename = AppManager::qt_tildeExpansion(w.filename);
#endif
                it = args.erase(it);
            }
        }

        writers.push_back(w);
    } // for (;;)


    //Parse readers
    for (;; ) {
        QStringList::iterator it = hasToken( QString::fromUtf8("reader"), QString::fromUtf8("i") );
        if ( it == args.end() ) {
            break;
        }

        if (!isBackground || isInterpreterMode) {
            std::cout << tr("You cannot use the -i option in interactive or interpreter mode").toStdString() << std::endl;
            error = 1;

            return;
        }

        it = args.erase(it);

        if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
            std::cout << tr("You must specify the name of a Read node when using the -i option").toStdString() << std::endl;
            error = 1;

            return;
        }


        //Check that the name is conform to a Python acceptable script name
        std::string pythonConform = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly( it->toStdString() );
        if (it->toStdString() != pythonConform) {
            std::cout << tr("The name of the Read node specified is not valid: it cannot contain non alpha-numerical "
                            "characters and must not start with a digit.").toStdString() << std::endl;
            error = 1;

            return;
        }

        CLArgs::ReaderArg r;
        r.name = *it;
        it = args.erase(it);

        if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
            std::cout << tr("You must specify the filename for the following Read node: ").toStdString()  << r.name.toStdString() << std::endl;
            error = 1;

            return;
        }


        r.filename = *it;
        it = args.erase(it);
#ifdef __NATRON_UNIX__
        r.filename = AppManager::qt_tildeExpansion(r.filename);
#endif


        readers.push_back(r);
    } // for (;;)

    bool atLeastOneOutput = false;
    ///Parse outputs
    for (;; ) {
        QString indexStr;
        QStringList::iterator it  = hasOutputToken(indexStr);
        if (error > 0) {
            return;
        }
        if ( it == args.end() ) {
            break;
        }

        if (!isBackground) {
            std::cout << tr("You cannot use the -o option in interactive or interpreter mode").toStdString() << std::endl;
            error = 1;

            return;
        }

        CLArgs::WriterArg w;
        w.name = QString( QString::fromUtf8("Output%1") ).arg(indexStr);
        w.mustCreate = true;
        atLeastOneOutput = true;

        //Check for a mandatory file name
        it = args.erase(it);

        if ( it == args.end() || it->startsWith( QChar::fromLatin1('-') ) ) {
            std::cout << tr("Filename is not optional with the -o option").toStdString() << std::endl;
            error = 1;

            return;
        }

        w.filename = *it;
        it = args.erase(it);

        writers.push_back(w);
    }

    //
    // Positional arguments
    //

    // Remove the (optional) end-of-options marker
    {
        QStringList::iterator it = args.begin();
        const QString endOfOptions( QString::fromUtf8("--") );
        if ( it != args.end() && *it == endOfOptions) {
            it = args.erase(it);
        }
    }

    {
        QStringList::iterator it = findFileNameWithExtension( QString::fromUtf8(NATRON_PROJECT_FILE_EXT) );
        if ( it == args.end() ) {
            it = findFileNameWithExtension( QString::fromUtf8("py") );
            if (it != args.end()) {
                isPythonScript = true;
            } else if (!isInterpreterMode && isBackground) {
                std::cout << tr("You must specify the filename of a script or %1 project. (.%2)").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ).toStdString() << std::endl;
                error = 1;

                return;
            }
        }
        if ( it != args.end() ) {
            filename = *it;
            it = args.erase(it);

#ifdef __NATRON_UNIX__
            filename = AppManager::qt_tildeExpansion(filename);
#endif
            QFileInfo fi(filename);
            if ( fi.exists() ) {
                filename = fi.canonicalFilePath();
            }
        }
    }

    //Parse frame range
    for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
        if ( tryParseMultipleFrameRanges(*it, frameRanges) ) {
            it = args.erase(it);
            rangeSet = true;
            break;
        }
    }

#ifdef DEBUG
    qDebug() << "* Command-line parsing results:";
    qDebug() << "clearCacheOnLaunch:" << clearCacheOnLaunch;
    qDebug() << "clearOpenFXCacheOnLaunch:" << clearOpenFXCacheOnLaunch;
    qDebug() << "useDefaultSettings:" << useDefaultSettings;
    qDebug() << "isBackground:" << isBackground;
    qDebug() << "isInterpreterMode:" << isInterpreterMode;
    qDebug() << "enableRenderStats:" << enableRenderStats;
#ifdef NATRON_USE_BREAKPAD
    qDebug() << "breakpadProcessPID:" << breakpadProcessPID;
    qDebug() << "breakpadProcessFilePath:" << breakpadProcessFilePath;
    qDebug() << "breakpadPipeClientID:" << breakpadPipeClientID;
    qDebug() << "breakpadPipeFilePath:" << breakpadPipeFilePath;
    qDebug() << "breakpadComPipeFilePath:" << breakpadComPipeFilePath;
#endif
    qDebug() << "exportDocsPath:" << exportDocsPath;
    qDebug() << "ipcPipe:" << ipcPipe;
    qDebug() << "defaultOnProjectLoadedScript:" << defaultOnProjectLoadedScript;
    qDebug() << "settingCommands:";
    for (auto&& it: settingCommands) {
        qDebug() << QString::fromUtf8(it.c_str());
    }
    qDebug() << "pythonCommands:";
    for (auto&& it: pythonCommands) {
        qDebug() << QString::fromUtf8(it.c_str());
    }
    qDebug() << "writers:";
    for (auto&& it: writers) {
        qDebug() << it.name << it.filename << it.mustCreate;
    }
    qDebug() << "readers:";
    for (auto&& it: readers) {
        qDebug() << it.name << it.filename;
    }
    qDebug() << "atLeastOneOutput:" << atLeastOneOutput;
    qDebug() << "filename:" << filename;
    qDebug() << "rangeSet:" << rangeSet;
    qDebug() << "frameRanges:";
    for (auto&& it: frameRanges) {
        qDebug() << it.first << it.second.first << it.second.second;
    }
    if ( !args.empty() ) {
        qDebug() << "Remaining args:";
        for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
            qDebug() << *it;
        }
    }
    qDebug() << "* End of command-line parsing results.";
#endif // DEBUG

    if (atLeastOneOutput && !rangeSet) {
        std::cout << tr("A frame range must be set when using the -o option").toStdString() << std::endl;
        error = 1;

        return;
    }

    if ( !args.empty() ) {
        std::cout << tr("Some command-line arguments could not be parsed (use option -h for help):").toStdString() << std::endl;
        for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
            std::cout << it->toStdString() << std::endl;
        }
        error = 1;
    }
} // CLArgsPrivate::parse

void
CLArgs::ensureCommandLineArgsUtf8(int argc, char **argv, std::vector<std::string>* utf8Args)
{
    assert(utf8Args);
#ifndef __NATRON_WIN32__
    // On Unix, command line args are Utf8
    assert(!argc || argv);
    for (int i = 0; i < argc; ++i) {
        std::string str(argv[i]);
        assert(StrUtils::is_utf8(str.c_str()));
        utf8Args->push_back(str);
    }
#else
    // On Windows, it must be converted: http://stackoverflow.com/questions/5408730/what-is-the-encoding-of-argv
    (void)argc;
    (void)argv;

    int nArgsOut;
    wchar_t** argList = CommandLineToArgvW(GetCommandLineW(), &nArgsOut);
    for (int i = 0; i < nArgsOut; ++i) {
        std::wstring wide(argList[i]);
        std::string utf8Str = StrUtils::utf16_to_utf8(wide);
        assert(StrUtils::is_utf8(utf8Str.c_str()));
        utf8Args->push_back(utf8Str);
        if (argv) {
            std::cout << "Non UTF-8 arg: " <<  argv[i] << std::endl;
        }
        std::cout << "UTF-8 arg: " <<  utf8Args->back() << std::endl;
    }
    // Free memory allocated for CommandLineToArgvW arguments.
    LocalFree(argList);


#endif
}

NATRON_NAMESPACE_EXIT
