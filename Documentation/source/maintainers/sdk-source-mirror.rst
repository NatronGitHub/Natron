.. _maint-sdk-source-mirror:

The SDK Source Mirror on SourceForge
====================================

The Linux SDK (``tools/jenkins/include/scripts/build-Linux-sdk.sh`` and the
package scripts in ``tools/jenkins/include/scripts/pkg/``) downloads the source
tarball of every library it builds. Natron keeps a copy of these tarballs on
SourceForge, so that the SDK can still be built when a project's site is down,
moves its files, or removes an old release. This chapter explains how the
mirror is used and how to keep it up to date. Updating it is part of every SDK
version bump.

.. contents::
   :local:
   :depth: 2

How the SDK uses the mirror
---------------------------

``THIRD_PARTY_SRC_URL`` in ``tools/jenkins/common.sh`` points to the mirror:
https://sourceforge.net/projects/natron/files/Third_Party/Sources.

Each package script calls ``download``, ``download_urls`` or
``download_github`` with the file name, the project's own download URL (the
*master site*) and the file's sha256, pinned in the script. These functions try
the master site first and the mirror only if the master site fails. A file
whose sha256 differs from the pinned one is deleted and counts as a failed
download. The SDK build therefore never depends on the mirror while the master
sites work, and a stale or wrong file on the mirror can't get into the SDK.
At the end, ``build-Linux-sdk.sh`` lists the files that came from the mirror
and the master URLs that failed: these URLs must be fixed in the package
scripts.

Sources fetched with ``git clone`` (checked against a pinned commit) and the
Python packages installed with ``pip`` don't go through the mirror.

Where the files are, and who can change them
--------------------------------------------

- Public listing: https://sourceforge.net/projects/natron/files/Third_Party/Sources/
- Download URL of a file:
  ``https://downloads.sourceforge.net/project/natron/Third_Party/Sources/<file>``
- Upload location: ``/home/frs/project/natron/Third_Party/Sources/`` on
  ``frs.sourceforge.net``, through SFTP or rsync over SSH (see
  `Release Files for Download <https://sourceforge.net/p/forge/documentation/Release%20Files%20for%20Download/>`_).

Uploads need a SourceForge account with the *Release Technician* or *Admin*
role in the ``natron`` project, and an SSH public key registered in that
account's settings (`SSH Keys <https://sourceforge.net/p/forge/documentation/SSH%20Keys/>`_).
The account name is the SSH user, e.g. ``<user>@frs.sourceforge.net``. Check
the host key fingerprint of ``frs.sourceforge.net`` against the one SourceForge
publishes before you accept it. Never put the key or a password in a script or
in the repository.

Rules
-----

- **Upload only files downloaded from the project's own site**, never from
  another mirror or a distribution, and only after checking their sha256
  against the value pinned in the package script. Never upload a file whose
  sha256 doesn't match: report it instead (the pin or the master site is
  wrong).
- **One name, one content.** Never replace a file on the mirror with a
  different file of the same name: older SDK scripts, which pin the old
  sha256, would no longer find a valid copy. If a package changes the file
  it downloads (e.g. from a release tarball to a GitHub archive of the same
  version), give the new file a different name in the package script.
- **Delete only after approval.** The mirror also serves the SDK scripts of
  released versions. Space on SourceForge is limited, so remove files that no
  maintained branch and no released tag uses, but only from a list that the
  maintainer has approved.

Updating the mirror after an SDK change
---------------------------------------

Do this when a PR that changes a package version or adds a package is merged
(or before, for a long-lived SDK PR).

#. **Download the sources.** On Linux, with ``wget`` and ``sha256sum``
   installed (the Rocky Linux SDK image has both), from ``tools/jenkins``::

      mkdir -p /tmp/sdk-src/src
      DOWNLOAD=1 WORKSPACE=/tmp/sdk-src include/scripts/build-Linux-sdk.sh

   This downloads every source tarball of the SDK into ``/tmp/sdk-src/src``,
   builds nothing, and keeps only the files whose sha256 matches the pinned
   one. If it stops with ``unable to download``, fix the URL or the pin first.

#. **List the mirror**::

      echo 'ls -1 /home/frs/project/natron/Third_Party/Sources' |
          sftp -b - <user>@frs.sourceforge.net > mirror.txt

   The files to upload are those in ``/tmp/sdk-src/src`` that are not in
   ``mirror.txt``.

#. **Upload the new files**, by name, without deleting anything::

      cd /tmp/sdk-src/src
      rsync -av --ignore-existing <file>... \
          <user>@frs.sourceforge.net:/home/frs/project/natron/Third_Party/Sources/

#. **Check the uploaded files.** Download each one from the public URL and
   check it against the pinned sha256 (``sha256sum -c`` with the values from
   the package scripts). A new file can take a few minutes to reach the
   SourceForge download mirrors.

#. **Propose the clean-up.** List the files on the mirror that no maintained
   branch (RB-2.6, RB-2.7), no open SDK PR and no released tag downloads, with
   their sizes, and ask the maintainer to approve the list. Delete the approved
   files with ``rm`` in an ``sftp`` session.
