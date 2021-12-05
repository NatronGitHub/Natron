#ifndef SBK_PYCOREAPPLICATIONWRAPPER_H
#define SBK_PYCOREAPPLICATIONWRAPPER_H

#include <shiboken.h>

#include <PyGlobalFunctions.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyCoreApplicationWrapper : public PyCoreApplication
{
public:
    PyCoreApplicationWrapper();
    virtual ~PyCoreApplicationWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_PYCOREAPPLICATIONWRAPPER_H

