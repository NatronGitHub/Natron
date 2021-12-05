#ifndef SBK_GROUPWRAPPER_H
#define SBK_GROUPWRAPPER_H

#include <PyNodeGroup.h>


// Extra includes
#include <PyNode.h>
#include <list>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class GroupWrapper : public Group
{
public:
    GroupWrapper();
    ~GroupWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_GROUPWRAPPER_H

