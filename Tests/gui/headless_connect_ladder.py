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
(Node::isNodeUpstream()) visited a node once per path, so each level about doubled the time of
a connection: with LEVELS = 30 the ladder takes more than MAX_SECONDS and the test fails. It now
visits each node once.

The nodes are Groups with two Input nodes, so that no OpenFX plug-in is needed.
"""

import time

import NatronEngine
from NatronEngine import natron

LEVELS = 30
MAX_SECONDS = 10  # the whole ladder takes well under a second


def check(condition, message=""):
    """Like assert, but not removed by python -O."""
    if not condition:
        raise AssertionError(message)


def two_input_group(app):
    """Returns a new Group node with two inputs."""
    group = app.createNode("fr.inria.built-in.Group")
    check(group is not None, "no Group node")
    second = app.createNode("fr.inria.built-in.Input", -1, group)
    check(second is not None, "no Input node")
    check(group.getMaxInputCount() == 2, group.getMaxInputCount())
    return group


status = 1
try:
    app = natron.getInstance(0)
    first = app.createNode("fr.inria.built-in.Dot")
    check(first is not None, "no Dot node")
    start = time.time()
    previous = first
    for level in range(LEVELS):
        node = two_input_group(app)
        connected = node.connectInput(0, previous)
        check(connected, level)
        connected = node.connectInput(1, previous)
        check(connected, level)
        previous = node
        # stop as soon as the time is over: the next levels would each take twice as long
        elapsed = time.time() - start
        check(elapsed < MAX_SECONDS, "connecting %d levels took %.1f s" % (level + 1, elapsed))

    # the check still refuses a cycle, through any of the paths
    check(not first.canConnectInput(0, previous), "a cycle can be connected")
    connected = first.connectInput(0, previous)
    check(not connected, "a cycle was connected")
    check(first.getInput(0) is None, "a cycle was connected")
    # and still accepts a node that is not downstream
    other = app.createNode("fr.inria.built-in.Dot")
    check(other.canConnectInput(0, previous), "a valid connection is refused")
    connected = other.connectInput(0, previous)
    check(connected, "a valid connection failed")
    check(not first.canConnectInput(0, other), "a cycle can be connected")
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
