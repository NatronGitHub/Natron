#ifndef SBK_FILEPARAMWRAPPER_H
#define SBK_FILEPARAMWRAPPER_H

#include <shiboken.h>

#include <PyParameter.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class FileParamWrapper : public FileParam
{
public:
    inline void _addAsDependencyOf_protected(Param * param, int fromExprDimension, int thisDimension, const QString & fromExprView, const QString & thisView) { FileParam::_addAsDependencyOf(param, fromExprDimension, thisDimension, fromExprView, thisView); }
    virtual ~FileParamWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_FILEPARAMWRAPPER_H

