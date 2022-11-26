
// default includes
#include "Global/Macros.h"
// clang-format off
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format on
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "beziercurve_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyParameter.h>
#include <PyRoto.h>
#include <list>


// Native ---------------------------------------------------------

void BezierCurveWrapper::pysideInitQtMetaTypes()
{
}

BezierCurveWrapper::~BezierCurveWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_BezierCurveFunc_addControlPoint(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addControlPoint", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: addControlPoint(double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_addControlPoint_TypeError:
        const char* overloads[] = {"float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.addControlPoint", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_addControlPointOnSegment(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addControlPointOnSegment", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: addControlPointOnSegment(int,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_addControlPointOnSegment_TypeError:
        const char* overloads[] = {"int, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.addControlPointOnSegment", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getActivatedParam(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getActivatedParam()const
            BooleanParam * cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getActivatedParam();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getColor(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getColor(double)
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
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getColor_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getColor", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getColorParam(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getColorParam()const
            ColorParam * cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getColorParam();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getCompositingOperator(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCompositingOperator()const
            NATRON_NAMESPACE::MergingFunctionEnum cppResult = NATRON_NAMESPACE::MergingFunctionEnum(const_cast<const ::BezierCurve*>(cppSelf)->getCompositingOperator());
            pyResult = Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_MERGINGFUNCTIONENUM_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getCompositingOperatorParam(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCompositingOperatorParam()const
            ChoiceParam * cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getCompositingOperatorParam();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getControlPointPosition(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getControlPointPosition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
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
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getControlPointPosition_TypeError:
        const char* overloads[] = {"int, float, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getControlPointPosition", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getFeatherDistance(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getFeatherDistance(double)const
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
            double cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getFeatherDistance(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getFeatherDistance_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getFeatherDistance", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getFeatherDistanceParam(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getFeatherDistanceParam()const
            DoubleParam * cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getFeatherDistanceParam();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getFeatherFallOff(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getFeatherFallOff(double)const
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
            double cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getFeatherFallOff(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getFeatherFallOff_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getFeatherFallOff", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getFeatherFallOffParam(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getFeatherFallOffParam()const
            DoubleParam * cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getFeatherFallOffParam();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getFeatherPointPosition(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getFeatherPointPosition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*)const
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
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getFeatherPointPosition_TypeError:
        const char* overloads[] = {"int, float, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getFeatherPointPosition", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getIsActivated(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getIsActivated(double)
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
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getIsActivated_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getIsActivated", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getKeyframes(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

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
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getNumControlPoints(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumControlPoints()const
            int cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getNumControlPoints();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getOpacity(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getOpacity(double)const
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
            double cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getOpacity(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getOpacity_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.getOpacity", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getOpacityParam(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getOpacityParam()const
            DoubleParam * cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getOpacityParam();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_getOverlayColor(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getOverlayColor()const
            ColorTuple* cppResult = new ColorTuple(const_cast<const ::BezierCurve*>(cppSelf)->getOverlayColor());
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_isCurveFinished(PyObject* self)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isCurveFinished()const
            bool cppResult = const_cast<const ::BezierCurve*>(cppSelf)->isCurveFinished();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_moveFeatherByIndex(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "moveFeatherByIndex", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: moveFeatherByIndex(int,double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveFeatherByIndex_TypeError:
        const char* overloads[] = {"int, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveFeatherByIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_moveLeftBezierPoint(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "moveLeftBezierPoint", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: moveLeftBezierPoint(int,double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveLeftBezierPoint_TypeError:
        const char* overloads[] = {"int, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveLeftBezierPoint", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_movePointByIndex(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "movePointByIndex", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: movePointByIndex(int,double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_movePointByIndex_TypeError:
        const char* overloads[] = {"int, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.movePointByIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_moveRightBezierPoint(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "moveRightBezierPoint", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: moveRightBezierPoint(int,double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveRightBezierPoint_TypeError:
        const char* overloads[] = {"int, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveRightBezierPoint", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_removeControlPointByIndex(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: removeControlPointByIndex(int)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_removeControlPointByIndex_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.removeControlPointByIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setActivated(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setActivated", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setActivated(double,bool)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setActivated_TypeError:
        const char* overloads[] = {"float, bool", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setActivated", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setColor(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setColor", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: setColor(double,double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setColor_TypeError:
        const char* overloads[] = {"float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setColor", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setCompositingOperator(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setCompositingOperator(NATRON_NAMESPACE::MergingFunctionEnum)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SBK_CONVERTER(SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_MERGINGFUNCTIONENUM_IDX]), (pyArg)))) {
        overloadId = 0; // setCompositingOperator(NATRON_NAMESPACE::MergingFunctionEnum)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setCompositingOperator_TypeError;

    // Call function/method
    {
        ::NATRON_NAMESPACE::MergingFunctionEnum cppArg0 = ((::NATRON_NAMESPACE::MergingFunctionEnum)0);
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setCompositingOperator(NATRON_NAMESPACE::MergingFunctionEnum)
            cppSelf->setCompositingOperator(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setCompositingOperator_TypeError:
        const char* overloads[] = {"NatronEngine.Natron.MergingFunctionEnum", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.setCompositingOperator", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setCurveFinished(PyObject* self, PyObject* pyArg)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setCurveFinished(bool)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setCurveFinished_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.setCurveFinished", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setFeatherDistance(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setFeatherDistance", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setFeatherDistance(double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setFeatherDistance_TypeError:
        const char* overloads[] = {"float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setFeatherDistance", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setFeatherFallOff(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setFeatherFallOff", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setFeatherFallOff(double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setFeatherFallOff_TypeError:
        const char* overloads[] = {"float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setFeatherFallOff", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setFeatherPointAtIndex(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setFeatherPointAtIndex", 8, 8, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4]), &(pyArgs[5]), &(pyArgs[6]), &(pyArgs[7])))
        return 0;


    // Overloaded function decisor
    // 0: setFeatherPointAtIndex(int,double,double,double,double,double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setFeatherPointAtIndex_TypeError:
        const char* overloads[] = {"int, float, float, float, float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setFeatherPointAtIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setOpacity(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setOpacity", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setOpacity(double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setOpacity_TypeError:
        const char* overloads[] = {"float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setOpacity", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setOverlayColor(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setOverlayColor", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: setOverlayColor(double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setOverlayColor_TypeError:
        const char* overloads[] = {"float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setOverlayColor", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setPointAtIndex(PyObject* self, PyObject* args)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setPointAtIndex", 8, 8, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4]), &(pyArgs[5]), &(pyArgs[6]), &(pyArgs[7])))
        return 0;


    // Overloaded function decisor
    // 0: setPointAtIndex(int,double,double,double,double,double,double,double)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setPointAtIndex_TypeError:
        const char* overloads[] = {"int, float, float, float, float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setPointAtIndex", overloads);
        return 0;
}

static PyMethodDef Sbk_BezierCurve_methods[] = {
    {"addControlPoint", (PyCFunction)Sbk_BezierCurveFunc_addControlPoint, METH_VARARGS},
    {"addControlPointOnSegment", (PyCFunction)Sbk_BezierCurveFunc_addControlPointOnSegment, METH_VARARGS},
    {"getActivatedParam", (PyCFunction)Sbk_BezierCurveFunc_getActivatedParam, METH_NOARGS},
    {"getColor", (PyCFunction)Sbk_BezierCurveFunc_getColor, METH_O},
    {"getColorParam", (PyCFunction)Sbk_BezierCurveFunc_getColorParam, METH_NOARGS},
    {"getCompositingOperator", (PyCFunction)Sbk_BezierCurveFunc_getCompositingOperator, METH_NOARGS},
    {"getCompositingOperatorParam", (PyCFunction)Sbk_BezierCurveFunc_getCompositingOperatorParam, METH_NOARGS},
    {"getControlPointPosition", (PyCFunction)Sbk_BezierCurveFunc_getControlPointPosition, METH_VARARGS},
    {"getFeatherDistance", (PyCFunction)Sbk_BezierCurveFunc_getFeatherDistance, METH_O},
    {"getFeatherDistanceParam", (PyCFunction)Sbk_BezierCurveFunc_getFeatherDistanceParam, METH_NOARGS},
    {"getFeatherFallOff", (PyCFunction)Sbk_BezierCurveFunc_getFeatherFallOff, METH_O},
    {"getFeatherFallOffParam", (PyCFunction)Sbk_BezierCurveFunc_getFeatherFallOffParam, METH_NOARGS},
    {"getFeatherPointPosition", (PyCFunction)Sbk_BezierCurveFunc_getFeatherPointPosition, METH_VARARGS},
    {"getIsActivated", (PyCFunction)Sbk_BezierCurveFunc_getIsActivated, METH_O},
    {"getKeyframes", (PyCFunction)Sbk_BezierCurveFunc_getKeyframes, METH_NOARGS},
    {"getNumControlPoints", (PyCFunction)Sbk_BezierCurveFunc_getNumControlPoints, METH_NOARGS},
    {"getOpacity", (PyCFunction)Sbk_BezierCurveFunc_getOpacity, METH_O},
    {"getOpacityParam", (PyCFunction)Sbk_BezierCurveFunc_getOpacityParam, METH_NOARGS},
    {"getOverlayColor", (PyCFunction)Sbk_BezierCurveFunc_getOverlayColor, METH_NOARGS},
    {"isCurveFinished", (PyCFunction)Sbk_BezierCurveFunc_isCurveFinished, METH_NOARGS},
    {"moveFeatherByIndex", (PyCFunction)Sbk_BezierCurveFunc_moveFeatherByIndex, METH_VARARGS},
    {"moveLeftBezierPoint", (PyCFunction)Sbk_BezierCurveFunc_moveLeftBezierPoint, METH_VARARGS},
    {"movePointByIndex", (PyCFunction)Sbk_BezierCurveFunc_movePointByIndex, METH_VARARGS},
    {"moveRightBezierPoint", (PyCFunction)Sbk_BezierCurveFunc_moveRightBezierPoint, METH_VARARGS},
    {"removeControlPointByIndex", (PyCFunction)Sbk_BezierCurveFunc_removeControlPointByIndex, METH_O},
    {"setActivated", (PyCFunction)Sbk_BezierCurveFunc_setActivated, METH_VARARGS},
    {"setColor", (PyCFunction)Sbk_BezierCurveFunc_setColor, METH_VARARGS},
    {"setCompositingOperator", (PyCFunction)Sbk_BezierCurveFunc_setCompositingOperator, METH_O},
    {"setCurveFinished", (PyCFunction)Sbk_BezierCurveFunc_setCurveFinished, METH_O},
    {"setFeatherDistance", (PyCFunction)Sbk_BezierCurveFunc_setFeatherDistance, METH_VARARGS},
    {"setFeatherFallOff", (PyCFunction)Sbk_BezierCurveFunc_setFeatherFallOff, METH_VARARGS},
    {"setFeatherPointAtIndex", (PyCFunction)Sbk_BezierCurveFunc_setFeatherPointAtIndex, METH_VARARGS},
    {"setOpacity", (PyCFunction)Sbk_BezierCurveFunc_setOpacity, METH_VARARGS},
    {"setOverlayColor", (PyCFunction)Sbk_BezierCurveFunc_setOverlayColor, METH_VARARGS},
    {"setPointAtIndex", (PyCFunction)Sbk_BezierCurveFunc_setPointAtIndex, METH_VARARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_BezierCurve_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_BezierCurve_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_BezierCurve_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.BezierCurve",
    /*tp_basicsize*/        sizeof(SbkObject),
    /*tp_itemsize*/         0,
    /*tp_dealloc*/          &SbkDeallocWrapper,
    /*tp_print*/            0,
    /*tp_getattr*/          0,
    /*tp_setattr*/          0,
    /*tp_compare*/          0,
    /*tp_repr*/             0,
    /*tp_as_number*/        0,
    /*tp_as_sequence*/      0,
    /*tp_as_mapping*/       0,
    /*tp_hash*/             0,
    /*tp_call*/             0,
    /*tp_str*/              0,
    /*tp_getattro*/         0,
    /*tp_setattro*/         0,
    /*tp_as_buffer*/        0,
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_BezierCurve_traverse,
    /*tp_clear*/            Sbk_BezierCurve_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_BezierCurve_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             0,
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             0,
    /*tp_alloc*/            0,
    /*tp_new*/              0,
    /*tp_free*/             0,
    /*tp_is_gc*/            0,
    /*tp_bases*/            0,
    /*tp_mro*/              0,
    /*tp_cache*/            0,
    /*tp_subclasses*/       0,
    /*tp_weaklist*/         0
}, },
    /*priv_data*/           0
};
} //extern

static void* Sbk_BezierCurve_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::ItemBase >()))
        return dynamic_cast< ::BezierCurve*>(reinterpret_cast< ::ItemBase*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void BezierCurve_PythonToCpp_BezierCurve_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_BezierCurve_Type, pyIn, cppOut);
}
static PythonToCppFunc is_BezierCurve_PythonToCpp_BezierCurve_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_BezierCurve_Type))
        return BezierCurve_PythonToCpp_BezierCurve_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* BezierCurve_PTR_CppToPython_BezierCurve(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::BezierCurve*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_BezierCurve_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_BezierCurve(PyObject* module)
{
    SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_BezierCurve_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "BezierCurve", "BezierCurve*",
        &Sbk_BezierCurve_Type, &Shiboken::callCppDestructor< ::BezierCurve >, (SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_BezierCurve_Type,
        BezierCurve_PythonToCpp_BezierCurve_PTR,
        is_BezierCurve_PythonToCpp_BezierCurve_PTR_Convertible,
        BezierCurve_PTR_CppToPython_BezierCurve);

    Shiboken::Conversions::registerConverterName(converter, "BezierCurve");
    Shiboken::Conversions::registerConverterName(converter, "BezierCurve*");
    Shiboken::Conversions::registerConverterName(converter, "BezierCurve&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::BezierCurve).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::BezierCurveWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_BezierCurve_Type, &Sbk_BezierCurve_typeDiscovery);


    BezierCurveWrapper::pysideInitQtMetaTypes();
}
