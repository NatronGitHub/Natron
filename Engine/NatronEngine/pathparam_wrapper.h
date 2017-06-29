#ifndef SBK_PATHPARAMWRAPPER_H
#define SBK_PATHPARAMWRAPPER_H

#include <shiboken.h>

#include <PyParameter.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PathParamWrapper : public PathParam
{
public:
    inline void _addAsDependencyOf_protected(Param * param, int fromExprDimension, int thisDimension, const QString & fromExprView, const QString & thisView) { PathParam::_addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView); }
    virtual ~PathParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_PATHPARAMWRAPPER_H

