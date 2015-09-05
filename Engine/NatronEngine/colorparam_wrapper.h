#ifndef SBK_COLORPARAMWRAPPER_H
#define SBK_COLORPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class ColorParamWrapper : public ColorParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { ColorParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~ColorParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_COLORPARAMWRAPPER_H

