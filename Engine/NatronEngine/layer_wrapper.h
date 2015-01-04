#ifndef SBK_LAYERWRAPPER_H
#define SBK_LAYERWRAPPER_H

#define protected public

#include <shiboken.h>

#include <RotoWrapper.h>

class LayerWrapper : public Layer
{
public:
    virtual ~LayerWrapper();
};

#endif // SBK_LAYERWRAPPER_H

