#ifndef SBK_FLOATNODECREATIONPROPERTYWRAPPER_H
#define SBK_FLOATNODECREATIONPROPERTYWRAPPER_H

#include <shiboken.h>

#include <PyAppInstance.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class FloatNodeCreationPropertyWrapper : public FloatNodeCreationProperty
{
public:
    FloatNodeCreationPropertyWrapper(const std::vector<double > & values = std::vector<double >());
    FloatNodeCreationPropertyWrapper(double value);
    virtual ~FloatNodeCreationPropertyWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_FLOATNODECREATIONPROPERTYWRAPPER_H

