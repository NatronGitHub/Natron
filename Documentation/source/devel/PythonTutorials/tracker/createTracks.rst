.. _createTracks:

Create tracks
=============

This script creates a Tracker node connected to the node ``Read1``, and adds one
track at each corner of a rectangle. Each track gets a script name, a pattern
window, a search window and a motion model.

::

    def addTrack(tracker, x, y, name=None, patternSize=21, searchSize=71,
                 motionModel="Trans."):

        # create the track: its center is set to the center of the input image
        track = tracker.createTrack()

        # give it a script name, so that it can be accessed as
        # app.Tracker1.tracker.<name>
        if name:
            track.setScriptName(name)

        # position of the point to track, in pixels
        track.getParam("centerPoint").set(x, y)

        # the pattern (the image patch that is followed from frame to frame)
        # is given by its 4 corners, relative to the center point
        p = patternSize / 2.
        track.getParam("patternBtmLeft").set(-p, -p)
        track.getParam("patternBtmRight").set(p, -p)
        track.getParam("patternTopRight").set(p, p)
        track.getParam("patternTopLeft").set(-p, p)

        # the search window (where the pattern is searched in the next frame)
        # is given by 2 corners, relative to the center point
        s = searchSize / 2.
        track.getParam("searchWndBtmLeft").set(-s, -s)
        track.getParam("searchWndTopRight").set(s, s)

        # the motion model, set by its label as shown in the Tracker panel:
        # "Trans.", "Trans.+Rot.", "Trans.+Scale", "Trans.+Rot.+Scale",
        # "Affine" or "Perspective"
        track.getParam("motionModel").set(motionModel)

        return track

    # create the Tracker node and connect it to the source
    source = app.getNode("Read1")
    trackerNode = app.createNode("fr.inria.built-in.Tracker")
    trackerNode.connectInput(0, source)
    tracker = trackerNode.getTrackerContext()

    # one track at each corner of a rectangle
    corners = [("bottomLeft", 576, 320), ("bottomRight", 1344, 320),
               ("topRight", 1344, 768), ("topLeft", 576, 768)]
    for name, x, y in corners:
        addTrack(tracker, x, y, name)

    # list all the tracks of the Tracker
    for track in tracker.getAllTracks():
        center = track.getParam("centerPoint").get()
        print(track.getScriptName(), center.x, center.y)

The tracks can now be accessed by their script name, e.g.
``app.Tracker1.tracker.topLeft.centerPoint``.

Notes:

- The default sizes of the pattern and search windows of new tracks are set by the
  *Default Pattern Size* (``defPatternWinSize``) and *Default Search Area Size*
  (``defSearchWinSize``) parameters of the Tracker node, and the default motion model
  by *Default Motion Model* (``defMotionModel``). Setting these parameters before
  calling :func:`createTrack()<NatronEngine.Tracker.createTrack>` has the same
  effect as the code above.
- :func:`getSelectedTracks()<NatronEngine.Tracker.getSelectedTracks>` returns the
  tracks selected by the user in the Tracker panel or in the viewer, which is
  useful in scripts added to a menu.
- :func:`reset()<NatronEngine.Track.reset>` removes all the tracked data from a
  track.
