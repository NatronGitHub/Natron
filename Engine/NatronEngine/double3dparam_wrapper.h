#ifndef SBK_DOUBLE3DPARAMWRAPPER_H
#define SBK_DOUBLE3DPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class Double3DParamWrapper : public Double3DParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { Double3DParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~Double3DParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_DOUBLE3DPARAMWRAPPER_H

