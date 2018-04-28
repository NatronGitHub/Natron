.. module:: NatronEngine
.. _Track:

Track
*****

*Inherits*: :ref:`ItemBase<ItemBase>`

Synopsis
--------

This class represents one track marker as visible in the tracker node or on the viewer.

See :ref:`detailed<track.details>` description below.

Functions
^^^^^^^^^
*    def :meth:`reset<NatronEngine.Track.reset>` ()

.. _track.details:

Detailed Description
--------------------

The track is internally represented by multiple :doc:`parameters<Param>` which holds
animation curve for various data, such as: the track center, the pattern 4 corners,
the error score, the search-window, etc...
Each of them can be retrieved with the :func:`getParam(scriptName)<NatronEngine.ItemBase.getParam>` function.

Here is a small example showing how to retrieve the tracking data for a track::

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

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.Track.reset()

    Resets the track completely removing any animation on all parameters and any keyframe
    on the pattern.




