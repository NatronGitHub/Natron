#ifndef SBK_INTPARAMWRAPPER_H
#define SBK_INTPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class IntParamWrapper : public IntParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { IntParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~IntParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_INTPARAMWRAPPER_H

