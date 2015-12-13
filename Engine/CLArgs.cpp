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

#include "CLArgs.h"

#include <cstddef>
#include <iostream>
#include <stdexcept>

#include <QDebug>
#include <QFile>

#include "Global/GitVersion.h"
#include "Global/QtCompat.h"
#include "Engine/AppManager.h"


struct CLArgsPrivate
{
    QStringList args;
    
    QString filename;
    
    bool isPythonScript;
    
    QString defaultOnProjectLoadedScript;
    
    std::list<CLArgs::WriterArg> writers;
    
    bool isBackground;
    
    QString ipcPipe;
    
    int error;
    
    bool isInterpreterMode;
    
    std::list<std::pair<int, std::pair<int,int> > > frameRanges;
    bool rangeSet;
    
    bool enableRenderStats;
    
    bool isEmpty;
    
    mutable QString imageFilename;
    
    CLArgsPrivate()
    : args()
    , filename()
    , isPythonScript(false)
    , defaultOnProjectLoadedScript()
    , writers()
    , isBackground(false)
    , ipcPipe()
    , error(0)
    , isInterpreterMode(false)
    , frameRanges()
    , rangeSet(false)
    , enableRenderStats(false)
    , isEmpty(true)
    , imageFilename()
    {
        
    }
 
    
    void parse();
    
    QStringList::iterator hasToken(const QString& longName,const QString& shortName);
    
    QStringList::iterator hasOutputToken(QString& indexStr);
    
    QStringList::iterator findFileNameWithExtension(const QString& extension);
    
};

CLArgs::CLArgs()
: _imp(new CLArgsPrivate())
{
    
}

CLArgs::CLArgs(int& argc,char* argv[],bool forceBackground)
: _imp(new CLArgsPrivate())
{    
    _imp->isEmpty = false;
    if (forceBackground) {
        _imp->isBackground = true;
    }
    for (int i = 0; i < argc; ++i) {
        QString str = argv[i];
        if (str.size() >= 2 && str[0] == '"' && str[str.size() - 1] == '"') {
            str.remove(0, 1);
            str.remove(str.size() - 1, 1);
        }
#ifdef DEBUG
        std::cout << "argv[" << i << "] = " << str.toStdString() << std::endl;
#endif
        _imp->args.push_back(str);
    }
    
    _imp->parse();
}

CLArgs::CLArgs(const QStringList &arguments, bool forceBackground)
    : _imp(new CLArgsPrivate())
{
    _imp->isEmpty = false;
    if (forceBackground) {
        _imp->isBackground = true;
    }
    for (int i = 0; i < arguments.size(); ++i) {
        QString str = arguments[i];
        if (str.size() >= 2 && str[0] == '"' && str[str.size() - 1] == '"') {
            str.remove(0, 1);
            str.remove(str.size() - 1, 1);
        }
        _imp->args.push_back(str);
    }
    _imp->parse();
}

// GCC 4.2 requires the copy constructor
CLArgs::CLArgs(const CLArgs& other)
: _imp(new CLArgsPrivate())
{
    *this = other;
}

CLArgs::~CLArgs()
{
    
}

void
CLArgs::operator=(const CLArgs& other)
{
    _imp->args = other._imp->args;
    _imp->filename = other._imp->filename;
    _imp->isPythonScript = other._imp->isPythonScript;
    _imp->defaultOnProjectLoadedScript = other._imp->defaultOnProjectLoadedScript;
    _imp->writers = other._imp->writers;
    _imp->isBackground = other._imp->isBackground;
    _imp->ipcPipe = other._imp->ipcPipe;
    _imp->error = other._imp->error;
    _imp->isInterpreterMode = other._imp->isInterpreterMode;
    _imp->frameRanges = other._imp->frameRanges;
    _imp->rangeSet = other._imp->rangeSet;
    _imp->enableRenderStats = other._imp->enableRenderStats;
    _imp->isEmpty = other._imp->isEmpty;
    _imp->imageFilename = other._imp->imageFilename;
}

bool
CLArgs::isEmpty() const
{
    return _imp->isEmpty;
}

void
CLArgs::printBackGroundWelcomeMessage()
{
    QString msg = QObject::tr("%1 Version %2\n"
                             "Copyright (C) 2015 the %1 developers\n"
                              ">>>Use the --help or -h option to print usage.<<<").arg(NATRON_APPLICATION_NAME).arg(NATRON_VERSION_STRING);
    std::cout << msg.toStdString() << std::endl;
}

void
CLArgs::printUsage(const std::string& programName)
{
    QString msg = QObject::tr(/* Text must hold in 80 columns ************************************************/
                              "%3 usage:\n"
                              "Three distinct execution modes exist in background mode:\n"
                              "- The execution of %1 projects (.%2)\n"
                              "- The execution of Python scripts that contain commands for %1\n"
                              "- An interpreter mode where commands can be given directly to the Python\n"
                              "  interpreter\n"
                              "\n"
                              "General options:\n"
                              "  -h [ --help ] :\n"
                              "    Produce help message.\n"
                              "  -v [ --version ]  :\n"
                              "    Print informations about %1 version.\n"
                              "  -b [ --background ] :\n"
                              "    Enable background rendering mode. No graphical interface is shown.\n"
                              "    When using %1Renderer or the -t option, this argument is implicit\n"
                              "    and does not have to be given.\n"
                              "    If using %1 and this option is not specified, the project is loaded\n"
                              "    as if opened from the file menu.\n"
                              "  -t [ --interpreter ] <python script file path> :\n"
                              "    Enable Python interpreter mode.\n"
                              "    Python commands can be given to the interpreter and executed on the fly.\n"
                              "    An optional Python script filename can be specified to source a script\n"
                              "    before the interpreter is made accessible.\n"
                              "    Note that %1 will not start rendering any Write node of the sourced\n"
                              "    script: it must be started explicitely.\n"
                              "    %1Renderer and %1 do the same thing in this mode, only the\n"
                              "    init.py script is loaded.\n"
                              "\n"
                              /* Text must hold in 80 columns ************************************************/
                              "Options for the execution of %1 projects:\n"
                              "  %3 <project file path> [options]\n"
                              "  -w [ --writer ] <Writer node script name> [<filename>] [<frameRange>] :\n"
                              "    Specify a Write node to render.\n"
                              "    When in background mode, the renderer only renders the node script name\n"
                              "    following this argument. If no such node exists in the project file, the\n"
                              "    process aborts.\n"
                              "    Note that if there is no --writer option, it renders all the writers in\n"
                              "    the project.\n"
                              "    After the writer node script name you can pass an optional output\n"
                              "    filename and pass an optional frame range in the format:\n"
                              "      <firstFrame>-<lastFrame> (e.g: 10-40).\n"
                              "      The frame-range can also contain a frame-step indicating how many steps\n"
                              "      the timeline should do before rendering a frame:\n"
                              "       <firstFrame>-<lastFrame>:<frameStep> (e.g:1-10:2 Would render 1,3,5,7,9)\n"
                              "      You can also specify multiple frame-ranges to render by separating them with commas:\n"
                              "      1-10:1,20-30:2,40-50\n"
                              "      Individual frames can also be specified:\n"
                              "      1329,2450,123,1-10:2\n"
                              "    Note that several -w options can be set to specify multiple Write nodes\n"
                              "    to render.\n"
                              "    Note that if specified, the frame range is the same for all Write nodes\n"
                              "    to render.\n"
                              "  -l [ --onload ] <python script file path> :\n"
                              "    Specify a Python script to be executed after a project is created or\n"
                              "    loaded.\n"
                              "    Note that this is executed in GUI mode or with NatronRenderer, after\n"
                              "    executing the callbacks onProjectLoaded and onProjectCreated.\n"
                              "    The rules on the execution of Python scripts (see below) also apply to\n"
                              "    this script.\n"
                              "  -s [ --render-stats] Enables render statistics that will be produced for\n"
                              "     each frame in form of a file located next to the image produced by\n"
                              "     the Writer node, with the same name and a -stats.txt extension. The\n"
                              "     breakdown contains informations about each nodes, render times etc...\n"
                              "     This option is useful for debugging purposes or to control that a render\n"
                              "     is working correctly.\n"
                              "     **Please note** that it does not work when writing video files."
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
                              "  %3 <Python script path> [options]\n"
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
                              "  -o [ --output ] <filename> <frameRange> :\n"
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
                              "  %3 -t [<Python script path>]\n"
                              "  %1 sources the optional script given as argument, if any, and then reads\n"
                              "  Python commands from the standard input, which are interpreted by Python.\n"
                              "Sample uses:\n"
                              "  %1 -t\n"
                              "  %1Renderer -t\n"
                              "  %1Renderer -t /Users/Me/MyNatronScripts/MyScript.py\n")
    .arg(/*%1=*/NATRON_APPLICATION_NAME).arg(/*%2=*/NATRON_PROJECT_FILE_EXT).arg(/*%3=*/programName.c_str());
    std::cout << msg.toStdString() << std::endl;
}


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

bool
CLArgs::hasFrameRange() const
{
    return _imp->rangeSet;
}

const std::list<std::pair<int,std::pair<int,int> > >&
CLArgs::getFrameRanges() const
{
    return _imp->frameRanges;
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
    if (_imp->imageFilename.isEmpty()) {
        ///Check for image file passed to command line
        for (QStringList::iterator it = _imp->args.begin(); it!=_imp->args.end(); ++it) {
            QString fileCopy = *it;
            QString ext = Natron::removeFileExtension(fileCopy);
            std::string readerId = appPTR->isImageFileSupportedByNatron(ext.toStdString());
            if (!readerId.empty()) {
                _imp->imageFilename = *it;
                _imp->args.erase(it);
                break;
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

QStringList::iterator
CLArgsPrivate::findFileNameWithExtension(const QString& extension)
{
    bool isPython = extension == "py";
    for (QStringList::iterator it = args.begin(); it != args.end() ; ++it) {
        if (isPython) {
            //Check that we do not take the python script specified for the --onload argument as the file to execute
            if (it == args.begin()) {
                continue;
            }
            QStringList::iterator prev = it;
            --prev;
            if (*prev == "--onload" || *prev == "-l") {
                continue;
            }
        }
        if (it->endsWith("." + extension)) {
            return it;
        }
    }
    return args.end();
}

QStringList::iterator
CLArgsPrivate::hasToken(const QString& longName,const QString& shortName)
{
    QString longToken = "--" + longName;
    QString shortToken =  !shortName.isEmpty() ? "-" + shortName : QString();
    for (QStringList::iterator it = args.begin(); it != args.end() ; ++it) {
        if (*it == longToken || (!shortToken.isEmpty() && *it == shortToken)) {
            return it;
        }
    }
    return args.end();
}


QStringList::iterator
CLArgsPrivate::hasOutputToken(QString& indexStr)
{
    QString outputLong("--output");
    QString outputShort("-o");
    
    for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
        int indexOf = it->indexOf(outputLong);
        if (indexOf != -1) {
            indexOf += outputLong.size();
            if (indexOf < it->size()) {
                indexStr = it->mid(indexOf);
                bool ok;
                indexStr.toInt(&ok);
                if (!ok) {
                    error = 1;
                    std::cout << QObject::tr("Wrong formating for the -o option").toStdString() << std::endl;
                    return args.end();
                }
            } else {
                indexStr = "1";
            }
            return it;
        } else {
            indexOf = it->indexOf(outputShort);
            if (indexOf != -1) {
                if (it->size() > 2 && !it->at(2).isDigit()) {
                    //This is probably the --onload option
                    return args.end();
                }
                indexOf += outputShort.size();
                if (indexOf < it->size()) {
                    indexStr = it->mid(indexOf);
                    bool ok;
                    indexStr.toInt(&ok);
                    if (!ok) {
                        error = 1;
                        std::cout << QObject::tr("Wrong formating for the -o option").toStdString() << std::endl;
                        return args.end();
                    }
                } else {
                    indexStr = "1";
                }
                return it;
            }
        }
    }
    return args.end();
}

static bool tryParseFrameRange(const QString& arg,std::pair<int,int>& range, int& frameStep)
{
    bool ok;
    int singleNumber = arg.toInt(&ok);
    if (ok) {
        //this is a single frame
        range.first = range.second = singleNumber;
        frameStep = INT_MIN;
        return true;
    }
    QStringList strRange = arg.split('-');
    if (strRange.size() != 2) {
        return false;
    }
    for (int i = 0; i < strRange.size(); ++i) {
        //whitespace removed from the start and the end.
        strRange[i] = strRange[i].trimmed();
    }
    range.first = strRange[0].toInt(&ok);
    if (!ok) {
        return false;
    }
    
    int foundColon = strRange[1].indexOf(':');
    if (foundColon != -1) {
        ///A frame-step has been specified
        QString lastFrameStr = strRange[1].mid(0,foundColon);
        QString frameStepStr = strRange[1].mid(foundColon + 1);
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
        range.second = strRange[1].toInt(&ok);
        if (!ok) {
            return false;
        }
    }
  
    return true;
    
}

static bool tryParseMultipleFrameRanges(const QString& args,std::list<std::pair<int,std::pair<int,int> > >& frameRanges)
{
    QStringList splits = args.split(',');
    bool added = false;
    for (int i = 0; i < splits.size(); ++i) {
        std::pair<int,int> frameRange;
        int frameStep;
        if (tryParseFrameRange(splits[i], frameRange, frameStep)) {
            added = true;
            frameRanges.push_back(std::make_pair(frameStep, frameRange));
        }
    }
    return added;
}

void
CLArgsPrivate::parse()
{
    {
        QStringList::iterator it = hasToken("version", "v");
        if (it != args.end()) {
            QString msg = QObject::tr("%1 version %2 at commit %3 on branch %4 built on %4").arg(NATRON_APPLICATION_NAME).arg(NATRON_VERSION_STRING).arg(GIT_COMMIT).arg(GIT_BRANCH).arg(__DATE__);
            std::cout << msg.toStdString() << std::endl;
            error = 1;
            return;
        }
    }
    
    {
        QStringList::iterator it = hasToken("help", "h");
        if (it != args.end()) {
            CLArgs::printUsage(args[0].toStdString());
            error = 1;
            return;
        }
    }
    
    {
        QStringList::iterator it = hasToken("background", "b");
        if (it != args.end()) {
            isBackground = true;
            args.erase(it);
        }
    }
    
    {
        QStringList::iterator it = hasToken("interpreter", "t");
        if (it != args.end()) {
            isInterpreterMode = true;
            isBackground = true;
            std::cout << QObject::tr("Note: -t argument given, loading in command-line interpreter mode, only Python commands / scripts are accepted").toStdString()
            << std::endl;
            args.erase(it);
        }
    }
    
    {
        QStringList::iterator it = hasToken("render-stats", "s");
        if (it != args.end()) {
            enableRenderStats = true;
            args.erase(it);
        }
    }
    
    {
        QStringList::iterator it = hasToken("IPCpipe", "");
        if (it != args.end()) {
            ++it;
            if (it != args.end()) {
                ipcPipe = *it;
                args.erase(it);
            } else {
                std::cout << QObject::tr("You must specify the IPC pipe filename").toStdString() << std::endl;
                error = 1;
                return;
            }
        }
    }
    
    {
        QStringList::iterator it = hasToken("onload", "l");
        if (it != args.end()) {
            ++it;
            if (it != args.end()) {
                defaultOnProjectLoadedScript = *it;
#ifdef __NATRON_UNIX__
                defaultOnProjectLoadedScript = AppManager::qt_tildeExpansion(defaultOnProjectLoadedScript);
#endif
                args.erase(it);
                if (!defaultOnProjectLoadedScript.endsWith(".py")) {
                    std::cout << QObject::tr("The optional on project load script must be a Python script (.py).").toStdString() << std::endl;
                    error = 1;
                    return;
                }
                if (!QFile::exists(defaultOnProjectLoadedScript)) {
                    std::cout << QObject::tr("WARNING: --onload %1 ignored because the file does not exist.").arg(defaultOnProjectLoadedScript).toStdString() << std::endl;
                }
            } else {
                std::cout << QObject::tr("--onload or -l specified, you must enter a script filename afterwards.").toStdString() << std::endl;
                error = 1;
                return;
            }
        }
    }
    
    {
        QStringList::iterator it = findFileNameWithExtension(NATRON_PROJECT_FILE_EXT);
        if (it == args.end()) {
            it = findFileNameWithExtension("py");
            if (it == args.end() && !isInterpreterMode && isBackground) {
                std::cout << QObject::tr("You must specify the filename of a script or %1 project. (.%2)").arg(NATRON_APPLICATION_NAME).arg(NATRON_PROJECT_FILE_EXT).toStdString() << std::endl;
                error = 1;
                return;
            }
            isPythonScript = true;
            
        }
        if (it != args.end()) {
            filename = *it;
#if defined(Q_OS_UNIX)
            filename = AppManager::qt_tildeExpansion(filename);
#endif
            args.erase(it);
        }
    }
    
    //Parse frame range
    for (int i = 0; i < args.size(); ++i) {
        if (tryParseMultipleFrameRanges(args[i], frameRanges)) {
            if (rangeSet) {
                std::cout << QObject::tr("Only a single frame range can be specified from the command-line for all Write nodes").toStdString() << std::endl;
                error = 1;
                return;
            }
            rangeSet = true;
        }
    }
    
    //Parse writers
    for (;;) {
        QStringList::iterator it = hasToken("writer", "w");
        if (it == args.end()) {
            break;
        }
        
        if (!isBackground || isInterpreterMode) {
            std::cout << QObject::tr("You cannot use the -w option in interactive or interpreter mode").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        QStringList::iterator next = it;
        if (next != args.end()) {
            ++next;
        }
        if (next == args.end()) {
            std::cout << QObject::tr("You must specify the name of a Write node when using the -w option").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        
        //Check that the name is conform to a Python acceptable script name
        std::string pythonConform = Natron::makeNameScriptFriendly(next->toStdString());
        if (next->toStdString() != pythonConform) {
            std::cout << QObject::tr("The name of the Write node specified is not valid: it cannot contain non alpha-numerical "
                                     "characters and must not start with a digit.").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        CLArgs::WriterArg w;
        w.name = *next;
        
        QStringList::iterator nextNext = next;
        if (nextNext != args.end()) {
            ++nextNext;
        }
        if (nextNext != args.end()) {
            //Check for an optional filename
            if (!nextNext->startsWith("-") && !nextNext->startsWith("--")) {
                w.filename = *nextNext;
#if defined(Q_OS_UNIX)
                w.filename = AppManager::qt_tildeExpansion(w.filename);
#endif
            }
        }
        
        writers.push_back(w);
        if (nextNext != args.end()) {
            ++nextNext;
        }
        args.erase(it,nextNext);
        
    } // for (;;)
    
    bool atLeastOneOutput = false;
    ///Parse outputs
    for (;;) {
        QString indexStr;
        QStringList::iterator it  = hasOutputToken(indexStr);
        if (error > 0) {
            return;
        }
        if (it == args.end()) {
            break;
        }
        
        if (!isBackground) {
            std::cout << QObject::tr("You cannot use the -o option in interactive or interpreter mode").toStdString() << std::endl;
            error = 1;
            return;
        }

        CLArgs::WriterArg w;
        w.name = QString("Output%1").arg(indexStr);
        w.mustCreate = true;
        atLeastOneOutput = true;
        
        //Check for a mandatory file name
        QStringList::iterator next = it;
        if (next != args.end()) {
            ++next;
        }
        if (next == args.end()) {
            std::cout << QObject::tr("Filename is not optional with the -o option").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        //Check for an optional filename
        if (!next->startsWith("-") && !next->startsWith("--")) {
            w.filename = *next;
        }
        
        writers.push_back(w);
        
        QStringList::iterator endToErase = next;
        ++endToErase;
        args.erase(it, endToErase);
    }
    
    if (atLeastOneOutput && !rangeSet) {
        std::cout << QObject::tr("A frame range must be set when using the -o option").toStdString() << std::endl;
        error = 1;
        return;
    }
  
}
