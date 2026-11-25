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

7. The PyPlugs that ship with OpenFX plug-ins: the ``Contents/Resources/PyPlugs`` directory of each
   OpenFX plug-in bundle that Natron loaded (for example
   ``Misc.ofx.bundle/Contents/Resources/PyPlugs``, which has PIKColor, and
   ``CImg.ofx.bundle/Contents/Resources/PyPlugs``, which has EdgeBlur), in the order of the OpenFX
   plug-in search path. These come last, so that a copy of one of these PyPlugs in any of the
   directories above overrides it: for example an old ``PIKColor.py`` left in ``~/.Natron`` hides
   the one of openfx-misc. A directory that is already in the list above is not added again. Like
   the other search paths, these directories may have startup scripts (``init.py`` and
   ``initGui.py``), which run at every startup.

If "Use bundled plug-ins" is unchecked, Natron loads no bundled PyPlugs at all. It doesn't load its
bundled OpenFX plug-ins either (``Plugins/OFX/Natron``), so their PyPlugs, such as PIKColor and
EdgeBlur in openfx-misc's bundles, aren't loaded. The PyPlugs of the other OpenFX plug-in bundles
are loaded if their OpenFX plug-ins are.

A PyPlug whose file name was already loaded from an earlier directory is skipped, whatever its ID
and version (see below). Among PyPlugs with different file names, when two have the same ID and
version, the first one found wins. A PyPlug with a higher version is also loaded, and new nodes
use the highest version.

.. mermaid::
   :alt: The search order is: the bundled PyPlugs if "Use bundled plug-ins" and "Prefer
         bundled plug-ins" are both checked; ~/.Natron; the system location; the directories of
         NATRON_PLUGIN_PATH; the "PyPlugs search path" preference; the bundled PyPlugs
         if "Use bundled plug-ins" is checked but "Prefer bundled plug-ins" is not; and last the
         PyPlugs of the OpenFX plug-in bundles.
   :caption: Order of the PyPlug search paths. The bundled PyPlugs are searched first or after the
             preferences, depending on the preferences, or not at all; the PyPlugs of the OpenFX
             plug-in bundles always come last.

   flowchart LR
       b1["Bundled PyPlugs<br/>(if Use bundled and<br/>Prefer bundled)"] --> user["~/.Natron"]
       user --> sys["System location"]
       sys --> env["NATRON_PLUGIN_PATH<br/>(; on Windows, : elsewhere)"]
       env --> pref["Preferences:<br/>PyPlugs search path"]
       pref --> b2["Bundled PyPlugs<br/>(if Use bundled and<br/>not Prefer bundled)"]
       b2 --> ofx["OpenFX plug-in bundles:<br/>Contents/Resources/PyPlugs"]


Each of these directories is searched **recursively**: Natron also looks into all its sub-directories.
Every directory and sub-directory is also added to the Python path (``sys.path``), so that your
scripts can import the Python modules that are stored next to them. Because of this, a PyPlug is
imported by its file name alone: two PyPlugs with the same file name, in different directories,
can't both be loaded (see :ref:`pyplugWriting-pitfalls`, and :ref:`pyplugWriting-distribute` to
install PyPlugs).

A startup script can add a directory with
:func:`natron.appendToNatronPath(path)<NatronEngine.PyCoreApplication.appendToNatronPath>`.
This adds the directory to the "PyPlugs search path" setting, which is saved in the Preferences,
and to the Python path. Because Natron gathers the PyPlug files while it runs the startup scripts,
the PyPlugs of a directory added this way are only registered the next time Natron starts.

OpenFX plug-ins have their own search paths, and they are set with the ``OFX_PLUGIN_PATH``
environment variable (see :doc:`../_environment`) and in the *Plug-ins* tab of the Preferences.
