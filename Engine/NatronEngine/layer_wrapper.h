#ifndef SBK_LAYERWRAPPER_H
#define SBK_LAYERWRAPPER_H

#include <shiboken.h>

#include <RotoWrapper.h>

NATRON_NAMESPACE_ENTER;
class LayerWrapper : public Layer
{
public:
    virtual ~LayerWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_LAYERWRAPPER_H

