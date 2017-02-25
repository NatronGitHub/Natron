
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

#include "pyoverlayinteract_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyNode.h>
#include <PyOverlayInteract.h>
#include <PyParameter.h>
#include <map>


// Native ---------------------------------------------------------

void PyOverlayInteractWrapper::pysideInitQtMetaTypes()
{
}

PyOverlayInteractWrapper::PyOverlayInteractWrapper() : PyOverlayInteract() {
    // ... middle
}

std::map<QString, PyOverlayParamDesc > PyOverlayInteractWrapper::describeParameters() const
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ::std::map<QString, PyOverlayParamDesc >();
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "describeParameters"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::PyOverlayInteract::describeParameters();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ::std::map<QString, PyOverlayParamDesc >();
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_PYOVERLAYPARAMDESC_IDX], pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyOverlayInteract.describeParameters", "map", pyResult->ob_type->tp_name);
        return ::std::map<QString, PyOverlayParamDesc >();
    }
    ::std::map<QString, PyOverlayParamDesc > cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyOverlayInteractWrapper::draw(double time, const Double2DTuple & renderScale, QString view)
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ;
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "draw"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::PyOverlayInteract::draw(time, renderScale, view);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(dNN)",
        time,
        Shiboken::Conversions::referenceToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], &renderScale),
        Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &view)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ;
    }
}

void PyOverlayInteractWrapper::fetchParameters(const std::map<QString, QString > & params)
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ;
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "fetchParameters"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::PyOverlayInteract::fetchParameters(params);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
        Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_QSTRING_IDX], &params)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ;
    }
}

PyOverlayInteractWrapper::~PyOverlayInteractWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_PyOverlayInteract_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::PyOverlayInteract >()))
        return -1;

    ::PyOverlayInteractWrapper* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // PyOverlayInteract()
            cptr = new ::PyOverlayInteractWrapper();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::PyOverlayInteract >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyObject* Sbk_PyOverlayInteractFunc_describeParameters(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // describeParameters()const
            std::map<QString, PyOverlayParamDesc > cppResult = Shiboken::Object::hasCppWrapper(reinterpret_cast<SbkObject*>(self)) ? const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->::PyOverlayInteract::describeParameters() : const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->describeParameters();
            pyResult = Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_PYOVERLAYPARAMDESC_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyOverlayInteractFunc_draw(PyObject* self, PyObject* args)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "draw", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: draw(double,Double2DTuple,QString)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
        overloadId = 0; // draw(double,Double2DTuple,QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyOverlayInteractFunc_draw_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Double2DTuple* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = ::QString();
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // draw(double,Double2DTuple,QString)
            Shiboken::Object::hasCppWrapper(reinterpret_cast<SbkObject*>(self)) ? cppSelf->::PyOverlayInteract::draw(cppArg0, *cppArg1, cppArg2) : cppSelf->draw(cppArg0, *cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyOverlayInteractFunc_draw_TypeError:
        const char* overloads[] = {"float, NatronEngine.Double2DTuple, unicode", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.PyOverlayInteract.draw", overloads);
        return 0;
}

static PyObject* Sbk_PyOverlayInteractFunc_fetchParameter(PyObject* self, PyObject* args)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "fetchParameter", 5, 5, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: fetchParameter(std::map<QString,QString>,QString,QString,int,bool)const
    if (numArgs == 5
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[4])))) {
        overloadId = 0; // fetchParameter(std::map<QString,QString>,QString,QString,int,bool)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyOverlayInteractFunc_fetchParameter_TypeError;

    // Call function/method
    {
        ::std::map<QString, QString > cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = ::QString();
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = ::QString();
        pythonToCpp[2](pyArgs[2], &cppArg2);
        int cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        bool cppArg4;
        pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // fetchParameter(std::map<QString,QString>,QString,QString,int,bool)const
            Param * cppResult = ((::PyOverlayInteractWrapper*) cppSelf)->PyOverlayInteractWrapper::fetchParameter_protected(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_PyOverlayInteractFunc_fetchParameter_TypeError:
        const char* overloads[] = {"dict, unicode, unicode, int, bool", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.PyOverlayInteract.fetchParameter", overloads);
        return 0;
}

static PyObject* Sbk_PyOverlayInteractFunc_fetchParameters(PyObject* self, PyObject* pyArg)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: fetchParameters(std::map<QString,QString>)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // fetchParameters(std::map<QString,QString>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyOverlayInteractFunc_fetchParameters_TypeError;

    // Call function/method
    {
        ::std::map<QString, QString > cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // fetchParameters(std::map<QString,QString>)
            Shiboken::Object::hasCppWrapper(reinterpret_cast<SbkObject*>(self)) ? cppSelf->::PyOverlayInteract::fetchParameters(cppArg0) : cppSelf->fetchParameters(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyOverlayInteractFunc_fetchParameters_TypeError:
        const char* overloads[] = {"dict", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.PyOverlayInteract.fetchParameters", overloads);
        return 0;
}

static PyObject* Sbk_PyOverlayInteractFunc_getColorPicker(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getColorPicker()const
            ColorTuple* cppResult = new ColorTuple(const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->getColorPicker());
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyOverlayInteractFunc_getHoldingEffect(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getHoldingEffect()const
            Effect * cppResult = const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->getHoldingEffect();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], cppResult);

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

static PyObject* Sbk_PyOverlayInteractFunc_getPixelScale(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPixelScale()const
            Double2DTuple* cppResult = new Double2DTuple(const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->getPixelScale());
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyOverlayInteractFunc_getSuggestedColor(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getSuggestedColor(ColorTuple*)const
            // Begin code injection

            ColorTuple ret;
            bool ok = cppSelf->getSuggestedColor(&ret);
            PyObject* pyret = PyTuple_New(5);
            PyTuple_SET_ITEM(pyret, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &ok));
            PyTuple_SET_ITEM(pyret, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.r));
            PyTuple_SET_ITEM(pyret, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.g));
            PyTuple_SET_ITEM(pyret, 3, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.b));
            PyTuple_SET_ITEM(pyret, 4, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret.a));
            return pyret;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyOverlayInteractFunc_getViewportSize(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getViewportSize()const
            Int2DTuple* cppResult = new Int2DTuple(const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->getViewportSize());
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_INT2DTUPLE_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyOverlayInteractFunc_initInternalInteract(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // initInternalInteract()
            ((::PyOverlayInteractWrapper*) cppSelf)->PyOverlayInteractWrapper::initInternalInteract_protected();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyOverlayInteractFunc_isColorPickerRequired(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isColorPickerRequired()const
            bool cppResult = const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->isColorPickerRequired();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyOverlayInteractFunc_isColorPickerValid(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isColorPickerValid()const
            bool cppResult = const_cast<const ::PyOverlayInteractWrapper*>(cppSelf)->isColorPickerValid();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyOverlayInteractFunc_redraw(PyObject* self)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // redraw()
            cppSelf->redraw();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyOverlayInteractFunc_renderText(PyObject* self, PyObject* args)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "renderText", 7, 7, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4]), &(pyArgs[5]), &(pyArgs[6])))
        return 0;


    // Overloaded function decisor
    // 0: renderText(double,double,QString,double,double,double,double)
    if (numArgs == 7
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[4])))
        && (pythonToCpp[5] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[5])))
        && (pythonToCpp[6] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[6])))) {
        overloadId = 0; // renderText(double,double,QString,double,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyOverlayInteractFunc_renderText_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = ::QString();
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        double cppArg4;
        pythonToCpp[4](pyArgs[4], &cppArg4);
        double cppArg5;
        pythonToCpp[5](pyArgs[5], &cppArg5);
        double cppArg6;
        pythonToCpp[6](pyArgs[6], &cppArg6);

        if (!PyErr_Occurred()) {
            // renderText(double,double,QString,double,double,double,double)
            cppSelf->renderText(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4, cppArg5, cppArg6);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyOverlayInteractFunc_renderText_TypeError:
        const char* overloads[] = {"float, float, unicode, float, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.PyOverlayInteract.renderText", overloads);
        return 0;
}

static PyObject* Sbk_PyOverlayInteractFunc_setColorPickerEnabled(PyObject* self, PyObject* pyArg)
{
    PyOverlayInteractWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyOverlayInteractWrapper*)((::PyOverlayInteract*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setColorPickerEnabled(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setColorPickerEnabled(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyOverlayInteractFunc_setColorPickerEnabled_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setColorPickerEnabled(bool)
            cppSelf->setColorPickerEnabled(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyOverlayInteractFunc_setColorPickerEnabled_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.PyOverlayInteract.setColorPickerEnabled", overloads);
        return 0;
}

static PyMethodDef Sbk_PyOverlayInteract_methods[] = {
    {"describeParameters", (PyCFunction)Sbk_PyOverlayInteractFunc_describeParameters, METH_NOARGS},
    {"draw", (PyCFunction)Sbk_PyOverlayInteractFunc_draw, METH_VARARGS},
    {"fetchParameter", (PyCFunction)Sbk_PyOverlayInteractFunc_fetchParameter, METH_VARARGS},
    {"fetchParameters", (PyCFunction)Sbk_PyOverlayInteractFunc_fetchParameters, METH_O},
    {"getColorPicker", (PyCFunction)Sbk_PyOverlayInteractFunc_getColorPicker, METH_NOARGS},
    {"getHoldingEffect", (PyCFunction)Sbk_PyOverlayInteractFunc_getHoldingEffect, METH_NOARGS},
    {"getPixelScale", (PyCFunction)Sbk_PyOverlayInteractFunc_getPixelScale, METH_NOARGS},
    {"getSuggestedColor", (PyCFunction)Sbk_PyOverlayInteractFunc_getSuggestedColor, METH_NOARGS},
    {"getViewportSize", (PyCFunction)Sbk_PyOverlayInteractFunc_getViewportSize, METH_NOARGS},
    {"initInternalInteract", (PyCFunction)Sbk_PyOverlayInteractFunc_initInternalInteract, METH_NOARGS},
    {"isColorPickerRequired", (PyCFunction)Sbk_PyOverlayInteractFunc_isColorPickerRequired, METH_NOARGS},
    {"isColorPickerValid", (PyCFunction)Sbk_PyOverlayInteractFunc_isColorPickerValid, METH_NOARGS},
    {"redraw", (PyCFunction)Sbk_PyOverlayInteractFunc_redraw, METH_NOARGS},
    {"renderText", (PyCFunction)Sbk_PyOverlayInteractFunc_renderText, METH_VARARGS},
    {"setColorPickerEnabled", (PyCFunction)Sbk_PyOverlayInteractFunc_setColorPickerEnabled, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_PyOverlayInteract_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_PyOverlayInteract_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_PyOverlayInteract_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.PyOverlayInteract",
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
    /*tp_traverse*/         Sbk_PyOverlayInteract_traverse,
    /*tp_clear*/            Sbk_PyOverlayInteract_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_PyOverlayInteract_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_PyOverlayInteract_Init,
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
static void PyOverlayInteract_PythonToCpp_PyOverlayInteract_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_PyOverlayInteract_Type, pyIn, cppOut);
}
static PythonToCppFunc is_PyOverlayInteract_PythonToCpp_PyOverlayInteract_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_PyOverlayInteract_Type))
        return PyOverlayInteract_PythonToCpp_PyOverlayInteract_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* PyOverlayInteract_PTR_CppToPython_PyOverlayInteract(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::PyOverlayInteract*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_PyOverlayInteract_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_PyOverlayInteract(PyObject* module)
{
    SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_PyOverlayInteract_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "PyOverlayInteract", "PyOverlayInteract*",
        &Sbk_PyOverlayInteract_Type, &Shiboken::callCppDestructor< ::PyOverlayInteract >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_PyOverlayInteract_Type,
        PyOverlayInteract_PythonToCpp_PyOverlayInteract_PTR,
        is_PyOverlayInteract_PythonToCpp_PyOverlayInteract_PTR_Convertible,
        PyOverlayInteract_PTR_CppToPython_PyOverlayInteract);

    Shiboken::Conversions::registerConverterName(converter, "PyOverlayInteract");
    Shiboken::Conversions::registerConverterName(converter, "PyOverlayInteract*");
    Shiboken::Conversions::registerConverterName(converter, "PyOverlayInteract&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyOverlayInteract).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyOverlayInteractWrapper).name());



    PyOverlayInteractWrapper::pysideInitQtMetaTypes();
}
