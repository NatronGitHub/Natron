#ifndef SBK_BEZIERCURVEWRAPPER_H
#define SBK_BEZIERCURVEWRAPPER_H

#define protected public

#include <shiboken.h>

#include <RotoWrapper.h>

class BezierCurveWrapper : public BezierCurve
{
public:
    virtual ~BezierCurveWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_BEZIERCURVEWRAPPER_H

