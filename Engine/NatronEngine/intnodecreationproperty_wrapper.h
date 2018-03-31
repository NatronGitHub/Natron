#ifndef SBK_INTNODECREATIONPROPERTYWRAPPER_H
#define SBK_INTNODECREATIONPROPERTYWRAPPER_H

#include <shiboken.h>

#include <PyAppInstance.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class IntNodeCreationPropertyWrapper : public IntNodeCreationProperty
{
public:
    IntNodeCreationPropertyWrapper(const std::vector<int > & values = std::vector<int >());
    IntNodeCreationPropertyWrapper(int value);
    virtual ~IntNodeCreationPropertyWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_INTNODECREATIONPROPERTYWRAPPER_H

