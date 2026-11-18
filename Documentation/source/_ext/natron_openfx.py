# -*- coding: utf-8 -*-
"""Include the OpenFX documentation of the libs/OpenFX submodule.

The OpenFX Programming Guide, Reference and Release Notes live in
NatronGitHub/openfx (``Documentation/sources``), the ``libs/OpenFX``
submodule. A toctree can only list documents under the Sphinx source
directory, so ``prepare()``, called from conf.py:

1. copies ``Guide/``, ``Reference/`` and ``ReleaseNotes/`` into
   ``devel/openfx/`` (ignored by git), through a temporary directory: only
   the files that changed are rewritten, so incremental builds only read
   again what changed;
2. runs Doxygen on ``libs/OpenFX/include`` (as OpenFX's own build does),
   writing the XML to ``Documentation/openfx-doxygen/`` (also ignored), and
   generates the breathe API pages (``Reference/api/``);
3. replaces the API index of the copy by one without the ``doxygenindex``,
   which declares again everything the API pages declare (see ``_adapt``);
4. returns the breathe configuration, the ``exclude_patterns`` and an
   ``|openfx_commit|`` substitution.

If the submodule is not checked out, breathe or Doxygen is missing, or
``NATRON_DOCS_OPENFX=0`` (the documentation bundled with Natron), it writes
short placeholder pages instead, so that the rest of the documentation still
builds and its links to these pages still resolve.
"""

import filecmp
import os
import shutil
import subprocess
import sys
import tempfile

SUBDIRS = ("Guide", "Reference", "ReleaseNotes")

ONLINE = "https://natron.readthedocs.io/en/rb-2.6/devel/openfx.html"

PLACEHOLDER = """\
{orphan}{title}
{underline}

.. note::

   This part of the documentation comes from the OpenFX documentation of
   the ``libs/OpenFX`` submodule, which is not included in this build:
   {reason}. Read it in the `online documentation <{online}>`_, or in the
   `NatronGitHub/openfx repository
   <https://github.com/NatronGitHub/openfx/tree/master/Documentation/sources>`_.
"""

# The placeholder pages: the indexes of the toctree, and the pages that
# Natron's own pages link to.
PLACEHOLDERS = {
    "Guide/index.rst": "OpenFX Programming Guide",
    "Reference/index.rst": "OpenFX reference",
    "ReleaseNotes/index.rst": "OpenFX Release Notes",
    "Reference/ofxForkExtensions.rst": "Extensions in the NatronGitHub fork",
}


def _log(msg):
    print("natron_openfx: " + msg, file=sys.stderr)


def _git_commit(path):
    try:
        out = subprocess.run(["git", "-C", path, "rev-parse", "HEAD"],
                             capture_output=True, text=True, check=True)
        return out.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def _write_placeholders(stage, reason):
    _log("OpenFX documentation not included: " + reason)
    for rel, title in PLACEHOLDERS.items():
        path = os.path.join(stage, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            f.write(PLACEHOLDER.format(
                orphan="" if rel.endswith("index.rst") else ":orphan:\n\n",
                title=title, underline="=" * len(title), reason=reason,
                online=ONLINE))


def _sync(stage, dest):
    """Make dest a copy of stage, rewriting only the files that changed, so
    that Sphinx only reads again the pages that changed."""
    keep = set()
    for dirpath, _dirnames, filenames in os.walk(stage):
        for name in filenames:
            src = os.path.join(dirpath, name)
            rel = os.path.relpath(src, stage)
            keep.add(rel)
            out = os.path.join(dest, rel)
            if os.path.isfile(out) and filecmp.cmp(src, out, shallow=False):
                continue
            os.makedirs(os.path.dirname(out), exist_ok=True)
            shutil.copy2(src, out)
    for dirpath, _dirnames, filenames in os.walk(dest, topdown=False):
        for name in filenames:
            path = os.path.join(dirpath, name)
            if os.path.relpath(path, dest) not in keep:
                os.remove(path)
        if not os.listdir(dirpath) and dirpath != dest:
            os.rmdir(dirpath)


def _newest_mtime(top):
    newest = 0
    for dirpath, _dirnames, filenames in os.walk(top):
        for name in filenames:
            newest = max(newest, os.path.getmtime(os.path.join(dirpath, name)))
    return newest


def _run_doxygen(include_dir, outdir):
    """Doxygen XML of the OpenFX headers, rebuilt only when a header changed."""
    xml = os.path.join(outdir, "xml")
    index = os.path.join(xml, "index.xml")
    if os.path.isfile(index) and os.path.getmtime(index) >= _newest_mtime(include_dir):
        return xml
    shutil.rmtree(outdir, ignore_errors=True)
    os.makedirs(outdir)
    with open(os.path.join(include_dir, "ofx.doxy"), encoding="utf-8") as f:
        doxyfile = f.read()
    # Later settings override earlier ones: write outside the submodule.
    doxyfile += "\nOUTPUT_DIRECTORY = %s\nQUIET = YES\nWARNINGS = NO\n" % outdir
    log = os.path.join(outdir, "doxygen.log")
    with open(log, "w", encoding="utf-8") as f:
        subprocess.run(["doxygen", "-"], input=doxyfile, text=True, cwd=include_dir,
                       check=True, stdout=f, stderr=subprocess.STDOUT)
    _log("Doxygen XML written to %s (log: %s)" % (xml, log))
    return xml


def prepare(confdir):
    natron = os.path.normpath(os.path.join(confdir, "..", ".."))
    ofx = os.path.join(natron, "libs", "OpenFX")
    dest = os.path.join(confdir, "devel", "openfx")
    result = {"breathe_projects": {}, "rst_epilog": "", "exclude_patterns": []}

    # (an uninitialized submodule is an empty directory, in which git would
    # report Natron's own commit)
    commit = _git_commit(ofx) if os.path.exists(os.path.join(ofx, ".git")) else None
    result["rst_epilog"] = ".. |openfx_commit| replace:: ``%s``\n" % (
        commit[:12] if commit else "unknown")

    with tempfile.TemporaryDirectory() as stage:
        xml = _stage(confdir, ofx, stage)
        if xml:
            _log("included the OpenFX documentation of libs/OpenFX at %s" % commit)
            result["breathe_projects"] = {"ofx_reference": xml}
            # Only used through ".. include::" in ofxCoreAPI.rst.
            result["exclude_patterns"] = ["devel/openfx/Reference/ofxLoadingSequence.rst"]
        _sync(stage, dest)
    return result


def _stage(confdir, ofx, stage):
    """Write the pages of devel/openfx/ into stage; return the Doxygen XML
    directory, or None if only placeholders were written."""
    src = os.path.join(ofx, "Documentation", "sources")
    if os.environ.get("NATRON_DOCS_OPENFX", "1") == "0":
        # tools/jenkins/gen-natron-doc.sh: keep the documentation bundled
        # with Natron small (the OpenFX part is about 10 MB of HTML).
        _write_placeholders(stage, "it is left out of the documentation "
                            "bundled with Natron (``NATRON_DOCS_OPENFX=0``)")
        return None
    if not os.path.isfile(os.path.join(src, "Reference", "index.rst")):
        _write_placeholders(stage, "the submodule is not checked out "
                            "(``git submodule update --init libs/OpenFX``)")
        return None
    try:
        import breathe  # noqa: F401
    except ImportError:
        _write_placeholders(stage, "breathe is not installed (``pip install -r "
                            "Documentation/source/requirements.txt``)")
        return None
    if shutil.which("doxygen") is None:
        _write_placeholders(stage, "Doxygen is not installed")
        return None

    try:
        version = subprocess.run(["doxygen", "--version"], capture_output=True,
                                 text=True).stdout.split()[0]
        if tuple(int(x) for x in version.split(".")[:2]) >= (1, 18):
            # As OpenFX's Documentation/build.sh says.
            _log("Doxygen %s writes the suite function pointers in a form breathe "
                 "cannot parse (\"Error in declarator\" warnings, missing "
                 "declarations); use Doxygen 1.17 or earlier" % version)
    except (OSError, IndexError, ValueError):
        pass
    doxygen_out = os.path.normpath(os.path.join(confdir, "..", "openfx-doxygen"))
    try:
        xml = _run_doxygen(os.path.join(ofx, "include"), doxygen_out)
        for sub in SUBDIRS:
            shutil.copytree(os.path.join(src, sub), os.path.join(stage, sub),
                            ignore=shutil.ignore_patterns("api", "*.pyc", "__pycache__"))
        # The breathe API pages, as OpenFX's Documentation/build.sh makes them.
        subprocess.run([sys.executable, "-m", "breathe.apidoc", "-p", "ofx_reference",
                        "-m", "--force", "-q",
                        "-g", "class,interface,struct,union,file,namespace,group",
                        "-o", os.path.join(stage, "Reference", "api"), xml],
                       check=True, stdout=subprocess.DEVNULL)
    except (OSError, subprocess.CalledProcessError) as e:
        shutil.rmtree(stage)
        os.makedirs(stage)
        _write_placeholders(stage, "Doxygen or breathe.apidoc failed (%s)" % e)
        return None
    _adapt(stage)
    return xml


# The "Complete Reference Index" (doxygenindex) of OpenFX's DoxygenIndex.rst
# declares again everything that the per-file and per-struct API pages
# declare (about 2000 duplicate-declaration warnings); keep only the API pages,
# and list the group pages that breathe.apidoc also generates.
DOXYGEN_INDEX = """\
.. _doxygenIndex:

==============================
Auto-generated Reference Index
==============================

.. toctree::
   :caption: API
   :maxdepth: 4

   api/filelist
   api/structlist
   api/grouplist
"""


def _rewrite(path, text):
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def _adapt(dest):
    """Small fixes to the copy, which Natron's build would otherwise warn about."""
    index = os.path.join(dest, "Reference", "DoxygenIndex.rst")
    if os.path.isfile(index):
        _rewrite(index, DOXYGEN_INDEX)


# The warnings that OpenFX's own Documentation/build.sh expects from these
# pages (its EXPECTED_ERRS), hidden here too, but only for devel/openfx/:
# they are about the OpenFX sources, and would drown Natron's own warnings.
import logging as _logging  # noqa: E402
import re as _re  # noqa: E402

EXPECTED = _re.compile(r"Explicit markup ends without|Duplicate C.*declaration|"
                       r"Declaration is|cpp:func targets a member")


class _ExpectedOpenFXWarnings(_logging.Filter):
    def __init__(self):
        super().__init__()
        self.hidden = 0

    def filter(self, record):
        if record.levelno < _logging.WARNING:
            return True
        location = getattr(record, "location", None)
        if isinstance(location, tuple):
            location = location[0]
        elif location is not None and hasattr(location, "source"):
            location = location.source
        if (location and "devel/openfx/" in str(location).replace(os.sep, "/")
                and EXPECTED.search(record.getMessage())):
            if not getattr(record, "natron_openfx_hidden", False):
                record.natron_openfx_hidden = True  # counted once, not per handler
                self.hidden += 1
            return False
        return True


def setup(app):
    flt = _ExpectedOpenFXWarnings()
    for handler in _logging.getLogger("sphinx").handlers:
        handler.filters.insert(0, flt)

    def report(app, exception):
        if flt.hidden:
            print("natron_openfx: hid %d warnings that OpenFX's own build expects "
                  "in devel/openfx/ (see EXPECTED in _ext/natron_openfx.py)"
                  % flt.hidden, file=sys.stderr)
    app.connect("build-finished", report)
    return {"version": "1.0", "parallel_read_safe": True,
            "parallel_write_safe": True}
