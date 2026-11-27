.. currentmodule:: NatronEngine
.. _Tracker:

Tracker
*******

Synopsis
--------

This class is a container for :doc:`tracks<Track>`
See :ref:`detailed<tracker.details>` description below.

Functions
^^^^^^^^^

- def :meth:`createTrack<NatronEngine.Tracker.createTrack>` ([usePatternMatching])
- def :meth:`getTrackByName<NatronEngine.Tracker.getTrackByName>` (scriptName)
- def :meth:`getAllTracks<NatronEngine.Tracker.getAllTracks>` ()
- def :meth:`getSelectedTracks<NatronEngine.Tracker.getSelectedTracks>` ()
- def :meth:`startTracking<NatronEngine.Tracker.startTracking>` (tracks, start, end, forward)
- def :meth:`stopTracking<NatronEngine.Tracker.stopTracking>` ()

.. _tracker.details:

Detailed Description
--------------------

The Tracker is a special class attached to :doc:`effects<Effect>` that needs
tracking capabilities. It contains all :doc:`tracks<Track>` for this node
and also allows one to start and stop tracking from a Python script.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: Tracker.createTrack([usePatternMatching])

    :param usePatternMatching: :class:`bool`
    :rtype: :class:`Track<NatronEngine.Track>`

    Creates a new track in the tracker with default values. It uses pattern matching (the
    TrackerPM plug-in of openfx-misc) if *usePatternMatching* is True, and not if it is False;
    without *usePatternMatching*, as the *Use Pattern Matching* parameter of the node says.
    Returns None, and prints why, if the track cannot be created (a pattern-matching track
    without the TrackerPM plug-in).

    .. versionchanged:: 2.6
       The *usePatternMatching* argument. None if the track cannot be created.


.. method:: Tracker.getTrackByName(scriptName)

    :rtype: :class:`Track<NatronEngine.Track>`

    Returns a track matching the given *scriptName* if any



.. method:: Tracker.getAllTracks()

    :rtype: :class:`sequence`

    Returns all the tracks in this Tracker.

.. method:: Tracker.getSelectedTracks()

    :rtype: :class:`sequence`

    Returns the user selected tracks

.. method:: Tracker.startTracking(tracks, start, end, forward)

    Start tracking the given *tracks* from *start* frame to *end* frame (*end* frame will
    not be tracked) in the direction given by *forward*.
    If *forward* is **False**, then *end* is expected to be lesser than *start*.

.. method:: Tracker.stopTracking()

    Stop any ongoing tracking for this Tracker.
