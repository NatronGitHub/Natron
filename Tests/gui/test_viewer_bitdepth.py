# The viewer shows the same image with 8-bit and 32-bit float textures (#1120): the first upload
# after a change of the "texturesBitDepth" setting must be displayed.
import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)

viewer = viewer_node(app)
gen = app.createNode(GENERATOR)
viewer.connectInput(0, gen)
fit_viewer()
x, y = rod_center(gen)
mode = natron.getSettings().getParam("texturesBitDepth")
check(mode is not None, "no texturesBitDepth setting")
expected = srgb8(0.5)

for m in (0, 1, 0, 1):
    # as the Preferences do (Settings::saveSettings()): set the mode, then render the viewers
    mode.setValue(m)
    app.getViewer("Viewer1").renderCurrentFrame(True)
    wait_idle()
    r, g, b, a = NatronGuiTest.getDisplayedPixel("Viewer1", x, y)
    print("texturesBitDepth %s (%s): displayed %r" % (m, mode.getOption(m), (r, g, b, a)))
    check_close(r, expected, 3, "displayed red with texturesBitDepth %d" % m)
    check_close(g, expected, 3, "displayed green with texturesBitDepth %d" % m)
