#ifndef SBK_TRACKWRAPPER_H
#define SBK_TRACKWRAPPER_H

#include <shiboken.h>

#include <PyTracker.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class TrackWrapper : public Track
{
public:
    ~TrackWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_TRACKWRAPPER_H

