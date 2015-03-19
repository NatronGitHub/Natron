#ifndef SBK_USERPARAMHOLDERWRAPPER_H
#define SBK_USERPARAMHOLDERWRAPPER_H

#include <shiboken.h>

#include <NodeWrapper.h>

class UserParamHolderWrapper : public UserParamHolder
{
public:
    UserParamHolderWrapper();
    virtual ~UserParamHolderWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_USERPARAMHOLDERWRAPPER_H

