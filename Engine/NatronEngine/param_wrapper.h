#ifndef SBK_PARAMWRAPPER_H
#define SBK_PARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class ParamWrapper : public Param
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { Param::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~ParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_PARAMWRAPPER_H

