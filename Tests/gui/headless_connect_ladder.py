# -*- coding: utf-8 -*-
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

"""Headless test: connecting an input stays fast when many paths lead to the same node.

Run by NatronRenderer -t (the UndoGroupHeadless test is run the same way). It exits with
natron.quit(status): 0 on success, 1 on failure.

The script builds a ladder: each node has its two inputs connected to the previous node, so
2^n paths lead from the n-th node to the first one. The cycle check of a connection
(Node::isNodeUpstream()) visited a node once per path: with LEVELS = 40 the last connections
would take days, and the test ends on its timeout. It now visits each node once.

The nodes are Groups with two Input nodes, so that no OpenFX plug-in is needed.
"""

import time

import NatronEngine
from NatronEngine import natron

LEVELS = 40
MAX_SECONDS = 30  # the whole ladder takes well under a second


def two_input_group(app):
    """Returns a new Group node with two inputs."""
    group = app.createNode("fr.inria.built-in.Group")
    assert group is not None
    assert app.createNode("fr.inria.built-in.Input", -1, group) is not None
    assert group.getMaxInputCount() == 2, group.getMaxInputCount()
    return group


status = 1
try:
    app = natron.getInstance(0)
    first = app.createNode("fr.inria.built-in.Dot")
    assert first is not None
    start = time.time()
    previous = first
    for level in range(LEVELS):
        node = two_input_group(app)
        assert node.connectInput(0, previous), level
        assert node.connectInput(1, previous), level
        previous = node
    elapsed = time.time() - start
    assert elapsed < MAX_SECONDS, "connecting %d levels took %.1f s" % (LEVELS, elapsed)

    # the check still refuses a cycle, through any of the paths
    assert not first.canConnectInput(0, previous)
    assert not first.connectInput(0, previous)
    assert first.getInput(0) is None
    # and still accepts a node that is not downstream
    other = app.createNode("fr.inria.built-in.Dot")
    assert other.canConnectInput(0, previous)
    assert other.connectInput(0, previous)
    assert not first.canConnectInput(0, other)
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
