#ifndef SBK_STRINGPARAMWRAPPER_H
#define SBK_STRINGPARAMWRAPPER_H

#include <shiboken.h>

#include <PyParameter.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class StringParamWrapper : public StringParam
{
public:
    inline void _addAsDependencyOf_protected(Param * param, int fromExprDimension, int thisDimension, const QString & fromExprView, const QString & thisView) { StringParam::_addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView); }
    virtual ~StringParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_STRINGPARAMWRAPPER_H

