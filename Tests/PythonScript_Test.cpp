/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript() with sys.stdout and sys.stderr captured, as in GUI
// sessions (AppManager::initPython()): text that was already in the captured sys.stderr made the next
// script fail, even a valid one (#1436). The success of a script must only depend on its own result,
// text written outside of it must be forwarded to the Script Editor rather than lost, and a nested call
// (a script that calls the Natron API, which runs another script) must not take the output of the
// calling script.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Global/PythonUtils.h"

NATRON_NAMESPACE_USING

namespace {
class PythonScriptTest
    : public ::testing::Test
{
protected:
    virtual void SetUp() OVERRIDE
    {
        // The Tests are a background session: capture sys.stdout and sys.stderr as in a GUI session.
        runSimple("import sys\n"
                  "class StreamCatcher:\n"
                  "   def __init__(self):\n"
                  "       self.value = ''\n"
                  "   def write(self,txt):\n"
                  "       self.value += txt\n"
                  "   def clear(self):\n"
                  "       self.value = ''\n"
                  "natronTestSavedStdout = sys.stdout\n"
                  "natronTestSavedStderr = sys.stderr\n"
                  "catchOut = StreamCatcher()\n"
                  "catchErr = StreamCatcher()\n"
                  "sys.stdout = catchOut\n"
                  "sys.stderr = catchErr\n");
    }

    virtual void TearDown() OVERRIDE
    {
        runSimple("import sys\n"
                  "sys.stdout = natronTestSavedStdout\n"
                  "sys.stderr = natronTestSavedStderr\n"
                  "del catchOut, catchErr, natronTestSavedStdout, natronTestSavedStderr\n");
    }

    // Runs Python code outside of interpretPythonScript(), like a PySide slot
    static void runSimple(const char* script)
    {
        PythonGILLocker pgl;

        ASSERT_EQ( 0, PyRun_SimpleString(script) );
    }

    // The text in the captured sys.stderr
    static std::string caughtStderr()
    {
        PythonGILLocker pgl;
        std::string ret;
        PyObject* catcher = PyObject_GetAttrString(NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::getMainModule(), "catchErr");

        if (catcher) {
            PyObject* value = PyObject_GetAttrString(catcher, "value");
            if (value) {
                ret = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyStringToStdString(value);
                Py_DECREF(value);
            }
            Py_DECREF(catcher);
        }
        PyErr_Clear();

        return ret;
    }
};
} // anon namespace

// A traceback left in sys.stderr by code run outside of interpretPythonScript() must not make the next,
// valid, script fail, and it must be forwarded (here, to stdout by AppInstance::appendToScriptEditor()).
TEST_F(PythonScriptTest, StaleStderrIsNotAnError)
{
    runSimple("import sys\n"
              "sys.stderr.write('Traceback (most recent call last):\\nValueError: stale-1436\\n')\n");
    ASSERT_NE( std::string::npos, caughtStderr().find("stale-1436") );

    std::string err, output;
    testing::internal::CaptureStdout();
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("natronTest1436 = 1\nprint('out-1436')\n", &err, &output);
    std::string forwarded = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(ok) << err;
    EXPECT_TRUE( err.empty() ) << err;
    EXPECT_EQ(std::string("out-1436\n"), output);
    EXPECT_NE( std::string::npos, forwarded.find("stale-1436") ) << forwarded;
    EXPECT_TRUE( caughtStderr().empty() );

    // and the next script succeeds too
    ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("assert natronTest1436 == 1\n", &err, 0);
    EXPECT_TRUE(ok) << err;
}

// A failing script is still reported, with its traceback, and does not leave anything behind.
TEST_F(PythonScriptTest, ExceptionIsAnError)
{
    std::string err;
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("raise ValueError('boom-1436')\n", &err, 0);

    EXPECT_FALSE(ok);
    EXPECT_NE( std::string::npos, err.find("boom-1436") ) << err;
    EXPECT_NE( std::string::npos, err.find("Traceback") ) << err;
    EXPECT_TRUE( caughtStderr().empty() );

    ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("pass\n", &err, 0);
    EXPECT_TRUE(ok) << err;
}

// Text that a successful script writes to sys.stderr (e.g. a warning) does not make it fail, and is
// forwarded instead of being left for the next script.
TEST_F(PythonScriptTest, StderrOfSuccessfulScript)
{
    std::string err;
    testing::internal::CaptureStdout();
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nsys.stderr.write('warning-1436\\n')\n", &err, 0);
    std::string forwarded = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(ok) << err;
    EXPECT_NE( std::string::npos, forwarded.find("warning-1436") ) << forwarded;
    EXPECT_TRUE( caughtStderr().empty() );
}

// When the caller asks for the output, the stderr of a successful script goes with it (the Script Editor
// shows it after the script, with its result).
TEST_F(PythonScriptTest, StderrGoesWithRequestedOutput)
{
    std::string err, output;
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nprint('out-1436')\nsys.stderr.write('warning-1436\\n')\n", &err, &output);

    EXPECT_TRUE(ok) << err;
    EXPECT_EQ(std::string("out-1436\nwarning-1436\n"), output);
    EXPECT_TRUE( caughtStderr().empty() );
}

// Text that cannot be encoded to UTF-8 (a lone surrogate, e.g. print() of a non-UTF-8 file name
// decoded with surrogateescape) must still be taken from the buffer: it used to stay there, and every
// later output was lost.
TEST_F(PythonScriptTest, UnencodableOutput)
{
    runSimple("import sys\n"
              "sys.stdout.write('bad\\udcff-1436\\n')\n");

    std::string err, output;
    testing::internal::CaptureStdout();
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("print('bad\\udcfe-1436')\n", &err, &output);
    std::string forwarded = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(ok) << err;
    EXPECT_EQ(std::string("bad\\udcfe-1436\n"), output);
    EXPECT_NE( std::string::npos, forwarded.find("bad\\udcff-1436") ) << forwarded;

    // and the next output is not lost
    ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("print('next-1436')\n", &err, &output);
    EXPECT_TRUE(ok) << err;
    EXPECT_EQ(std::string("next-1436\n"), output);
}

// A caller that does not ask for the error still gets false, and the traceback is not lost.
TEST_F(PythonScriptTest, ExceptionWithoutErrorString)
{
    testing::internal::CaptureStdout();
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("raise ValueError('unasked-1436')\n", 0, 0);
    std::string forwarded = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(ok);
    EXPECT_NE( std::string::npos, forwarded.find("unasked-1436") ) << forwarded;
    EXPECT_TRUE( caughtStderr().empty() );
}

// A script that calls the Natron API, which runs another script through interpretPythonScript()
// (createNode() declares the node variable): the nested script must succeed although the calling script
// already wrote to sys.stderr, and the calling script must get all of its own stderr back.
TEST_F(PythonScriptTest, NestedCall)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    const std::string appID = app->getAppIDString();
    std::string err;
    std::string script =
        "import sys\n"
        "sys.stderr.write('outer-before-1436\\n')\n"
        "natronTestNode1436 = " + appID + ".createNode('fr.inria.built-in.Dot')\n"
        "natronTestNested1436 = getattr(" + appID + ", natronTestNode1436.getScriptName(), None) is not None\n"
        "sys.stderr.write('outer-after-1436\\n')\n"
        "raise RuntimeError('outer-1436')\n";

    testing::internal::CaptureStdout();
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    std::string forwarded = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(ok);
    EXPECT_NE( std::string::npos, err.find("outer-before-1436") ) << err;
    EXPECT_NE( std::string::npos, err.find("outer-after-1436") ) << err;
    EXPECT_NE( std::string::npos, err.find("outer-1436") ) << err;
    EXPECT_EQ( std::string::npos, forwarded.find("outer-before-1436") ) << forwarded;
    EXPECT_TRUE( caughtStderr().empty() );

    // the nested script declared the node variable
    ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("assert natronTestNested1436\n"
                                                        "natronTestNode1436.destroy()\n"
                                                        "del natronTestNode1436, natronTestNested1436\n", &err, 0);
    EXPECT_TRUE(ok) << err;
}
