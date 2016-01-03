#ifndef SBK_SEPARATORPARAMWRAPPER_H
#define SBK_SEPARATORPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class SeparatorParamWrapper : public SeparatorParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { SeparatorParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~SeparatorParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_SEPARATORPARAMWRAPPER_H

