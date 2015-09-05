#ifndef SBK_DOUBLEPARAMWRAPPER_H
#define SBK_DOUBLEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class DoubleParamWrapper : public DoubleParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { DoubleParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~DoubleParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_DOUBLEPARAMWRAPPER_H

