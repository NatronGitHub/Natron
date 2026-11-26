"""Default node names (#1565): each new node gets the smallest free number.

Naming thousands of nodes is fast (it was cubic: building a 5000-node chain took minutes); the run
time of the loop is printed.
"""

import time

import natronheadless


def main(app):
    count = 2000
    start = time.monotonic()
    nodes = [app.createNode("fr.inria.built-in.Dot") for _ in range(count)]
    print("created %d Dot nodes in %.1f s" % (count, time.monotonic() - start))
    names = [n.getScriptName() for n in nodes]
    assert names == ["Dot%d" % (i + 1) for i in range(count)], names[:10]

    # a freed name is reused, the smallest first. The names are freed by renaming the nodes (for Effect.destroy(),
    # see headless_destroy_node.py).
    renamed10 = nodes[9].setScriptName("Renamed10")
    renamed7 = nodes[6].setScriptName("Renamed7")
    assert renamed10 and renamed7
    new_names = [app.createNode("fr.inria.built-in.Dot").getScriptName() for _ in range(3)]
    assert new_names == ["Dot7", "Dot10", "Dot%d" % (count + 1)], new_names
    # an explicit name that is taken is refused
    renamed = nodes[0].setScriptName("Dot2")
    assert not renamed
    assert nodes[0].getScriptName() == "Dot1"


natronheadless.run(main)
