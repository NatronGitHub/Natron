#ifndef SBK_STRINGPARAMBASEWRAPPER_H
#define SBK_STRINGPARAMBASEWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class StringParamBaseWrapper : public StringParamBase
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { StringParamBase::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~StringParamBaseWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_STRINGPARAMBASEWRAPPER_H

