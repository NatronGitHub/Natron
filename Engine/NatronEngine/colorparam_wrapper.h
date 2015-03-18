#ifndef SBK_COLORPARAMWRAPPER_H
#define SBK_COLORPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class ColorParamWrapper : public ColorParam
{
public:
    virtual ~ColorParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_COLORPARAMWRAPPER_H

