#ifndef SBK_PARAMETRICPARAMWRAPPER_H
#define SBK_PARAMETRICPARAMWRAPPER_H

#include <shiboken.h>

#include <PyParameter.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class ParametricParamWrapper : public ParametricParam
{
public:
    inline void _addAsDependencyOf_protected(Param * param, int fromExprDimension, int thisDimension, const QString & fromExprView, const QString & thisView) { ParametricParam::_addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView); }
    virtual ~ParametricParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_PARAMETRICPARAMWRAPPER_H

