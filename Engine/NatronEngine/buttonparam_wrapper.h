#ifndef SBK_BUTTONPARAMWRAPPER_H
#define SBK_BUTTONPARAMWRAPPER_H

#include <shiboken.h>

#include <PyParameter.h>

NATRON_NAMESPACE_ENTER;
class ButtonParamWrapper : public ButtonParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { ButtonParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~ButtonParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_BUTTONPARAMWRAPPER_H

