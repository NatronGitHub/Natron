#ifndef SBK_CHOICEPARAMWRAPPER_H
#define SBK_CHOICEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class ChoiceParamWrapper : public ChoiceParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { ChoiceParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~ChoiceParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_CHOICEPARAMWRAPPER_H

