#ifndef SBK_CHOICEPARAMWRAPPER_H
#define SBK_CHOICEPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class ChoiceParamWrapper : public ChoiceParam
{
public:
    virtual ~ChoiceParamWrapper();
};

#endif // SBK_CHOICEPARAMWRAPPER_H

