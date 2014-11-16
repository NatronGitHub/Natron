#ifndef SBK_INT3DPARAMWRAPPER_H
#define SBK_INT3DPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class Int3DParamWrapper : public Int3DParam
{
public:
    virtual ~Int3DParamWrapper();
};

#endif // SBK_INT3DPARAMWRAPPER_H

