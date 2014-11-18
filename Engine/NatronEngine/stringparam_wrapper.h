#ifndef SBK_STRINGPARAMWRAPPER_H
#define SBK_STRINGPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class StringParamWrapper : public StringParam
{
public:
    virtual ~StringParamWrapper();
};

#endif // SBK_STRINGPARAMWRAPPER_H

