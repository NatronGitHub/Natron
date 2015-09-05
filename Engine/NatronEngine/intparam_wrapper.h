#ifndef SBK_INTPARAMWRAPPER_H
#define SBK_INTPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class IntParamWrapper : public IntParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { IntParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~IntParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_INTPARAMWRAPPER_H

