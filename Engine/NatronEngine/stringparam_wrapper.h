#ifndef SBK_STRINGPARAMWRAPPER_H
#define SBK_STRINGPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class StringParamWrapper : public StringParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { StringParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~StringParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_STRINGPARAMWRAPPER_H

