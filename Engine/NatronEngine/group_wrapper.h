#ifndef SBK_GROUPWRAPPER_H
#define SBK_GROUPWRAPPER_H

#include <shiboken.h>

#include <NodeGroupWrapper.h>

NATRON_NAMESPACE_ENTER;
class GroupWrapper : public Group
{
public:
    GroupWrapper();
    virtual ~GroupWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_GROUPWRAPPER_H

