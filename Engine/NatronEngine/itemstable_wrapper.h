#ifndef SBK_ITEMSTABLEWRAPPER_H
#define SBK_ITEMSTABLEWRAPPER_H

#include <shiboken.h>

#include <PyItemsTable.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class ItemsTableWrapper : public ItemsTable
{
public:
    virtual ~ItemsTableWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_ITEMSTABLEWRAPPER_H

