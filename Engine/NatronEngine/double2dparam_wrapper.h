#ifndef SBK_DOUBLE2DPARAMWRAPPER_H
#define SBK_DOUBLE2DPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class Double2DParamWrapper : public Double2DParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param) { Double2DParam::_addAsDependencyOf(fromExprDimension, param); }
    virtual ~Double2DParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_DOUBLE2DPARAMWRAPPER_H

