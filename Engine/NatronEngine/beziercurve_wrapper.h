#ifndef SBK_BEZIERCURVEWRAPPER_H
#define SBK_BEZIERCURVEWRAPPER_H

#include <shiboken.h>

#include <RotoWrapper.h>

NATRON_NAMESPACE_ENTER;
class BezierCurveWrapper : public BezierCurve
{
public:
    virtual ~BezierCurveWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_BEZIERCURVEWRAPPER_H

