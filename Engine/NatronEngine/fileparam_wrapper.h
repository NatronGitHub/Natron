#ifndef SBK_FILEPARAMWRAPPER_H
#define SBK_FILEPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class FileParamWrapper : public FileParam
{
public:
    virtual ~FileParamWrapper();
};

#endif // SBK_FILEPARAMWRAPPER_H

