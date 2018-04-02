#ifndef SBK_BOOLNODECREATIONPROPERTYWRAPPER_H
#define SBK_BOOLNODECREATIONPROPERTYWRAPPER_H

#include <shiboken.h>

#include <PyAppInstance.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class BoolNodeCreationPropertyWrapper : public BoolNodeCreationProperty
{
public:
    BoolNodeCreationPropertyWrapper(bool value);
    BoolNodeCreationPropertyWrapper(const std::vector<bool > & values = std::vector<bool >());
    virtual ~BoolNodeCreationPropertyWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_BOOLNODECREATIONPROPERTYWRAPPER_H

