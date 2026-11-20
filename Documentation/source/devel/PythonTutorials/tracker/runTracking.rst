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
        # (the previous setting is restored at the end)
        autoKey = trackerNode.getParam("autoKeyEnabled")
        autoKeyWasOn = autoKey.get()
        autoKey.set(True)

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

        autoKey.set(autoKeyWasOn)

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

- The *enabled* keyframes set by a failed track stay after the tracking: the
  track is disabled from the frame where it failed on (until its next keyframe),
  so it is not used by the transform computed by the Tracker node at those
  frames. A later ``startTracking`` with *Animate Enabled* on sets a *True*
  keyframe at its start frame.
- The tracking parameters of the Tracker node apply to all tracks. Set them before
  calling ``startTracking``:

  - ``trackRed``, ``trackGreen``, ``trackBlue``: the channels used for tracking.
  - ``maxError`` (*Max. Error*): the largest error (1 minus the normalized
    cross-correlation between the pattern and the tracked patch) for which the
    tracking of a frame is a success.
  - ``maxIterations`` (*Maximum Iterations*): the maximum number of iterations of
    the minimization at each frame before the tracker gives up.
  - ``bruteForcePretTrack`` (*Use Brute-Force Pre-track*): search the translation
    by brute force before refining it.
  - ``normalizeIntensities`` (*Normalize Intensities*): normalize the patches by
    their mean before comparing them (slower).
  - ``preBlurSigma`` (*Pre-blur Sigma*): the size in pixels of the blur applied to
    the image before tracking.
- Keyframes that are already set on ``centerPoint`` before the start frame are used
  to predict the motion of the track.
- :func:`stopTracking()<NatronEngine.Tracker.stopTracking>` stops the tracking.
- While the script waits, the Natron interface does not respond. When the
  tracking is run from the Script Editor, another option is to call
  ``startTracking`` alone, and to read the result in a second script once the
  tracking is finished.

.. warning::

   Up to Natron 2.6.x (see `issue #1255
   <https://github.com/NatronGitHub/Natron/issues/1255>`_), ``startTracking`` with
   ``forward=False`` does not track anything. To track backward, use the *Track Backward* button of the
   Tracker node.
