#ifndef SBK_PAGEPARAMWRAPPER_H
#define SBK_PAGEPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class PageParamWrapper : public PageParam
{
public:
    virtual ~PageParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_PAGEPARAMWRAPPER_H

