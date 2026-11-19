.. _maint-serialization:

Project Files and Backward Compatibility
========================================

Users keep their projects for years, and expect a new version of Natron to open
them. This chapter explains how project files are written and read, and how to
change the format without breaking old files.

.. warning::

   Serialization is **backward-compatibility-critical**. Any change to a
   ``…Serialization`` class can make existing project files unreadable, or read
   them wrongly. Always bump the class version and keep a version-guarded path
   that reads the old layout; never silently change the meaning or the order
   of a field. This is the single easiest place to cause data-loss regressions.

What is written, and where
--------------------------

Natron writes projects (``.ntp``) and autosaves with
`Boost.Serialization <https://www.boost.org/doc/libs/release/libs/serialization/>`_
XML archives (``boost::archive::xml_oarchive`` / ``xml_iarchive``).
``Project::saveProjectInternal`` and ``Project::loadProjectInternal``
(``Engine/Project.cpp``) write and read, in this order:

1. ``Background_project``, a boolean;
2. ``Project``, a ``ProjectSerialization`` (``Engine/ProjectSerialization.h``):
   the version of Natron that saved the file (``VersionMajor``,
   ``VersionMinor``, ``VersionRev``, git branch and commit, OS), the nodes
   (``NodeCollectionSerialization`` → ``NodeSerialization`` →
   ``KnobSerialization``, recursively for groups), the project knobs, the
   additional formats, the current frame and the creation date;
3. unless it is a background project, the window layout
   (``ProjectGuiSerialization``, through ``AppInstance::saveProjectGui`` /
   ``loadProjectGui``, implemented in ``Gui``).

The same classes serialize the node clipboard: ``NodeGraph::copySelectedNodes``
(``Gui/NodeGraph40.cpp``) writes a ``NodeClipBoard`` to an XML string, which
is how nodes can be copied between two Natron windows.

There are 28 ``*Serialization.h`` files (25 in ``Engine``, 3 in ``Gui``). A few
of them are not about projects: ``CacheSerialization.h``,
``ImageSerialization.h``, ``FrameEntrySerialization.h`` and the
``*Params*`` ones write the table of contents of the disk caches (see
:ref:`maint-cache`); those are versioned by ``NATRON_CACHE_VERSION``, and an
old cache is simply wiped.

PyPlugs are not Boost archives: exporting a group as a PyPlug writes a Python
script (``NodeCollection::exportGroupToPython`` in
``Engine/NodeGroup.cpp``), which recreates the nodes and sets their values
through the Python API when it runs. Their compatibility depends on the
stability of the Python API (see :ref:`maint-python`).

Versions
--------

Each versioned class declares its version at the end of its header with
``BOOST_CLASS_VERSION``. Eight of the ``*Serialization.h`` headers have no
``BOOST_CLASS_VERSION``: ``Curve``, ``RectI``, ``RectD`` and ``Variant``,
whose classes are not versioned (their ``serialize`` ignores the version), and
the cache headers ``Cache``, ``NonKeyParams``, ``ImageParams`` and
``FrameParams`` (``ImageParams`` is versioned in ``ImageParams.h``). Adding a
field to an unversioned class needs a ``BOOST_CLASS_VERSION`` first. The convention is to ``#define`` one constant per
change, and to set the current version to the last one. For example, in
``Engine/KnobSerialization.h``::

    #define MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME 2
    #define MASTER_SERIALIZATION_INTRODUCE_MASTER_NODE_NAME_FULL 3
    #define MASTER_SERIALIZATION_VERSION MASTER_SERIALIZATION_INTRODUCE_MASTER_NODE_NAME_FULL
    …
    BOOST_CLASS_VERSION(NATRON_NAMESPACE::MasterSerialization, MASTER_SERIALIZATION_VERSION)

The class then has separate ``save`` and ``load`` functions
(``BOOST_SERIALIZATION_SPLIT_MEMBER()``). ``save`` always writes the current
layout; ``load`` receives the version of the file being read and tests it
before each field that was added later::

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar & ::boost::serialization::make_nvp("MasterDimension", masterDimension);
        ar & ::boost::serialization::make_nvp("MasterNodeName", masterNodeName);
        if (version >= MASTER_SERIALIZATION_INTRODUCE_MASTER_NODE_NAME_FULL) {
            ar & ::boost::serialization::make_nvp("MasterNodeNameFull", masterNodeNameFull);
        }
        ar & ::boost::serialization::make_nvp("MasterKnobName", masterKnobName);
        if (version >= MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME) {
            ar & ::boost::serialization::make_nvp("MasterTrackName", masterTrackName);
        }
    }

The main version constants are ``PROJECT_SERIALIZATION_VERSION``
(``ProjectSerialization.h``), ``NODE_SERIALIZATION_CURRENT_VERSION``
(``NodeSerialization.h``), and ``KNOB_SERIALIZATION_VERSION``,
``VALUE_SERIALIZATION_VERSION`` and ``MASTER_SERIALIZATION_VERSION``
(``KnobSerialization.h``). The list of ``#define`` lines above each of them is
the history of the format.

Adding a field
--------------

1. Add a ``#define`` with the next version number and a name that says what
   changed, and make the current version point to it.
2. In ``save``, write the new field (at the place where it belongs in the
   layout; ``save`` has no version test).
3. In ``load``, read it inside ``if (version >= YOUR_NEW_VERSION)``, at the same
   place, and give it a sensible default otherwise (in the constructor), so
   that old files load as before.
4. If a field is removed or changes meaning, keep reading the old one in
   ``load`` for the old versions, and convert it.
5. Save a project with the new code and load it again, and load a project
   saved by the previous release.

Never reorder fields, and never reuse a version number: a file written by a
development build with an unreleased version would then be read wrongly.

Old and new files
-----------------

Projects are not forward compatible, but nothing checks it up front. Only
files in the old format, which stored the version as a ``NatronVersion``
string, are refused when they come from a more recent Natron
(``ProjectSerialization::load``). A newer file in the current format is read
like any other: it loads if Boost can read it. If reading throws,
``Project::loadProjectInternal`` reports "This project was saved with a more
recent version …" when the file's version is newer, or "Unrecognized or
damaged project file" otherwise.

A project that loads may still render differently, if a plug-in that it uses
changed its output (for example after a library upgrade). That is not a
serialization problem, but it is the other half of what users mean by
"compatibility": changes of plug-in output should be mentioned in the release
notes.

There is no automated test that loads old project files yet (see
:ref:`maint-todo`). Until there is one, test format changes by hand with
projects saved by the previous releases.
