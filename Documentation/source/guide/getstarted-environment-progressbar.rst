.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Progress bars
=============

Natron shows the progress of long operations in the **Progress** panel:
renders of Write nodes, and long tasks run by some nodes, such as tracking.
This page explains how to read that panel, and how to pause, cancel, queue
and restart renders.


Starting a render
-----------------

A render writes the output of a :ref:`Write node <fr.inria.built-in.Write>`
to disk, for the frame range set in its *Frame Range* parameter. Start it
with one of:

- the **Render** button in the properties panel of the Write node;
- :menuselection:`Render --> Render Selected Writers` (:kbd:`F7`), with
  one or more Write nodes selected in the node graph;
- :menuselection:`Render --> Render All Writers` (:kbd:`F5`), to render all
  the Write nodes of the project that are not disabled.

See also :doc:`compositing-projects-render`.


The Progress panel
------------------

When a render starts, a row is added to the Progress panel. If the render is
still running after a few seconds, or when it ends, the panel is brought to
the front. The first time it is needed, the Progress panel is added as a tab
to the pane of the node graph. To show it in another pane, click the button
at the top left corner of that pane and choose **Progress Panel here**.

Each row of the panel is one task, with the following columns:

**Node**
   The label of the node that runs the task, with the color of the node.

**Progress**
   A progress bar. For a render, it shows the fraction of the frames that
   are written.

**Status**
   *Queued* (yellow) before the task starts, *Running* (green), *Paused*
   (orange), *Finished*, or *Failed* (red).

**Controls**
   The buttons to pause, restart or cancel the task (see below).

**Time remaining**
   An estimate of the time left, computed from the time spent so far and the
   progress.

**Frame Range**
   The first and last frames of a render.

**Task**
   For a render, the output file name, as set in the Write node. For other
   tasks, a description, for example "Tracking...".

On Windows, the progress of the tasks is also shown on the Natron button of
the taskbar, and on macOS on the Natron icon in the Dock.

Tasks other than renders, such as tracking or the analysis run by some
plug-ins, are only added to the panel when they are expected to last more
than a few seconds.


Pausing, restarting and canceling
---------------------------------

The **Controls** column has three buttons:

**Pause**
   Stops the render. Only renders to image sequences can be paused: renders
   to a movie file (for example with
   :ref:`WriteFFmpeg <fr.inria.openfx.WriteFFmpeg>`) cannot.

**Restart**
   Starts a paused or failed render again, from the last frame written (that
   frame is written again). For a render that finished successfully, it
   renders the whole frame range again. With *Render in a separate process*
   (see below), the restarted render uses the project as it is now, so the
   changes made since the first start are included.

**Cancel**
   Stops the task and removes it from the list. You can also select one or
   more rows and press :kbd:`Delete` or :kbd:`Backspace`.

A task that cannot be paused can only be canceled, and it is then removed
from the list.

At the top of the panel, **Remove Finished Tasks** removes the renders from
the list as soon as they finish successfully; failed renders stay in the
list. When it is unchecked, finished renders stay in the list, so that you
can restart them.


Queuing renders
---------------

By default, each render starts as soon as you launch it, and several renders
run at the same time, sharing the computer.

When **Queue Renders** is checked at the top of the Progress panel, a new
render waits (with the *Queued* status) until the renders before it are
done, and the renders run one after the other. This does not apply to other
tasks such as tracking. The same setting is *Append new renders to queue* in
the *Threading* section of the :doc:`Preferences </_prefs>`.


Background rendering
--------------------

By default, renders are computed by the Natron process you are working in.
If Natron crashes during the render, the render stops too.

With *Render in a separate process* in the *Threading* section of the
:doc:`Preferences </_prefs>`, Natron saves a copy of the project when the
render starts, and a separate Natron process renders that copy. The render
then goes on even if the main application crashes, and changes made to the
project after the start do not affect it. Its progress is shown in the
Progress panel as for other renders. Right-click on the row and choose
**Show Process Log** to see the messages of that process.

You can also render a project without the interface, from the command line,
with ``NatronRenderer``: see :ref:`natronExec`.


Progress in the Viewer and the node graph
-----------------------------------------

The Viewer does not use the Progress panel, but shows that it is rendering
in other ways:

- the **Refresh** button of the Viewer toolbar is highlighted while the
  Viewer is rendering;
- in the node graph, a yellow frame is drawn around the nodes that are
  rendering, and the links between nodes change color while images are
  computed through them;
- the frames that are already rendered and cached are marked with a green
  line under the timeline of the Viewer.

To make renders of Write nodes faster, *Turbo mode* (in the player of the
Viewer) stops redrawing the rest of the interface during renders and
playback. With the *Auto-turbo* preference, in the *Nodegraph* section of the
:doc:`Preferences </_prefs>`, it is turned on automatically during renders of
Write nodes, except renders in a separate process. See :doc:`getstarted-environment-viewer` for
the Viewer controls.


Render statistics
-----------------

:menuselection:`Render --> Enable Render Statistics` (:kbd:`F2`) opens the
render statistics window and collects statistics during the next renders:
for each node, the time spent, the region of definition, the mipmap level
(render scale), and whether the node was skipped because it did not change
its input (identity). This is useful to find which nodes make a composition
slow. By default the statistics are cleared at each frame: check
**Accumulate** to add them up over several frames, and click **Reset** to
clear them. In the Viewer, :kbd:`Ctrl+Shift+U` renders the current frame
with statistics.
