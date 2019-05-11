.. module:: NatronEngine
.. _Tracker:

Tracker
*******

*Inherits*: :ref:`ItemsTable<ItemsTable>`

Synopsis
--------

This class is a container for :doc:`tracks<NatronEngine.Track>`
See :ref:`detailed<tracker.details>` description below.

Functions
^^^^^^^^^

- def :meth:`createTrack<NatronEngine.Tracker.createTrack>` ()
- def :meth:`startTracking<NatronEngine.Tracker.startTracking>` (tracks, start, end, forward)
- def :meth:`stopTracking<NatronEngine.Tracker.stopTracking>` ()

.. _tracker.details:

Detailed Description
--------------------

The Tracker is a special class attached to :doc:`effects<NatronEngine.Effect>` that needs
tracking capabilities. It contains all :doc:`tracks<NatronEngine.Track>` for this node
and also allow to start and stop tracking from a Python script.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.Tracker.createTrack()

    :rtype: :class:`Track<NatronEngine.Track>`

    Creates a new track in the tracker with default values

.. method:: NatronEngine.Tracker.startTracking (tracks, start, end, forward)

    Start tracking the given *tracks* from *start* frame to *end* frame (*end* frame will
    not be tracked) in the direction given by *forward*.
    If *forward* is **False**, then *end* is expected to be lesser than *start*.

.. method::  NatronEngine.Tracker.stopTracking ()

    Stop any ongoing tracking for this Tracker.
