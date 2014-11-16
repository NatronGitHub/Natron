#ifndef SBK_INT2DPARAMWRAPPER_H
#define SBK_INT2DPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class Int2DParamWrapper : public Int2DParam
{
public:
    virtual ~Int2DParamWrapper();
};

#endif // SBK_INT2DPARAMWRAPPER_H

