#ifndef SBK_INTPARAMWRAPPER_H
#define SBK_INTPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class IntParamWrapper : public IntParam
{
public:
    virtual ~IntParamWrapper();
};

#endif // SBK_INTPARAMWRAPPER_H

