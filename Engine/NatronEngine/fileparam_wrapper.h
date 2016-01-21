#ifndef SBK_FILEPARAMWRAPPER_H
#define SBK_FILEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

NATRON_NAMESPACE_ENTER;
class FileParamWrapper : public FileParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { FileParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~FileParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_FILEPARAMWRAPPER_H

