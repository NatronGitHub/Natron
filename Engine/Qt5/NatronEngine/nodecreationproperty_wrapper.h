#ifndef SBK_NODECREATIONPROPERTYWRAPPER_H
#define SBK_NODECREATIONPROPERTYWRAPPER_H

#include <PyAppInstance.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class NodeCreationPropertyWrapper : public NodeCreationProperty
{
public:
    NodeCreationPropertyWrapper();
    ~NodeCreationPropertyWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_NODECREATIONPROPERTYWRAPPER_H

