#ifndef SBK_STROKEITEMWRAPPER_H
#define SBK_STROKEITEMWRAPPER_H

#include <shiboken.h>

#include <PyRoto.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class StrokeItemWrapper : public StrokeItem
{
public:
    virtual ~StrokeItemWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_STROKEITEMWRAPPER_H

