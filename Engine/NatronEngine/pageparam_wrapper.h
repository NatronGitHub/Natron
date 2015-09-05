#ifndef SBK_PAGEPARAMWRAPPER_H
#define SBK_PAGEPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class PageParamWrapper : public PageParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param, int thisDimension) { PageParam::_addAsDependencyOf(fromExprDimension, param, thisDimension); }
    virtual ~PageParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_PAGEPARAMWRAPPER_H

