#ifndef SBK_GROUPPARAMWRAPPER_H
#define SBK_GROUPPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class GroupParamWrapper : public GroupParam
{
public:
    virtual ~GroupParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_GROUPPARAMWRAPPER_H

