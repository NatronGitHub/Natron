# Regression test for #1554: natron.quit() with an empty project. The instance used to quit, and delete
# its window, inside Gui::abortProject(), which then wrote to the deleted window. The test passes only if
# Natron then exits cleanly.
import NatronGuiTest
from natrontest import *

for n in app.getChildren():
    n.destroy(False)
wait_idle()
check(not app.getChildren(), "the project still has nodes")
natron.quit(0)
print("natron.quit() returned")
