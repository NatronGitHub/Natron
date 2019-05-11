.. _tracking:

Using the tracker functionalities
=================================

All tracking functionalities are gathered in the :ref:`Tracker<Tracker>` class.
For now, only the tracker node can have a :ref:`Tracker<Tracker>` object.
The :ref:`Tracker<Tracker>` object is :ref:`auto-declared<autoVar>` by Natron and can be accessed
as an attribute of the tracker node::

    app.Tracker1.tracker

The tracker object itself is a container for :ref:`tracks<Track>`.
The :ref:`Track<Track>` class represent one marker as visible by the user
on the viewer.


:ref:`Tracks<Track>` can be accessed via their script-name directly::

    app.Tracker1.tracker.track1

The *script-name* of the tracks can be found in the :ref:`settings panel<trackerScriptName>` of the Tracker node.


Getting data out of the tracks:
-------------------------------

In Natron, a :ref:`track<Track>` contains internally just :ref:`parameters<Param>`
which can hold animated data just like regular parameters of the :ref:`effect class<Effect>`

You can access the parameters directly with their script-name::

    app.Tracker1.tracker.track1.centerPoint

Or you can use the :func:`getParam(paramScriptName)<NatronEngine.Track.getScriptName>` function::

    app.Tracker1.tracker.track1.getParam("centerPoint")

Here is an example that retrieves all keyframes available on the center point for a given
track::

    myTrack = app.Tracker1.tracker.track1

    keyframes = []

    # get the number of keys for the X dimension only and try match the Y keyframes
    nKeys = myTrack.centerPoint.getNumKeys(0)
    for k in range(0,nKeys):

        # getKeyTime returns a tuple with a boolean value indicating if it succeeded and
        # the keyframe time

        gotXKeyTuple = myTrack.centerPoint.getKeyTime(k, 0)
        frame = gotXKeyTuple[1]

        # Only consider keyframes which have an X and Y value
        # If Y does not have a keyframe at this frame, ignore the keyframe
        # getKeyIndex returns a value >=0 if there is a keyframe
        yKeyIndex = myTrack.centerPoint.getKeyIndex(frame, 1)

        if yKeyIndex == -1:
            continue

        # Note that even if the x curve or y curve didn't have a keyframe we
        # could still call getValueAtTime but the value would be interpolated by
        # Natron with surrounding keyframes, which is not what we want.

        x = myTrack.centerPoint.getValueAtTime(frame, 0)
        y = myTrack.centerPoint.getValueAtTime(frame, 1)

        keyframes.append((x,y))

    print keyframes


Creating Tracks
----------------

To create a new :ref:`track<Track>`, use the :func:`createTrack()<NatronEngine.Tracker.createTrack>` function made available by the :ref:`Tracker<Tracker>` class.
You can then set values on parameters much like everything else in Natron.
