#ifndef SBK_STRINGNODECREATIONPROPERTYWRAPPER_H
#define SBK_STRINGNODECREATIONPROPERTYWRAPPER_H

#include <shiboken.h>

#include <PyAppInstance.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class StringNodeCreationPropertyWrapper : public StringNodeCreationProperty
{
public:
    StringNodeCreationPropertyWrapper(const std::string & value);
    StringNodeCreationPropertyWrapper(const std::vector<std::string > & values = std::vector< std::string >());
    virtual ~StringNodeCreationPropertyWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_STRINGNODECREATIONPROPERTYWRAPPER_H

