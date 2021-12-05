
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

#include "exprutils_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyParameter.h>
#include <vector>



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_ExprUtils_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::ExprUtils >()))
        return -1;

    ::ExprUtils* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // ExprUtils()
            cptr = new ::ExprUtils();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::ExprUtils >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyObject* Sbk_ExprUtilsFunc_boxstep(PyObject* self, PyObject* args)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "boxstep", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: boxstep(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // boxstep(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_boxstep_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // boxstep(double,double)
            double cppResult = ::ExprUtils::boxstep(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_boxstep_TypeError:
        const char* overloads[] = {"float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.boxstep", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_ccellnoise(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: ccellnoise(Double3DTuple)
    if (true) {
        overloadId = 0; // ccellnoise(Double3DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_ccellnoise_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // ccellnoise(Double3DTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple ret = ExprUtils::ccellnoise(p);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_ccellnoise_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.ccellnoise", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_cellnoise(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: cellnoise(Double3DTuple)
    if (true) {
        overloadId = 0; // cellnoise(Double3DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_cellnoise_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // cellnoise(Double3DTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            double ret = ExprUtils::cellnoise(p);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cellnoise_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.cellnoise", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_cfbm(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:cfbm", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: cfbm(Double3DTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // cfbm(Double3DTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // cfbm(Double3DTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // cfbm(Double3DTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // cfbm(Double3DTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_cfbm_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_cfbm_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_cfbm_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_cfbm_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // cfbm(Double3DTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple ret = ExprUtils::cfbm(p, cppArg1, cppArg2, cppArg3);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cfbm_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.cfbm", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_cfbm4(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:cfbm4", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: cfbm4(ColorTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // cfbm4(ColorTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // cfbm4(ColorTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // cfbm4(ColorTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // cfbm4(ColorTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_cfbm4_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_cfbm4_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_cfbm4_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_cfbm4_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // cfbm4(ColorTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 4) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 4 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            double x4;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 3), &(x4));
            ColorTuple p = {x1, x2, x3, x4};
            Double3DTuple ret = ExprUtils::cfbm4(p, cppArg1, cppArg2, cppArg3);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cfbm4_TypeError:
        const char* overloads[] = {"NatronEngine.ColorTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.cfbm4", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_cnoise(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: cnoise(Double3DTuple)
    if (true) {
        overloadId = 0; // cnoise(Double3DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_cnoise_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // cnoise(Double3DTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple ret = ExprUtils::cnoise(p);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cnoise_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.cnoise", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_cnoise4(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: cnoise4(ColorTuple)
    if (true) {
        overloadId = 0; // cnoise4(ColorTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_cnoise4_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // cnoise4(ColorTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 4) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 4 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            double x4;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 3), &(x4));
            ColorTuple p = {x1, x2, x3, x4};
            Double3DTuple ret = ExprUtils::cnoise4(p);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cnoise4_TypeError:
        const char* overloads[] = {"NatronEngine.ColorTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.cnoise4", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_cturbulence(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:cturbulence", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: cturbulence(Double3DTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // cturbulence(Double3DTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // cturbulence(Double3DTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // cturbulence(Double3DTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // cturbulence(Double3DTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_cturbulence_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_cturbulence_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_cturbulence_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_cturbulence_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // cturbulence(Double3DTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple ret = ExprUtils::cturbulence(p, cppArg1, cppArg2, cppArg3);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cturbulence_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.cturbulence", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_fbm(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:fbm", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: fbm(Double3DTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // fbm(Double3DTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // fbm(Double3DTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // fbm(Double3DTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // fbm(Double3DTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_fbm_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_fbm_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_fbm_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_fbm_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // fbm(Double3DTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            double ret = ExprUtils::fbm(p, cppArg1, cppArg2, cppArg3);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_fbm_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.fbm", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_fbm4(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:fbm4", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: fbm4(ColorTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // fbm4(ColorTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // fbm4(ColorTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // fbm4(ColorTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // fbm4(ColorTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_fbm4_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_fbm4_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_fbm4_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_fbm4_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // fbm4(ColorTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 4) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 4 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            double x4;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 3), &(x4));
            ColorTuple p = {x1, x2, x3, x4};
            double ret = ExprUtils::fbm4(p, cppArg1, cppArg2, cppArg3);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_fbm4_TypeError:
        const char* overloads[] = {"NatronEngine.ColorTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.fbm4", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_gaussstep(PyObject* self, PyObject* args)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "gaussstep", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: gaussstep(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // gaussstep(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_gaussstep_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // gaussstep(double,double,double)
            double cppResult = ::ExprUtils::gaussstep(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_gaussstep_TypeError:
        const char* overloads[] = {"float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.gaussstep", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_hash(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: hash(std::vector<double>)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX], (pyArg)))) {
        overloadId = 0; // hash(std::vector<double>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_hash_TypeError;

    // Call function/method
    {
        ::std::vector<double > cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // hash(std::vector<double>)
            double cppResult = ::ExprUtils::hash(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_hash_TypeError:
        const char* overloads[] = {"list", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.hash", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_linearstep(PyObject* self, PyObject* args)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "linearstep", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: linearstep(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // linearstep(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_linearstep_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // linearstep(double,double,double)
            double cppResult = ::ExprUtils::linearstep(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_linearstep_TypeError:
        const char* overloads[] = {"float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.linearstep", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_mix(PyObject* self, PyObject* args)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "mix", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: mix(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // mix(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_mix_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // mix(double,double,double)
            double cppResult = ::ExprUtils::mix(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_mix_TypeError:
        const char* overloads[] = {"float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.mix", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_noise(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: noise(Double2DTuple)
    // 1: noise(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 1; // noise(double)
    } else if (true) {
        overloadId = 0; // noise(Double2DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_noise_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // noise(const Double2DTuple & p)
        {

            if (!PyErr_Occurred()) {
                // noise(Double2DTuple)
                // Begin code injection

                int tupleSize = PyTuple_GET_SIZE(pyArg);
                if (tupleSize != 4 && tupleSize != 3  && tupleSize != 2) {
                 PyErr_SetString(PyExc_TypeError, "the tuple must have 2, 3 or 4 items.");
                 return 0;
                }

                double ret = 0.;
                double x1;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
                if (tupleSize == 2) {
                 double x2;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
                 Double2DTuple p = {x1, x2};
                 ret = ExprUtils::noise(p);
                } else if (tupleSize == 3) {
                 double x2;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
                 double x3;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
                 Double3DTuple p = {x1, x2, x3};
                 ret = ExprUtils::noise(p);
                } else if (tupleSize == 4) {
                 double x2;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
                 double x3;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
                 double x4;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 3), &(x4));
                 ColorTuple p = {x1, x2, x3, x4};
                 ret = ExprUtils::noise(p);
                }
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);

                return pyResult;

                // End of code injection


            }
            break;
        }
        case 1: // noise(double x)
        {
            double cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // noise(double)
                double cppResult = ::ExprUtils::noise(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_noise_TypeError:
        const char* overloads[] = {"NatronEngine.Double2DTuple", "float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.noise", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_pnoise(PyObject* self, PyObject* args)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "pnoise", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: pnoise(Double3DTuple,Double3DTuple)
    if (numArgs == 2) {
        overloadId = 0; // pnoise(Double3DTuple,Double3DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_pnoise_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // pnoise(Double3DTuple,Double3DTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            tupleSize = PyTuple_GET_SIZE(pyArgs[2-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            double p1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[2-1], 0), &(p1));
            double p2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[2-1], 1), &(p2));
            double p3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[2-1], 2), &(p3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple period = {p1, p2, p3};
            double ret = ExprUtils::pnoise(p, period);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_pnoise_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple, NatronEngine.Double3DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.pnoise", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_remap(PyObject* self, PyObject* args)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "remap", 5, 5, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: remap(double,double,double,double,double)
    if (numArgs == 5
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[4])))) {
        overloadId = 0; // remap(double,double,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_remap_TypeError;

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
        double cppArg4;
        pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // remap(double,double,double,double,double)
            double cppResult = ::ExprUtils::remap(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_remap_TypeError:
        const char* overloads[] = {"float, float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.remap", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_smoothstep(PyObject* self, PyObject* args)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "smoothstep", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: smoothstep(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // smoothstep(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_smoothstep_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // smoothstep(double,double,double)
            double cppResult = ::ExprUtils::smoothstep(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_smoothstep_TypeError:
        const char* overloads[] = {"float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.smoothstep", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_snoise(PyObject* self, PyObject* pyArg)
{
    ::ExprUtils* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ExprUtils*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EXPRUTILS_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: snoise(Double3DTuple)
    if (true) {
        overloadId = 0; // snoise(Double3DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_snoise_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // snoise(Double3DTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            double ret = cppSelf->snoise(p);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_snoise_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.snoise", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_snoise4(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: snoise4(ColorTuple)
    if (true) {
        overloadId = 0; // snoise4(ColorTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_snoise4_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // snoise4(ColorTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 4) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 4 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            double x4;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 3), &(x4));
            ColorTuple p = {x1, x2, x3, x4};
            double ret = ExprUtils::snoise4(p);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_snoise4_TypeError:
        const char* overloads[] = {"NatronEngine.ColorTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.snoise4", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_turbulence(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:turbulence", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: turbulence(Double3DTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // turbulence(Double3DTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // turbulence(Double3DTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // turbulence(Double3DTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // turbulence(Double3DTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_turbulence_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_turbulence_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_turbulence_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_turbulence_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // turbulence(Double3DTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            double ret = ExprUtils::turbulence(p, cppArg1, cppArg2, cppArg3);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_turbulence_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.turbulence", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_vfbm(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:vfbm", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: vfbm(Double3DTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // vfbm(Double3DTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // vfbm(Double3DTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // vfbm(Double3DTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // vfbm(Double3DTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_vfbm_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_vfbm_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_vfbm_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_vfbm_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // vfbm(Double3DTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple ret = ExprUtils::vfbm(p, cppArg1, cppArg2, cppArg3);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vfbm_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.vfbm", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_vfbm4(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:vfbm4", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: vfbm4(ColorTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // vfbm4(ColorTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // vfbm4(ColorTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // vfbm4(ColorTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // vfbm4(ColorTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_vfbm4_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_vfbm4_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_vfbm4_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_vfbm4_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // vfbm4(ColorTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 4) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 4 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            double x4;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 3), &(x4));
            ColorTuple p = {x1, x2, x3, x4};
            Double3DTuple ret = ExprUtils::vfbm4(p, cppArg1, cppArg2, cppArg3);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vfbm4_TypeError:
        const char* overloads[] = {"NatronEngine.ColorTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.vfbm4", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_vnoise(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: vnoise(Double3DTuple)
    if (true) {
        overloadId = 0; // vnoise(Double3DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_vnoise_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // vnoise(Double3DTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple ret = ExprUtils::vnoise(p);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vnoise_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.vnoise", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_vnoise4(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: vnoise4(ColorTuple)
    if (true) {
        overloadId = 0; // vnoise4(ColorTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_vnoise4_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // vnoise4(ColorTuple)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArg);
            if (tupleSize != 4) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 4 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 2), &(x3));
            double x4;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArg, 3), &(x4));
            ColorTuple p = {x1, x2, x3, x4};
            Double3DTuple ret = ExprUtils::vnoise4(p);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vnoise4_TypeError:
        const char* overloads[] = {"NatronEngine.ColorTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.vnoise4", overloads);
        return 0;
}

static PyObject* Sbk_ExprUtilsFunc_vturbulence(PyObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:vturbulence", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: vturbulence(Double3DTuple,int,double,double)
    if (true) {
        if (numArgs == 1) {
            overloadId = 0; // vturbulence(Double3DTuple,int,double,double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // vturbulence(Double3DTuple,int,double,double)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // vturbulence(Double3DTuple,int,double,double)
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
                    overloadId = 0; // vturbulence(Double3DTuple,int,double,double)
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ExprUtilsFunc_vturbulence_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "octaves");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): got multiple values for keyword argument 'octaves'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ExprUtilsFunc_vturbulence_TypeError;
            }
            value = PyDict_GetItemString(kwds, "lacunarity");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): got multiple values for keyword argument 'lacunarity'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                    goto Sbk_ExprUtilsFunc_vturbulence_TypeError;
            }
            value = PyDict_GetItemString(kwds, "gain");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): got multiple values for keyword argument 'gain'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                    goto Sbk_ExprUtilsFunc_vturbulence_TypeError;
            }
        }
        int cppArg1 = 6;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2 = 2.;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3 = 0.5;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // vturbulence(Double3DTuple,int,double,double)
            // Begin code injection

            int tupleSize = PyTuple_GET_SIZE(pyArgs[1-1]);
            if (tupleSize != 3) {
            PyErr_SetString(PyExc_TypeError, "the tuple must have 3 items.");
            return 0;
            }
            double x1;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 0), &(x1));
            double x2;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 1), &(x2));
            double x3;
                Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), PyTuple_GET_ITEM(pyArgs[1-1], 2), &(x3));
            Double3DTuple p = {x1, x2, x3};
            Double3DTuple ret = ExprUtils::vturbulence(p, cppArg1, cppArg2, cppArg3);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.y));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.z));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vturbulence_TypeError:
        const char* overloads[] = {"NatronEngine.Double3DTuple, int = 6, float = 2., float = 0.5", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.vturbulence", overloads);
        return 0;
}

static PyMethodDef Sbk_ExprUtils_methods[] = {
    {"boxstep", (PyCFunction)Sbk_ExprUtilsFunc_boxstep, METH_VARARGS|METH_STATIC},
    {"ccellnoise", (PyCFunction)Sbk_ExprUtilsFunc_ccellnoise, METH_O|METH_STATIC},
    {"cellnoise", (PyCFunction)Sbk_ExprUtilsFunc_cellnoise, METH_O|METH_STATIC},
    {"cfbm", (PyCFunction)Sbk_ExprUtilsFunc_cfbm, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"cfbm4", (PyCFunction)Sbk_ExprUtilsFunc_cfbm4, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"cnoise", (PyCFunction)Sbk_ExprUtilsFunc_cnoise, METH_O|METH_STATIC},
    {"cnoise4", (PyCFunction)Sbk_ExprUtilsFunc_cnoise4, METH_O|METH_STATIC},
    {"cturbulence", (PyCFunction)Sbk_ExprUtilsFunc_cturbulence, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"fbm", (PyCFunction)Sbk_ExprUtilsFunc_fbm, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"fbm4", (PyCFunction)Sbk_ExprUtilsFunc_fbm4, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"gaussstep", (PyCFunction)Sbk_ExprUtilsFunc_gaussstep, METH_VARARGS|METH_STATIC},
    {"hash", (PyCFunction)Sbk_ExprUtilsFunc_hash, METH_O|METH_STATIC},
    {"linearstep", (PyCFunction)Sbk_ExprUtilsFunc_linearstep, METH_VARARGS|METH_STATIC},
    {"mix", (PyCFunction)Sbk_ExprUtilsFunc_mix, METH_VARARGS|METH_STATIC},
    {"noise", (PyCFunction)Sbk_ExprUtilsFunc_noise, METH_O|METH_STATIC},
    {"pnoise", (PyCFunction)Sbk_ExprUtilsFunc_pnoise, METH_VARARGS|METH_STATIC},
    {"remap", (PyCFunction)Sbk_ExprUtilsFunc_remap, METH_VARARGS|METH_STATIC},
    {"smoothstep", (PyCFunction)Sbk_ExprUtilsFunc_smoothstep, METH_VARARGS|METH_STATIC},
    {"snoise", (PyCFunction)Sbk_ExprUtilsFunc_snoise, METH_O},
    {"snoise4", (PyCFunction)Sbk_ExprUtilsFunc_snoise4, METH_O|METH_STATIC},
    {"turbulence", (PyCFunction)Sbk_ExprUtilsFunc_turbulence, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"vfbm", (PyCFunction)Sbk_ExprUtilsFunc_vfbm, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"vfbm4", (PyCFunction)Sbk_ExprUtilsFunc_vfbm4, METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"vnoise", (PyCFunction)Sbk_ExprUtilsFunc_vnoise, METH_O|METH_STATIC},
    {"vnoise4", (PyCFunction)Sbk_ExprUtilsFunc_vnoise4, METH_O|METH_STATIC},
    {"vturbulence", (PyCFunction)Sbk_ExprUtilsFunc_vturbulence, METH_VARARGS|METH_KEYWORDS|METH_STATIC},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_ExprUtils_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ExprUtils_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ExprUtils_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ExprUtils",
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
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_ExprUtils_traverse,
    /*tp_clear*/            Sbk_ExprUtils_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ExprUtils_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_ExprUtils_Init,
    /*tp_alloc*/            0,
    /*tp_new*/              SbkObjectTpNew,
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


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ExprUtils_PythonToCpp_ExprUtils_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ExprUtils_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ExprUtils_PythonToCpp_ExprUtils_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ExprUtils_Type))
        return ExprUtils_PythonToCpp_ExprUtils_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ExprUtils_PTR_CppToPython_ExprUtils(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ExprUtils*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ExprUtils_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_ExprUtils(PyObject* module)
{
    SbkNatronEngineTypes[SBK_EXPRUTILS_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ExprUtils_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ExprUtils", "ExprUtils*",
        &Sbk_ExprUtils_Type, &Shiboken::callCppDestructor< ::ExprUtils >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ExprUtils_Type,
        ExprUtils_PythonToCpp_ExprUtils_PTR,
        is_ExprUtils_PythonToCpp_ExprUtils_PTR_Convertible,
        ExprUtils_PTR_CppToPython_ExprUtils);

    Shiboken::Conversions::registerConverterName(converter, "ExprUtils");
    Shiboken::Conversions::registerConverterName(converter, "ExprUtils*");
    Shiboken::Conversions::registerConverterName(converter, "ExprUtils&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ExprUtils).name());



}
