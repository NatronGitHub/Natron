#ifndef SBK_PYTRANSFORMOVERLAYINTERACTWRAPPER_H
#define SBK_PYTRANSFORMOVERLAYINTERACTWRAPPER_H

#include <shiboken.h>

#include <PyOverlayInteract.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyTransformOverlayInteractWrapper : public PyTransformOverlayInteract
{
public:
    PyTransformOverlayInteractWrapper();
    virtual std::map<QString, PyOverlayParamDesc > describeParameters() const;
    virtual void draw(double time, const Double2DTuple & renderScale, QString view);
    inline Param * fetchParameter_protected(const std::map<QString, QString > & params, const QString & role, const QString & type, int nDims, bool optional) const { return PyTransformOverlayInteract::fetchParameter(params, role, type, nDims, optional); }
    virtual void fetchParameters(const std::map<QString, QString > & params);
    inline void initInternalInteract_protected() { PyTransformOverlayInteract::initInternalInteract(); }
    virtual ~PyTransformOverlayInteractWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_PYTRANSFORMOVERLAYINTERACTWRAPPER_H

