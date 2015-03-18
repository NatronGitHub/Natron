#ifndef SBK_STRINGPARAMBASEWRAPPER_H
#define SBK_STRINGPARAMBASEWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class StringParamBaseWrapper : public StringParamBase
{
public:
    virtual ~StringParamBaseWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_STRINGPARAMBASEWRAPPER_H

