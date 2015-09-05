#ifndef SBK_INT2DPARAMWRAPPER_H
#define SBK_INT2DPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class Int2DParamWrapper : public Int2DParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { Int2DParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~Int2DParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_INT2DPARAMWRAPPER_H

