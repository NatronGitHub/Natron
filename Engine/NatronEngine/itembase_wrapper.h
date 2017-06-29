#ifndef SBK_ITEMBASEWRAPPER_H
#define SBK_ITEMBASEWRAPPER_H

#include <shiboken.h>

#include <PyItemsTable.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class ItemBaseWrapper : public ItemBase
{
public:
    virtual ~ItemBaseWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_ITEMBASEWRAPPER_H

