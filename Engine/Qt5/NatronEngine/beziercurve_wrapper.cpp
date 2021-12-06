
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <pysidesignal.h>
#include <shiboken.h> // produces many warnings
#ifndef QT_NO_VERSION_TAGGING
#  define QT_NO_VERSION_TAGGING
#endif
#include <QDebug>
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <pysideqenum.h>
#include <feature_select.h>
#include <qapp_macro.h>

QT_WARNING_DISABLE_DEPRECATED

#include <typeinfo>
#include <iterator>

// module include
#include "natronengine_python.h"

// main header
#include "beziercurve_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

#include <cctype>
#include <cstring>



template <class T>
static const char *typeNameOf(const T &t)
{
    const char *typeName =  typeid(t).name();
    auto size = std::strlen(typeName);
#if defined(Q_CC_MSVC) // MSVC: "class QPaintDevice * __ptr64"
    if (auto lastStar = strchr(typeName, '*')) {
        // MSVC: "class QPaintDevice * __ptr64"
        while (*--lastStar == ' ') {
        }
        size = lastStar - typeName + 1;
    }
#else // g++, Clang: "QPaintDevice *" -> "P12QPaintDevice"
    if (size > 2 && typeName[0] == 'P' && std::isdigit(typeName[1])) {
        ++typeName;
        --size;
    }
#endif
    char *result = new char[size + 1];
    result[size] = '\0';
    memcpy(result, typeName, size);
    return result;
}

// Native ---------------------------------------------------------

void BezierCurveWrapper::pysideInitQtMetaTypes()
{
}

void BezierCurveWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

BezierCurveWrapper::~BezierCurveWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_BezierCurveFunc_addControlPoint(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addControlPoint", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::addControlPoint(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // addControlPoint(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_addControlPoint_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // addControlPoint(double,double)
            cppSelf->addControlPoint(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_addControlPoint_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.addControlPoint");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_addControlPointOnSegment(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addControlPointOnSegment", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::addControlPointOnSegment(int,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // addControlPointOnSegment(int,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_addControlPointOnSegment_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // addControlPointOnSegment(int,double)
            cppSelf->addControlPointOnSegment(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_addControlPointOnSegment_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.addControlPointOnSegment");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getActivatedParam(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getActivatedParam()const
            BooleanParam * cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getActivatedParam();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getColor(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::getColor(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getColor(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getColor_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getColor(double)
            ColorTuple* cppResult = new ColorTuple(cppSelf->getColor(cppArg0));
            pyResult = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX]), cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BezierCurveFunc_getColor_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getColor");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getColorParam(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getColorParam()const
            ColorParam * cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getColorParam();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_COLORPARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getCompositingOperator(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCompositingOperator()const
            NATRON_ENUM::MergingFunctionEnum cppResult = NATRON_ENUM::MergingFunctionEnum(const_cast<const ::BezierCurve *>(cppSelf)->getCompositingOperator());
            pyResult = Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX])->converter, &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getCompositingOperatorParam(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCompositingOperatorParam()const
            ChoiceParam * cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getCompositingOperatorParam();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getControlPointPosition(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getControlPointPosition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getControlPointPosition_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
            // Begin code injection
            double x,y,rx,ry,lx,ly;
            cppSelf->getControlPointPosition(cppArg0, cppArg1, &x,&y, &lx,&ly,&rx,&ry);

            PyObject* ret = PyTuple_New(6);
            PyTuple_SET_ITEM(ret, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &x));
            PyTuple_SET_ITEM(ret, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &y));
            PyTuple_SET_ITEM(ret, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &lx));
            PyTuple_SET_ITEM(ret, 3, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ly));
            PyTuple_SET_ITEM(ret, 4, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &rx));
            PyTuple_SET_ITEM(ret, 5, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ry));
            return ret;

            // End of code injection

            pyResult = Py_None;
            Py_INCREF(Py_None);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BezierCurveFunc_getControlPointPosition_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getControlPointPosition");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getFeatherDistance(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::getFeatherDistance(double)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getFeatherDistance(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getFeatherDistance_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getFeatherDistance(double)const
            double cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getFeatherDistance(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BezierCurveFunc_getFeatherDistance_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getFeatherDistance");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getFeatherDistanceParam(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getFeatherDistanceParam()const
            DoubleParam * cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getFeatherDistanceParam();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getFeatherFallOff(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::getFeatherFallOff(double)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getFeatherFallOff(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getFeatherFallOff_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getFeatherFallOff(double)const
            double cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getFeatherFallOff(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BezierCurveFunc_getFeatherFallOff_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getFeatherFallOff");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getFeatherFallOffParam(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getFeatherFallOffParam()const
            DoubleParam * cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getFeatherFallOffParam();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getFeatherPointPosition(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getFeatherPointPosition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getFeatherPointPosition_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
            // Begin code injection
            double x,y,rx,ry,lx,ly;
            cppSelf->getFeatherPointPosition(cppArg0, cppArg1, &x,&y, &lx,&ly,&rx,&ry);

            PyObject* ret = PyTuple_New(6);
            PyTuple_SET_ITEM(ret, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &x));
            PyTuple_SET_ITEM(ret, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &y));
            PyTuple_SET_ITEM(ret, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &lx));
            PyTuple_SET_ITEM(ret, 3, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ly));
            PyTuple_SET_ITEM(ret, 4, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &rx));
            PyTuple_SET_ITEM(ret, 5, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ry));
            return ret;

            // End of code injection

            pyResult = Py_None;
            Py_INCREF(Py_None);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BezierCurveFunc_getFeatherPointPosition_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getFeatherPointPosition");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getIsActivated(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::getIsActivated(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getIsActivated(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getIsActivated_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getIsActivated(double)
            bool cppResult = cppSelf->getIsActivated(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BezierCurveFunc_getIsActivated_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getIsActivated");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getKeyframes(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getKeyframes(std::list<double>*)const
            // Begin code injection
            std::list<double> keys;
            cppSelf->getKeyframes(&keys);
            PyObject* ret = PyList_New((int) keys.size());
            int idx = 0;
            for (std::list<double>::iterator it = keys.begin(); it!=keys.end(); ++it,++idx) {
            PyList_SET_ITEM(ret, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &*it));
            }
            return ret;

            // End of code injection

            pyResult = Py_None;
            Py_INCREF(Py_None);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getNumControlPoints(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumControlPoints()const
            int cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getNumControlPoints();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getOpacity(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::getOpacity(double)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getOpacity(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getOpacity_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getOpacity(double)const
            double cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getOpacity(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BezierCurveFunc_getOpacity_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getOpacity");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_getOpacityParam(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getOpacityParam()const
            DoubleParam * cppResult = const_cast<const ::BezierCurve *>(cppSelf)->getOpacityParam();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_getOverlayColor(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getOverlayColor()const
            ColorTuple* cppResult = new ColorTuple(const_cast<const ::BezierCurve *>(cppSelf)->getOverlayColor());
            pyResult = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX]), cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_isCurveFinished(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isCurveFinished()const
            bool cppResult = const_cast<const ::BezierCurve *>(cppSelf)->isCurveFinished();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BezierCurveFunc_moveFeatherByIndex(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "moveFeatherByIndex", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::moveFeatherByIndex(int,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 0; // moveFeatherByIndex(int,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_moveFeatherByIndex_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // moveFeatherByIndex(int,double,double,double)
            cppSelf->moveFeatherByIndex(cppArg0, cppArg1, cppArg2, cppArg3);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveFeatherByIndex_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveFeatherByIndex");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_moveLeftBezierPoint(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "moveLeftBezierPoint", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::moveLeftBezierPoint(int,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 0; // moveLeftBezierPoint(int,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_moveLeftBezierPoint_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // moveLeftBezierPoint(int,double,double,double)
            cppSelf->moveLeftBezierPoint(cppArg0, cppArg1, cppArg2, cppArg3);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveLeftBezierPoint_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveLeftBezierPoint");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_movePointByIndex(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "movePointByIndex", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::movePointByIndex(int,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 0; // movePointByIndex(int,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_movePointByIndex_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // movePointByIndex(int,double,double,double)
            cppSelf->movePointByIndex(cppArg0, cppArg1, cppArg2, cppArg3);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_movePointByIndex_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.movePointByIndex");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_moveRightBezierPoint(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "moveRightBezierPoint", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::moveRightBezierPoint(int,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 0; // moveRightBezierPoint(int,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_moveRightBezierPoint_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // moveRightBezierPoint(int,double,double,double)
            cppSelf->moveRightBezierPoint(cppArg0, cppArg1, cppArg2, cppArg3);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveRightBezierPoint_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveRightBezierPoint");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_removeControlPointByIndex(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::removeControlPointByIndex(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // removeControlPointByIndex(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_removeControlPointByIndex_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // removeControlPointByIndex(int)
            cppSelf->removeControlPointByIndex(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_removeControlPointByIndex_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.removeControlPointByIndex");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setActivated(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setActivated", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setActivated(double,bool)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[1])))) {
        overloadId = 0; // setActivated(double,bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setActivated_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        bool cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setActivated(double,bool)
            cppSelf->setActivated(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setActivated_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setActivated");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setColor(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setColor", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setColor(double,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 0; // setColor(double,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setColor_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // setColor(double,double,double,double)
            cppSelf->setColor(cppArg0, cppArg1, cppArg2, cppArg3);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setColor_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setColor");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setCompositingOperator(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::setCompositingOperator(NATRON_ENUM::MergingFunctionEnum)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(*PepType_SGTP(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX])->converter, (pyArg)))) {
        overloadId = 0; // setCompositingOperator(NATRON_ENUM::MergingFunctionEnum)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setCompositingOperator_TypeError;

    // Call function/method
    {
        ::NATRON_ENUM::MergingFunctionEnum cppArg0{NATRON_ENUM::eMergeATop};
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setCompositingOperator(NATRON_ENUM::MergingFunctionEnum)
            cppSelf->setCompositingOperator(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setCompositingOperator_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.setCompositingOperator");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setCurveFinished(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BezierCurve::setCurveFinished(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setCurveFinished(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setCurveFinished_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setCurveFinished(bool)
            cppSelf->setCurveFinished(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setCurveFinished_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.setCurveFinished");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setFeatherDistance(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setFeatherDistance", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setFeatherDistance(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setFeatherDistance(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setFeatherDistance_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setFeatherDistance(double,double)
            cppSelf->setFeatherDistance(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setFeatherDistance_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setFeatherDistance");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setFeatherFallOff(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setFeatherFallOff", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setFeatherFallOff(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setFeatherFallOff(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setFeatherFallOff_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setFeatherFallOff(double,double)
            cppSelf->setFeatherFallOff(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setFeatherFallOff_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setFeatherFallOff");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setFeatherPointAtIndex(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setFeatherPointAtIndex", 8, 8, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4]), &(pyArgs[5]), &(pyArgs[6]), &(pyArgs[7])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setFeatherPointAtIndex(int,double,double,double,double,double,double,double)
    if (numArgs == 8
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[4])))
        && (pythonToCpp[5] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[5])))
        && (pythonToCpp[6] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[6])))
        && (pythonToCpp[7] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[7])))) {
        overloadId = 0; // setFeatherPointAtIndex(int,double,double,double,double,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setFeatherPointAtIndex_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        double cppArg4;
        pythonToCpp[4](pyArgs[4], &cppArg4);
        double cppArg5;
        pythonToCpp[5](pyArgs[5], &cppArg5);
        double cppArg6;
        pythonToCpp[6](pyArgs[6], &cppArg6);
        double cppArg7;
        pythonToCpp[7](pyArgs[7], &cppArg7);

        if (!PyErr_Occurred()) {
            // setFeatherPointAtIndex(int,double,double,double,double,double,double,double)
            cppSelf->setFeatherPointAtIndex(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4, cppArg5, cppArg6, cppArg7);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setFeatherPointAtIndex_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setFeatherPointAtIndex");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setOpacity(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setOpacity", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setOpacity(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setOpacity(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setOpacity_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setOpacity(double,double)
            cppSelf->setOpacity(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setOpacity_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setOpacity");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setOverlayColor(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setOverlayColor", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setOverlayColor(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // setOverlayColor(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setOverlayColor_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setOverlayColor(double,double,double)
            cppSelf->setOverlayColor(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setOverlayColor_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setOverlayColor");
        return {};
}

static PyObject *Sbk_BezierCurveFunc_setPointAtIndex(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setPointAtIndex", 8, 8, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4]), &(pyArgs[5]), &(pyArgs[6]), &(pyArgs[7])))
        return {};


    // Overloaded function decisor
    // 0: BezierCurve::setPointAtIndex(int,double,double,double,double,double,double,double)
    if (numArgs == 8
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[4])))
        && (pythonToCpp[5] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[5])))
        && (pythonToCpp[6] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[6])))
        && (pythonToCpp[7] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[7])))) {
        overloadId = 0; // setPointAtIndex(int,double,double,double,double,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setPointAtIndex_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        double cppArg4;
        pythonToCpp[4](pyArgs[4], &cppArg4);
        double cppArg5;
        pythonToCpp[5](pyArgs[5], &cppArg5);
        double cppArg6;
        pythonToCpp[6](pyArgs[6], &cppArg6);
        double cppArg7;
        pythonToCpp[7](pyArgs[7], &cppArg7);

        if (!PyErr_Occurred()) {
            // setPointAtIndex(int,double,double,double,double,double,double,double)
            cppSelf->setPointAtIndex(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4, cppArg5, cppArg6, cppArg7);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setPointAtIndex_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setPointAtIndex");
        return {};
}


static const char *Sbk_BezierCurve_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_BezierCurve_methods[] = {
    {"addControlPoint", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_addControlPoint), METH_VARARGS},
    {"addControlPointOnSegment", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_addControlPointOnSegment), METH_VARARGS},
    {"getActivatedParam", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getActivatedParam), METH_NOARGS},
    {"getColor", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getColor), METH_O},
    {"getColorParam", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getColorParam), METH_NOARGS},
    {"getCompositingOperator", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getCompositingOperator), METH_NOARGS},
    {"getCompositingOperatorParam", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getCompositingOperatorParam), METH_NOARGS},
    {"getControlPointPosition", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getControlPointPosition), METH_VARARGS},
    {"getFeatherDistance", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getFeatherDistance), METH_O},
    {"getFeatherDistanceParam", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getFeatherDistanceParam), METH_NOARGS},
    {"getFeatherFallOff", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getFeatherFallOff), METH_O},
    {"getFeatherFallOffParam", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getFeatherFallOffParam), METH_NOARGS},
    {"getFeatherPointPosition", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getFeatherPointPosition), METH_VARARGS},
    {"getIsActivated", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getIsActivated), METH_O},
    {"getKeyframes", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getKeyframes), METH_NOARGS},
    {"getNumControlPoints", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getNumControlPoints), METH_NOARGS},
    {"getOpacity", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getOpacity), METH_O},
    {"getOpacityParam", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getOpacityParam), METH_NOARGS},
    {"getOverlayColor", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_getOverlayColor), METH_NOARGS},
    {"isCurveFinished", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_isCurveFinished), METH_NOARGS},
    {"moveFeatherByIndex", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_moveFeatherByIndex), METH_VARARGS},
    {"moveLeftBezierPoint", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_moveLeftBezierPoint), METH_VARARGS},
    {"movePointByIndex", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_movePointByIndex), METH_VARARGS},
    {"moveRightBezierPoint", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_moveRightBezierPoint), METH_VARARGS},
    {"removeControlPointByIndex", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_removeControlPointByIndex), METH_O},
    {"setActivated", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setActivated), METH_VARARGS},
    {"setColor", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setColor), METH_VARARGS},
    {"setCompositingOperator", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setCompositingOperator), METH_O},
    {"setCurveFinished", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setCurveFinished), METH_O},
    {"setFeatherDistance", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setFeatherDistance), METH_VARARGS},
    {"setFeatherFallOff", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setFeatherFallOff), METH_VARARGS},
    {"setFeatherPointAtIndex", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setFeatherPointAtIndex), METH_VARARGS},
    {"setOpacity", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setOpacity), METH_VARARGS},
    {"setOverlayColor", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setOverlayColor), METH_VARARGS},
    {"setPointAtIndex", reinterpret_cast<PyCFunction>(Sbk_BezierCurveFunc_setPointAtIndex), METH_VARARGS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_BezierCurve_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::BezierCurve *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<BezierCurveWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_BezierCurve_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_BezierCurve_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_BezierCurve_Type = nullptr;
static SbkObjectType *Sbk_BezierCurve_TypeF(void)
{
    return _Sbk_BezierCurve_Type;
}

static PyType_Slot Sbk_BezierCurve_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_BezierCurve_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_BezierCurve_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_BezierCurve_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_BezierCurve_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_BezierCurve_spec = {
    "1:NatronEngine.BezierCurve",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_BezierCurve_slots
};

} //extern "C"

static void *Sbk_BezierCurve_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::ItemBase >()))
        return dynamic_cast< ::BezierCurve *>(reinterpret_cast< ::ItemBase *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void BezierCurve_PythonToCpp_BezierCurve_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_BezierCurve_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_BezierCurve_PythonToCpp_BezierCurve_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_BezierCurve_TypeF())))
        return BezierCurve_PythonToCpp_BezierCurve_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *BezierCurve_PTR_CppToPython_BezierCurve(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::BezierCurve *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_BezierCurve_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *BezierCurve_SignatureStrings[] = {
    "NatronEngine.BezierCurve.addControlPoint(self,x:double,y:double)",
    "NatronEngine.BezierCurve.addControlPointOnSegment(self,index:int,t:double)",
    "NatronEngine.BezierCurve.getActivatedParam(self)->NatronEngine.BooleanParam",
    "NatronEngine.BezierCurve.getColor(self,time:double)->NatronEngine.ColorTuple",
    "NatronEngine.BezierCurve.getColorParam(self)->NatronEngine.ColorParam",
    "NatronEngine.BezierCurve.getCompositingOperator(self)->NatronEngine.NATRON_ENUM.MergingFunctionEnum",
    "NatronEngine.BezierCurve.getCompositingOperatorParam(self)->NatronEngine.ChoiceParam",
    "NatronEngine.BezierCurve.getControlPointPosition(self,index:int,time:double,x:double*,y:double*,lx:double*,ly:double*,rx:double*,ry:double*)",
    "NatronEngine.BezierCurve.getFeatherDistance(self,time:double)->double",
    "NatronEngine.BezierCurve.getFeatherDistanceParam(self)->NatronEngine.DoubleParam",
    "NatronEngine.BezierCurve.getFeatherFallOff(self,time:double)->double",
    "NatronEngine.BezierCurve.getFeatherFallOffParam(self)->NatronEngine.DoubleParam",
    "NatronEngine.BezierCurve.getFeatherPointPosition(self,index:int,time:double,x:double*,y:double*,lx:double*,ly:double*,rx:double*,ry:double*)",
    "NatronEngine.BezierCurve.getIsActivated(self,time:double)->bool",
    "NatronEngine.BezierCurve.getKeyframes(self,keys:std.list[double])",
    "NatronEngine.BezierCurve.getNumControlPoints(self)->int",
    "NatronEngine.BezierCurve.getOpacity(self,time:double)->double",
    "NatronEngine.BezierCurve.getOpacityParam(self)->NatronEngine.DoubleParam",
    "NatronEngine.BezierCurve.getOverlayColor(self)->NatronEngine.ColorTuple",
    "NatronEngine.BezierCurve.isCurveFinished(self)->bool",
    "NatronEngine.BezierCurve.moveFeatherByIndex(self,index:int,time:double,dx:double,dy:double)",
    "NatronEngine.BezierCurve.moveLeftBezierPoint(self,index:int,time:double,dx:double,dy:double)",
    "NatronEngine.BezierCurve.movePointByIndex(self,index:int,time:double,dx:double,dy:double)",
    "NatronEngine.BezierCurve.moveRightBezierPoint(self,index:int,time:double,dx:double,dy:double)",
    "NatronEngine.BezierCurve.removeControlPointByIndex(self,index:int)",
    "NatronEngine.BezierCurve.setActivated(self,time:double,activated:bool)",
    "NatronEngine.BezierCurve.setColor(self,time:double,r:double,g:double,b:double)",
    "NatronEngine.BezierCurve.setCompositingOperator(self,op:NatronEngine.NATRON_ENUM.MergingFunctionEnum)",
    "NatronEngine.BezierCurve.setCurveFinished(self,finished:bool)",
    "NatronEngine.BezierCurve.setFeatherDistance(self,dist:double,time:double)",
    "NatronEngine.BezierCurve.setFeatherFallOff(self,falloff:double,time:double)",
    "NatronEngine.BezierCurve.setFeatherPointAtIndex(self,index:int,time:double,x:double,y:double,lx:double,ly:double,rx:double,ry:double)",
    "NatronEngine.BezierCurve.setOpacity(self,opacity:double,time:double)",
    "NatronEngine.BezierCurve.setOverlayColor(self,r:double,g:double,b:double)",
    "NatronEngine.BezierCurve.setPointAtIndex(self,index:int,time:double,x:double,y:double,lx:double,ly:double,rx:double,ry:double)",
    nullptr}; // Sentinel

void init_BezierCurve(PyObject *module)
{
    _Sbk_BezierCurve_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "BezierCurve",
        "BezierCurve*",
        &Sbk_BezierCurve_spec,
        &Shiboken::callCppDestructor< ::BezierCurve >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_BezierCurve_Type);
    InitSignatureStrings(pyType, BezierCurve_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_BezierCurve_Type), Sbk_BezierCurve_PropertyStrings);
    SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_BezierCurve_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_BezierCurve_TypeF(),
        BezierCurve_PythonToCpp_BezierCurve_PTR,
        is_BezierCurve_PythonToCpp_BezierCurve_PTR_Convertible,
        BezierCurve_PTR_CppToPython_BezierCurve);

    Shiboken::Conversions::registerConverterName(converter, "BezierCurve");
    Shiboken::Conversions::registerConverterName(converter, "BezierCurve*");
    Shiboken::Conversions::registerConverterName(converter, "BezierCurve&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::BezierCurve).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::BezierCurveWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_BezierCurve_TypeF(), &Sbk_BezierCurve_typeDiscovery);


    BezierCurveWrapper::pysideInitQtMetaTypes();
}
