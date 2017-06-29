#ifndef SBK_TRACKERWRAPPER_H
#define SBK_TRACKERWRAPPER_H

#include <shiboken.h>

#include <PyTracker.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class TrackerWrapper : public Tracker
{
public:
    virtual ~TrackerWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_TRACKERWRAPPER_H

