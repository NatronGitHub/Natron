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

#ifndef Engine_CLArgs_h
#define Engine_CLArgs_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <string>
#include <vector>
#include <utility>

#include "Global/GlobalDefines.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct CLArgsPrivate;
class CLArgs //: boost::noncopyable // GCC 4.2 requires the copy constructor
{
    Q_DECLARE_TR_FUNCTIONS(CLArgs)

public:

    struct WriterArg
    {
        QString name;
        QString filename;
        bool mustCreate;

        WriterArg()
            : name(), filename(), mustCreate(false)
        {
        }
    };

    struct ReaderArg
    {
        QString name;
        QString filename;
    };

    CLArgs();

    CLArgs(int& argc,
           char* argv[],
           bool forceBackground);

    CLArgs(int& argc,
           wchar_t* argv[],
           bool forceBackground);

    CLArgs(const QStringList& arguments,
           bool forceBackground);

    CLArgs(const CLArgs& other); // GCC 4.2 requires the copy constructor

    ~CLArgs();

    /**
     * @brief Ensures that the command line arguments passed to main are Utf8 encoded. On Windows
     * the command line arguments must be processed to be safe. 
     **/
    static void ensureCommandLineArgsUtf8(int argc, char **argv, std::vector<std::string>* utf8Args);

    void operator=(const CLArgs& other);

    bool isEmpty() const;

    static void printBackGroundWelcomeMessage();
    static void printUsage(const std::string& programName);

    int getError() const;

    const std::list<CLArgs::WriterArg>& getWriterArgs() const;
    const std::list<CLArgs::ReaderArg>& getReaderArgs() const;
    const std::list<std::string>& getPythonCommands() const;
    const std::list<std::string>& getSettingCommands() const;

    bool hasFrameRange() const;

    const std::list<std::pair<int, std::pair<int, int> > >& getFrameRanges() const;

    bool isLoadedUsingDefaultSettings() const;

    bool isBackgroundMode() const;

    bool isInterpreterMode() const;

    bool isCacheClearRequestedOnLaunch() const;
    
    /*
     * @brief Has a Natron project or Python script been passed to the command line ?
     */
    const QString& getScriptFilename() const;

    /*
     * @brief Has a regular image/video file been passed to the command line ?
     * Warning: This may only be called once that all the plug-ins have been loaded in Natron
     * otherwise it will always return an empty string.
     */
    const QString& getImageFilename() const;
    const QString& getDefaultOnProjectLoadedScript() const;
    const QString& getIPCPipeName() const;

    bool isPythonScript() const;

    bool areRenderStatsEnabled() const;

    const QString& getBreakpadProcessExecutableFilePath() const;

    qint64 getBreakpadProcessPID() const;

    int getBreakpadClientFD() const;

    const QString& getBreakpadPipeFilePath() const;
    const QString& getBreakpadComPipeFilePath() const;
    const QString& getExportDocsPath() const;

private:

    boost::scoped_ptr<CLArgsPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_CLArgs_h

