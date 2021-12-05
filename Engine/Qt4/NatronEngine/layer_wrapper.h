#ifndef SBK_LAYERWRAPPER_H
#define SBK_LAYERWRAPPER_H

#include <shiboken.h>

#include <PyRoto.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class LayerWrapper : public Layer
{
public:
    virtual ~LayerWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_LAYERWRAPPER_H

