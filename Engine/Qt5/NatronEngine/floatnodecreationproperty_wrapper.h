#ifndef SBK_FLOATNODECREATIONPROPERTYWRAPPER_H
#define SBK_FLOATNODECREATIONPROPERTYWRAPPER_H

#include <PyAppInstance.h>


// Extra includes
#include <vector>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class FloatNodeCreationPropertyWrapper : public FloatNodeCreationProperty
{
public:
    FloatNodeCreationPropertyWrapper(const std::vector<double > & values = std::vector< double >());
    FloatNodeCreationPropertyWrapper(double value);
    ~FloatNodeCreationPropertyWrapper();
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

#endif // SBK_FLOATNODECREATIONPROPERTYWRAPPER_H

