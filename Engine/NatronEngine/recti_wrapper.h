#ifndef SBK_RECTIWRAPPER_H
#define SBK_RECTIWRAPPER_H

#include <shiboken.h>

#include <RectI.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class RectIWrapper : public RectI
{
public:
    RectIWrapper();
    RectIWrapper(const RectI& self) : RectI(self)
    {
    }

    RectIWrapper(int l, int b, int r, int t);
    virtual ~RectIWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_RECTIWRAPPER_H

