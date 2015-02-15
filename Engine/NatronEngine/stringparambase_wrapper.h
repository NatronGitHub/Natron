#ifndef SBK_STRINGPARAMBASEWRAPPER_H
#define SBK_STRINGPARAMBASEWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class StringParamBaseWrapper : public StringParamBase
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param) { StringParamBase::_addAsDependencyOf(fromExprDimension, param); }
    virtual ~StringParamBaseWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_STRINGPARAMBASEWRAPPER_H

