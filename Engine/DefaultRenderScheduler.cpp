/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "DefaultRenderScheduler.h"

#include <iostream>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/RenderEngine.h"
#include "Engine/RenderThreadTask.h"
#include "Engine/WriteNode.h"


NATRON_NAMESPACE_ENTER;

DefaultScheduler::DefaultScheduler(RenderEngine* engine,
                                   const NodePtr& effect)
: OutputSchedulerThread(engine, effect, eProcessFrameBySchedulerThread)
, _currentTimeMutex()
, _currentTime(0)
{
    engine->setPlaybackMode(ePlaybackModeOnce);
}

DefaultScheduler::~DefaultScheduler()
{
}



RenderThreadTask*
DefaultScheduler::createRunnable(TimeValue frame,
                                 bool useRenderStarts,
                                 const std::vector<ViewIdx>& viewsToRender)
{
    return new DefaultRenderFrameRunnable(getOutputNode(), this, frame, useRenderStarts, viewsToRender);
}


void
DefaultScheduler::processFrame(const ProcessFrameArgsBase& /*args*/)
{
    // We don't have anymore writer that need to process things in order. WriteFFMPEG is doing it for us
} // DefaultScheduler::processFrame


void
DefaultScheduler::timelineGoTo(TimeValue time)
{
    QMutexLocker k(&_currentTimeMutex);

    _currentTime =  time;
}

TimeValue
DefaultScheduler::timelineGetTime() const
{
    QMutexLocker k(&_currentTimeMutex);

    return _currentTime;
}

void
DefaultScheduler::getFrameRangeToRender(TimeValue& first,
                                        TimeValue& last) const
{
    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();

    first = args->firstFrame;
    last = args->lastFrame;
}

void
DefaultScheduler::handleRenderFailure(ActionRetCodeEnum /*stat*/, const std::string& errorMessage)
{
    if ( appPTR->isBackground() ) {
        if (!errorMessage.empty()) {
            std::cerr << errorMessage << std::endl;
        }
    }
}

SchedulingPolicyEnum
DefaultScheduler::getSchedulingPolicy() const
{
    return eSchedulingPolicyFFA;
}

void
DefaultScheduler::aboutToStartRender()
{
    OutputSchedulerThreadStartArgsPtr args = getCurrentRunArgs();
    NodePtr outputNode = getOutputNode();

    {
        QMutexLocker k(&_currentTimeMutex);
        _currentTime  = args->startingFrame;
    }
    bool isBackGround = appPTR->isBackground();

    if (isBackGround) {
        QString longText = QString::fromUtf8( outputNode->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering started");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingStartedShort), true);
    }

    // Activate the internal writer node for a write node
    WriteNodePtr isWrite = toWriteNode( outputNode->getEffectInstance() );
    if (isWrite) {
        isWrite->onSequenceRenderStarted();
    }

    std::string cb = outputNode->getEffectInstance()->getBeforeRenderCallback();
    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            outputNode->getApp()->appendToScriptEditor( std::string("Failed to run beforeRender callback: ")
                                                       + e.what() );

            return;
        }

        if ( !error.empty() ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The beforeRender callback supports the following signature(s):\n");
        signatureError.append("- callback(thisNode, app)");
        if (args.size() != 2) {
            outputNode->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + signatureError);

            return;
        }

        if ( (args[0] != "thisNode") || (args[1] != "app") ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run beforeRender callback: " + signatureError);

            return;
        }


        std::stringstream ss;
        std::string appStr = outputNode->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
        ss << cb << "(" << outputNodeName << ", " << appStr << ")";
        std::string script = ss.str();
        try {
            runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        } catch (const std::exception &e) {
            notifyRenderFailure( eActionStatusFailed, e.what() );
        }
    }
} // DefaultScheduler::aboutToStartRender

void
DefaultScheduler::onRenderStopped(bool aborted)
{
    NodePtr outputNode = getOutputNode();

    {
        QString longText = QString::fromUtf8( outputNode->getScriptName_mt_safe().c_str() ) + tr(" ==> Rendering finished");
        appPTR->writeToOutputPipe(longText, QString::fromUtf8(kRenderingFinishedStringShort), true);
    }


    std::string cb = outputNode->getEffectInstance()->getAfterRenderCallback();
    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            outputNode->getApp()->appendToScriptEditor( std::string("Failed to run afterRender callback: ")
                                                       + e.what() );

            return;
        }

        if ( !error.empty() ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The after render callback supports the following signature(s):\n");
        signatureError.append("- callback(aborted, thisNode, app)");
        if (args.size() != 3) {
            outputNode->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + signatureError);

            return;
        }

        if ( (args[0] != "aborted") || (args[1] != "thisNode") || (args[2] != "app") ) {
            outputNode->getApp()->appendToScriptEditor("Failed to run afterRender callback: " + signatureError);

            return;
        }


        std::stringstream ss;
        std::string appStr = outputNode->getApp()->getAppIDString();
        std::string outputNodeName = appStr + "." + outputNode->getFullyQualifiedName();
        ss << cb << "(";
        if (aborted) {
            ss << "True, ";
        } else {
            ss << "False, ";
        }
        ss << outputNodeName << ", " << appStr << ")";
        std::string script = ss.str();
        try {
            runCallbackWithVariables( QString::fromUtf8( script.c_str() ) );
        } catch (...) {
            //Ignore expcetions in callback since the render is finished anyway
        }
    }
} // DefaultScheduler::onRenderStopped

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_DefaultRenderScheduler.cpp"
