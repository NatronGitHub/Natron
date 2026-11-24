# Regression test for #1730: SIGTERM (logout, kill, timeout) while a modal dialog is open. Natron quit
# inside the dialog's event loop, and deleting the main window deleted the dialog, which is on the stack
# of Gui::onDoDialog(): "free(): invalid pointer" (the test then crashes). The dialog must be closed
# without an answer, not with the answer of the previous question, and Natron must quit once it is closed.
import os
import signal

import NatronEngine
import NatronGuiTest
from natrontest import *

if os.name != "posix":
    NatronGuiTest.skip("Natron handles SIGTERM only on Unix")

Buttons = NatronEngine.Natron.StandardButtonEnum


def send_sigterm(title, text, class_name):
    """Called while the dialog is shown: send SIGTERM, and leave the dialog open."""
    print("dialog %r (%s) shown: sending SIGTERM" % (title, class_name))
    os.kill(os.getpid(), signal.SIGTERM)
    return None


# a first question, answered Yes: its answer must not become the answer of the next one
NatronGuiTest.setDialogAnswers(["yes"])
reply = natron.questionDialog("First question", "Answered Yes by the test.")
check(reply == Buttons.eStandardButtonYes, "the first question returned %r instead of Yes" % (reply,))

NatronGuiTest.setDialogAnswers([send_sigterm])
reply = natron.questionDialog("SIGTERM test", "This dialog is closed by SIGTERM.")
print("the dialog was closed, reply %r, dialogs %r" % (reply, NatronGuiTest.getDialogLog()))
check(reply not in (Buttons.eStandardButtonYes, Buttons.eStandardButtonNo),
      "the question closed by SIGTERM returned %r, an answer the user did not give" % (reply,))
check([d["answer"] for d in NatronGuiTest.getDialogLog()] == ["yes", "handler"],
      "unexpected dialogs: %r" % (NatronGuiTest.getDialogLog(),))
# Natron quits once the script has returned
