.. _exportTrackData:

Read, export and import track data
==================================

After tracking, each tracked frame has a keyframe on the ``centerPoint``
parameter of the track, and the tracking error for that frame is in
``errorValue``. This script writes the tracked positions of all the tracks of a
Tracker node to a CSV file, with one line per track and per tracked frame.

::

    import csv
    import os
    import tempfile

    def getTrackedFrames(track):
        # the frames that have a keyframe on both x and y of the center point
        center = track.getParam("centerPoint")
        frames = []
        for k in range(center.getNumKeys(0)):
            ok, frame = center.getKeyTime(k, 0)
            if ok and center.getKeyIndex(frame, 1) >= 0:
                frames.append(int(frame))
        return frames

    def exportTracks(trackerNode, filename):
        tracker = trackerNode.getTrackerContext()
        with open(filename, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["track", "frame", "x", "y", "error"])
            for track in tracker.getAllTracks():
                center = track.getParam("centerPoint")
                error = track.getParam("errorValue")
                for frame in getTrackedFrames(track):
                    writer.writerow([track.getScriptName(), frame,
                                     center.getValueAtTime(frame, 0),
                                     center.getValueAtTime(frame, 1),
                                     error.getValueAtTime(frame)])

    exportTracks(app.Tracker1, os.path.join(tempfile.gettempdir(), "tracks.csv"))

The file can be read back into the tracks of another Tracker node. Tracks that
do not exist yet are created. Each line sets a keyframe on ``centerPoint``.

::

    import csv
    import os
    import tempfile

    def importTracks(trackerNode, filename):
        tracker = trackerNode.getTrackerContext()
        with open(filename, newline="") as f:
            for row in csv.DictReader(f):
                track = tracker.getTrackByName(row["track"])
                if track is None:
                    track = tracker.createTrack()
                    track.setScriptName(row["track"])
                center = track.getParam("centerPoint")
                frame = int(row["frame"])
                center.setValueAtTime(float(row["x"]), frame, 0)
                center.setValueAtTime(float(row["y"]), frame, 1)

    trackerNode = app.createNode("fr.inria.built-in.Tracker")
    trackerNode.connectInput(0, app.getNode("Read1"))
    importTracks(trackerNode, os.path.join(tempfile.gettempdir(), "tracks.csv"))

The same loops can be used to write any other format, or to read the data of a
point tracked by another application. The y axis of Natron goes up: data from an
application where y goes down must be converted with ``y = height - y``, where
``height`` is the height of the image.
