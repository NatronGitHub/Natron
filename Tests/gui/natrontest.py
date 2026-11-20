# Helpers shared by the GUI tests (Natron --test test_*.py); see Documentation/source/devel/guitesting.rst.
# The test script directory is on sys.path, so a test does "from natrontest import *".
import ctypes
import os

import NatronGui
import NatronGuiTest

natron = NatronGui.natron

# In CI, the GUI tests only run where the job enables them (NATRON_GUI_TESTS=1): Natron --test skips
# them itself (exit status 77) before it starts.

GENERATOR = "net.natron.test.Generator"  # fills its output with 0.5 (Tests/OfxTestPlugin)


def require_plugin(pluginID):
    """Skip the test if the plug-in is not available (e.g. openfx-misc is not on OFX_PLUGIN_PATH)."""
    if pluginID not in natron.getPluginIDs():
        NatronGuiTest.skip("plug-in %s not found (OFX_PLUGIN_PATH=%s)" % (pluginID, os.environ.get("OFX_PLUGIN_PATH", "")))


def test_plugin_counter(name):
    """A counter of the test plug-in (see Tests/OfxTestPlugin/NatronTestPlugin.cpp), e.g. "Generator.render"."""
    path = os.environ.get("NATRON_TESTS_OFX_PLUGIN_BINARY")
    if not path or not os.path.exists(path):
        NatronGuiTest.skip("NATRON_TESTS_OFX_PLUGIN_BINARY is not set")
    lib = ctypes.CDLL(path)  # the library that Natron loaded: same counters
    lib.NatronTestPluginGetCounter.restype = ctypes.c_int
    lib.NatronTestPluginGetCounter.argtypes = [ctypes.c_char_p]
    return lib.NatronTestPluginGetCounter(name.encode("utf-8"))


def wait_idle(timeout=30.0):
    """Wait until no render runs and the events are processed; fail on timeout."""
    check(NatronGuiTest.waitIdle(timeout), "the GUI was not idle after %g s" % timeout)


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def check_close(value, expected, tol, what):
    check(abs(value - expected) <= tol, "%s: %r, expected %r +/- %r" % (what, value, expected, tol))


def same_node(a, b):
    """Whether two Effect objects are the same node (each getNode()/getInput() call makes a new Effect)."""
    if a is None or b is None:
        return a is None and b is None
    return a.getScriptName() == b.getScriptName()


def viewer_node(app):
    v = app.getNode("Viewer1")
    check(v is not None, "no Viewer1 in the project")
    return v


def rod_center(node):
    """The centre of the region of definition of node at frame 1, in canvas coordinates."""
    rod = node.getRegionOfDefinition(1, 0)
    return ((rod.left() + rod.right()) / 2.0, (rod.bottom() + rod.top()) / 2.0)


def fit_viewer(name="Viewer1"):
    """Fit the image in the viewer (the F key)."""
    NatronGuiTest.sendKey(name, "F")
    wait_idle()


def srgb8(linear):
    """The 8-bit sRGB value of a linear value, as displayed by the viewer."""
    if linear <= 0.0031308:
        v = 12.92 * linear
    else:
        v = 1.055 * linear ** (1.0 / 2.4) - 0.055
    return int(round(v * 255.0))
