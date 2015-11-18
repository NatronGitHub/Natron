#ifndef SBK_APPWRAPPER_H
#define SBK_APPWRAPPER_H

#include <shiboken.h>

#include <AppInstanceWrapper.h>

class AppWrapper : public App
{
public:
    inline void renderInternal_protected(bool forceBlocking, Effect * writeNode, int firstFrame, int lastFrame) { App::renderInternal(forceBlocking, writeNode, firstFrame, lastFrame); }
    inline void renderInternal_protected(bool forceBlocking, const std::list<Effect * > & effects, const std::list<int > & firstFrames, const std::list<int > & lastFrames) { App::renderInternal(forceBlocking, effects, firstFrames, lastFrames); }
    virtual ~AppWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_APPWRAPPER_H

