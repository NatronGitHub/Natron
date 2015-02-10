#ifndef SBK_OUTPUTFILEPARAMWRAPPER_H
#define SBK_OUTPUTFILEPARAMWRAPPER_H

#define protected public

#include <shiboken.h>

#include <ParameterWrapper.h>

class OutputFileParamWrapper : public OutputFileParam
{
public:
    virtual ~OutputFileParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_OUTPUTFILEPARAMWRAPPER_H

