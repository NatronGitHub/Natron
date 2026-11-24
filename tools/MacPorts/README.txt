Natron's MacPorts overlay. The up-to-date instructions are in INSTALL_MACOS.md
(installing, ./sync.sh, keeping MacPorts up to date) and in
Documentation/source/maintainers/build-macos.rst (reintegrating upstream).
- ./sync.sh copies this directory to /opt/MacPorts-Natron and runs portindex.
- ./check.sh [--all] compares the overlay with upstream MacPorts and with
  /opt/MacPorts-Natron (see the comment at the top of check.sh).
- A PINNED file in a port directory marks a pinned port: keep its Portfile;
  only Portfile.orig and Portfile.patch follow upstream. Line 1 is the kind,
  line 2 the reason. "ahead": our version is newer than upstream's; unpin as
  soon as upstream catches up (check.sh then says "upstream caught up: unpin").
  "older": held back at an older version until the reason is gone.

