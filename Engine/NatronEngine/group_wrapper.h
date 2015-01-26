#ifndef SBK_GROUPWRAPPER_H
#define SBK_GROUPWRAPPER_H

#define protected public

#include <shiboken.h>

#include <NodeGroupWrapper.h>

class GroupWrapper : public Group
{
public:
    GroupWrapper();
    virtual ~GroupWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_GROUPWRAPPER_H

