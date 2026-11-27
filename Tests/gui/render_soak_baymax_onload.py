# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2026 The Natron developers
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

"""Run by NatronRenderer -l on a BayMax project of Natron-Tests, for the render soak test (run_render_soak_test.py).

It inserts a Grade by SOAK_RUN_GAIN ("SoakRunGain<n>") after each Read node, so that the images change slightly from
run to run, and adds the WriteOIIO "SoakWrite" that renders SOAK_OUTPUT_NODE to SOAK_OUTPUT_FILE, frames SOAK_FIRST to
SOAK_LAST, with the JPEG quality SOAK_QUALITY, as Natron-Tests' runTests.sh does, after another such Grade
("SoakRunGainOut", rendered at every frame).
"""

import os

GAIN = float(os.environ["SOAK_RUN_GAIN"])


def children(group):
    """Every node of group, and of the groups inside it."""
    for node in group.getChildren():
        yield node
        if node.getPluginID() == "fr.inria.built-in.Group":
            for child in children(node):
                yield child


def reconnect(node, i, source):
    """Connects input i of node to source (connectInput() refuses an input that is connected)."""
    node.disconnectInput(i)
    if not node.connectInput(i, source):
        raise RuntimeError("could not connect %s to input %d of %s" % (source.getScriptName(), i, node.getScriptName()))


def insert_run_gains():
    """Inserts a Grade by GAIN after each Read node; returns (Grade nodes, connections to them)."""
    nodes = list(children(app))  # pylint: disable=undefined-variable  (app: the project, set by Natron)
    reads = [n for n in nodes if n.getPluginID() == "fr.inria.built-in.Read"]
    connections = 0
    for index, read in enumerate(reads):
        group = read.getContainerGroup()
        grade = app.createNode("net.sf.openfx.GradePlugin", -1, group)  # pylint: disable=undefined-variable
        grade.setScriptName("SoakRunGain%d" % index)
        grade.getParam("multiply").set(GAIN, GAIN, GAIN, GAIN)
        for node in nodes:
            for i in range(node.getMaxInputCount()):
                if node.getInput(i) == read:  # Effects are equal when they are the same node (#777)
                    reconnect(node, i, grade)
                    connections += 1
        reconnect(grade, 0, read)
    return len(reads), connections


def add_writer():
    """Adds the WriteOIIO SoakWrite, as runTests.sh does."""
    writer = app.createNode("fr.inria.openfx.WriteOIIO")  # pylint: disable=undefined-variable
    writer.setScriptName("SoakWrite")
    writer = app.SoakWrite  # pylint: disable=undefined-variable  (the attributes of the node's parameters)
    # a Grade by GAIN before the writer: rendered at every frame (the probe of the render statistics check)
    grade = app.createNode("net.sf.openfx.GradePlugin")  # pylint: disable=undefined-variable
    grade.setScriptName("SoakRunGainOut")
    grade.getParam("multiply").set(GAIN, GAIN, GAIN, GAIN)
    reconnect(grade, 0, getattr(app, os.environ["SOAK_OUTPUT_NODE"]))  # pylint: disable=undefined-variable
    reconnect(writer, 0, grade)
    writer.filename.set(os.environ["SOAK_OUTPUT_FILE"])
    writer.encodingPluginChoice.set("fr.inria.openfx.WriteOIIO")
    writer.frameRange.set(2)  # manual
    writer.firstFrame.set(int(os.environ["SOAK_FIRST"]))
    writer.lastFrame.set(int(os.environ["SOAK_LAST"]))
    writer.quality.set(int(os.environ["SOAK_QUALITY"]))


print("SOAK inserted %d SoakRunGain nodes, with %d outputs" % insert_run_gains())
add_writer()
