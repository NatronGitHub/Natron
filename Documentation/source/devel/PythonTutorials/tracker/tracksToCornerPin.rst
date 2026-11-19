.. _tracksToCornerPin:

Use tracks in a CornerPin or a Transform
========================================

The Tracker node can compute a transform from its tracks and create a linked
:ref:`CornerPin<net.sf.openfx.CornerPinPlugin>` or
:ref:`Transform<net.sf.openfx.TransformPlugin>` node: set *Motion Type*
(``motionType``) and *Transform Type* (``transformType``) in its *Transform* tab,
then press *Export* (``export``). This is described in
:doc:`/guide/compositing-tracking`.

The scripts below do a simpler version of this by hand, which gives full control
over how the tracked data is used.

Match-move with a CornerPin
---------------------------

This script creates a CornerPin node whose 4 corners follow 4 tracks.
At the reference frame, the CornerPin does nothing (its *from* and *to* points
are equal); at each tracked frame, its *to* points are the tracked positions.
An image connected to the input of the CornerPin then moves like the 4 tracked
points.

::

    def cornerPinFromTracks(tracks, refFrame):
        cornerPin = app.createNode("net.sf.openfx.CornerPinPlugin")
        for i, track in enumerate(tracks):
            center = track.getParam("centerPoint")
            fromParam = cornerPin.getParam("from%d" % (i + 1))
            toParam = cornerPin.getParam("to%d" % (i + 1))

            # the "from" points are the positions at the reference frame
            fromParam.set(center.getValueAtTime(refFrame, 0),
                          center.getValueAtTime(refFrame, 1))

            # the "to" points follow the tracks
            for k in range(center.getNumKeys(0)):
                ok, frame = center.getKeyTime(k, 0)
                if ok:
                    toParam.setValueAtTime(center.getValueAtTime(frame, 0), frame, 0)
                    toParam.setValueAtTime(center.getValueAtTime(frame, 1), frame, 1)
        return cornerPin

    tracker = app.Tracker1.tracker
    # the 4 tracks, in the order of the CornerPin corners:
    # bottom-left, bottom-right, top-right, top-left
    tracks = [tracker.getTrackByName(name)
              for name in ("bottomLeft", "bottomRight", "topRight", "topLeft")]
    cornerPin = cornerPinFromTracks(tracks, 1)

To stabilize the tracked image instead, connect the CornerPin to the tracked
image and check its *Invert* parameter: ``cornerPin.getParam("invert").set(True)``.

Instead of copying the keyframes, the *to* points can be linked to the tracks
with an :ref:`expression<paramExpr>`, so that the CornerPin is updated
when the tracks change::

    for i, name in enumerate(("bottomLeft", "bottomRight", "topRight", "topLeft")):
        toParam = cornerPin.getParam("to%d" % (i + 1))
        for dim in range(2):
            toParam.setExpression("Tracker1.tracker.%s.centerPoint.getValueAtTime(frame, dimension)" % name,
                                  False, dim)

Translation with a Transform
----------------------------

With a single track, only the translation can be computed. This script creates a
Transform node whose translation follows a track, relative to its position at
the reference frame. As with the CornerPin, set ``invert`` to stabilize instead
of match-moving.

::

    def transformFromTrack(track, refFrame, stabilize=False):
        transform = app.createNode("net.sf.openfx.TransformPlugin")
        center = track.getParam("centerPoint")
        x0 = center.getValueAtTime(refFrame, 0)
        y0 = center.getValueAtTime(refFrame, 1)

        # rotations and scales are around the tracked point
        transform.getParam("center").set(x0, y0)

        translate = transform.getParam("translate")
        for k in range(center.getNumKeys(0)):
            ok, frame = center.getKeyTime(k, 0)
            if ok:
                translate.setValueAtTime(center.getValueAtTime(frame, 0) - x0, frame, 0)
                translate.setValueAtTime(center.getValueAtTime(frame, 1) - y0, frame, 1)

        transform.getParam("invert").set(stabilize)
        return transform

    transform = transformFromTrack(app.Tracker1.tracker.topLeft, 1)

With two tracks or more, the rotation and scale can be computed too; the
simplest way is then to let the Tracker node compute the transform and to use
its *Export* button.

See also
--------

- :doc:`/guide/compositing-transform`: the Transform and CornerPin nodes.
- :doc:`/guide/compositing-tracking`: exporting a CornerPin or a Transform
  from the Tracker node without Python.
