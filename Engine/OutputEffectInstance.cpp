/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "OutputEffectInstance.h"

#include <map>
#include <sstream>
#include <algorithm> // min, max
#include <fstream>
#include <cassert>
#include <stdexcept>

#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QRegExp>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtConcurrentRun> // QtCore on Qt4, QtConcurrent on Qt5

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <SequenceParsing.h> // for removePath

#include "Global/QtCompat.h"
#include "Global/FStreamsSupport.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Log.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS


NATRON_NAMESPACE_ENTER


OutputEffectInstance::OutputEffectInstance(NodePtr node)
    : EffectInstance(node)
    , _outputEffectDataLock()
    , _renderSequenceRequests()
    , _engine()
{
}

OutputEffectInstance::OutputEffectInstance(const OutputEffectInstance& other)
: EffectInstance(other)
, _outputEffectDataLock()
, _renderSequenceRequests()
, _engine(other._engine)
{
}

OutputEffectInstance::~OutputEffectInstance()
{
}

void
OutputEffectInstance::renderCurrentFrame(bool canAbort)
{
    _engine->renderCurrentFrame(getApp()->isRenderStatsActionChecked(), canAbort);
}

void
OutputEffectInstance::renderCurrentFrameWithRenderStats(bool canAbort)
{
    _engine->renderCurrentFrame(true, canAbort);
}

bool
OutputEffectInstance::ifInfiniteclipRectToProjectDefault(RectD* rod) const
{
    if ( !getApp()->getProject() ) {
        return false;
    }
    /*If the rod is infinite clip it to the project's default*/
    Format projectDefault;
    getApp()->getProject()->getProjectDefaultFormat(&projectDefault);
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    // an int can not be equal to (or compared to) std::numeric_limits<double>::infinity()
    bool isRodProjectFormat = false;
    if (rod->left() <= kOfxFlagInfiniteMin) {
        rod->set_left( projectDefault.left() );
        isRodProjectFormat = true;
    }
    if (rod->bottom() <= kOfxFlagInfiniteMin) {
        rod->set_bottom( projectDefault.bottom() );
        isRodProjectFormat = true;
    }
    if (rod->right() >= kOfxFlagInfiniteMax) {
        rod->set_right( projectDefault.right() );
        isRodProjectFormat = true;
    }
    if (rod->top() >= kOfxFlagInfiniteMax) {
        rod->set_top( projectDefault.top() );
        isRodProjectFormat = true;
    }

    return isRodProjectFormat;
}

void
OutputEffectInstance::renderFullSequence(bool isBlocking,
                                         bool enableRenderStats,
                                         BlockingBackgroundRender* renderController,
                                         int first,
                                         int last,
                                         int frameStep)
{
    int viewsCount = getApp()->getProject()->getProjectViewsCount();
    const ViewIdx mainView(0);
    std::vector<ViewIdx> viewsToRender(viewsCount);

    for (int i = 0; i < viewsCount; ++i) {
        viewsToRender[i] = ViewIdx(i);
    }

    ///The effect is sequential (e.g: WriteFFMPEG), and thus cannot render multiple views, we have to choose one
    ///We pick the user defined main view in the project settings
    SequentialPreferenceEnum sequentiallity = getSequentialPreference();
    bool canOnlyHandleOneView = sequentiallity == eSequentialPreferenceOnlySequential || sequentiallity == eSequentialPreferencePreferSequential;

    if (canOnlyHandleOneView) {
        viewsToRender.clear();
        viewsToRender.push_back(mainView);
    }

    KnobIPtr outputFileNameKnob = getKnobByName(kOfxImageEffectFileParamName);
    KnobOutputFile* outputFileName = outputFileNameKnob ? dynamic_cast<KnobOutputFile*>( outputFileNameKnob.get() ) : 0;
    std::string pattern = outputFileName ? outputFileName->getValue() : std::string();

    if ( isViewAware() ) {
        //If the Writer is view aware, check if it wants to render all views at once or not
        std::size_t foundViewPattern = pattern.find_first_of("%v");
        if (foundViewPattern == std::string::npos) {
            foundViewPattern = pattern.find_first_of("%V");
        }
        if (foundViewPattern == std::string::npos) {
            ///No view pattern
            ///all views will be overwritten to the same file
            ///If this is WriteOIIO, check the parameter "viewsSelector" to determine if the user wants to encode all
            ///views to a single file or not
            KnobIPtr viewsKnob = getKnobByName(kWriteOIIOParamViewsSelector);
            bool hasViewChoice = false;
            if ( viewsKnob && !viewsKnob->getIsSecret() ) {
                KnobChoice* viewsChoice = dynamic_cast<KnobChoice*>( viewsKnob.get() );
                if (viewsChoice) {
                    hasViewChoice = true;
                    int viewChoice_i = viewsChoice->getValue();
                    if (viewChoice_i == 0) { // the "All" choice
                        viewsToRender.clear();
                        // note: if the plugin renders all views to a single file, then rendering view 0 will do the job.
                        viewsToRender.push_back( ViewIdx(0) );
                    } else {
                        //The user has specified a view
                        viewsToRender.clear();
                        assert(viewChoice_i >= 1);
                        viewsToRender.push_back( ViewIdx(viewChoice_i - 1) );
                    }
                }
            }
            if (!hasViewChoice) {
                if (viewsToRender.size() > 1) {
                    std::string mainViewName;
                    const std::vector<std::string>& viewNames = getApp()->getProject()->getProjectViewNames();
                    if ( mainView < (int)viewNames.size() ) {
                        mainViewName = viewNames[mainView];
                    }
                    QString message = tr("%1 does not support multi-view, only the view %2 will be rendered.")
                    .arg( QString::fromUtf8( getNode()->getLabel_mt_safe().c_str() ) )
                    .arg( QString::fromUtf8( mainViewName.c_str() ) );
                    if (!renderController) {
                        message.append( QChar::fromLatin1('\n') );
                        message.append( QString::fromUtf8("You can use the %v or %V indicator in the filename to render to separate files.\n"
                                                          "Would you like to continue?") );
                        StandardButtonEnum rep = Dialogs::questionDialog(tr("Multi-view support").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                        if (rep != eStandardButtonOk) {
                            return;
                        }
                    } else {
                        Dialogs::warningDialog( tr("Multi-view support").toStdString(), message.toStdString() );
                    }
                }
                //Render the main-view only...
                viewsToRender.clear();
                viewsToRender.push_back(mainView);
            }
        } else {
            ///The user wants to write each view into a separate file
            ///This will disregard the content of kWriteOIIOParamViewsSelector and the Writer
            ///should write one view per-file.
        }


    } else { // !isViewAware
        if (viewsToRender.size() > 1) {
            std::string mainViewName;
            const std::vector<std::string>& viewNames = getApp()->getProject()->getProjectViewNames();
            if ( mainView < (int)viewNames.size() ) {
                mainViewName = viewNames[mainView];
            }
            QString message = tr("%1 does not support multi-view, only the view %2 will be rendered.")
                              .arg( QString::fromUtf8( getNode()->getLabel_mt_safe().c_str() ) )
                              .arg( QString::fromUtf8( mainViewName.c_str() ) );
            if (!renderController) {
                message.append( QChar::fromLatin1('\n') );
                message.append( QString::fromUtf8("You can use the %v or %V indicator in the filename to render to separate files.\n"
                                                  "Would you like to continue?") );
                StandardButtonEnum rep = Dialogs::questionDialog(tr("Multi-view support").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                if (rep != eStandardButtonOk) {
                    // Notify progress that we were aborted
                    getRenderEngine()->s_renderFinished(1);

                    return;
                }
            } else {
                Dialogs::warningDialog( tr("Multi-view support").toStdString(), message.toStdString() );
            }
        }
    }

    if (first != last && !isVideoWriter() && isWriter()) {
        // We render a sequence, check that the user wants to render multiple images
        // Look first for # character
        std::size_t foundHash = pattern.find_first_of("#");
        if (foundHash == std::string::npos) {
            // Look for printf style numbering
            QRegExp exp(QString::fromUtf8("%[0-9]*d"));
            QString qp(QString::fromUtf8(pattern.c_str()));
            if (!qp.contains(exp)) {
                QString message = tr("You are trying to render the frame range [%1 - %2] but you did not specify any hash ('#') character(s) or printf-like format ('%d') for the padding. This will result in the same image being overwritten multiple times.").arg(first).arg(last);
                if (!renderController) {
                    message += tr("Would you like to continue?");
                    StandardButtonEnum rep = Dialogs::questionDialog(tr("Image Sequence").toStdString(), message.toStdString(), false, StandardButtons(eStandardButtonOk | eStandardButtonCancel), eStandardButtonOk);
                    if (rep != eStandardButtonOk) {
                        // Notify progress that we were aborted
                        getRenderEngine()->s_renderFinished(1);

                        return;
                    }
                } else {
                    Dialogs::warningDialog( tr("Image Sequence").toStdString(), message.toStdString() );
                }
                
            }
        }
    }
    RenderSequenceArgs args;
    {
        QMutexLocker k(&_outputEffectDataLock);
        args.firstFrame = first;
        args.lastFrame = last;
        args.frameStep = frameStep;
        args.renderController = renderController;
        args.useStats = enableRenderStats;
        args.blocking = isBlocking;
        args.viewsToRender = viewsToRender;
        _renderSequenceRequests.push_back(args);
        if (_renderSequenceRequests.size() > 1) {
            //The node is already rendering a sequence, queue it and dequeue it in notifyRenderFinished()
            return;
        }
    }
    launchRenderSequence(args);
} // OutputEffectInstance::renderFullSequence

void
OutputEffectInstance::launchRenderSequence(const RenderSequenceArgs& args)
{
    createWriterPath();

    ///If you want writers to render backward (from last to first), just change the flag in parameter here
    _engine->renderFrameRange(args.blocking,
                              args.useStats,
                              args.firstFrame,
                              args.lastFrame,
                              args.frameStep,
                              args.viewsToRender,
                              eRenderDirectionForward);
}

void
OutputEffectInstance::createWriterPath()
{
    ///Make sure that the file path exists
    KnobIPtr fileParam = getKnobByName(kOfxImageEffectFileParamName);

    if (fileParam) {
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>( fileParam.get() );
        if (isString) {
            std::string pattern = isString->getValue();
            std::string path = SequenceParsing::removePath(pattern);
            std::map<std::string, std::string> env;
            getApp()->getProject()->getEnvironmentVariables(env);
            Project::expandVariable(env, path);
            if ( !path.empty() ) {
                QDir().mkpath( QString::fromUtf8( path.c_str() ) );
            }
        }
    }
}

void
OutputEffectInstance::notifyRenderFinished()
{
    RenderSequenceArgs newArgs;

    {
        QMutexLocker k(&_outputEffectDataLock);
        if ( !_renderSequenceRequests.empty() ) {
            const RenderSequenceArgs& args = _renderSequenceRequests.front();
            if (args.renderController) {
                args.renderController->notifyFinished();
            }
            _renderSequenceRequests.pop_front();
        }
        if ( _renderSequenceRequests.empty() ) {
            return;
        }
        newArgs = _renderSequenceRequests.front();
    }

    launchRenderSequence(newArgs);
}

bool
OutputEffectInstance::isSequentialRenderBeingAborted() const
{
    return _engine ? _engine->isSequentialRenderBeingAborted() : false;
}

bool
OutputEffectInstance::isDoingSequentialRender() const
{
    return _engine ? _engine->isDoingSequentialRender() : false;
}

void
OutputEffectInstance::initializeData()
{
    _engine.reset( createRenderEngine() );
}

RenderEngine*
OutputEffectInstance::createRenderEngine()
{
    OutputEffectInstancePtr thisShared = boost::dynamic_pointer_cast<OutputEffectInstance>( shared_from_this() );

    assert(thisShared);

    return new RenderEngine(thisShared);
}

void
OutputEffectInstance::reportStats(int time,
                                  ViewIdx view,
                                  double wallTime,
                                  const std::map<NodePtr, NodeRenderStats > & stats)
{
    std::string filename;
    KnobIPtr fileKnob = getKnobByName(kOfxImageEffectFileParamName);

    if (fileKnob) {
        KnobOutputFile* strKnob = dynamic_cast<KnobOutputFile*>( fileKnob.get() );
        if  (strKnob) {
            QString qfileName = QString::fromUtf8( SequenceParsing::generateFileNameFromPattern(strKnob->getValue( 0, ViewIdx(view) ), getApp()->getProject()->getProjectViewNames(), time, view).c_str() );
            QtCompat::removeFileExtension(qfileName);
            qfileName.append( QString::fromUtf8("-stats.txt") );
            filename = qfileName.toStdString();
        }
    }

    //If there's no filename knob, do not write anything
    if ( filename.empty() ) {
        std::cout << tr("Cannot write render statistics file: "
                        "%1 does not seem to have a parameter named \"filename\" "
                        "to determine the location where to write the stats file.")
            .arg( QString::fromUtf8( getScriptName_mt_safe().c_str() ) ).toStdString();

        return;
    }


    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, filename);
    if (!ofile) {
        std::cout << tr("Failure to write render statistics file.").toStdString() << std::endl;

        return;
    }

    ofile << "Time spent to render frame (wall clock time): " << Timer::printAsTime(wallTime, false).toStdString() << std::endl;
    for (std::map<NodePtr, NodeRenderStats >::const_iterator it = stats.begin(); it != stats.end(); ++it) {
        ofile << "------------------------------- " << it->first->getScriptName_mt_safe() << "------------------------------- " << std::endl;
        ofile << "Time spent rendering: " << Timer::printAsTime(it->second.getTotalTimeSpentRendering(), false).toStdString() << std::endl;
        const RectD & rod = it->second.getRoD();
        ofile << "Region of definition: x1 = " << rod.x1  << " y1 = " << rod.y1 << " x2 = " << rod.x2 << " y2 = " << rod.y2 << std::endl;
        ofile << "Is Identity to Effect? ";
        NodePtr identity = it->second.getInputImageIdentity();
        if (identity) {
            ofile << "Yes, to " << identity->getScriptName_mt_safe() << std::endl;
        } else {
            ofile << "No" << std::endl;
        }
        ofile << "Has render-scale support? ";
        if ( it->second.isRenderScaleSupportEnabled() ) {
            ofile << "Yes";
        } else {
            ofile << "No";
        }
        ofile << std::endl;
        ofile << "Has tiles support? ";
        if ( it->second.isTilesSupportEnabled() ) {
            ofile << "Yes";
        } else {
            ofile << "No";
        }
        ofile << std::endl;
        ofile << "Channels processed: ";

        std::bitset<4> processChannels = it->second.getChannelsRendered();
        if (processChannels[0]) {
            ofile << "red ";
        }
        if (processChannels[1]) {
            ofile << "green ";
        }
        if (processChannels[2]) {
            ofile << "blue ";
        }
        if (processChannels[3]) {
            ofile << "alpha";
        }
        ofile << std::endl;

        ofile << "Output alpha premultiplication: ";
        switch ( it->second.getOutputPremult() ) {
        case eImagePremultiplicationOpaque:
            ofile << "opaque";
            break;
        case eImagePremultiplicationPremultiplied:
            ofile << "premultiplied";
            break;
        case eImagePremultiplicationUnPremultiplied:
            ofile << "unpremultiplied";
            break;
        }
        ofile << std::endl;

        ofile << "Mipmap level(s) rendered: ";
        for (std::set<unsigned int>::const_iterator it2 = it->second.getMipMapLevelsRendered().begin(); it2 != it->second.getMipMapLevelsRendered().end(); ++it2) {
            ofile << *it2 << ' ';
        }
        ofile << std::endl;
        int nbCacheMiss, nbCacheHit, nbCacheHitButDownscaled;
        it->second.getCacheAccessInfos(&nbCacheMiss, &nbCacheHit, &nbCacheHitButDownscaled);
        ofile << "Nb cache hit: " << nbCacheMiss << std::endl;
        ofile << "Nb cache miss: " << nbCacheMiss << std::endl;
        ofile << "Nb cache hit requiring mipmap downscaling: " << nbCacheHitButDownscaled << std::endl;

        const std::set<std::string> & planes = it->second.getPlanesRendered();
        ofile << "Plane(s) rendered: ";
        for (std::set<std::string>::const_iterator it2 = planes.begin(); it2 != planes.end(); ++it2) {
            ofile << *it2 << ' ';
        }
        ofile << std::endl;

        std::list<std::pair<RectI, NodePtr> > identityRectangles = it->second.getIdentityRectangles();
        const std::list<RectI> & renderedRectangles = it->second.getRenderedRectangles();

        ofile << "Identity rectangles: " << identityRectangles.size() << std::endl;
        for (std::list<std::pair<RectI, NodePtr> > ::iterator it2 = identityRectangles.begin(); it2 != identityRectangles.end(); ++it2) {
            ofile << "Origin: " << it2->second->getScriptName_mt_safe() << ", rect: x1 = " << it2->first.x1
                  << " y1 = " << it2->first.y1 << " x2 = " << it2->first.x2 << " y2 = " << it2->first.y2 << std::endl;
        }

        ofile << "Rectangles rendered: " << renderedRectangles.size() << std::endl;
        for (std::list<RectI>::const_iterator it2 = renderedRectangles.begin(); it2 != renderedRectangles.end(); ++it2) {
            ofile << "x1 = " << it2->x1 << " y1 = " << it2->y1 << " x2 = " << it2->x2 << " y2 = " << it2->y2 << std::endl;
        }
    }
} // OutputEffectInstance::reportStats

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_OutputEffectInstance.cpp"
