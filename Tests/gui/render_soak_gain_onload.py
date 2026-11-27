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

"""Run by NatronRenderer -l on the saved projects of the render soak test (run_render_soak_test.py).

It sets the gain of the "RunGain" Grade nodes (see render_soak_graph.py) to SOAK_RUN_GAIN.
"""

import os

GAIN = float(os.environ["SOAK_RUN_GAIN"])
for name in ("RunGain", "RunGainRead"):
    node = getattr(app, name, None)  # pylint: disable=undefined-variable  (app: the project, set by Natron)
    if node is not None:
        node.getParam("multiply").set(GAIN, GAIN, GAIN, GAIN)
