#ifndef SBK_OUTPUTFILEPARAMWRAPPER_H
#define SBK_OUTPUTFILEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class OutputFileParamWrapper : public OutputFileParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { OutputFileParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~OutputFileParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_OUTPUTFILEPARAMWRAPPER_H

