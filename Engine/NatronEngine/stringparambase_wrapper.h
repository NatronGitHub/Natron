#ifndef SBK_STRINGPARAMBASEWRAPPER_H
#define SBK_STRINGPARAMBASEWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class StringParamBaseWrapper : public StringParamBase
{
public:
    virtual ~StringParamBaseWrapper();
};

#endif // SBK_STRINGPARAMBASEWRAPPER_H

