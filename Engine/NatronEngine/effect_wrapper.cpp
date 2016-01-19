
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include <set>
#include "natronengine_python.h"

#include "effect_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <NodeWrapper.h>
#include <ParameterWrapper.h>
#include <RectD.h>
#include <RotoWrapper.h>
#include <list>
#include <map>
#include <vector>


// Native ---------------------------------------------------------

void EffectWrapper::pysideInitQtMetaTypes()
{
}

EffectWrapper::~EffectWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_EffectFunc_addUserPlane(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addUserPlane", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: addUserPlane(std::string,std::vector<std::string>)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX], (pyArgs[1])))) {
        overloadId = 0; // addUserPlane(std::string,std::vector<std::string>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_addUserPlane_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::vector<std::string > cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // addUserPlane(std::string,std::vector<std::string>)
            bool cppResult = cppSelf->addUserPlane(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_addUserPlane_TypeError:
        const char* overloads[] = {"std::string, list", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.addUserPlane", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_beginChanges(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // beginChanges()
            cppSelf->beginChanges();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_EffectFunc_canConnectInput(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "canConnectInput", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: canConnectInput(int,const Effect*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[1])))) {
        overloadId = 0; // canConnectInput(int,const Effect*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_canConnectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Effect* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // canConnectInput(int,const Effect*)const
            bool cppResult = const_cast<const ::Effect*>(cppSelf)->canConnectInput(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_canConnectInput_TypeError:
        const char* overloads[] = {"int, NatronEngine.Effect", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.canConnectInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_connectInput(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "connectInput", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: connectInput(int,const Effect*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[1])))) {
        overloadId = 0; // connectInput(int,const Effect*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_connectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Effect* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // connectInput(int,const Effect*)
            // Begin code injection

            bool cppResult = cppSelf->connectInput(cppArg0,cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_connectInput_TypeError:
        const char* overloads[] = {"int, NatronEngine.Effect", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.connectInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createChild(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // createChild()
            // Begin code injection

            Effect * cppResult = cppSelf->createChild();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], cppResult);

            // End of code injection



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

static PyObject* Sbk_EffectFunc_destroy(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Effect.destroy(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:destroy", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: destroy(bool)
    if (numArgs == 0) {
        overloadId = 0; // destroy(bool)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))) {
        overloadId = 0; // destroy(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_destroy_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "autoReconnect");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.Effect.destroy(): got multiple values for keyword argument 'autoReconnect'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0]))))
                    goto Sbk_EffectFunc_destroy_TypeError;
            }
        }
        bool cppArg0 = true;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // destroy(bool)
            // Begin code injection

            cppSelf->destroy(cppArg0);

            // End of code injection


        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_destroy_TypeError:
        const char* overloads[] = {"bool = true", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.destroy", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_disconnectInput(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: disconnectInput(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // disconnectInput(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_disconnectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // disconnectInput(int)
            // Begin code injection

            cppSelf->disconnectInput(cppArg0);

            // End of code injection


        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_disconnectInput_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.disconnectInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_endChanges(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // endChanges()
            cppSelf->endChanges();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_EffectFunc_getAvailableLayers(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAvailableLayers()const
            // Begin code injection

            std::map<ImageLayer,Effect*> comps = cppSelf->getAvailableLayers();

            PyObject* ret = PyDict_New();
            std::map<ImageLayer,Effect*>::iterator it = comps.begin();
            for (; it != comps.end(); ++it) {
                const ImageLayer& key = it->first;
                Effect* value = it->second;
                PyObject* pyKey = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &key);
                PyObject* pyValue = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], value);
                // Ownership transferences.
                Shiboken::Object::getOwnership(pyValue);
                PyDict_SetItem(ret, pyKey, pyValue);
                Py_DECREF(pyKey);
                Py_DECREF(pyValue);
            }
            return ret;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getColor(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getColor(double*,double*,double*)const
            // Begin code injection

            double r,g,b;
            cppSelf->getColor(&r,&g,&b);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &r));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &g));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &b));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getCurrentTime(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentTime()const
            int cppResult = const_cast<const ::Effect*>(cppSelf)->getCurrentTime();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getInput(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getInput(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getInput(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getInput(int)const
            Effect * cppResult = const_cast<const ::Effect*>(cppSelf)->getInput(cppArg0);
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

    Sbk_EffectFunc_getInput_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.getInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_getInputLabel(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getInputLabel(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getInputLabel(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getInputLabel_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getInputLabel(int)
            std::string cppResult = cppSelf->getInputLabel(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_getInputLabel_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.getInputLabel", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_getLabel(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLabel()const
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getLabel();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getMaxInputCount(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getMaxInputCount()const
            int cppResult = const_cast<const ::Effect*>(cppSelf)->getMaxInputCount();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getParam(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getParam(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getParam(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(std::string)const
            Param * cppResult = const_cast<const ::Effect*>(cppSelf)->getParam(cppArg0);
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

    Sbk_EffectFunc_getParam_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.getParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_getParams(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParams()const
            // Begin code injection

            std::list<Param*> params = cppSelf->getParams();
            PyObject* ret = PyList_New((int) params.size());
            int idx = 0;
            for (std::list<Param*>::iterator it = params.begin(); it!=params.end(); ++it,++idx) {
                PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], *it);
                // Ownership transferences.
                Shiboken::Object::getOwnership(item);
                PyList_SET_ITEM(ret, idx, item);
            }
            return ret;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getPluginID(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPluginID()const
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getPluginID();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getPosition(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPosition(double*,double*)const
            // Begin code injection

            double x,y;
            cppSelf->getPosition(&x,&y);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &y));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getRegionOfDefinition(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getRegionOfDefinition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getRegionOfDefinition(double,int)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // getRegionOfDefinition(double,int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getRegionOfDefinition_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getRegionOfDefinition(double,int)const
            RectD* cppResult = new RectD(const_cast<const ::Effect*>(cppSelf)->getRegionOfDefinition(cppArg0, cppArg1));
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_getRegionOfDefinition_TypeError:
        const char* overloads[] = {"float, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.getRegionOfDefinition", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_getRotoContext(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getRotoContext()const
            Roto * cppResult = const_cast<const ::Effect*>(cppSelf)->getRotoContext();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_ROTO_IDX], cppResult);

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

static PyObject* Sbk_EffectFunc_getScriptName(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getSize(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getSize(double*,double*)const
            // Begin code injection

            double x,y;
            cppSelf->getSize(&x,&y);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &y));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getUserPageParam(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getUserPageParam()const
            PageParam * cppResult = const_cast<const ::Effect*>(cppSelf)->getUserPageParam();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PAGEPARAM_IDX], cppResult);

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

static PyObject* Sbk_EffectFunc_isNodeSelected(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isNodeSelected()const
            bool cppResult = const_cast<const ::Effect*>(cppSelf)->isNodeSelected();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_setColor(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setColor", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: setColor(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // setColor(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setColor_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setColor(double,double,double)
            cppSelf->setColor(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setColor_TypeError:
        const char* overloads[] = {"float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.setColor", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_setLabel(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setLabel(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setLabel(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setLabel_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setLabel(std::string)
            // Begin code injection

            cppSelf->setLabel(cppArg0);

            // End of code injection


        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setLabel_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.setLabel", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_setPagesOrder(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setPagesOrder(std::list<std::string>)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX], (pyArg)))) {
        overloadId = 0; // setPagesOrder(std::list<std::string>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setPagesOrder_TypeError;

    // Call function/method
    {
        ::std::list<std::string > cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setPagesOrder(std::list<std::string>)
            cppSelf->setPagesOrder(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setPagesOrder_TypeError:
        const char* overloads[] = {"list", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.setPagesOrder", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_setPosition(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setPosition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setPosition(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setPosition(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setPosition_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setPosition(double,double)
            cppSelf->setPosition(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setPosition_TypeError:
        const char* overloads[] = {"float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.setPosition", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_setScriptName(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setScriptName(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setScriptName(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setScriptName_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setScriptName(std::string)
            // Begin code injection

            bool cppResult = cppSelf->setScriptName(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_setScriptName_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.setScriptName", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_setSize(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setSize", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setSize(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setSize(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setSize_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setSize(double,double)
            cppSelf->setSize(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setSize_TypeError:
        const char* overloads[] = {"float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.setSize", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_setSubGraphEditable(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setSubGraphEditable(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setSubGraphEditable(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setSubGraphEditable_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setSubGraphEditable(bool)
            cppSelf->setSubGraphEditable(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setSubGraphEditable_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.setSubGraphEditable", overloads);
        return 0;
}

static PyMethodDef Sbk_Effect_methods[] = {
    {"addUserPlane", (PyCFunction)Sbk_EffectFunc_addUserPlane, METH_VARARGS},
    {"beginChanges", (PyCFunction)Sbk_EffectFunc_beginChanges, METH_NOARGS},
    {"canConnectInput", (PyCFunction)Sbk_EffectFunc_canConnectInput, METH_VARARGS},
    {"connectInput", (PyCFunction)Sbk_EffectFunc_connectInput, METH_VARARGS},
    {"createChild", (PyCFunction)Sbk_EffectFunc_createChild, METH_NOARGS},
    {"destroy", (PyCFunction)Sbk_EffectFunc_destroy, METH_VARARGS|METH_KEYWORDS},
    {"disconnectInput", (PyCFunction)Sbk_EffectFunc_disconnectInput, METH_O},
    {"endChanges", (PyCFunction)Sbk_EffectFunc_endChanges, METH_NOARGS},
    {"getAvailableLayers", (PyCFunction)Sbk_EffectFunc_getAvailableLayers, METH_NOARGS},
    {"getColor", (PyCFunction)Sbk_EffectFunc_getColor, METH_NOARGS},
    {"getCurrentTime", (PyCFunction)Sbk_EffectFunc_getCurrentTime, METH_NOARGS},
    {"getInput", (PyCFunction)Sbk_EffectFunc_getInput, METH_O},
    {"getInputLabel", (PyCFunction)Sbk_EffectFunc_getInputLabel, METH_O},
    {"getLabel", (PyCFunction)Sbk_EffectFunc_getLabel, METH_NOARGS},
    {"getMaxInputCount", (PyCFunction)Sbk_EffectFunc_getMaxInputCount, METH_NOARGS},
    {"getParam", (PyCFunction)Sbk_EffectFunc_getParam, METH_O},
    {"getParams", (PyCFunction)Sbk_EffectFunc_getParams, METH_NOARGS},
    {"getPluginID", (PyCFunction)Sbk_EffectFunc_getPluginID, METH_NOARGS},
    {"getPosition", (PyCFunction)Sbk_EffectFunc_getPosition, METH_NOARGS},
    {"getRegionOfDefinition", (PyCFunction)Sbk_EffectFunc_getRegionOfDefinition, METH_VARARGS},
    {"getRotoContext", (PyCFunction)Sbk_EffectFunc_getRotoContext, METH_NOARGS},
    {"getScriptName", (PyCFunction)Sbk_EffectFunc_getScriptName, METH_NOARGS},
    {"getSize", (PyCFunction)Sbk_EffectFunc_getSize, METH_NOARGS},
    {"getUserPageParam", (PyCFunction)Sbk_EffectFunc_getUserPageParam, METH_NOARGS},
    {"isNodeSelected", (PyCFunction)Sbk_EffectFunc_isNodeSelected, METH_NOARGS},
    {"setColor", (PyCFunction)Sbk_EffectFunc_setColor, METH_VARARGS},
    {"setLabel", (PyCFunction)Sbk_EffectFunc_setLabel, METH_O},
    {"setPagesOrder", (PyCFunction)Sbk_EffectFunc_setPagesOrder, METH_O},
    {"setPosition", (PyCFunction)Sbk_EffectFunc_setPosition, METH_VARARGS},
    {"setScriptName", (PyCFunction)Sbk_EffectFunc_setScriptName, METH_O},
    {"setSize", (PyCFunction)Sbk_EffectFunc_setSize, METH_VARARGS},
    {"setSubGraphEditable", (PyCFunction)Sbk_EffectFunc_setSubGraphEditable, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Effect_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Effect_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
static int mi_offsets[] = { -1, -1, -1, -1, -1 };
int*
Sbk_Effect_mi_init(const void* cptr)
{
    if (mi_offsets[0] == -1) {
        std::set<int> offsets;
        std::set<int>::iterator it;
        const Effect* class_ptr = reinterpret_cast<const Effect*>(cptr);
        size_t base = (size_t) class_ptr;
        offsets.insert(((size_t) static_cast<const Group*>(class_ptr)) - base);
        offsets.insert(((size_t) static_cast<const Group*>((Effect*)((void*)class_ptr))) - base);
        offsets.insert(((size_t) static_cast<const UserParamHolder*>(class_ptr)) - base);
        offsets.insert(((size_t) static_cast<const UserParamHolder*>((Effect*)((void*)class_ptr))) - base);

        offsets.erase(0);

        int i = 0;
        for (it = offsets.begin(); it != offsets.end(); it++) {
            mi_offsets[i] = *it;
            i++;
        }
    }
    return mi_offsets;
}
static void* Sbk_EffectSpecialCastFunction(void* obj, SbkObjectType* desiredType)
{
    Effect* me = reinterpret_cast< ::Effect*>(obj);
    if (desiredType == reinterpret_cast<SbkObjectType*>(SbkNatronEngineTypes[SBK_GROUP_IDX]))
        return static_cast< ::Group*>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType*>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]))
        return static_cast< ::UserParamHolder*>(me);
    return me;
}


// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Effect_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Effect",
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
    /*tp_traverse*/         Sbk_Effect_traverse,
    /*tp_clear*/            Sbk_Effect_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Effect_methods,
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

static void* Sbk_Effect_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Group >()))
        return dynamic_cast< ::Effect*>(reinterpret_cast< ::Group*>(cptr));
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::UserParamHolder >()))
        return dynamic_cast< ::Effect*>(reinterpret_cast< ::UserParamHolder*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Effect_PythonToCpp_Effect_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Effect_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Effect_PythonToCpp_Effect_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Effect_Type))
        return Effect_PythonToCpp_Effect_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Effect_PTR_CppToPython_Effect(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Effect*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Effect_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Effect(PyObject* module)
{
    SbkNatronEngineTypes[SBK_EFFECT_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Effect_Type);

    PyObject* Sbk_Effect_Type_bases = PyTuple_Pack(2,
        (PyObject*)SbkNatronEngineTypes[SBK_GROUP_IDX],
        (PyObject*)SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Effect", "Effect*",
        &Sbk_Effect_Type, &Shiboken::callCppDestructor< ::Effect >, (SbkObjectType*)SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX], Sbk_Effect_Type_bases)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Effect_Type,
        Effect_PythonToCpp_Effect_PTR,
        is_Effect_PythonToCpp_Effect_PTR_Convertible,
        Effect_PTR_CppToPython_Effect);

    Shiboken::Conversions::registerConverterName(converter, "Effect");
    Shiboken::Conversions::registerConverterName(converter, "Effect*");
    Shiboken::Conversions::registerConverterName(converter, "Effect&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Effect).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::EffectWrapper).name());


    MultipleInheritanceInitFunction func = Sbk_Effect_mi_init;
    Shiboken::ObjectType::setMultipleIheritanceFunction(&Sbk_Effect_Type, func);
    Shiboken::ObjectType::setCastFunction(&Sbk_Effect_Type, &Sbk_EffectSpecialCastFunction);
    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_Effect_Type, &Sbk_Effect_typeDiscovery);


    EffectWrapper::pysideInitQtMetaTypes();
}
