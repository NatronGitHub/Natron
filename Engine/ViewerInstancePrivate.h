//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 20/03/2014.
//
//

#ifndef Natron_Engine_ViewerInstancePrivate_h
#define Natron_Engine_ViewerInstancePrivate_h

#include "ViewerInstance.h"

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/Settings.h"
#include "Engine/TextureRect.h"

namespace Natron {
    class FrameEntry;
    class FrameParams;
}

//namespace Natron {

struct RenderViewerArgs {
    RenderViewerArgs(boost::shared_ptr<const Natron::Image> inputImage_,
                     const TextureRect& texRect_,
                     ViewerInstance::DisplayChannels channels_,
                     int closestPowerOf2_,
                     int bitDepth_,
                     double gain_,
                     double offset_,
                     const Natron::Color::Lut* colorSpace_)
    : inputImage(inputImage_)
    , texRect(texRect_)
    , channels(channels_)
    , closestPowerOf2(closestPowerOf2_)
    , bitDepth(bitDepth_)
    , gain(gain_)
    , offset(offset_)
    , colorSpace(colorSpace_)
    {
    }

    boost::shared_ptr<const Natron::Image> inputImage;
    TextureRect texRect;
    ViewerInstance::DisplayChannels channels;
    int closestPowerOf2;
    int bitDepth;
    double gain;
    double offset;
    const Natron::Color::Lut* colorSpace;
};

/// parameters send from the VideoEngine thread to updateViewer() (which runs in the main thread)
struct UpdateViewerParams
{
    UpdateViewerParams()
    : ramBuffer(NULL)
    , textureIndex(0)
    , textureRect()
    , bytesCount(0)
    , gain(1.)
    , offset(0.)
    , mipMapLevel(0)
    , lut(Natron::sRGB)
    {}

    unsigned char* ramBuffer;
    int textureIndex;
    TextureRect textureRect;
    size_t bytesCount;
    double gain;
    double offset;
    unsigned int mipMapLevel;
    Natron::ViewerColorSpace lut;
    boost::shared_ptr<Natron::FrameEntry> cachedFrame; //!< put a shared_ptr here, so that the cache entry is never released before the end of updateViewer()
};

struct ViewerInstance::ViewerInstancePrivate : public QObject {
    Q_OBJECT
public:
    ViewerInstancePrivate(const ViewerInstance* parent)
    : instance(parent)
    , uiContext(NULL)
    , forceRenderMutex()
    , forceRender(false)
    , updateViewerCond()
    , updateViewerMutex()
    , updateViewerRunning(false)
    , updateViewerPboIndex(0)
    , buffer(NULL)
    , bufferAllocated(0)
    , viewerParamsMutex()
    , viewerParamsGain(1.)
    , viewerParamsLut(Natron::sRGB)
    , viewerParamsAutoContrast(false)
    , viewerParamsChannels(ViewerInstance::RGB)
    , viewerMipMapLevel(0)
    , lastRenderedImageMutex()
    , lastRenderedImage()
    , threadIdMutex()
    , threadIdVideoEngine(NULL)
    , activeInputsMutex()
    , activeInputs()
    {
        connect(this,SIGNAL(doUpdateViewer(boost::shared_ptr<UpdateViewerParams>)),this,
                SLOT(updateViewer(boost::shared_ptr<UpdateViewerParams>)));
        activeInputs[0] = -1;
        activeInputs[1] = -1;
    }

    void assertVideoEngine()
    {
#ifdef NATRON_DEBUG
        int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
        if (nbThreads == -1) {
            assert(QThread::currentThread() == qApp->thread());
        } else {
            QMutexLocker l(&threadIdMutex);
            if (threadIdVideoEngine == NULL) {
                threadIdVideoEngine = QThread::currentThread();
            }
            assert(QThread::currentThread() == threadIdVideoEngine);
        }
#endif
    }

    /// function that emits the signal to call updateViewer() from the main thread
    void updateViewerVideoEngine(const boost::shared_ptr<UpdateViewerParams> &params);
    
    void redrawViewer() { emit mustRedrawViewer(); }

    public slots:
    /**
     * @brief Slot called internally by the renderViewer() function when it wants to refresh the OpenGL viewer.
     * Do not call this yourself.
     **/
    void updateViewer(boost::shared_ptr<UpdateViewerParams> params);

signals:
    /**
     *@brief Signal emitted when the engine needs to inform the main thread that it should refresh the viewer
     **/
    void doUpdateViewer(boost::shared_ptr<UpdateViewerParams> params);
    
    void mustRedrawViewer();

public:
    const ViewerInstance* const instance;

    OpenGLViewerI* uiContext; // written in the main thread before VideoEngine thread creation, accessed from VideoEngine


    mutable QMutex forceRenderMutex;
    bool forceRender;/*!< true when we want to by-pass the cache*/


    // updateViewer: stuff for handling the execution of updateViewer() in the main thread, @see UpdateViewerParams
    QWaitCondition     updateViewerCond;
    mutable QMutex     updateViewerMutex; //!< protects updateViewerRunning, updateViewerPboIndex
    bool               updateViewerRunning; //<! This flag is true when the updateViewer() function is called. That function
    //is always called on the main thread, but the thread running renderViewer MUST
    //wait the entire time. This flag is here to make the renderViewer() thread wait
    //until the texture upload is finished by the main thread.
    int                updateViewerPboIndex; // always accessed in the main thread: initialized in the constructor, then always accessed and modified by updateViewer()

    /// a private buffer for storing frames that are not in the viewer cache.
    /// This buffer only grows in size, and is definitely freed in the destructor
    void* buffer;
    size_t bufferAllocated;

    // viewerParams: The viewer parameters that may be accessed from the GUI
    mutable QMutex   viewerParamsMutex; //< protects viewerParamsGain, viewerParamsLut, viewerParamsAutoContrast, viewerParamsChannels
    double           viewerParamsGain ;/*!< Current gain setting in the GUI. Not affected by autoContrast. */
    Natron::ViewerColorSpace viewerParamsLut; /*!< a value coding the current color-space used to render.
                                       0 = sRGB ,  1 = linear , 2 = Rec 709*/
    bool             viewerParamsAutoContrast;
    DisplayChannels  viewerParamsChannels;
    unsigned int viewerMipMapLevel; //< the mipmap level the viewer should render at (0 == no downscaling)

    mutable QMutex lastRenderedImageMutex;
    boost::shared_ptr<Natron::Image> lastRenderedImage[2]; //< A ptr to the last returned image by renderRoI. @see getLastRenderedImage()

    // store the threadId of the VideoEngine thread - used for debugging purposes
    mutable QMutex threadIdMutex;
    QThread *threadIdVideoEngine;
    
    mutable QMutex activeInputsMutex;
    int activeInputs[2]; //< indexes of the inputs used for the wipe
    
    
};
//} // namespace Natron


#endif
