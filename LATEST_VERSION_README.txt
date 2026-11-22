The Natron version number (eg 2.6.0) appears in:

- Global/Macros.h in variables NATRON_VERSION_MAJOR NATRON_VERSION_MINOR NATRON_VERSION_REVISION
- CMakeLists.txt in project(Natron VERSION ...)
- App/App.pro in variable VERSION (used for the macOS Info.plist of the qmake build)
- Documentation/source/conf.py, Documentation/source/devel/conf.py and
  Documentation/source/guide/conf.py in variables version and release
- This file!

Update all of them together, including the revision for patch releases (eg 2.6.1).

Documentation/source/index.rst also contains links to the versioned documentation on readthedocs (currently rb-2.6)
