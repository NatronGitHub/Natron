.. module:: NatronEngine
.. _Track:

Track
*****

Synopsis
--------

This class represents one track marker as visible in the tracker node or on the viewer.
It is available to Python to easily retrieve the tracked data.
See :ref:`detailed<track.details>` description below.

Functions
^^^^^^^^^

- def :meth:`setScriptName<NatronEngine.Track.setScriptName>` (scriptName)
- def :meth:`getScriptName<NatronEngine.Track.getScriptName>` ()
- def :meth:`getParam<NatronEngine.Track.getParam>` (paramScriptName)
- def :meth:`getParams<NatronEngine.Track.getParams>` ()
- def :meth:`reset<NatronEngine.Track.reset>` ()

.. _track.details:

Detailed Description
--------------------

The track is internally represented by multiple :doc:`parameters<Param>` which holds
animation curve for various data, such as: the track center, the pattern 4 corners,
the error score, the search-window, etc...
Each of them can be retrieved with the :func:`getParam(scriptName)<NatronEngine.Track.getParam>` function.

Here is an example briefly explaining how to retrieve the tracking data for a track::

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


.. method:: NatronEngine.Track.setScriptName(scriptName)

    :param scriptName: :class:`str<NatronEngine.std::string>`


Set the script-name of the track. It will then be accessible via a Python script as such::

    Tracker1.tracker.MyTrackScriptName

.. method:: NatronEngine.Track.getScriptName()

    :rtype: :class:`str<NatronEngine.std::string>`

    Get the script-name of the track

.. method:: NatronEngine.Track.getParam(paramScriptName)

    :rtype: :class:`Param<NatronEngine.Param>`

    Get the :class:`Param<NatronEngine.Param>` with the given *paramScriptName*.
    The parameter can also be retrieved as an attribute of the *tracker* object like this::

        Tracker1.tracker.center


.. method:: NatronEngine.Track.getParams()

    :rtype: :class:`Param<NatronEngine.Param>`

    Returns a list of all the :class:`Param<NatronEngine.Param>` for this track.


.. method:: NatronEngine.Track.reset()

    Resets the track completely removing any animation on all parameters and any keyframe
    on the pattern.




