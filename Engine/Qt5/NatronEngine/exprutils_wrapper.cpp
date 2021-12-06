
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
#include "exprutils_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

// Extra includes
#include <PyParameter.h>
#include <vector>


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


// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_ExprUtils_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::ExprUtils >()))
        return -1;

    ::ExprUtils *cptr{};

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
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyObject *Sbk_ExprUtilsFunc_boxstep(PyObject *self, PyObject *args)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "boxstep", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::boxstep(double,double)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_boxstep_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.boxstep");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_ccellnoise(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::ccellnoise(Double3DTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_ccellnoise_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.ccellnoise");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_cellnoise(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::cellnoise(Double3DTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cellnoise_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.cellnoise");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_cfbm(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:cfbm", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::cfbm(Double3DTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_cfbm_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_cfbm_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_cfbm_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cfbm_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.cfbm");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_cfbm4(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:cfbm4", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::cfbm4(ColorTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_cfbm4_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_cfbm4_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cfbm4(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_cfbm4_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cfbm4_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.cfbm4");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_cnoise(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::cnoise(Double3DTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cnoise_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.cnoise");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_cnoise4(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::cnoise4(ColorTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cnoise4_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.cnoise4");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_cturbulence(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:cturbulence", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::cturbulence(Double3DTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_cturbulence_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_cturbulence_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.cturbulence(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_cturbulence_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_cturbulence_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.cturbulence");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_fbm(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:fbm", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::fbm(Double3DTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_fbm_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_fbm_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_fbm_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_fbm_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.fbm");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_fbm4(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:fbm4", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::fbm4(ColorTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_fbm4_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_fbm4_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.fbm4(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_fbm4_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_fbm4_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.fbm4");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_gaussstep(PyObject *self, PyObject *args)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "gaussstep", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::gaussstep(double,double,double)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_gaussstep_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.gaussstep");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_hash(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::hash(std::vector<double>)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_hash_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.hash");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_linearstep(PyObject *self, PyObject *args)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "linearstep", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::linearstep(double,double,double)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_linearstep_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.linearstep");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_mix(PyObject *self, PyObject *args)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "mix", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::mix(double,double,double)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_mix_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.mix");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_noise(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::noise(Double2DTuple)
    // 1: static ExprUtils::noise(double)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_noise_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.noise");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_pnoise(PyObject *self, PyObject *args)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "pnoise", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::pnoise(Double3DTuple,Double3DTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_pnoise_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.pnoise");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_remap(PyObject *self, PyObject *args)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "remap", 5, 5, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::remap(double,double,double,double,double)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_remap_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.remap");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_smoothstep(PyObject *self, PyObject *args)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "smoothstep", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::smoothstep(double,double,double)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_smoothstep_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.smoothstep");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_snoise(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ExprUtils *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EXPRUTILS_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: ExprUtils::snoise(Double3DTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_snoise_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.snoise");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_snoise4(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::snoise4(ColorTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_snoise4_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.snoise4");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_turbulence(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:turbulence", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::turbulence(Double3DTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_turbulence_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_turbulence_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.turbulence(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_turbulence_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_turbulence_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.turbulence");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_vfbm(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:vfbm", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::vfbm(Double3DTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_vfbm_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_vfbm_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_vfbm_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vfbm_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.vfbm");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_vfbm4(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:vfbm4", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::vfbm4(ColorTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_vfbm4_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_vfbm4_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vfbm4(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_vfbm4_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vfbm4_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.vfbm4");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_vnoise(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::vnoise(Double3DTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vnoise_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.vnoise");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_vnoise4(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ExprUtils::vnoise4(ColorTuple)
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vnoise4_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ExprUtils.vnoise4");
        return {};
}

static PyObject *Sbk_ExprUtilsFunc_vturbulence(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:vturbulence", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: static ExprUtils::vturbulence(Double3DTuple,int,double,double)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","octaves");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): got multiple values for keyword argument 'octaves'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ExprUtilsFunc_vturbulence_TypeError;
                }
            }
            keyName = Py_BuildValue("s","lacunarity");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): got multiple values for keyword argument 'lacunarity'.");
                    return {};
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2]))))
                        goto Sbk_ExprUtilsFunc_vturbulence_TypeError;
                }
            }
            keyName = Py_BuildValue("s","gain");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[3]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ExprUtils.vturbulence(): got multiple values for keyword argument 'gain'.");
                    return {};
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3]))))
                        goto Sbk_ExprUtilsFunc_vturbulence_TypeError;
                }
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
        return {};
    }
    return pyResult;

    Sbk_ExprUtilsFunc_vturbulence_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ExprUtils.vturbulence");
        return {};
}


static const char *Sbk_ExprUtils_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_ExprUtils_methods[] = {
    {"boxstep", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_boxstep), METH_VARARGS|METH_STATIC},
    {"ccellnoise", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_ccellnoise), METH_O|METH_STATIC},
    {"cellnoise", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_cellnoise), METH_O|METH_STATIC},
    {"cfbm", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_cfbm), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"cfbm4", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_cfbm4), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"cnoise", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_cnoise), METH_O|METH_STATIC},
    {"cnoise4", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_cnoise4), METH_O|METH_STATIC},
    {"cturbulence", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_cturbulence), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"fbm", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_fbm), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"fbm4", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_fbm4), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"gaussstep", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_gaussstep), METH_VARARGS|METH_STATIC},
    {"hash", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_hash), METH_O|METH_STATIC},
    {"linearstep", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_linearstep), METH_VARARGS|METH_STATIC},
    {"mix", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_mix), METH_VARARGS|METH_STATIC},
    {"noise", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_noise), METH_O|METH_STATIC},
    {"pnoise", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_pnoise), METH_VARARGS|METH_STATIC},
    {"remap", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_remap), METH_VARARGS|METH_STATIC},
    {"smoothstep", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_smoothstep), METH_VARARGS|METH_STATIC},
    {"snoise", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_snoise), METH_O},
    {"snoise4", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_snoise4), METH_O|METH_STATIC},
    {"turbulence", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_turbulence), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"vfbm", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_vfbm), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"vfbm4", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_vfbm4), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"vnoise", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_vnoise), METH_O|METH_STATIC},
    {"vnoise4", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_vnoise4), METH_O|METH_STATIC},
    {"vturbulence", reinterpret_cast<PyCFunction>(Sbk_ExprUtilsFunc_vturbulence), METH_VARARGS|METH_KEYWORDS|METH_STATIC},

    {nullptr, nullptr} // Sentinel
};

} // extern "C"

static int Sbk_ExprUtils_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_ExprUtils_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_ExprUtils_Type = nullptr;
static SbkObjectType *Sbk_ExprUtils_TypeF(void)
{
    return _Sbk_ExprUtils_Type;
}

static PyType_Slot Sbk_ExprUtils_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_ExprUtils_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_ExprUtils_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_ExprUtils_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_ExprUtils_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    {0, nullptr}
};
static PyType_Spec Sbk_ExprUtils_spec = {
    "1:NatronEngine.ExprUtils",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_ExprUtils_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ExprUtils_PythonToCpp_ExprUtils_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_ExprUtils_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_ExprUtils_PythonToCpp_ExprUtils_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_ExprUtils_TypeF())))
        return ExprUtils_PythonToCpp_ExprUtils_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *ExprUtils_PTR_CppToPython_ExprUtils(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::ExprUtils *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_ExprUtils_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *ExprUtils_SignatureStrings[] = {
    "NatronEngine.ExprUtils(self)",
    "NatronEngine.ExprUtils.boxstep(x:double,a:double)->double",
    "NatronEngine.ExprUtils.ccellnoise(p:NatronEngine.Double3DTuple)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.cellnoise(p:NatronEngine.Double3DTuple)->double",
    "NatronEngine.ExprUtils.cfbm(p:NatronEngine.Double3DTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.cfbm4(p:NatronEngine.ColorTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.cnoise(p:NatronEngine.Double3DTuple)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.cnoise4(p:NatronEngine.ColorTuple)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.cturbulence(p:NatronEngine.Double3DTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.fbm(p:NatronEngine.Double3DTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->double",
    "NatronEngine.ExprUtils.fbm4(p:NatronEngine.ColorTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->double",
    "NatronEngine.ExprUtils.gaussstep(x:double,a:double,b:double)->double",
    "NatronEngine.ExprUtils.hash(args:std.vector[double])->double",
    "NatronEngine.ExprUtils.linearstep(x:double,a:double,b:double)->double",
    "NatronEngine.ExprUtils.mix(x:double,y:double,alpha:double)->double",
    "1:NatronEngine.ExprUtils.noise(p:NatronEngine.Double2DTuple)->double",
    "0:NatronEngine.ExprUtils.noise(x:double)->double",
    "NatronEngine.ExprUtils.pnoise(p:NatronEngine.Double3DTuple,period:NatronEngine.Double3DTuple)->double",
    "NatronEngine.ExprUtils.remap(x:double,source:double,range:double,falloff:double,interp:double)->double",
    "NatronEngine.ExprUtils.smoothstep(x:double,a:double,b:double)->double",
    "NatronEngine.ExprUtils.snoise(self,p:NatronEngine.Double3DTuple)->double",
    "NatronEngine.ExprUtils.snoise4(p:NatronEngine.ColorTuple)->double",
    "NatronEngine.ExprUtils.turbulence(p:NatronEngine.Double3DTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->double",
    "NatronEngine.ExprUtils.vfbm(p:NatronEngine.Double3DTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.vfbm4(p:NatronEngine.ColorTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.vnoise(p:NatronEngine.Double3DTuple)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.vnoise4(p:NatronEngine.ColorTuple)->NatronEngine.Double3DTuple",
    "NatronEngine.ExprUtils.vturbulence(p:NatronEngine.Double3DTuple,octaves:int=6,lacunarity:double=2.,gain:double=0.5)->NatronEngine.Double3DTuple",
    nullptr}; // Sentinel

void init_ExprUtils(PyObject *module)
{
    _Sbk_ExprUtils_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "ExprUtils",
        "ExprUtils*",
        &Sbk_ExprUtils_spec,
        &Shiboken::callCppDestructor< ::ExprUtils >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_ExprUtils_Type);
    InitSignatureStrings(pyType, ExprUtils_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_ExprUtils_Type), Sbk_ExprUtils_PropertyStrings);
    SbkNatronEngineTypes[SBK_EXPRUTILS_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_ExprUtils_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_ExprUtils_TypeF(),
        ExprUtils_PythonToCpp_ExprUtils_PTR,
        is_ExprUtils_PythonToCpp_ExprUtils_PTR_Convertible,
        ExprUtils_PTR_CppToPython_ExprUtils);

    Shiboken::Conversions::registerConverterName(converter, "ExprUtils");
    Shiboken::Conversions::registerConverterName(converter, "ExprUtils*");
    Shiboken::Conversions::registerConverterName(converter, "ExprUtils&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ExprUtils).name());



}
