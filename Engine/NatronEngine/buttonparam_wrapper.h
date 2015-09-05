#ifndef SBK_BUTTONPARAMWRAPPER_H
#define SBK_BUTTONPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class ButtonParamWrapper : public ButtonParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { ButtonParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~ButtonParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_BUTTONPARAMWRAPPER_H

