#ifndef SBK_COLORPARAMWRAPPER_H
#define SBK_COLORPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class ColorParamWrapper : public ColorParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { ColorParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~ColorParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_COLORPARAMWRAPPER_H

