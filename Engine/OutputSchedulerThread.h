//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef OUTPUTSCHEDULERTHREAD_H
#define OUTPUTSCHEDULERTHREAD_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <QThread>

#include "Global/Macros.h"

/**
 * @brief Stub class used by internal implementation of OutputSchedulerThread to pass objects through signal/slots
 **/
class BufferableObject
{
public:
    
    
    BufferableObject() {}
    
    virtual ~BufferableObject() {}
};



/**
 * @brief Used when rendering in sequential order (playback or on disk) to render the frames in the order as they were asked for
 * before rendering.
 **/
struct OutputSchedulerThreadPrivate;
class OutputSchedulerThread : public QThread
{
    Q_OBJECT
    
public:
    
    enum Mode
    {
        TREAT_ON_SCHEDULER_THREAD = 0, //< the treatFrame_blocking function will be called by the OutputSchedulerThread thread.
        TREAT_ON_MAIN_THREAD //< the treatFrame_blocking function will be called by the application's main-thread.
    };
    
    OutputSchedulerThread(Mode mode);
    
    virtual ~OutputSchedulerThread();
    
    /**
     * @brief When the render thread (i.e: the VideoEngine thread) has finished rendering a frame, it must
     * append it here for buffering to make sure the output device (Viewer, Writer, etc...) will proceed the frames
     * in respect to the time parameter.
     **/
    void appendToBuffer(double time,int view,const boost::shared_ptr<BufferableObject>& frame);
    
    /**
     * @brief Once returned from that function, the object's thread will be finished and the object unusable.
     **/
    void quitThread();
    
    /**
     * @brief Starts the timer that will regulate the FPS at the FPS set by setDesiredFPS.
     * By default the FPS is set to 24.
     * Render on disk should never call this otherwise it would regulate the writing FPS.
     **/
    void startFPSTimer();
    
    /**
     * @brief Stop FPS regulation that was started by startFPSTimer()
     **/
    void stopFPSTimer();
    
    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);
    
    /**
     * @brief Called when a frame has been rendered completetly
     **/
    void notifyFrameRendered(int frame);
    
public slots:
    
    void doTreatFrameMainThread(double time,int view,const boost::shared_ptr<BufferableObject>& frame);
    
signals:
    
    void s_doTreatOnMainThread(double time,int view,const boost::shared_ptr<BufferableObject>& frame);
    
    void fpsChanged(double actualFps,double desiredFps);
    
    void frameRendered(int time);
    
protected:
    
    
    /**
     * @brief Called whenever there are images available to treat in the buffer.
     * Once treated, the frame will be removed from the buffer.
     *
     * According to the Mode given to the scheduler this function will be called either by the scheduler thread (this)
     * or by the application's main-thread (typically to do OpenGL rendering).
     **/
    virtual void treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& frame) = 0;
    
    
private:
    
    virtual void run() OVERRIDE FINAL;
    
    boost::scoped_ptr<OutputSchedulerThreadPrivate> _imp;
    
};

namespace Natron {
class OutputEffectInstance;
}
class DefaultScheduler : public OutputSchedulerThread
{
public:
    
    DefaultScheduler(Natron::OutputEffectInstance* effect);
    
    virtual ~DefaultScheduler();
    
private:
    
    virtual void treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& frame) OVERRIDE FINAL;
    
    Natron::OutputEffectInstance* _effect;
};


class ViewerInstance;
class ViewerDisplayScheduler : public OutputSchedulerThread
{
public:
    
    ViewerDisplayScheduler(ViewerInstance* viewer);
    
    virtual ~ViewerDisplayScheduler();
    
private:

    virtual void treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& frame) OVERRIDE FINAL;
    
    ViewerInstance* _viewer;
};

#endif // OUTPUTSCHEDULERTHREAD_H
