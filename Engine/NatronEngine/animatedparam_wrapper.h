#ifndef SBK_ANIMATEDPARAMWRAPPER_H
#define SBK_ANIMATEDPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class AnimatedParamWrapper : public AnimatedParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param) { AnimatedParam::_addAsDependencyOf(fromExprDimension, param); }
    virtual ~AnimatedParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_ANIMATEDPARAMWRAPPER_H

