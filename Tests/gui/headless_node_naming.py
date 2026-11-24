# Default node names (#1565): each new node gets the smallest free number, and naming thousands of nodes
# is fast (it was cubic: building a 5000-node chain took minutes). Run by NatronRenderer -t (see
# Tests/CMakeLists.txt); the run time of the loop is printed.
import time

from NatronEngine import natron

status = 1
try:
    app = natron.getInstance(0)
    count = 2000
    start = time.time()
    nodes = [app.createNode("fr.inria.built-in.Dot") for _ in range(count)]
    print("created %d Dot nodes in %.1f s" % (count, time.time() - start))
    names = [n.getScriptName() for n in nodes]
    assert names == ["Dot%d" % (i + 1) for i in range(count)], names[:10]

    # a freed name is reused, the smallest first
    nodes[9].destroy()
    nodes[6].destroy()
    assert app.createNode("fr.inria.built-in.Dot").getScriptName() == "Dot7"
    assert app.createNode("fr.inria.built-in.Dot").getScriptName() == "Dot10"
    assert app.createNode("fr.inria.built-in.Dot").getScriptName() == "Dot%d" % (count + 1)
    # an explicit name that is taken is refused
    assert not nodes[0].setScriptName("Dot2")
    assert nodes[0].getScriptName() == "Dot1"
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
