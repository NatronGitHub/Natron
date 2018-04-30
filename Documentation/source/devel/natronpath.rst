.. _natronPath:

Natron plug-in paths
=====================

When looking for startup scripts or Python group plug-ins, Natron will look into
the following search paths in order:

- The bundled plug-ins path. There are 2 kinds of plug-ins: PyPlugs and OpenFX plug-ins.
  The bundled OpenFX plug-ins are located in Plugins/OFX/Natron in your Natron installation and
  the bundled PyPlugs in the directory Plugins/PyPlugs.

- The standard user location for non OpenFX plug-ins (i.e. PyPlugs): that is the directory
  .Natron in the home directory, e.g.:

            On Windows that would be::

                C:\Users\<username>\.Natron

            On OS X & Linux that would be::

                ~/.Natron

- The standard system location for non OpenFX plug-ins (i.e. PyPlugs):

            Windows::

                C:\Program Files\Common Files\Natron\Plugins

            OS X::

                /Library/Application Support/Natron/Plugins

            Linux::

                /usr/share/Natron/Plugins

- All the paths indicated by the **NATRON_PLUGIN_PATH** environment variable. This
  environment variable should contain the separator *;* between each path, such as::

        /home/<username>/NatronPluginsA;/home/<username>/NatronPluginsB

- The user extra search paths in the Plug-ins tab of the Preferences of Natron.

If the setting "Prefer bundled plug-ins over system-wide plug-ins" is checked in the preferences
then Natron will first look into the bundled plug-ins before checking the standard location.
Otherwise, Natron will check bundled plug-ins as the *last* location.

Note that if the "User bundled plug-ins" setting in the preferences is unchecked, Natron
will not attempt to load any bundled plug-ins.
