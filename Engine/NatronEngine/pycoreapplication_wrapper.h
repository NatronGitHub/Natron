#ifndef SBK_PYCOREAPPLICATIONWRAPPER_H
#define SBK_PYCOREAPPLICATIONWRAPPER_H

#include <shiboken.h>

#include <GlobalFunctionsWrapper.h>

NATRON_NAMESPACE_ENTER;
class PyCoreApplicationWrapper : public PyCoreApplication
{
public:
    PyCoreApplicationWrapper();
    virtual ~PyCoreApplicationWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_PYCOREAPPLICATIONWRAPPER_H

