#ifndef SBK_DOUBLE3DPARAMWRAPPER_H
#define SBK_DOUBLE3DPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class Double3DParamWrapper : public Double3DParam
{
public:
    virtual ~Double3DParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_DOUBLE3DPARAMWRAPPER_H

