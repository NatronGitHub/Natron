#ifndef SBK_LAYERWRAPPER_H
#define SBK_LAYERWRAPPER_H

#include <PyRoto.h>


// Extra includes
#include <PyRoto.h>
#include <list>
#include <PyParameter.h>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class LayerWrapper : public Layer
{
public:
    ~LayerWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  ifndef SBK_ITEMBASEWRAPPER_H
#  define SBK_ITEMBASEWRAPPER_H

// Inherited base class:
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class ItemBaseWrapper : public ItemBase
{
public:
    ~ItemBaseWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  endif // SBK_ITEMBASEWRAPPER_H

#endif // SBK_LAYERWRAPPER_H

