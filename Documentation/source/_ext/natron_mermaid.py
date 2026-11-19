# -*- coding: utf-8 -*-
"""Mermaid diagrams in every output format of the Natron documentation.

The tools come from ``npm ci`` in ``Documentation/`` (see package.json and
package-lock.json), which installs mermaid.js and mermaid-cli (``mmdc``, which
draws with a headless Chrome) into ``Documentation/node_modules``:

- HTML: sphinxcontrib-mermaid draws the diagrams in the browser, with the
  mermaid.js of node_modules copied into ``_static/mermaid/`` (no CDN, so the
  offline and in-app documentation work too). The ``.mjs`` files are renamed
  ``.js``, because Natron's documentation server only serves ``.js``.
- LaTeX (PDF) and ePub: a test diagram is rendered once with ``mmdc``; if that
  works, the diagrams become PDF (LaTeX) or PNG (ePub) pictures.

If sphinxcontrib-mermaid is not installed (e.g. the SDKs that build the
installers), or the tool a format needs is missing or broken, each diagram is
shown as its source with a short note, and the log says why ("mermaid:"
lines). The documentation always builds.
"""

import os
import re
import shutil
import subprocess
import tempfile

from docutils import nodes
from docutils.parsers.rst import Directive, directives
from sphinx.util import logging

try:
    from sphinxcontrib.mermaid import mermaid as mermaid_node
except ImportError:
    mermaid_node = None

logger = logging.getLogger(__name__)

_PROBE = "flowchart LR\n  A --> B\n"
# Documentation/, where "npm ci" puts node_modules/
_DOCUMENTATION = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
_MERMAID_DIST = os.path.join(_DOCUMENTATION, "node_modules", "mermaid", "dist")
_MERMAID_JS = "mermaid/mermaid.esm.min.js"  # under _static/
_NOTE = ("Diagram source (Mermaid). The online documentation at "
         "https://natron.readthedocs.io shows it as a figure.")


def _is_browser_builder(builder):
    """HTML output read in a web browser, which runs the mermaid JavaScript."""
    return builder.format == "html" and not builder.name.startswith("epub")


def _source_block(code, caption=None):
    block = nodes.literal_block(code, code)
    block["language"] = "text"
    children = [nodes.paragraph("", _NOTE), block]
    if caption:
        children.append(nodes.paragraph("", "", nodes.emphasis("", caption)))
    return nodes.container("", *children, classes=["mermaid-source"])


class _SourceOnlyMermaid(Directive):
    """``mermaid`` directive used when sphinxcontrib-mermaid is not installed."""
    has_content = True
    optional_arguments = 1
    final_argument_whitespace = True
    option_spec = {"alt": directives.unchanged, "caption": directives.unchanged,
                   "align": directives.unchanged, "name": directives.unchanged,
                   "zoom": directives.flag, "config": directives.unchanged,
                   "title": directives.unchanged}

    def run(self):
        if self.arguments:
            path = self.state.document.settings.env.relfn2path(self.arguments[0])[1]
            with open(path, encoding="utf-8") as f:
                code = f.read()
        else:
            code = "\n".join(self.content)
        return [_source_block(code, self.options.get("caption"))]


def _find_mmdc(app):
    """The mmdc of node_modules first (the version package.json pins)."""
    cmd = app.config.mermaid_cmd
    if isinstance(cmd, str) and cmd and os.sep not in cmd:
        local = os.path.join(_DOCUMENTATION, "node_modules", ".bin", cmd)
        if os.access(local, os.X_OK):
            return local
        return shutil.which(cmd)
    return cmd or None


def _probe_mmdc(app, mmdc, fmt):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "probe.mmd")
        out = os.path.join(tmp, "probe." + fmt)
        with open(src, "w", encoding="utf-8") as f:
            f.write(_PROBE)
        args = [mmdc] + list(app.config.mermaid_params) + ["-i", src, "-o", out]
        try:
            p = subprocess.run(args, capture_output=True, text=True, timeout=60)
        except (OSError, subprocess.TimeoutExpired) as e:
            return str(e)
        if p.returncode != 0 or not os.path.isfile(out):
            return (p.stderr or p.stdout or "no output file").strip()[-2000:]
    return None


def _builder_inited(app):
    app.natron_mermaid_fallback = False
    if _is_browser_builder(app.builder):
        if os.path.isfile(os.path.join(_MERMAID_DIST, "mermaid.esm.min.mjs")):
            app.config.mermaid_use_local = _MERMAID_JS
            logger.info("mermaid: HTML diagrams drawn by the mermaid.js of %s"
                        % _MERMAID_DIST)
            return
        reason = "%s not found" % _MERMAID_DIST
    else:
        fmt = "pdf" if app.builder.format == "latex" else "png"
        mmdc = _find_mmdc(app)
        reason = "mmdc not found" if mmdc is None else _probe_mmdc(app, mmdc, fmt)
        if reason is None:
            app.config.mermaid_cmd = mmdc
            if app.builder.name.startswith("epub"):
                app.config.mermaid_output_format = "png"
            logger.info("mermaid: diagrams rendered by %s" % mmdc)
            return
    logger.info("mermaid: %s; the %s output shows the source of each diagram "
                "instead (run 'npm ci' in Documentation/ to draw them)"
                % (reason, app.builder.name))
    app.natron_mermaid_fallback = True


def _doctree_resolved(app, doctree, docname):
    if not app.natron_mermaid_fallback:
        return
    for node in list(doctree.findall(mermaid_node)
                     if hasattr(doctree, "findall")
                     else doctree.traverse(mermaid_node)):
        node.replace_self(_source_block(node["code"]))


_MJS_IMPORT = re.compile(r'("\./[^"]*?)\.mjs"')


def _copy_mermaid_js(app, exception):
    """Copy mermaid.js (the ES module and its chunks) to _static/mermaid/."""
    if (exception is not None or app.natron_mermaid_fallback
            or not _is_browser_builder(app.builder)):
        return
    dest = os.path.join(app.outdir, "_static", "mermaid")
    files = ["mermaid.esm.min.mjs"]
    chunks = os.path.join("chunks", "mermaid.esm.min")
    files += [os.path.join(chunks, f)
              for f in sorted(os.listdir(os.path.join(_MERMAID_DIST, chunks)))
              if f.endswith(".mjs")]
    for rel in files:
        with open(os.path.join(_MERMAID_DIST, rel), encoding="utf-8") as f:
            text = _MJS_IMPORT.sub(r'\1.js"', f.read())
        out = os.path.join(dest, rel[:-len(".mjs")] + ".js")
        os.makedirs(os.path.dirname(out), exist_ok=True)
        if os.path.isfile(out):
            with open(out, encoding="utf-8") as f:
                if f.read() == text:
                    continue
        with open(out, "w", encoding="utf-8") as f:
            f.write(text)


def setup(app):
    if mermaid_node is None:
        logger.info("mermaid: sphinxcontrib-mermaid is not installed; diagrams "
                    "are shown as source (pip install -r "
                    "Documentation/source/requirements.txt to draw them)")
        app.add_directive("mermaid", _SourceOnlyMermaid)
        return {"version": "2.0", "parallel_read_safe": True,
                "parallel_write_safe": True}
    app.setup_extension("sphinxcontrib.mermaid")
    app.connect("builder-inited", _builder_inited)
    app.connect("doctree-resolved", _doctree_resolved)
    app.connect("build-finished", _copy_mermaid_js)
    return {"version": "2.0", "parallel_read_safe": True,
            "parallel_write_safe": True}
