TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT += gui core opengl network

SOURCES += \
    ../../Engine/ChannelSet.cpp \
    ../../Engine/Curve.cpp \
    ../../Engine/CurveSerialization.cpp \
    ../../Engine/EffectInstance.cpp \
    ../../Engine/Hash64.cpp \
    ../../Engine/Image.cpp \
    ../../Engine/Knob.cpp \
    ../../Engine/KnobSerialization.cpp \
    ../../Engine/KnobFactory.cpp \
    ../../Engine/KnobFile.cpp \
    ../../Engine/KnobTypes.cpp \
    ../../Engine/Log.cpp \
    ../../Engine/Lut.cpp \
    ../../Engine/MemoryFile.cpp \
    ../../Engine/Node.cpp \
    ../../Engine/NodeSerialization.cpp \
    ../../Engine/OfxClipInstance.cpp \
    ../../Engine/OfxHost.cpp \
    ../../Engine/OfxImageEffectInstance.cpp \
    ../../Engine/OfxEffectInstance.cpp \
    ../../Engine/OfxOverlayInteract.cpp \
    ../../Engine/OfxParamInstance.cpp \
    ../../Engine/Project.cpp \
    ../../Engine/ProjectPrivate.cpp \
    ../../Engine/ProjectSerialization.cpp \
    ../../Engine/Row.cpp \
    ../../Engine/Settings.cpp \
    ../../Engine/TimeLine.cpp \
    ../../Engine/Timer.cpp \
    ../../Engine/VideoEngine.cpp \
    ../../Engine/ViewerInstance.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhBinary.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhClip.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhHost.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhImageEffect.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhImageEffectAPI.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhInteract.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhMemory.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhParam.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhPluginAPICache.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhPluginCache.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhPropertySuite.cpp \
    ../../libs/OpenFX/HostSupport/src/ofxhUtilities.cpp \
    ../../libs/OpenFX_extensions/ofxhParametricParam.cpp \
    BaseTest.cpp

HEADERS += \
    ../../Engine/Cache.h \
    ../../Engine/Curve.h \
    ../../Engine/CurveSerialization.h \
    ../../Engine/CurvePrivate.h \
    ../../Engine/Rect.h \
    ../../Engine/ChannelSet.h \
    ../../Engine/EffectInstance.h \
    ../../Engine/Format.h \
    ../../Engine/FrameEntry.h \
    ../../Engine/Hash64.h \
    ../../Engine/ImageInfo.h \
    ../../Engine/Image.h \
    ../../Engine/Interpolation.h \
    ../../Engine/Knob.h \
    ../../Engine/KnobSerialization.h \
    ../../Engine/KnobFactory.h \
    ../../Engine/KnobFile.h \
    ../../Engine/KnobTypes.h \
    ../../Engine/Log.h \
    ../../Engine/LRUHashTable.h \
    ../../Engine/Lut.h \
    ../../Engine/MemoryFile.h \
    ../../Engine/Node.h \
    ../../Engine/NodeSerialization.h \
    ../../Engine/OfxClipInstance.h \
    ../../Engine/OfxHost.h \
    ../../Engine/OfxImageEffectInstance.h \
    ../../Engine/OfxEffectInstance.h \
    ../../Engine/OfxOverlayInteract.h \
    ../../Engine/OfxParamInstance.h \
    ../../Engine/Project.h \
    ../../Engine/ProjectPrivate.h \
    ../../Engine/ProjectSerialization.h \
    ../../Engine/Row.h \
    ../../Engine/Settings.h \
    ../../Engine/Singleton.h \
    ../../Engine/TextureRect.h \
    ../../Engine/TextureRectSerialization.h \
    ../../Engine/TimeLine.h \
    ../../Engine/Timer.h \
    ../../Engine/Variant.h \
    ../../Engine/VideoEngine.h \
    ../../Engine/ViewerInstance.h \
    ../../libs/OpenFX/HostSupport/include/ofxhBinary.h \
    ../../libs/OpenFX/HostSupport/include/ofxhClip.h \
    ../../libs/OpenFX/HostSupport/include/ofxhHost.h \
    ../../libs/OpenFX/HostSupport/include/ofxhImageEffect.h \
    ../../libs/OpenFX/HostSupport/include/ofxhImageEffectAPI.h \
    ../../libs/OpenFX/HostSupport/include/ofxhInteract.h \
    ../../libs/OpenFX/HostSupport/include/ofxhMemory.h \
    ../../libs/OpenFX/HostSupport/include/ofxhParam.h \
    ../../libs/OpenFX/HostSupport/include/ofxhPluginAPICache.h \
    ../../libs/OpenFX/HostSupport/include/ofxhPluginCache.h \
    ../../libs/OpenFX/HostSupport/include/ofxhProgress.h \
    ../../libs/OpenFX/HostSupport/include/ofxhPropertySuite.h \
    ../../libs/OpenFX/HostSupport/include/ofxhTimeLine.h \
    ../../libs/OpenFX/HostSupport/include/ofxhUtilities.h \
    ../../libs/OpenFX/HostSupport/include/ofxhXml.h \
    ../../libs/OpenFX/include/ofxCore.h \
    ../../libs/OpenFX/include/ofxImageEffect.h \
    ../../libs/OpenFX/include/ofxInteract.h \
    ../../libs/OpenFX/include/ofxKeySyms.h \
    ../../libs/OpenFX/include/ofxMemory.h \
    ../../libs/OpenFX/include/ofxMessage.h \
    ../../libs/OpenFX/include/ofxMultiThread.h \
    ../../libs/OpenFX/include/ofxOpenGLRender.h \
    ../../libs/OpenFX/include/ofxParam.h \
    ../../libs/OpenFX/include/ofxParametricParam.h \
    ../../libs/OpenFX/include/ofxPixels.h \
    ../../libs/OpenFX/include/ofxProgress.h \
    ../../libs/OpenFX/include/ofxProperty.h \
    ../../libs/OpenFX/include/ofxTimeLine.h \
    ../../libs/OpenFX_extensions/tuttle/ofxGraphAPI.h \
    ../../libs/OpenFX_extensions/tuttle/ofxMetadata.h \
    ../../libs/OpenFX_extensions/tuttle/ofxParam.h \
    ../../libs/OpenFX_extensions/tuttle/ofxParamAPI.h \
    ../../libs/OpenFX_extensions/tuttle/ofxReadWrite.h \
    ../../libs/OpenFX_extensions/ofxhParametricParam.h \
    ../../libs/OpenFX/include/natron/IOExtensions.h \
    BaseTest.h

INCLUDEPATH += google_test/include
LIBS += google-test/build/libgtest.a google-test/build/ligtest_main.a


