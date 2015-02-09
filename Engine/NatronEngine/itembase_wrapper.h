#ifndef SBK_ITEMBASEWRAPPER_H
#define SBK_ITEMBASEWRAPPER_H

#define protected public

#include <shiboken.h>

#include <RotoWrapper.h>

class ItemBaseWrapper : public ItemBase
{
public:
    virtual ~ItemBaseWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_ITEMBASEWRAPPER_H

