#ifndef SBK_STRINGPARAMBASEWRAPPER_H
#define SBK_STRINGPARAMBASEWRAPPER_H

#include <shiboken.h>

#include <PyParameter.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class StringParamBaseWrapper : public StringParamBase
{
public:
    inline void _addAsDependencyOf_protected(Param * param, int fromExprDimension, int thisDimension, const QString & fromExprView, const QString & thisView) { StringParamBase::_addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView); }
    virtual ~StringParamBaseWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_STRINGPARAMBASEWRAPPER_H

