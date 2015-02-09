#ifndef SBK_EFFECTWRAPPER_H
#define SBK_EFFECTWRAPPER_H

#define protected public

#include <shiboken.h>

#include <NodeWrapper.h>

class EffectWrapper : public Effect
{
public:
    ~EffectWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_EFFECTWRAPPER_H

