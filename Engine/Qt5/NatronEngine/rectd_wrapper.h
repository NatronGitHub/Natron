#ifndef SBK_RECTDWRAPPER_H
#define SBK_RECTDWRAPPER_H

#include <RectD.h>


// Extra includes
#include <RectD.h>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class RectDWrapper : public RectD
{
public:
    RectDWrapper();
    RectDWrapper(const RectD& self) : RectD(self)
    {
    }

    RectDWrapper(double l, double b, double r, double t);
    ~RectDWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_RECTDWRAPPER_H

