.. module:: NatronEngine
.. _AppSettings:

AppSettings
***********


Synopsis
--------

This class gathers all settings of Natron. You can access them exactly like you would for 
the :doc:`Effect` class.

Functions
^^^^^^^^^

- def :meth:`getParam<NatronEngine.AppSettings.getParam>` (scriptName)
- def :meth:`getParams<NatronEngine.AppSettings.getParams>` ()
- def :meth:`restoreDefaultSettings<NatronEngine.AppSettings.restoreDefaultSettings>` ()
- def :meth:`saveSettings<NatronEngine.AppSettings.saveSettings>` ()

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.AppSettings.getParam(scriptName)


    :param scriptName: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Param<NatronEngine.Param>`

Returns a :doc:`Param` by its *scriptName*. See :ref:`this<autoVar>` section for a detailed
explanation of what is the *script-name*.




.. method:: NatronEngine.AppSettings.getParams()


    :rtype: :class:`sequence`

Returns a sequence with all :doc:`Param` composing the settings.




.. method:: NatronEngine.AppSettings.restoreDefaultSettings()


Restores all settings to their default value shipped with Natron.





.. method:: NatronEngine.AppSettings.saveSettings()



Saves all the settings on disk so that they will be restored with their current value
on the following runs of Natron.





