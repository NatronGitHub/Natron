#ifndef SBK_DOUBLE2DPARAMWRAPPER_H
#define SBK_DOUBLE2DPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class Double2DParamWrapper : public Double2DParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { Double2DParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~Double2DParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_DOUBLE2DPARAMWRAPPER_H

