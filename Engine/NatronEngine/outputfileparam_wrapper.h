#ifndef SBK_OUTPUTFILEPARAMWRAPPER_H
#define SBK_OUTPUTFILEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class OutputFileParamWrapper : public OutputFileParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param) { OutputFileParam::_addAsDependencyOf(fromExprDimension, param); }
    virtual ~OutputFileParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_OUTPUTFILEPARAMWRAPPER_H

