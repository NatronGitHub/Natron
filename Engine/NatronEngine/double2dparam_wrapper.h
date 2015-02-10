#ifndef SBK_DOUBLE2DPARAMWRAPPER_H
#define SBK_DOUBLE2DPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class Double2DParamWrapper : public Double2DParam
{
public:
    virtual ~Double2DParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_DOUBLE2DPARAMWRAPPER_H

