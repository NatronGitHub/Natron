#ifndef SBK_SEPARATORPARAMWRAPPER_H
#define SBK_SEPARATORPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class SeparatorParamWrapper : public SeparatorParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { SeparatorParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~SeparatorParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_SEPARATORPARAMWRAPPER_H

