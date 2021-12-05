#ifndef SBK_APPWRAPPER_H
#define SBK_APPWRAPPER_H

#include <PyAppInstance.h>


// Extra includes
#include <PyNode.h>
#include <PyNodeGroup.h>
#include <map>
#include <PyAppInstance.h>
#include <list>
#include <PyParameter.h>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class AppWrapper : public App
{
public:
    inline void renderInternal_protected(bool forceBlocking, Effect * writeNode, int firstFrame, int lastFrame, int frameStep) { App::renderInternal(forceBlocking, writeNode, firstFrame, lastFrame, frameStep); }
    inline void renderInternal_protected(bool forceBlocking, const std::list<Effect* > & effects, const std::list<int > & firstFrames, const std::list<int > & lastFrames, const std::list<int > & frameSteps) { App::renderInternal(forceBlocking, effects, firstFrames, lastFrames, frameSteps); }
    ~AppWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  ifndef SBK_GROUPWRAPPER_H
#  define SBK_GROUPWRAPPER_H

// Inherited base class:
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class GroupWrapper : public Group
{
public:
    GroupWrapper();
    ~GroupWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  endif // SBK_GROUPWRAPPER_H

#endif // SBK_APPWRAPPER_H

