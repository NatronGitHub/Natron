#ifndef SBK_PAGEPARAMWRAPPER_H
#define SBK_PAGEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class PageParamWrapper : public PageParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param) { PageParam::_addAsDependencyOf(fromExprDimension, param); }
    virtual ~PageParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_PAGEPARAMWRAPPER_H

