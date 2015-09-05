#ifndef SBK_FILEPARAMWRAPPER_H
#define SBK_FILEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class FileParamWrapper : public FileParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { FileParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~FileParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_FILEPARAMWRAPPER_H

