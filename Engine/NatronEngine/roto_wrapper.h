#ifndef SBK_ROTOWRAPPER_H
#define SBK_ROTOWRAPPER_H

#include <shiboken.h>

#include <PyRoto.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class RotoWrapper : public Roto
{
public:
    virtual ~RotoWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_ROTOWRAPPER_H

