#ifndef SBK_BOOLEANPARAMWRAPPER_H
#define SBK_BOOLEANPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class BooleanParamWrapper : public BooleanParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { BooleanParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~BooleanParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_BOOLEANPARAMWRAPPER_H

