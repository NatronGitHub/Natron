#ifndef SBK_OUTPUTFILEPARAMWRAPPER_H
#define SBK_OUTPUTFILEPARAMWRAPPER_H

#include <shiboken.h>

#include <PyParameter.h>

NATRON_NAMESPACE_ENTER;
class OutputFileParamWrapper : public OutputFileParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { OutputFileParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~OutputFileParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_OUTPUTFILEPARAMWRAPPER_H

