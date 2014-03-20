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
    , textureRect()
    , bytesCount(0)
    , gain(1.)
    , offset(0.)
    , lut(ViewerInstance::sRGB)
    {}

    unsigned char* ramBuffer;
    TextureRect textureRect;
    size_t bytesCount;
    double gain;
    double offset;
    ViewerInstance::ViewerColorSpace lut;
    boost::shared_ptr<Natron::FrameEntry> cachedFrame; //!< this pointer is at least valid until this function exits, the the cache entry cannot be released
    boost::shared_ptr<const Natron::FrameParams> cachedFrameParams; //!< FIXME: do we need to keep this?
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
    , viewerParamsLut(ViewerInstance::sRGB)
    , viewerParamsAutoContrast(false)
    , viewerParamsChannels(ViewerInstance::RGB)
    , lastRenderedImage()
    , threadIdMutex()
    , threadIdVideoEngine(NULL)
    {
        connect(this,SIGNAL(doUpdateViewer(UpdateViewerParams)),this,SLOT(updateViewer(UpdateViewerParams)));
    }

    void assertVideoEngine()
    {
#ifdef NATRON_DEBUG
        QMutexLocker l(&threadIdMutex);
        if (threadIdVideoEngine == NULL) {
            threadIdVideoEngine = QThread::currentThread();
        }
        assert(QThread::currentThread() == threadIdVideoEngine);
#endif
    }

    /// function that emits the signal to call updateViewer() from the main thread
    void updateViewerVideoEngine(const UpdateViewerParams &params);

    public slots:
    /**
     * @brief Slot called internally by the renderViewer() function when it wants to refresh the OpenGL viewer.
     * Do not call this yourself.
     **/
    void updateViewer(UpdateViewerParams params);

signals:
    /**
     *@brief Signal emitted when the engine needs to inform the main thread that it should refresh the viewer
     **/
    void doUpdateViewer(UpdateViewerParams params);

public:
    const ViewerInstance* const instance;

    OpenGLViewerI* uiContext; // written in the main thread before VideoEngine thread creation, accessed from VideoEngine


    mutable QMutex forceRenderMutex;
    bool forceRender;/*!< true when we want to by-pass the cache*/


    QWaitCondition     updateViewerCond;
    mutable QMutex     updateViewerMutex; //!< protects updateViewerRunning, updateViewerParams, updateViewerPboIndex
    bool               updateViewerRunning; //<! This flag is true when the updateViewer() function is called. That function
    //is always called on the main thread, but the thread running renderViewer MUST
    //wait the entire time. This flag is here to make the renderViewer() thread wait
    //until the texture upload is finished by the main thread.
    int                updateViewerPboIndex; // always accessed in the main thread: initialized in the constructor, then always accessed and modified by updateViewer()

    /// a private buffer for storing frames that are not in the viewer cache.
    /// This buffer only grows in size, and is definitely freed in the destructor
    void* buffer;
    size_t bufferAllocated;

    mutable QMutex   viewerParamsMutex; //< protects viewerParamsGain, viewerParamsLut, viewerParamsAutoContrast, viewerParamsChannels
    double           viewerParamsGain ;/*!< Current gain setting in the GUI. Not affected by autoContrast. */
    ViewerColorSpace viewerParamsLut; /*!< a value coding the current color-space used to render.
                                       0 = sRGB ,  1 = linear , 2 = Rec 709*/
    bool             viewerParamsAutoContrast;
    DisplayChannels  viewerParamsChannels;

    boost::shared_ptr<Natron::Image> lastRenderedImage; //< A ptr to the last returned image by renderRoI. @see getLastRenderedImage()

    // store the threadId of the VideoEngine thread - used for debugging purposes
    mutable QMutex threadIdMutex;
    QThread *threadIdVideoEngine;
};
//} // namespace Natron


#endif
