#ifndef SBK_LAYERWRAPPER_H
#define SBK_LAYERWRAPPER_H

#include <shiboken.h>

#include <RotoWrapper.h>

class LayerWrapper : public Layer
{
public:
    virtual ~LayerWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_LAYERWRAPPER_H

