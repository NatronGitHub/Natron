# A backdrop keeps the size the user gives it when its label changes: the label wraps to the
# backdrop's width (#626).
from natrontest import *

BACKDROP = "fr.inria.built-in.BackDrop"

bd = app.createNode(BACKDROP)
check(bd is not None, "createNode(%s) failed" % BACKDROP)
wait_idle()
label = bd.getParam("Label")
check(label is not None, "no Label parameter on %s" % bd.getScriptName())

bd.setSize(300, 200)
wait_idle()
size = bd.getSize()
check_close(size[0], 300, 0.5, "width after setSize()")
check_close(size[1], 200, 0.5, "height after setSize()")

# a long label in a big font: the backdrop used to grow to fit it, and never shrink back
label.setValue("<font size=\"7\">" + " ".join(["a long sticky note"] * 20) + "</font>")
wait_idle()
size = bd.getSize()
check_close(size[0], 300, 0.5, "width after a long label")
check_close(size[1], 200, 0.5, "height after a long label")

# the user can still make it smaller than its label
bd.setSize(150, 100)
wait_idle()
size = bd.getSize()
check_close(size[0], 150, 0.5, "width after setSize() smaller than the label")
check_close(size[1], 100, 0.5, "height after setSize() smaller than the label")

# but not to a negative width
bd.setSize(-10, 100)
wait_idle()
check(bd.getSize()[0] > 0, "width after setSize(-10, 100): %r" % (bd.getSize(),))
