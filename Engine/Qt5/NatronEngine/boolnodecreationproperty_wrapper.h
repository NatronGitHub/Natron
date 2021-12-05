#ifndef SBK_BOOLNODECREATIONPROPERTYWRAPPER_H
#define SBK_BOOLNODECREATIONPROPERTYWRAPPER_H

#include <PyAppInstance.h>


// Extra includes
#include <vector>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class BoolNodeCreationPropertyWrapper : public BoolNodeCreationProperty
{
public:
    BoolNodeCreationPropertyWrapper(bool value);
    BoolNodeCreationPropertyWrapper(const std::vector<bool > & values = std::vector< bool >());
    ~BoolNodeCreationPropertyWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  ifndef SBK_NODECREATIONPROPERTYWRAPPER_H
#  define SBK_NODECREATIONPROPERTYWRAPPER_H

// Inherited base class:
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

#  endif // SBK_NODECREATIONPROPERTYWRAPPER_H

#endif // SBK_BOOLNODECREATIONPROPERTYWRAPPER_H

