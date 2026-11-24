# Regression test for #1730: SIGTERM (logout, kill, timeout) while a modal dialog is open. Natron quit
# inside the dialog's event loop, and deleting the main window deleted the dialog, which is on the stack
# of Gui::onDoDialog(): "free(): invalid pointer". The dialog must be closed, and Natron must quit once
# it is. The test passes only if Natron then exits cleanly.
import os
import signal
import sys

import NatronGuiTest
from natrontest import *

if os.name != "posix":
    NatronGuiTest.skip("Natron handles SIGTERM only on Unix")


def send_sigterm(title, text, className):
    """Called while the dialog is shown: send SIGTERM, and leave the dialog open."""
    print("dialog %r (%s) shown: sending SIGTERM" % (title, className))
    os.kill(os.getpid(), signal.SIGTERM)
    return None


NatronGuiTest.setDialogAnswers([send_sigterm])
natron.informationDialog("SIGTERM test", "This dialog is closed by SIGTERM.")
print("the dialog was closed: %r" % NatronGuiTest.getDialogLog())
# Natron quits once the script has returned
