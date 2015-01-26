#ifndef SBK_BUTTONPARAMWRAPPER_H
#define SBK_BUTTONPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class ButtonParamWrapper : public ButtonParam
{
public:
    virtual ~ButtonParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_BUTTONPARAMWRAPPER_H

