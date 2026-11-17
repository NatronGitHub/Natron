.. _runTracking:

Run the tracking
================

:func:`startTracking(tracks, start, end, forward)<NatronEngine.Tracker.startTracking>`
tracks the given tracks from frame *start* to frame *end*, *end* excluded.
It returns as soon as the tracking has started: the tracking itself runs in a
separate thread, and writes a keyframe on the ``centerPoint`` parameter of each
track at each tracked frame.

There is no function to know when the tracking is finished, so a script that needs
the result has to wait for it. The ``trackRange`` function below starts the
tracking and waits until each track either has a keyframe on the last frame or has
failed. To know whether and where a track failed, the script turns on the
*Animate Enabled* (``autoKeyEnabled``) parameter of the Tracker node: Natron then
sets a keyframe with the value *False* on the ``enabled`` parameter of the track
at the frame where it failed. The tracking stops when all the tracks have failed.

::

    import time

    def hasFailed(track, first, last):
        # True if the "enabled" parameter of the track was set to False by
        # the tracking at a frame between first and last
        enabled = track.getParam("enabled")
        for k in range(enabled.getNumKeys(0)):
            ok, frame = enabled.getKeyTime(k, 0)
            if ok and first < frame <= last and not enabled.getValueAtTime(frame):
                return True
        return False

    def trackRange(trackerNode, tracks, first, last, timeout=600):

        tracker = trackerNode.getTrackerContext()

        # set a keyframe on "enabled" where a track fails
        trackerNode.getParam("autoKeyEnabled").set(True)

        # track from first to last (included)
        tracker.startTracking(tracks, first, last + 1, True)

        # wait until every track is finished or failed
        startTime = time.time()
        running = list(tracks)
        while running and time.time() - startTime < timeout:
            time.sleep(0.1)
            running = [t for t in running
                       if t.getParam("centerPoint").getKeyIndex(last, 0) < 0
                       and not hasFailed(t, first, last)]
        if running:
            # timeout: stop the tracking
            tracker.stopTracking()

        # return the tracks that were tracked up to the last frame
        return [t for t in tracks
                if t.getParam("centerPoint").getKeyIndex(last, 0) >= 0
                and not hasFailed(t, first, last)]

    trackerNode = app.Tracker1
    tracks = trackerNode.getTrackerContext().getAllTracks()
    good = trackRange(trackerNode, tracks, 1, 10)
    print("tracked:", [t.getScriptName() for t in good])
    print("failed:", [t.getScriptName() for t in tracks if t not in good])

Notes:

- The tracking parameters of the Tracker node apply to all tracks: the channels
  used (``trackRed``, ``trackGreen``, ``trackBlue``), *Max. Error* (``maxError``),
  *Maximum Iterations* (``maxIterations``), *Pre-blur Sigma* (``preBlurSigma``),
  etc. They are described on the :ref:`Tracker node<fr.inria.built-in.Tracker>`
  page. Set them before calling ``startTracking``.
- Keyframes that are already set on ``centerPoint`` before the start frame are used
  to predict the motion of the track.
- :func:`stopTracking()<NatronEngine.Tracker.stopTracking>` stops the tracking.
- While the script waits, the Natron interface does not respond. When the
  tracking is run from the Script Editor, another option is to call
  ``startTracking`` alone, and to read the result in a second script once the
  tracking is finished.

.. warning::

   In Natron 2.6 and earlier, ``startTracking`` with ``forward=False`` does not
   track anything. To track backward, use the *Track Backward* button of the
   Tracker node.
