
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "effect_wrapper.h"

// Extra includes
#include <NodeWrapper.h>
#include <ParameterWrapper.h>
#include <RotoWrapper.h>
#include <list>


// Native ---------------------------------------------------------

EffectWrapper::~EffectWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_EffectFunc_allowEvaluation(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // allowEvaluation()
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->allowEvaluation();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_EffectFunc_blockEvaluation(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // blockEvaluation()
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->blockEvaluation();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_EffectFunc_canSetInput(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "canSetInput", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: canSetInput(int,const Effect*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[1])))) {
        overloadId = 0; // canSetInput(int,const Effect*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_canSetInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Effect* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // canSetInput(int,const Effect*)const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            bool cppResult = const_cast<const ::Effect*>(cppSelf)->canSetInput(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_canSetInput_TypeError:
        const char* overloads[] = {"int, NatronEngine.Effect", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.canSetInput", overloads);
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            bool cppResult = cppSelf->connectInput(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
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

static PyObject* Sbk_EffectFunc_createBooleanParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createBooleanParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createBooleanParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createBooleanParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createBooleanParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createBooleanParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            BooleanParam * cppResult = cppSelf->createBooleanParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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

    Sbk_EffectFunc_createBooleanParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createBooleanParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createButtonParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createButtonParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createButtonParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createButtonParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createButtonParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createButtonParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            ButtonParam * cppResult = cppSelf->createButtonParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createButtonParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createButtonParam", overloads);
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

static PyObject* Sbk_EffectFunc_createChoiceParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createChoiceParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createChoiceParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createChoiceParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createChoiceParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createChoiceParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            ChoiceParam * cppResult = cppSelf->createChoiceParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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

    Sbk_EffectFunc_createChoiceParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createChoiceParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createColorParam(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createColorParam", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: createColorParam(std::string,std::string,bool)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[2])))) {
        overloadId = 0; // createColorParam(std::string,std::string,bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createColorParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        bool cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // createColorParam(std::string,std::string,bool)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            ColorParam * cppResult = cppSelf->createColorParam(cppArg0, cppArg1, cppArg2);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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

    Sbk_EffectFunc_createColorParam_TypeError:
        const char* overloads[] = {"std::string, std::string, bool", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createColorParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createDouble2DParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createDouble2DParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createDouble2DParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createDouble2DParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createDouble2DParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createDouble2DParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Double2DParam * cppResult = cppSelf->createDouble2DParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createDouble2DParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createDouble2DParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createDouble3DParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createDouble3DParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createDouble3DParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createDouble3DParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createDouble3DParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createDouble3DParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Double3DParam * cppResult = cppSelf->createDouble3DParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE3DPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createDouble3DParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createDouble3DParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createDoubleParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createDoubleParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createDoubleParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createDoubleParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createDoubleParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createDoubleParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            DoubleParam * cppResult = cppSelf->createDoubleParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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

    Sbk_EffectFunc_createDoubleParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createDoubleParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createFileParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createFileParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createFileParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createFileParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createFileParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createFileParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            FileParam * cppResult = cppSelf->createFileParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_FILEPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createFileParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createFileParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createGroupParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createGroupParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createGroupParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createGroupParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createGroupParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createGroupParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            GroupParam * cppResult = cppSelf->createGroupParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createGroupParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createGroupParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createInt2DParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createInt2DParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createInt2DParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createInt2DParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createInt2DParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createInt2DParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Int2DParam * cppResult = cppSelf->createInt2DParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_INT2DPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createInt2DParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createInt2DParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createInt3DParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createInt3DParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createInt3DParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createInt3DParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createInt3DParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createInt3DParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Int3DParam * cppResult = cppSelf->createInt3DParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_INT3DPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createInt3DParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createInt3DParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createIntParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createIntParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createIntParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createIntParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createIntParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createIntParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            IntParam * cppResult = cppSelf->createIntParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_INTPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createIntParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createIntParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createOutputFileParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createOutputFileParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createOutputFileParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createOutputFileParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createOutputFileParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createOutputFileParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            OutputFileParam * cppResult = cppSelf->createOutputFileParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createOutputFileParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createOutputFileParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createPageParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createPageParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createPageParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createPageParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createPageParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createPageParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            PageParam * cppResult = cppSelf->createPageParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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

    Sbk_EffectFunc_createPageParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createPageParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createParametricParam(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createParametricParam", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: createParametricParam(std::string,std::string,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // createParametricParam(std::string,std::string,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createParametricParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // createParametricParam(std::string,std::string,int)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            ParametricParam * cppResult = cppSelf->createParametricParam(cppArg0, cppArg1, cppArg2);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createParametricParam_TypeError:
        const char* overloads[] = {"std::string, std::string, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createParametricParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createPathParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createPathParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createPathParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createPathParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createPathParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createPathParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            PathParam * cppResult = cppSelf->createPathParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PATHPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createPathParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createPathParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_createStringParam(PyObject* self, PyObject* args)
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


    if (!PyArg_UnpackTuple(args, "createStringParam", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: createStringParam(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // createStringParam(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_createStringParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // createStringParam(std::string,std::string)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            StringParam * cppResult = cppSelf->createStringParam(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_STRINGPARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_createStringParam_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.createStringParam", overloads);
        return 0;
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->disconnectInput(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            int cppResult = const_cast<const ::Effect*>(cppSelf)->getCurrentTime();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Effect * cppResult = const_cast<const ::Effect*>(cppSelf)->getInput(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getLabel();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            int cppResult = const_cast<const ::Effect*>(cppSelf)->getMaxInputCount();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Param * cppResult = const_cast<const ::Effect*>(cppSelf)->getParam(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getPluginID();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Roto * cppResult = const_cast<const ::Effect*>(cppSelf)->getRotoContext();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getScriptName();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            PageParam * cppResult = const_cast<const ::Effect*>(cppSelf)->getUserPageParam();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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

static PyObject* Sbk_EffectFunc_isNull(PyObject* self)
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
            // isNull()const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            bool cppResult = const_cast<const ::Effect*>(cppSelf)->isNull();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_refreshUserParamsGUI(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // refreshUserParamsGUI()
            // Begin code injection

            cppSelf->refreshUserParamsGUI();

            // End of code injection


        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->setColor(cppArg0, cppArg1, cppArg2);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->setPosition(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->setSize(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
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

static PyMethodDef Sbk_Effect_methods[] = {
    {"allowEvaluation", (PyCFunction)Sbk_EffectFunc_allowEvaluation, METH_NOARGS},
    {"blockEvaluation", (PyCFunction)Sbk_EffectFunc_blockEvaluation, METH_NOARGS},
    {"canSetInput", (PyCFunction)Sbk_EffectFunc_canSetInput, METH_VARARGS},
    {"connectInput", (PyCFunction)Sbk_EffectFunc_connectInput, METH_VARARGS},
    {"createBooleanParam", (PyCFunction)Sbk_EffectFunc_createBooleanParam, METH_VARARGS},
    {"createButtonParam", (PyCFunction)Sbk_EffectFunc_createButtonParam, METH_VARARGS},
    {"createChild", (PyCFunction)Sbk_EffectFunc_createChild, METH_NOARGS},
    {"createChoiceParam", (PyCFunction)Sbk_EffectFunc_createChoiceParam, METH_VARARGS},
    {"createColorParam", (PyCFunction)Sbk_EffectFunc_createColorParam, METH_VARARGS},
    {"createDouble2DParam", (PyCFunction)Sbk_EffectFunc_createDouble2DParam, METH_VARARGS},
    {"createDouble3DParam", (PyCFunction)Sbk_EffectFunc_createDouble3DParam, METH_VARARGS},
    {"createDoubleParam", (PyCFunction)Sbk_EffectFunc_createDoubleParam, METH_VARARGS},
    {"createFileParam", (PyCFunction)Sbk_EffectFunc_createFileParam, METH_VARARGS},
    {"createGroupParam", (PyCFunction)Sbk_EffectFunc_createGroupParam, METH_VARARGS},
    {"createInt2DParam", (PyCFunction)Sbk_EffectFunc_createInt2DParam, METH_VARARGS},
    {"createInt3DParam", (PyCFunction)Sbk_EffectFunc_createInt3DParam, METH_VARARGS},
    {"createIntParam", (PyCFunction)Sbk_EffectFunc_createIntParam, METH_VARARGS},
    {"createOutputFileParam", (PyCFunction)Sbk_EffectFunc_createOutputFileParam, METH_VARARGS},
    {"createPageParam", (PyCFunction)Sbk_EffectFunc_createPageParam, METH_VARARGS},
    {"createParametricParam", (PyCFunction)Sbk_EffectFunc_createParametricParam, METH_VARARGS},
    {"createPathParam", (PyCFunction)Sbk_EffectFunc_createPathParam, METH_VARARGS},
    {"createStringParam", (PyCFunction)Sbk_EffectFunc_createStringParam, METH_VARARGS},
    {"destroy", (PyCFunction)Sbk_EffectFunc_destroy, METH_VARARGS|METH_KEYWORDS},
    {"disconnectInput", (PyCFunction)Sbk_EffectFunc_disconnectInput, METH_O},
    {"getColor", (PyCFunction)Sbk_EffectFunc_getColor, METH_NOARGS},
    {"getCurrentTime", (PyCFunction)Sbk_EffectFunc_getCurrentTime, METH_NOARGS},
    {"getInput", (PyCFunction)Sbk_EffectFunc_getInput, METH_O},
    {"getLabel", (PyCFunction)Sbk_EffectFunc_getLabel, METH_NOARGS},
    {"getMaxInputCount", (PyCFunction)Sbk_EffectFunc_getMaxInputCount, METH_NOARGS},
    {"getParam", (PyCFunction)Sbk_EffectFunc_getParam, METH_O},
    {"getParams", (PyCFunction)Sbk_EffectFunc_getParams, METH_NOARGS},
    {"getPluginID", (PyCFunction)Sbk_EffectFunc_getPluginID, METH_NOARGS},
    {"getPosition", (PyCFunction)Sbk_EffectFunc_getPosition, METH_NOARGS},
    {"getRotoContext", (PyCFunction)Sbk_EffectFunc_getRotoContext, METH_NOARGS},
    {"getScriptName", (PyCFunction)Sbk_EffectFunc_getScriptName, METH_NOARGS},
    {"getSize", (PyCFunction)Sbk_EffectFunc_getSize, METH_NOARGS},
    {"getUserPageParam", (PyCFunction)Sbk_EffectFunc_getUserPageParam, METH_NOARGS},
    {"isNull", (PyCFunction)Sbk_EffectFunc_isNull, METH_NOARGS},
    {"refreshUserParamsGUI", (PyCFunction)Sbk_EffectFunc_refreshUserParamsGUI, METH_NOARGS},
    {"setColor", (PyCFunction)Sbk_EffectFunc_setColor, METH_VARARGS},
    {"setLabel", (PyCFunction)Sbk_EffectFunc_setLabel, METH_O},
    {"setPosition", (PyCFunction)Sbk_EffectFunc_setPosition, METH_VARARGS},
    {"setScriptName", (PyCFunction)Sbk_EffectFunc_setScriptName, METH_O},
    {"setSize", (PyCFunction)Sbk_EffectFunc_setSize, METH_VARARGS},

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

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Effect", "Effect*",
        &Sbk_Effect_Type, &Shiboken::callCppDestructor< ::Effect >, (SbkObjectType*)SbkNatronEngineTypes[SBK_GROUP_IDX])) {
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


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_Effect_Type, &Sbk_Effect_typeDiscovery);


}
