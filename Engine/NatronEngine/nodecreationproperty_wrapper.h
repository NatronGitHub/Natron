#ifndef SBK_NODECREATIONPROPERTYWRAPPER_H
#define SBK_NODECREATIONPROPERTYWRAPPER_H

#include <shiboken.h>

#include <PyAppInstance.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class NodeCreationPropertyWrapper : public NodeCreationProperty
{
public:
    NodeCreationPropertyWrapper();
    virtual ~NodeCreationPropertyWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_NODECREATIONPROPERTYWRAPPER_H

