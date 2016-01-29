#ifndef SBK_ANIMATEDPARAMWRAPPER_H
#define SBK_ANIMATEDPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class AnimatedParamWrapper : public AnimatedParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { AnimatedParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~AnimatedParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_ANIMATEDPARAMWRAPPER_H

