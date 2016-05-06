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

*    def :meth:`setScriptName<NatronEngine.Track.setScriptName>` (scriptName)
*    def :meth:`getScriptName<NatronEngine.Track.getScriptName>` ()
*    def :meth:`getParam<NatronEngine.Track.getParam>` (paramScriptName)
*    def :meth:`reset<NatronEngine.Track.reset>` ()

.. _track.details:

Detailed Description
--------------------

The track is internally represented by multiple :doc:`parameters<Param>` which holds
animation curve for various data, such as: the track center, the pattern 4 corners, 
the error score, the search-window, etc...
Each of them can be retrieved with the :func:`getParam(scriptName)<NatronEngine.Track.getParam>` function.

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

	Get the :doc:`Param<NatronEngine.Param>` with the given *paramScriptName*.
	The parameter can also be retrieved as an attribute of the *tracker* object like this::
	
		Tracker1.tracker.center

.. method:: NatronEngine.Track.reset()

	Resets the track completely removing any animation on all parameters and any keyframe
	on the pattern.
	



