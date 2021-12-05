#ifndef SBK_USERPARAMHOLDERWRAPPER_H
#define SBK_USERPARAMHOLDERWRAPPER_H

#include <shiboken.h>

#include <PyNode.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class UserParamHolderWrapper : public UserParamHolder
{
public:
    UserParamHolderWrapper();
    virtual ~UserParamHolderWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_USERPARAMHOLDERWRAPPER_H

