#ifndef SBK_PARAMETRICPARAMWRAPPER_H
#define SBK_PARAMETRICPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class ParametricParamWrapper : public ParametricParam
{
public:
    virtual ~ParametricParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_PARAMETRICPARAMWRAPPER_H

