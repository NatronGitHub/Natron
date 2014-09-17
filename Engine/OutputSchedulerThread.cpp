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

#include "OutputSchedulerThread.h"

#include <map>
#include <QMetaType>
#include <QMutex>
#include <QWaitCondition>
#include <QCoreApplication>
#include <QString>

#include "Engine/AppManager.h"
#include "Engine/Image.h"
#include "Engine/Timer.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerInstancePrivate.h"

#define NATRON_FPS_REFRESH_RATE_SECONDS 1.5


struct BufferedKey
{
    int view;
    double time;
};

struct BufferedKeyCompare_less
{
    bool operator()(const BufferedKey& lhs,const BufferedKey& rhs) const
    {
        if (lhs.time < rhs.time) {
            return true;
        } else if (lhs.time > rhs.time) {
            return false;
        } else {
            if (lhs.view < rhs.view) {
                return true;
            } else if (lhs.view > rhs.view) {
                return false;
            } else {
                return false;
            }
        }
    }
};

typedef std::map<BufferedKey, boost::shared_ptr<BufferableObject> , BufferedKeyCompare_less > FrameBuffer;


namespace {
    class MetaTypesRegistration
    {
    public:
        inline MetaTypesRegistration()
        {
            qRegisterMetaType<boost::shared_ptr<BufferableObject> >("boost::shared_ptr<BufferableObject>");
        }
    };
}
static MetaTypesRegistration registration;


struct OutputSchedulerThreadPrivate
{
    
    FrameBuffer buf;
    QWaitCondition bufCondition;
    QMutex bufMutex;
    
    bool mustQuit;
    QWaitCondition mustQuitCond;
    QMutex mustQuitMutex;
    
    bool treatRunning;
    QWaitCondition treatCondition;
    QMutex treatMutex;
    
    OutputSchedulerThread::Mode mode;
    
    mutable QMutex timerMutex; ///protects timer & lastFpsChangedTime
    boost::scoped_ptr<Timer> timer; /*!< Timer regulating the engine execution. It is controlled by the GUI.*/
    
    OutputSchedulerThreadPrivate(OutputSchedulerThread::Mode mode)
    : buf()
    , bufCondition()
    , bufMutex()
    , mustQuit(false)
    , mustQuitCond()
    , mustQuitMutex()
    , treatRunning(false)
    , treatCondition()
    , treatMutex()
    , mode(mode)
    , timerMutex()
    , timer(new Timer)
    {
       
    }
    
    void appendBufferedFrame(double time,int view,const boost::shared_ptr<BufferableObject>& image)
    {
        BufferedKey k;
        k.time = time;
        k.view = view;
        buf.insert(std::make_pair(k, image));
    }
};


OutputSchedulerThread::OutputSchedulerThread(Mode mode)
: QThread()
, _imp(new OutputSchedulerThreadPrivate(mode))
{
    QObject::connect(this, SIGNAL(s_doTreatOnMainThread(double,int,boost::shared_ptr<BufferableObject>)), this,
                     SLOT(doTreatFrameMainThread(double,int,boost::shared_ptr<BufferableObject>)));
    
    QObject::connect(_imp->timer.get(), SIGNAL(fpsChanged(double,double)), this, SIGNAL(fpsChanged(double,double)));
}

OutputSchedulerThread::~OutputSchedulerThread()
{
    
}

void
OutputSchedulerThread::run()
{
    for (;;) { ///infinite loop
        
        QMutexLocker l(&_imp->bufMutex);
        while (!_imp->buf.empty()) {
            
            ///Check for exit
            {
                QMutexLocker l(&_imp->mustQuitMutex);
                if (_imp->mustQuit) {
                    _imp->mustQuit = false;
                    _imp->mustQuitCond.wakeOne();
                    return;
                }
            }
            
            {
                QMutexLocker timerLocker(&_imp->timerMutex);
                if (_imp->timer->playState == RUNNING) {
                    _imp->timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
                }
            }
            
            
            ///dequeue the first image, by time
            FrameBuffer::iterator it = _imp->buf.begin();
            
            if (_imp->mode == TREAT_ON_SCHEDULER_THREAD) {
                treatFrame(it->first.time, it->first.view, it->second);
            } else {
                QMutexLocker treatLocker (&_imp->treatMutex);
                _imp->treatRunning = true;
                
                emit s_doTreatOnMainThread(it->first.time, it->first.view, it->second);
                
                while (_imp->treatRunning) {
                    _imp->treatCondition.wait(&_imp->treatMutex);
                }
            }
            
            int frameTime = (int)it->first.time;
            notifyFrameRendered(frameTime);
            
            _imp->buf.erase(it);
            
            
        }
        _imp->bufCondition.wait(&_imp->bufMutex);
    }
}

void
OutputSchedulerThread::notifyFrameRendered(int frame)
{
    emit frameRendered(frame);
    if ( appPTR->isBackground() ) {
        QString frameStr = QString::number(frame);
        appPTR->writeToOutputPipe(kFrameRenderedStringLong + frameStr,kFrameRenderedStringShort + frameStr);
    }
    

}

void
OutputSchedulerThread::appendToBuffer(double time,int view,const boost::shared_ptr<BufferableObject>& image)
{
    if (QThread::currentThread() == qApp->thread()) {
        ///Single-threaded VideoEngine, call directly the function
        treatFrame(time, view, image);
    } else {
        
        ///Called by the VideoEngine thread when an image is rendered
        
        QMutexLocker l(&_imp->bufMutex);
        _imp->appendBufferedFrame(time, view, image);
        
        ///Wake up the scheduler thread that an image is available if it is asleep so it can treat it.
        _imp->bufCondition.wakeOne();
    }
}


void
OutputSchedulerThread::doTreatFrameMainThread(double time,int view,const boost::shared_ptr<BufferableObject>& frame)
{
    assert(QThread::currentThread() == qApp->thread());
    
    treatFrame(time, view, frame);
    QMutexLocker treatLocker (&_imp->treatMutex);
    _imp->treatRunning = false;
    _imp->treatCondition.wakeOne();
}

void
OutputSchedulerThread::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    if (QThread::currentThread() == qApp->thread()){
        ///If the scheduler thread was sleeping in the treat condition, waiting for the main-thread to finish
        ///treating the frame then waiting in the mustQuitCond would create a deadlock.
        ///Instead we discard the treating of the frame by taking the lock and setting treatRunning to false
        QMutexLocker treatLocker (&_imp->treatMutex);
        _imp->treatRunning = false;
        _imp->treatCondition.wakeOne();
    }
    
    {
        QMutexLocker l(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        ///Push a fake frame on the buffer to make sure we wake-up the thread
        _imp->appendBufferedFrame(0, 0, boost::shared_ptr<BufferableObject>());
        _imp->bufCondition.wakeOne();
        
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
}

void
OutputSchedulerThread::startFPSTimer()
{
    QMutexLocker l(&_imp->timerMutex);
    _imp->timer->playState = RUNNING;
}

void
OutputSchedulerThread::stopFPSTimer()
{
    QMutexLocker l(&_imp->timerMutex);
    _imp->timer->playState = PAUSE;
}

void
OutputSchedulerThread::setDesiredFPS(double d)
{
    QMutexLocker timerLocker(&_imp->timerMutex);
    _imp->timer->setDesiredFrameRate(d);
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// DefaultScheduler ////////////


DefaultScheduler::DefaultScheduler(Natron::OutputEffectInstance* effect)
: OutputSchedulerThread(TREAT_ON_SCHEDULER_THREAD)
, _effect(effect)
{
    
}

DefaultScheduler::~DefaultScheduler()
{
    
}


/**
 * @brief Called whenever there are images available to treat in the buffer.
 * Once treated, the frame will be removed from the buffer.
 *
 * According to the Mode given to the scheduler this function will be called either by the scheduler thread (this)
 * or by the application's main-thread (typically to do OpenGL rendering).
 **/
void
DefaultScheduler::treatFrame(double time,int view,const boost::shared_ptr<BufferableObject>& /*frame*/)
{
    
    ///Writers render to scale 1 always
    RenderScale scale;
    scale.x = scale.y = 1.;
    
    U64 hash = _effect->getHash();
    
    bool isProjectFormat;
    RectD rod;
    RectI roi;
    
    Natron::ImageComponents components;
    Natron::ImageBitDepth imageDepth;
    _effect->getPreferredDepthAndComponents(-1, &components, &imageDepth);
    
    (void)_effect->getRegionOfDefinition_public(hash,time, scale, view, &rod, &isProjectFormat);
    rod.toPixelEnclosing(0, &roi);

    
    Natron::EffectInstance::RenderRoIArgs args(time,
                                               scale,0,
                                               view,
                                               roi,
                                               true,
                                               false,
                                               false,
                                               rod,
                                               components,
                                               imageDepth,
                                               3);
    (void)_effect->renderRoI(args);
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////// ViewerDisplayScheduler ////////////


ViewerDisplayScheduler::ViewerDisplayScheduler(ViewerInstance* viewer)
: OutputSchedulerThread(TREAT_ON_MAIN_THREAD) //< OpenGL rendering is done on the main-thread
, _viewer(viewer)
{
    
}

ViewerDisplayScheduler::~ViewerDisplayScheduler()
{
    
}


/**
 * @brief Called whenever there are images available to treat in the buffer.
 * Once treated, the frame will be removed from the buffer.
 *
 * According to the Mode given to the scheduler this function will be called either by the scheduler thread (this)
 * or by the application's main-thread (typically to do OpenGL rendering).
 **/
void
ViewerDisplayScheduler::treatFrame(double /*time*/,int /*view*/,const boost::shared_ptr<BufferableObject>& frame)
{
    boost::shared_ptr<UpdateViewerParams> params = boost::dynamic_pointer_cast<UpdateViewerParams>(frame);
    assert(params);
    _viewer->_imp->updateViewer(params);
}