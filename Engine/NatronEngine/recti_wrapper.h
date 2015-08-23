#ifndef SBK_RECTIWRAPPER_H
#define SBK_RECTIWRAPPER_H

#include <shiboken.h>

#include <RectI.h>

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

#endif // SBK_RECTIWRAPPER_H

