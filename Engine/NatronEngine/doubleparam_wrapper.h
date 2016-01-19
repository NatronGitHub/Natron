#ifndef SBK_DOUBLEPARAMWRAPPER_H
#define SBK_DOUBLEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class DoubleParamWrapper : public DoubleParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { DoubleParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~DoubleParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_DOUBLEPARAMWRAPPER_H

