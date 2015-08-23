
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "rectd_wrapper.h"

// Extra includes
#include <RectD.h>


// Native ---------------------------------------------------------

void RectDWrapper::pysideInitQtMetaTypes()
{
}

RectDWrapper::RectDWrapper() : RectD() {
    // ... middle
}

RectDWrapper::RectDWrapper(double l, double b, double r, double t) : RectD(l, b, r, t) {
    // ... middle
}

RectDWrapper::~RectDWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_RectD_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::RectD >()))
        return -1;

    ::RectDWrapper* cptr = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectD_Init_TypeError;

    if (!PyArg_UnpackTuple(args, "RectD", 0, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return -1;


    // Overloaded function decisor
    // 0: RectD()
    // 1: RectD(RectD)
    // 2: RectD(double,double,double,double)
    if (numArgs == 0) {
        overloadId = 0; // RectD()
    } else if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 2; // RectD(double,double,double,double)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArgs[0])))) {
        overloadId = 1; // RectD(RectD)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectD_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // RectD()
        {

            if (!PyErr_Occurred()) {
                // RectD()
                cptr = new ::RectDWrapper();
            }
            break;
        }
        case 1: // RectD(const RectD & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::RectD* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // RectD(RectD)
                cptr = new ::RectDWrapper(*cppArg0);
            }
            break;
        }
        case 2: // RectD(double l, double b, double r, double t)
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
                // RectD(double,double,double,double)
                cptr = new ::RectDWrapper(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::RectD >(), cptr)) {
        delete cptr;
        return -1;
    }
    if (!cptr) goto Sbk_RectD_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_RectD_Init_TypeError:
        const char* overloads[] = {"", "NatronEngine.RectD", "float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectD", overloads);
        return -1;
}

static PyObject* Sbk_RectDFunc_area(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // area()const
            double cppResult = const_cast<const ::RectD*>(cppSelf)->area();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_bottom(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // bottom()const
            double cppResult = const_cast<const ::RectD*>(cppSelf)->bottom();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_clear(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // clear()
            cppSelf->clear();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_RectDFunc_contains(PyObject* self, PyObject* args)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "contains", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: contains(RectD)const
    // 1: contains(double,double)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 1; // contains(double,double)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArgs[0])))) {
        overloadId = 0; // contains(RectD)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_contains_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // contains(const RectD & other) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectD* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // contains(RectD)const
                bool cppResult = const_cast<const ::RectD*>(cppSelf)->contains(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // contains(double x, double y) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // contains(double,double)const
                bool cppResult = const_cast<const ::RectD*>(cppSelf)->contains(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RectDFunc_contains_TypeError:
        const char* overloads[] = {"NatronEngine.RectD", "float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectD.contains", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_height(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // height()const
            double cppResult = const_cast<const ::RectD*>(cppSelf)->height();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_intersect(PyObject* self, PyObject* args)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_intersect_TypeError;

    if (!PyArg_UnpackTuple(args, "intersect", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: intersect(RectD,RectD*)const
    // 1: intersect(double,double,double,double,RectD*)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // intersect(double,double,double,double,RectD*)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArgs[0])))) {
        overloadId = 0; // intersect(RectD,RectD*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_intersect_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersect(const RectD & r, RectD * intersection) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectD* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersect(RectD,RectD*)const
                // Begin code injection

                RectD t;
                cppSelf->intersect(*cppArg0,&t);
                pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], &t);
                return pyResult;

                // End of code injection


            }
            break;
        }
        case 1: // intersect(double l, double b, double r, double t, RectD * intersection) const
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
                // intersect(double,double,double,double,RectD*)const
                // Begin code injection

                RectD t;
                cppSelf->intersect(cppArg0,cppArg1,cppArg2,cppArg3,&t);
                pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], &t);
                return pyResult;

                // End of code injection


            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RectDFunc_intersect_TypeError:
        const char* overloads[] = {"NatronEngine.RectD, NatronEngine.RectD", "float, float, float, float, NatronEngine.RectD", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectD.intersect", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_intersects(PyObject* self, PyObject* args)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_intersects_TypeError;

    if (!PyArg_UnpackTuple(args, "intersects", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: intersects(RectD)const
    // 1: intersects(double,double,double,double)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // intersects(double,double,double,double)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArgs[0])))) {
        overloadId = 0; // intersects(RectD)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_intersects_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersects(const RectD & r) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectD* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersects(RectD)const
                bool cppResult = const_cast<const ::RectD*>(cppSelf)->intersects(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // intersects(double l, double b, double r, double t) const
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
                // intersects(double,double,double,double)const
                bool cppResult = const_cast<const ::RectD*>(cppSelf)->intersects(cppArg0, cppArg1, cppArg2, cppArg3);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RectDFunc_intersects_TypeError:
        const char* overloads[] = {"NatronEngine.RectD", "float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectD.intersects", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_isInfinite(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isInfinite()const
            bool cppResult = const_cast<const ::RectD*>(cppSelf)->isInfinite();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_isNull(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isNull()const
            bool cppResult = const_cast<const ::RectD*>(cppSelf)->isNull();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_left(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // left()const
            double cppResult = const_cast<const ::RectD*>(cppSelf)->left();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_merge(PyObject* self, PyObject* args)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_merge_TypeError;

    if (!PyArg_UnpackTuple(args, "merge", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: merge(RectD)
    // 1: merge(double,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // merge(double,double,double,double)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArgs[0])))) {
        overloadId = 0; // merge(RectD)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_merge_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // merge(const RectD & box)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectD* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // merge(RectD)
                cppSelf->merge(*cppArg0);
            }
            break;
        }
        case 1: // merge(double l, double b, double r, double t)
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
                // merge(double,double,double,double)
                cppSelf->merge(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_merge_TypeError:
        const char* overloads[] = {"NatronEngine.RectD", "float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectD.merge", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_right(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // right()const
            double cppResult = const_cast<const ::RectD*>(cppSelf)->right();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_set(PyObject* self, PyObject* args)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_set_TypeError;

    if (!PyArg_UnpackTuple(args, "set", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: set(RectD)
    // 1: set(double,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // set(double,double,double,double)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArgs[0])))) {
        overloadId = 0; // set(RectD)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const RectD & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectD* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(RectD)
                cppSelf->set(*cppArg0);
            }
            break;
        }
        case 1: // set(double l, double b, double r, double t)
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
                // set(double,double,double,double)
                cppSelf->set(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_TypeError:
        const char* overloads[] = {"NatronEngine.RectD", "float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectD.set", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_set_bottom(PyObject* self, PyObject* pyArg)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_bottom(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_bottom(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_bottom_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_bottom(double)
            cppSelf->set_bottom(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_bottom_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectD.set_bottom", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_set_left(PyObject* self, PyObject* pyArg)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_left(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_left(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_left_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_left(double)
            cppSelf->set_left(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_left_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectD.set_left", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_set_right(PyObject* self, PyObject* pyArg)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_right(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_right(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_right_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_right(double)
            cppSelf->set_right(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_right_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectD.set_right", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_set_top(PyObject* self, PyObject* pyArg)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_top(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_top(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_top_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_top(double)
            cppSelf->set_top(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_top_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectD.set_top", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_setupInfinity(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setupInfinity()
            cppSelf->setupInfinity();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_RectDFunc_top(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // top()const
            double cppResult = const_cast<const ::RectD*>(cppSelf)->top();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectDFunc_translate(PyObject* self, PyObject* args)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "translate", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: translate(int,int)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // translate(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_translate_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // translate(int,int)
            cppSelf->translate(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_translate_TypeError:
        const char* overloads[] = {"int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectD.translate", overloads);
        return 0;
}

static PyObject* Sbk_RectDFunc_width(PyObject* self)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // width()const
            double cppResult = const_cast<const ::RectD*>(cppSelf)->width();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_RectD_methods[] = {
    {"area", (PyCFunction)Sbk_RectDFunc_area, METH_NOARGS},
    {"bottom", (PyCFunction)Sbk_RectDFunc_bottom, METH_NOARGS},
    {"clear", (PyCFunction)Sbk_RectDFunc_clear, METH_NOARGS},
    {"contains", (PyCFunction)Sbk_RectDFunc_contains, METH_VARARGS},
    {"height", (PyCFunction)Sbk_RectDFunc_height, METH_NOARGS},
    {"intersect", (PyCFunction)Sbk_RectDFunc_intersect, METH_VARARGS},
    {"intersects", (PyCFunction)Sbk_RectDFunc_intersects, METH_VARARGS},
    {"isInfinite", (PyCFunction)Sbk_RectDFunc_isInfinite, METH_NOARGS},
    {"isNull", (PyCFunction)Sbk_RectDFunc_isNull, METH_NOARGS},
    {"left", (PyCFunction)Sbk_RectDFunc_left, METH_NOARGS},
    {"merge", (PyCFunction)Sbk_RectDFunc_merge, METH_VARARGS},
    {"right", (PyCFunction)Sbk_RectDFunc_right, METH_NOARGS},
    {"set", (PyCFunction)Sbk_RectDFunc_set, METH_VARARGS},
    {"set_bottom", (PyCFunction)Sbk_RectDFunc_set_bottom, METH_O},
    {"set_left", (PyCFunction)Sbk_RectDFunc_set_left, METH_O},
    {"set_right", (PyCFunction)Sbk_RectDFunc_set_right, METH_O},
    {"set_top", (PyCFunction)Sbk_RectDFunc_set_top, METH_O},
    {"setupInfinity", (PyCFunction)Sbk_RectDFunc_setupInfinity, METH_NOARGS},
    {"top", (PyCFunction)Sbk_RectDFunc_top, METH_NOARGS},
    {"translate", (PyCFunction)Sbk_RectDFunc_translate, METH_VARARGS},
    {"width", (PyCFunction)Sbk_RectDFunc_width, METH_NOARGS},

    {0} // Sentinel
};

// Rich comparison
static PyObject* Sbk_RectD_richcompare(PyObject* self, PyObject* pyArg, int op)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    ::RectD& cppSelf = *(((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self)));
    SBK_UNUSED(cppSelf)
    PyObject* pyResult = 0;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    switch (op) {
        case Py_NE:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArg)))) {
                // operator!=(const RectD & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return 0;
                ::RectD* cppArg0;
                pythonToCpp(pyArg, &cppArg0);
                bool cppResult = cppSelf != (*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_True;
                Py_INCREF(pyResult);
            }

            break;
        case Py_EQ:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], (pyArg)))) {
                // operator==(const RectD & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return 0;
                ::RectD* cppArg0;
                pythonToCpp(pyArg, &cppArg0);
                bool cppResult = cppSelf == (*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_False;
                Py_INCREF(pyResult);
            }

            break;
        default:
            goto Sbk_RectD_RichComparison_TypeError;
    }

    if (pyResult && !PyErr_Occurred())
        return pyResult;
    Sbk_RectD_RichComparison_TypeError:
    PyErr_SetString(PyExc_NotImplementedError, "operator not implemented.");
    return 0;

}

static PyObject* Sbk_RectD_get_y1(PyObject* self, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->y1);
    return pyOut;
}
static int Sbk_RectD_set_y1(PyObject* self, PyObject* pyIn, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'y1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y1', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->y1;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_RectD_get_x1(PyObject* self, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->x1);
    return pyOut;
}
static int Sbk_RectD_set_x1(PyObject* self, PyObject* pyIn, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'x1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x1', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->x1;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_RectD_get_y2(PyObject* self, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->y2);
    return pyOut;
}
static int Sbk_RectD_set_y2(PyObject* self, PyObject* pyIn, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'y2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y2', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->y2;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_RectD_get_x2(PyObject* self, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->x2);
    return pyOut;
}
static int Sbk_RectD_set_x2(PyObject* self, PyObject* pyIn, void*)
{
    ::RectD* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectD*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'x2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x2', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->x2;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

// Getters and Setters for RectD
static PyGetSetDef Sbk_RectD_getsetlist[] = {
    {const_cast<char*>("y1"), Sbk_RectD_get_y1, Sbk_RectD_set_y1},
    {const_cast<char*>("x1"), Sbk_RectD_get_x1, Sbk_RectD_set_x1},
    {const_cast<char*>("y2"), Sbk_RectD_get_y2, Sbk_RectD_set_y2},
    {const_cast<char*>("x2"), Sbk_RectD_get_x2, Sbk_RectD_set_x2},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_RectD_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_RectD_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_RectD_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.RectD",
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
    /*tp_traverse*/         Sbk_RectD_traverse,
    /*tp_clear*/            Sbk_RectD_clear,
    /*tp_richcompare*/      Sbk_RectD_richcompare,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_RectD_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_RectD_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_RectD_Init,
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
static void RectD_PythonToCpp_RectD_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_RectD_Type, pyIn, cppOut);
}
static PythonToCppFunc is_RectD_PythonToCpp_RectD_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_RectD_Type))
        return RectD_PythonToCpp_RectD_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* RectD_PTR_CppToPython_RectD(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::RectD*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_RectD_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_RectD(PyObject* module)
{
    SbkNatronEngineTypes[SBK_RECTD_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_RectD_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "RectD", "RectD*",
        &Sbk_RectD_Type, &Shiboken::callCppDestructor< ::RectD >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_RectD_Type,
        RectD_PythonToCpp_RectD_PTR,
        is_RectD_PythonToCpp_RectD_PTR_Convertible,
        RectD_PTR_CppToPython_RectD);

    Shiboken::Conversions::registerConverterName(converter, "RectD");
    Shiboken::Conversions::registerConverterName(converter, "RectD*");
    Shiboken::Conversions::registerConverterName(converter, "RectD&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectD).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectDWrapper).name());



    RectDWrapper::pysideInitQtMetaTypes();
}
