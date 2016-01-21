#ifndef SBK_EFFECTWRAPPER_H
#define SBK_EFFECTWRAPPER_H

#include <shiboken.h>

#include <NodeWrapper.h>

NATRON_NAMESPACE_ENTER;
class EffectWrapper : public Effect
{
public:
    ~EffectWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_EFFECTWRAPPER_H

