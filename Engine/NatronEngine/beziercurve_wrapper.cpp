
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
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
#include <PyItemsTable.h>
#include <PyParameter.h>
#include <RectD.h>
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
static PyObject* Sbk_BezierCurveFunc_addControlPoint(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.addControlPoint(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.addControlPoint(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:addControlPoint", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: addControlPoint(double,double,QString)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // addControlPoint(double,double,QString)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
            overloadId = 0; // addControlPoint(double,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_addControlPoint_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.addControlPoint(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_BezierCurveFunc_addControlPoint_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // addControlPoint(double,double,QString)
            cppSelf->addControlPoint(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_addControlPoint_TypeError:
        const char* overloads[] = {"float, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.addControlPoint", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_addControlPointOnSegment(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.addControlPointOnSegment(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.addControlPointOnSegment(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:addControlPointOnSegment", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: addControlPointOnSegment(int,double,QString)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // addControlPointOnSegment(int,double,QString)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
            overloadId = 0; // addControlPointOnSegment(int,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_addControlPointOnSegment_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.addControlPointOnSegment(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_BezierCurveFunc_addControlPointOnSegment_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // addControlPointOnSegment(int,double,QString)
            cppSelf->addControlPointOnSegment(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_addControlPointOnSegment_TypeError:
        const char* overloads[] = {"int, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.addControlPointOnSegment", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getBoundingBox(PyObject* self, PyObject* args, PyObject* kwds)
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
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getBoundingBox(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getBoundingBox(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getBoundingBox", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getBoundingBox(double,QString)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getBoundingBox(double,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // getBoundingBox(double,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getBoundingBox_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getBoundingBox(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_BezierCurveFunc_getBoundingBox_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getBoundingBox(double,QString)const
            RectD* cppResult = new RectD(const_cast<const ::BezierCurve*>(cppSelf)->getBoundingBox(cppArg0, cppArg1));
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getBoundingBox_TypeError:
        const char* overloads[] = {"float, unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getBoundingBox", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getControlPointPosition(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getControlPointPosition(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getControlPointPosition(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:getControlPointPosition", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
            overloadId = 0; // getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getControlPointPosition_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getControlPointPosition(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_BezierCurveFunc_getControlPointPosition_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // getControlPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
            // Begin code injection

            double x,y,rx,ry,lx,ly;
            cppSelf->getControlPointPosition(cppArg0, cppArg1, &x,&y, &lx,&ly,&rx,&ry, cppArg2);

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
        const char* overloads[] = {"int, float, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getControlPointPosition", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getFeatherPointPosition(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getFeatherPointPosition(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getFeatherPointPosition(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:getFeatherPointPosition", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
            overloadId = 0; // getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getFeatherPointPosition_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getFeatherPointPosition(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_BezierCurveFunc_getFeatherPointPosition_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // getFeatherPointPosition(int,double,double*,double*,double*,double*,double*,double*,QString)const
            // Begin code injection

            double x,y,rx,ry,lx,ly;
            cppSelf->getFeatherPointPosition(cppArg0, cppArg1, &x,&y, &lx,&ly,&rx,&ry,cppArg2);

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
        const char* overloads[] = {"int, float, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getFeatherPointPosition", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getNumControlPoints(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getNumControlPoints(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getNumControlPoints", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getNumControlPoints(QString)const
    if (numArgs == 0) {
        overloadId = 0; // getNumControlPoints(QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 0; // getNumControlPoints(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_getNumControlPoints_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.getNumControlPoints(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_BezierCurveFunc_getNumControlPoints_TypeError;
            }
        }
        ::QString cppArg0 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getNumControlPoints(QString)const
            int cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getNumControlPoints(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_getNumControlPoints_TypeError:
        const char* overloads[] = {"unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.getNumControlPoints", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_getViewsList(PyObject* self)
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
            // getViewsList()const
            QStringList cppResult = const_cast<const ::BezierCurve*>(cppSelf)->getViewsList();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_BezierCurveFunc_isActivated(PyObject* self, PyObject* args, PyObject* kwds)
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
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.isActivated(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.isActivated(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:isActivated", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: isActivated(double,QString)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // isActivated(double,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // isActivated(double,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_isActivated_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.isActivated(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_BezierCurveFunc_isActivated_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // isActivated(double,QString)const
            bool cppResult = const_cast<const ::BezierCurve*>(cppSelf)->isActivated(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_isActivated_TypeError:
        const char* overloads[] = {"float, unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.isActivated", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_isCurveFinished(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.isCurveFinished(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:isCurveFinished", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: isCurveFinished(QString)const
    if (numArgs == 0) {
        overloadId = 0; // isCurveFinished(QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 0; // isCurveFinished(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_isCurveFinished_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.isCurveFinished(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_BezierCurveFunc_isCurveFinished_TypeError;
            }
        }
        ::QString cppArg0 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // isCurveFinished(QString)const
            bool cppResult = const_cast<const ::BezierCurve*>(cppSelf)->isCurveFinished(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_BezierCurveFunc_isCurveFinished_TypeError:
        const char* overloads[] = {"unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.isCurveFinished", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_moveFeatherByIndex(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 5) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveFeatherByIndex(): too many arguments");
        return 0;
    } else if (numArgs < 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveFeatherByIndex(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOOO:moveFeatherByIndex", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: moveFeatherByIndex(int,double,double,double,QString)
    if (numArgs >= 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        if (numArgs == 4) {
            overloadId = 0; // moveFeatherByIndex(int,double,double,double,QString)
        } else if ((pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4])))) {
            overloadId = 0; // moveFeatherByIndex(int,double,double,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_moveFeatherByIndex_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[4]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveFeatherByIndex(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[4] = value;
                if (!(pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4]))))
                    goto Sbk_BezierCurveFunc_moveFeatherByIndex_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        ::QString cppArg4 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[4]) pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // moveFeatherByIndex(int,double,double,double,QString)
            cppSelf->moveFeatherByIndex(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveFeatherByIndex_TypeError:
        const char* overloads[] = {"int, float, float, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveFeatherByIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_moveLeftBezierPoint(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 5) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveLeftBezierPoint(): too many arguments");
        return 0;
    } else if (numArgs < 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveLeftBezierPoint(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOOO:moveLeftBezierPoint", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: moveLeftBezierPoint(int,double,double,double,QString)
    if (numArgs >= 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        if (numArgs == 4) {
            overloadId = 0; // moveLeftBezierPoint(int,double,double,double,QString)
        } else if ((pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4])))) {
            overloadId = 0; // moveLeftBezierPoint(int,double,double,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_moveLeftBezierPoint_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[4]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveLeftBezierPoint(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[4] = value;
                if (!(pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4]))))
                    goto Sbk_BezierCurveFunc_moveLeftBezierPoint_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        ::QString cppArg4 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[4]) pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // moveLeftBezierPoint(int,double,double,double,QString)
            cppSelf->moveLeftBezierPoint(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveLeftBezierPoint_TypeError:
        const char* overloads[] = {"int, float, float, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveLeftBezierPoint", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_movePointByIndex(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 5) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.movePointByIndex(): too many arguments");
        return 0;
    } else if (numArgs < 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.movePointByIndex(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOOO:movePointByIndex", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: movePointByIndex(int,double,double,double,QString)
    if (numArgs >= 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        if (numArgs == 4) {
            overloadId = 0; // movePointByIndex(int,double,double,double,QString)
        } else if ((pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4])))) {
            overloadId = 0; // movePointByIndex(int,double,double,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_movePointByIndex_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[4]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.movePointByIndex(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[4] = value;
                if (!(pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4]))))
                    goto Sbk_BezierCurveFunc_movePointByIndex_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        ::QString cppArg4 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[4]) pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // movePointByIndex(int,double,double,double,QString)
            cppSelf->movePointByIndex(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_movePointByIndex_TypeError:
        const char* overloads[] = {"int, float, float, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.movePointByIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_moveRightBezierPoint(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 5) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveRightBezierPoint(): too many arguments");
        return 0;
    } else if (numArgs < 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveRightBezierPoint(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOOO:moveRightBezierPoint", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: moveRightBezierPoint(int,double,double,double,QString)
    if (numArgs >= 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        if (numArgs == 4) {
            overloadId = 0; // moveRightBezierPoint(int,double,double,double,QString)
        } else if ((pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4])))) {
            overloadId = 0; // moveRightBezierPoint(int,double,double,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_moveRightBezierPoint_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[4]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.moveRightBezierPoint(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[4] = value;
                if (!(pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4]))))
                    goto Sbk_BezierCurveFunc_moveRightBezierPoint_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        ::QString cppArg4 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[4]) pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // moveRightBezierPoint(int,double,double,double,QString)
            cppSelf->moveRightBezierPoint(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_moveRightBezierPoint_TypeError:
        const char* overloads[] = {"int, float, float, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.moveRightBezierPoint", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_removeControlPointByIndex(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.removeControlPointByIndex(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.removeControlPointByIndex(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:removeControlPointByIndex", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: removeControlPointByIndex(int,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // removeControlPointByIndex(int,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // removeControlPointByIndex(int,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_removeControlPointByIndex_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.removeControlPointByIndex(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_BezierCurveFunc_removeControlPointByIndex_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // removeControlPointByIndex(int,QString)
            cppSelf->removeControlPointByIndex(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_removeControlPointByIndex_TypeError:
        const char* overloads[] = {"int, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.removeControlPointByIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setCurveFinished(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setCurveFinished(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setCurveFinished(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:setCurveFinished", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setCurveFinished(bool,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setCurveFinished(bool,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // setCurveFinished(bool,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setCurveFinished_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setCurveFinished(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_BezierCurveFunc_setCurveFinished_TypeError;
            }
        }
        bool cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setCurveFinished(bool,QString)
            cppSelf->setCurveFinished(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setCurveFinished_TypeError:
        const char* overloads[] = {"bool, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setCurveFinished", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setFeatherPointAtIndex(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 9) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setFeatherPointAtIndex(): too many arguments");
        return 0;
    } else if (numArgs < 8) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setFeatherPointAtIndex(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOOOOOOO:setFeatherPointAtIndex", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4]), &(pyArgs[5]), &(pyArgs[6]), &(pyArgs[7]), &(pyArgs[8])))
        return 0;


    // Overloaded function decisor
    // 0: setFeatherPointAtIndex(int,double,double,double,double,double,double,double,QString)
    if (numArgs >= 8
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[4])))
        && (pythonToCpp[5] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[5])))
        && (pythonToCpp[6] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[6])))
        && (pythonToCpp[7] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[7])))) {
        if (numArgs == 8) {
            overloadId = 0; // setFeatherPointAtIndex(int,double,double,double,double,double,double,double,QString)
        } else if ((pythonToCpp[8] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[8])))) {
            overloadId = 0; // setFeatherPointAtIndex(int,double,double,double,double,double,double,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setFeatherPointAtIndex_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[8]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setFeatherPointAtIndex(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[8] = value;
                if (!(pythonToCpp[8] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[8]))))
                    goto Sbk_BezierCurveFunc_setFeatherPointAtIndex_TypeError;
            }
        }
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
        ::QString cppArg8 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[8]) pythonToCpp[8](pyArgs[8], &cppArg8);

        if (!PyErr_Occurred()) {
            // setFeatherPointAtIndex(int,double,double,double,double,double,double,double,QString)
            cppSelf->setFeatherPointAtIndex(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4, cppArg5, cppArg6, cppArg7, cppArg8);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setFeatherPointAtIndex_TypeError:
        const char* overloads[] = {"int, float, float, float, float, float, float, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setFeatherPointAtIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_setPointAtIndex(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::BezierCurve* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::BezierCurve*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 9) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setPointAtIndex(): too many arguments");
        return 0;
    } else if (numArgs < 8) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setPointAtIndex(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOOOOOOO:setPointAtIndex", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4]), &(pyArgs[5]), &(pyArgs[6]), &(pyArgs[7]), &(pyArgs[8])))
        return 0;


    // Overloaded function decisor
    // 0: setPointAtIndex(int,double,double,double,double,double,double,double,QString)
    if (numArgs >= 8
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[4])))
        && (pythonToCpp[5] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[5])))
        && (pythonToCpp[6] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[6])))
        && (pythonToCpp[7] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[7])))) {
        if (numArgs == 8) {
            overloadId = 0; // setPointAtIndex(int,double,double,double,double,double,double,double,QString)
        } else if ((pythonToCpp[8] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[8])))) {
            overloadId = 0; // setPointAtIndex(int,double,double,double,double,double,double,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_setPointAtIndex_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[8]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.BezierCurve.setPointAtIndex(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[8] = value;
                if (!(pythonToCpp[8] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[8]))))
                    goto Sbk_BezierCurveFunc_setPointAtIndex_TypeError;
            }
        }
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
        ::QString cppArg8 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[8]) pythonToCpp[8](pyArgs[8], &cppArg8);

        if (!PyErr_Occurred()) {
            // setPointAtIndex(int,double,double,double,double,double,double,double,QString)
            cppSelf->setPointAtIndex(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4, cppArg5, cppArg6, cppArg7, cppArg8);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_setPointAtIndex_TypeError:
        const char* overloads[] = {"int, float, float, float, float, float, float, float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.BezierCurve.setPointAtIndex", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_splitView(PyObject* self, PyObject* pyArg)
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
    // 0: splitView(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // splitView(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_splitView_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // splitView(QString)
            cppSelf->splitView(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_splitView_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.splitView", overloads);
        return 0;
}

static PyObject* Sbk_BezierCurveFunc_unSplitView(PyObject* self, PyObject* pyArg)
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
    // 0: unSplitView(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // unSplitView(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BezierCurveFunc_unSplitView_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // unSplitView(QString)
            cppSelf->unSplitView(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_BezierCurveFunc_unSplitView_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.BezierCurve.unSplitView", overloads);
        return 0;
}

static PyMethodDef Sbk_BezierCurve_methods[] = {
    {"addControlPoint", (PyCFunction)Sbk_BezierCurveFunc_addControlPoint, METH_VARARGS|METH_KEYWORDS},
    {"addControlPointOnSegment", (PyCFunction)Sbk_BezierCurveFunc_addControlPointOnSegment, METH_VARARGS|METH_KEYWORDS},
    {"getBoundingBox", (PyCFunction)Sbk_BezierCurveFunc_getBoundingBox, METH_VARARGS|METH_KEYWORDS},
    {"getControlPointPosition", (PyCFunction)Sbk_BezierCurveFunc_getControlPointPosition, METH_VARARGS|METH_KEYWORDS},
    {"getFeatherPointPosition", (PyCFunction)Sbk_BezierCurveFunc_getFeatherPointPosition, METH_VARARGS|METH_KEYWORDS},
    {"getNumControlPoints", (PyCFunction)Sbk_BezierCurveFunc_getNumControlPoints, METH_VARARGS|METH_KEYWORDS},
    {"getViewsList", (PyCFunction)Sbk_BezierCurveFunc_getViewsList, METH_NOARGS},
    {"isActivated", (PyCFunction)Sbk_BezierCurveFunc_isActivated, METH_VARARGS|METH_KEYWORDS},
    {"isCurveFinished", (PyCFunction)Sbk_BezierCurveFunc_isCurveFinished, METH_VARARGS|METH_KEYWORDS},
    {"moveFeatherByIndex", (PyCFunction)Sbk_BezierCurveFunc_moveFeatherByIndex, METH_VARARGS|METH_KEYWORDS},
    {"moveLeftBezierPoint", (PyCFunction)Sbk_BezierCurveFunc_moveLeftBezierPoint, METH_VARARGS|METH_KEYWORDS},
    {"movePointByIndex", (PyCFunction)Sbk_BezierCurveFunc_movePointByIndex, METH_VARARGS|METH_KEYWORDS},
    {"moveRightBezierPoint", (PyCFunction)Sbk_BezierCurveFunc_moveRightBezierPoint, METH_VARARGS|METH_KEYWORDS},
    {"removeControlPointByIndex", (PyCFunction)Sbk_BezierCurveFunc_removeControlPointByIndex, METH_VARARGS|METH_KEYWORDS},
    {"setCurveFinished", (PyCFunction)Sbk_BezierCurveFunc_setCurveFinished, METH_VARARGS|METH_KEYWORDS},
    {"setFeatherPointAtIndex", (PyCFunction)Sbk_BezierCurveFunc_setFeatherPointAtIndex, METH_VARARGS|METH_KEYWORDS},
    {"setPointAtIndex", (PyCFunction)Sbk_BezierCurveFunc_setPointAtIndex, METH_VARARGS|METH_KEYWORDS},
    {"splitView", (PyCFunction)Sbk_BezierCurveFunc_splitView, METH_O},
    {"unSplitView", (PyCFunction)Sbk_BezierCurveFunc_unSplitView, METH_O},

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
