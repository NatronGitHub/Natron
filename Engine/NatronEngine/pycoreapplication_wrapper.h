#ifndef SBK_PYCOREAPPLICATIONWRAPPER_H
#define SBK_PYCOREAPPLICATIONWRAPPER_H

#define protected public

#include <shiboken.h>

#include <GlobalFunctionsWrapper.h>

class PyCoreApplicationWrapper : public PyCoreApplication
{
public:
    PyCoreApplicationWrapper();
    virtual ~PyCoreApplicationWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_PYCOREAPPLICATIONWRAPPER_H

