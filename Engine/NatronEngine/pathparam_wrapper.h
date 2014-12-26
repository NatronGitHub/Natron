#ifndef SBK_PATHPARAMWRAPPER_H
#define SBK_PATHPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class PathParamWrapper : public PathParam
{
public:
    virtual ~PathParamWrapper();
};

#endif // SBK_PATHPARAMWRAPPER_H

