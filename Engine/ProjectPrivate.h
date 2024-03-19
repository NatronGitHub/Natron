/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef PROJECTPRIVATE_H
#define PROJECTPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <list>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QFuture>
#include <QFutureWatcher>
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobFactory.h"
#include "Engine/TLSHolder.h"
#include "Engine/EngineFwd.h"
#include "Engine/Project.h"
#include "Engine/GenericSchedulerThreadWatcher.h"


NATRON_NAMESPACE_ENTER

struct ProjectPrivate
{
    Q_DECLARE_TR_FUNCTIONS(Project)

public:
    Project* _publicInterface;
    mutable QMutex projectLock; //< protects the whole project
    QString lastAutoSaveFilePath; //< absolute file path of the last auto-save file
    bool hasProjectBeenSavedByUser; //< has this project ever been saved by the user?
    QDateTime ageSinceLastSave; //< the last time the user saved
    QDateTime lastAutoSave; //< the last time since autosave
    QDateTime projectCreationTime; //< the project creation time
    std::list<Format> builtinFormats;
    std::list<Format> additionalFormats; //< added by the user
    mutable QRecursiveMutex formatMutex;

    ///Project parameters (settings)
    KnobPathPtr envVars;
    KnobStringPtr projectName; //< name of the project, e.g: "Untitled.ntp"
    KnobStringPtr projectPath;  //< path of the project, e.g: /Users/Lala/Projects/
    KnobChoicePtr formatKnob; //< built from builtinFormats & additionalFormats
    KnobButtonPtr addFormatKnob;
    KnobPathPtr viewsList;
    KnobLayersPtr defaultLayersList;
    KnobButtonPtr setupForStereoButton;
    KnobBoolPtr previewMode; //< auto or manual
    KnobChoicePtr colorSpace8u;
    KnobChoicePtr colorSpace16u;
    KnobChoicePtr colorSpace32f;
    KnobDoublePtr frameRate;
    KnobChoicePtr gpuSupport;
    KnobIntPtr frameRange;
    KnobBoolPtr lockFrameRange;
    KnobStringPtr natronVersion;
    KnobStringPtr originalAuthorName, lastAuthorName;
    KnobStringPtr projectCreationDate;
    KnobStringPtr saveDate;
    KnobStringPtr onProjectLoadCB;
    KnobStringPtr onProjectSaveCB;
    KnobStringPtr onProjectCloseCB;
    KnobStringPtr onNodeCreated;
    KnobStringPtr onNodeDeleted;
    TimeLinePtr timeline; // global timeline
    bool autoSetProjectFormat;
    mutable QMutex isLoadingProjectMutex;
    bool isLoadingProject; //< true when the project is loading
    bool isLoadingProjectInternal; //< true when loading the internal project (not gui)
    mutable QMutex isSavingProjectMutex;
    bool isSavingProject; //< true when the project is saving
    std::shared_ptr<QTimer> autoSaveTimer;
    std::list<std::shared_ptr<QFutureWatcher<void> > > autoSaveFutures;
    mutable QMutex projectClosingMutex;
    bool projectClosing;
    std::shared_ptr<TLSHolder<Project::ProjectTLSData> > tlsData;

    // only used on the main-thread
    struct RenderWatcher
    {
        NodeRenderWatcherPtr watcher;
        AfterQuitProcessingI* receiver;
    };

    std::list<RenderWatcher> renderWatchers;

    ProjectPrivate(Project* project);

    bool restoreFromSerialization(const ProjectSerialization & obj, const QString& name, const QString& path, bool* mustSave);

    bool findFormat(int index, Format* format) const;
    bool findFormat(const std::string& formatSpec, Format* format) const;
    /**
     * @brief Auto fills the project directory parameter given the project file path
     **/
    void autoSetProjectDirectory(const QString& path);

    std::string runOnProjectSaveCallback(const std::string& filename, bool autoSave);

    void runOnProjectCloseCallback();

    void runOnProjectLoadCallback();

    void setProjectFilename(const std::string& filename);
    std::string getProjectFilename() const;

    void setProjectPath(const std::string& path);
    std::string getProjectPath() const;
    static QString generateStringFromFormat(const Format & f)
    {
        QString formatStr;

        if (!f.getName().empty()) {
            formatStr.append( QString::fromUtf8( f.getName().c_str() ) );
            formatStr.append( QLatin1Char(' ') );
        }
        formatStr.append( QString::number( f.width() ) );
        formatStr.append( QLatin1Char('x') );
        formatStr.append( QString::number( f.height() ) );
        double par = f.getPixelAspectRatio();
        if (par != 1.) {
            formatStr.append( QLatin1Char(' ') );
            formatStr.append( QString::number( f.getPixelAspectRatio(), 'f', 2 ) );
        }

        return formatStr;
    }

    static bool generateFormatFromString(const QString& spec,
                                         Format* f)
    {
        QString str = spec.trimmed();

        // Some old specs were like this: ' 960 x 540 1" meaning we have to remove spaces before 'x' and after
        int foundX = str.lastIndexOf( QLatin1Char('x') );

        if (foundX == -1) {
            return false;
        }

        QString widthStr, heightStr, formatName, parStr;
        int i = foundX - 1;

        // Iterate backward to get width string, then forward
        // ignore initial spaces
        while ( i >= 0 && str[i] == QLatin1Char(' ') ) {
            --i;
        }

        while ( i >= 0 && str[i] != QLatin1Char(' ') ) {
            widthStr.push_front(str[i]);
            --i;
        }

        while ( i >= 0 && str[i] == QLatin1Char(' ') ) {
            --i;
        }

        while ( i >= 0 && str[i] != QLatin1Char(' ') ) {
            formatName.push_front(str[i]);
            --i;
        }

        i = foundX + 1;
        while ( i < str.size() && str[i] == QLatin1Char(' ') ) {
            ++i;
        }

        while ( i < str.size() && str[i] != QLatin1Char(' ') ) {
            heightStr.push_back(str[i]);
            ++i;
        }

        while ( i < str.size() && str[i] == QLatin1Char(' ') ) {
            ++i;
        }

        while ( i < str.size() && str[i] != QLatin1Char(' ') ) {
            parStr.push_back(str[i]);
            ++i;
        }


        f->setName( formatName.toStdString() );
        f->x1 = 0;
        f->y1 = 0;
        f->x2 = widthStr.toInt();
        f->y2 = heightStr.toInt();

        if ( !parStr.isEmpty() ) {
            f->setPixelAspectRatio( parStr.toDouble() );
        }

        return true;
    } // generateFormatFromString
};

NATRON_NAMESPACE_EXIT

#endif // PROJECTPRIVATE_H
