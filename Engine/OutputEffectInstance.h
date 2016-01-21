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

#ifndef Engine_OutputEffectInstance_h
#define Engine_OutputEffectInstance_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QtCore/QMutex>

#include "Global/Macros.h"

#include "Engine/EffectInstance.h"
#include "Engine/EngineFwd.h"


class QThread;


NATRON_NAMESPACE_ENTER;

class OutputEffectInstance
    : public EffectInstance
{
    
    struct RenderSequenceArgs
    {
        BlockingBackgroundRender* renderController;
        std::vector<int> viewsToRender;
        int firstFrame;
        int lastFrame;
        int frameStep;
        bool useStats;
        bool blocking;
    };

    
    mutable QMutex _outputEffectDataLock;
    std::list<RenderSequenceArgs> _renderSequenceRequests;
    RenderEngine* _engine;
    std::list<double> _timeSpentPerFrameRendered;
    SequenceTime _writerCurrentFrame; /*!< for writers only: indicates the current frame
                                       It avoids snchronizing all viewers in the app to the render*/
    SequenceTime _writerFirstFrame;
    SequenceTime _writerLastFrame;


public:

    OutputEffectInstance(boost::shared_ptr<Node> node);

    virtual ~OutputEffectInstance();

    virtual bool isOutput() const OVERRIDE
    {
        return true;
    }

    RenderEngine* getRenderEngine() const
    {
        return _engine;
    }

    /**
     * @brief Returns true if a sequential render is being aborted
     **/
    bool isSequentialRenderBeingAborted() const;

    /**
     * @brief Is this node currently doing playback ?
     **/
    bool isDoingSequentialRender() const;


    /**
     * @brief Starts rendering of all the sequence available, from start to end.
     * This function is meant to be called for on-disk renderer only (i.e: not viewers).
     **/
    void renderFullSequence(bool isBlocking, bool enableRenderStats, BlockingBackgroundRender* renderController, int first, int last, int frameStep);

    void notifyRenderFinished();

    void renderCurrentFrame(bool canAbort);

    void renderCurrentFrameWithRenderStats(bool canAbort);

    bool ifInfiniteclipRectToProjectDefault(RectD* rod) const;

    /**
     * @brief Returns the frame number this effect is currently rendering.
     * Note that this function can be used only for Writers or OpenFX writers,
     * it doesn't work with the Viewer.
     **/
    int getCurrentFrame() const;

    void setCurrentFrame(int f);

    void incrementCurrentFrame();
    void decrementCurrentFrame();

    int getFirstFrame() const;

    void setFirstFrame(int f);

    int getLastFrame() const;

    void setLastFrame(int f);

    virtual void initializeData() OVERRIDE FINAL;

    void updateRenderTimeInfos(double lastTimeSpent, double *averageTimePerFrame, double *totalTimeSpent);

    virtual void reportStats(int time, int view, double wallTime, const std::map<boost::shared_ptr<Node>, NodeRenderStats > & stats);

protected:
    
    void createWriterPath();

    void launchRenderSequence(const RenderSequenceArgs& args);
    
    /**
     * @brief Creates the engine that will control the output rendering
     **/
    virtual RenderEngine* createRenderEngine();
    virtual void resetTimeSpentRenderingInfos() OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT;

#endif // Engine_OutputEffectInstance_h
