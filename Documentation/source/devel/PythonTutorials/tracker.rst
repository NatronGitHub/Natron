.. _pythontracker:

Tracker
=======

This section gives Python tutorials for the :ref:`Tracker node<fr.inria.built-in.Tracker>`:
creating tracks, running the tracking, reading and exporting the tracked data,
and using it to drive a CornerPin or a Transform node.
See :ref:`tracking` for an overview of the :ref:`Tracker<Tracker>` and
:ref:`Track<Track>` classes, and :doc:`/guide/compositing-tracking` for how the
Tracker node is used interactively.

A few things to know before starting:

- The Tracker of a Tracker node is returned by
  :func:`getTrackerContext()<NatronEngine.Effect.getTrackerContext>`, and is also
  available as the ``tracker`` attribute of the node, e.g. ``app.Tracker1.tracker``.
- Each :ref:`Track<Track>` is a set of :ref:`parameters<Param>`. The most useful
  ones are listed below. Positions are in pixels, in the canonical coordinates of
  the project (the origin is the bottom-left corner of the image, y goes up).

  ===================== ===============================================================
  Script name           Content
  ===================== ===============================================================
  ``centerPoint``       Position of the track (2D, animated by the tracking).
  ``offset``            Offset of the pattern relative to ``centerPoint`` (2D).
  ``patternBtmLeft``    Bottom-left corner of the pattern, relative to ``centerPoint``.
                        ``patternBtmRight``, ``patternTopRight`` and ``patternTopLeft``
                        are the other 3 corners.
  ``searchWndBtmLeft``  Bottom-left corner of the search window, relative to
                        ``centerPoint``. ``searchWndTopRight`` is the top-right corner.
  ``motionModel``       Motion model searched by the tracker (choice).
  ``errorValue``        Tracking error at each tracked frame (1 minus the correlation).
  ``enabled``           Whether the track is used to compute the transform (animatable).
  ===================== ===============================================================

- The tracking runs in a separate thread:
  :func:`startTracking()<NatronEngine.Tracker.startTracking>` returns immediately.
  :ref:`runTracking` shows how to wait for the result.

The scripts can be run in the Script Editor of Natron, or saved in a file
and run with ``NatronRenderer -t script.py``.
In both cases, ``app`` is the current project.

.. toctree::
    :maxdepth: 1

    tracker/createTracks.rst
    tracker/runTracking.rst
    tracker/exportTrackData.rst
    tracker/tracksToCornerPin.rst
