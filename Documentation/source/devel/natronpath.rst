.. _natronPath:

Natron plug-in paths
=====================

When looking for startup scripts (:ref:`init.py and initGui.py<startupScripts>`) and
PyPlugs (Python group plug-ins and toolsets), Natron looks into the following search paths, in this order:

1. The bundled PyPlugs, in ``Plugins/PyPlugs`` next to the ``bin`` directory of your Natron installation
   (``Natron.app/Contents/Plugins/PyPlugs`` on macOS), but only if both "Use bundled plug-ins" and
   "Prefer bundled plug-ins over system-wide plug-ins" are checked in the *Plug-ins* tab of the
   Preferences (both are checked by default).

2. The standard user location: the directory ``.Natron`` in your home directory. Natron creates it
   if it doesn't exist.

   Windows::

       C:\Users\<username>\.Natron

   macOS and Linux::

       ~/.Natron

3. The standard system location:

   Windows::

       C:\Program Files\Common Files\Natron\Plugins

   macOS::

       /Library/Application Support/Natron/Plugins

   Linux::

       /usr/share/Natron/Plugins

4. All the directories listed in the **NATRON_PLUGIN_PATH** environment variable. The directories
   are separated by ``;`` on Windows and by ``:`` on macOS and Linux, as in ``PATH``:

   Windows::

       C:\NatronPluginsA;D:\Studio\NatronPluginsB

   macOS and Linux::

       /home/<username>/NatronPluginsA:/studio/NatronPluginsB

5. The directories of the "PyPlugs search path" setting in the *Plug-ins* tab of the Preferences.

6. The bundled PyPlugs (see 1.), if "Use bundled plug-ins" is checked and
   "Prefer bundled plug-ins over system-wide plug-ins" is not.

If "Use bundled plug-ins" is unchecked, Natron loads no bundled PyPlugs at all.

.. mermaid::
   :alt: The search order is: the bundled PyPlugs if "Use bundled plug-ins" and "Prefer
         bundled plug-ins" are both checked; ~/.Natron; the system location; the directories of
         NATRON_PLUGIN_PATH; the "PyPlugs search path" preference; and last the bundled PyPlugs
         if "Use bundled plug-ins" is checked but "Prefer bundled plug-ins" is not.
   :caption: Order of the PyPlug search paths. The bundled PyPlugs are searched first or last,
             depending on the preferences, or not at all.

   flowchart LR
       b1["Bundled PyPlugs<br/>(if Use bundled and<br/>Prefer bundled)"] --> user["~/.Natron"]
       user --> sys["System location"]
       sys --> env["NATRON_PLUGIN_PATH<br/>(; on Windows, : elsewhere)"]
       env --> pref["Preferences:<br/>PyPlugs search path"]
       pref --> b2["Bundled PyPlugs<br/>(if Use bundled and<br/>not Prefer bundled)"]


Each of these directories is searched **recursively**: Natron also looks into all its sub-directories.
Every directory and sub-directory is also added to the Python path (``sys.path``), so that your
scripts can import the Python modules that are stored next to them.

A startup script can add a directory with
:func:`natron.appendToNatronPath(path)<NatronEngine.PyCoreApplication.appendToNatronPath>`.
This adds the directory to the "PyPlugs search path" setting, which is saved in the Preferences,
and to the Python path. Because Natron gathers the PyPlug files while it runs the startup scripts,
the PyPlugs of a directory added this way are only registered the next time Natron starts.

OpenFX plug-ins have their own search paths, and they are set with the ``OFX_PLUGIN_PATH``
environment variable (see :doc:`../_environment`) and in the *Plug-ins* tab of the Preferences.
