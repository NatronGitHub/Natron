#ifndef SBK_ITEMBASEWRAPPER_H
#define SBK_ITEMBASEWRAPPER_H

#include <shiboken.h>

#include <PyRoto.h>

NATRON_NAMESPACE_ENTER;
class ItemBaseWrapper : public ItemBase
{
public:
    virtual ~ItemBaseWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_ITEMBASEWRAPPER_H

