# -*- coding: utf-8 -*-
"""Mermaid diagrams in every output format of the Natron documentation.

sphinxcontrib-mermaid renders ``.. mermaid::`` blocks in the browser for the
HTML builders (``mermaid_output_format = 'raw'``: no build-time tool needed).
The LaTeX (PDF) and ePub builders need images, which mermaid-cli (``mmdc``,
Node.js plus a headless Chrome) produces.  This extension:

- finds ``mmdc`` on ``PATH`` or in ``Documentation/node_modules/.bin`` (where
  ``npm install`` in ``Documentation/`` puts it, see package.json);
- for the non-HTML builders, renders a test diagram once, and if that works
  uses ``mmdc`` (PDF for LaTeX, PNG for ePub);
- otherwise replaces each diagram with its source in a literal block and a
  note, so that the PDF and ePub always build, whatever the build machine has.
"""

import os
import shutil
import subprocess
import tempfile

from docutils import nodes
from sphinx.util import logging
from sphinxcontrib.mermaid import mermaid as mermaid_node

logger = logging.getLogger(__name__)

_PROBE = "flowchart LR\n  A --> B\n"
# Documentation/, where "npm install" puts node_modules/
_DOCUMENTATION = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def _is_browser_builder(builder):
    """HTML output read in a web browser, which runs the mermaid JavaScript."""
    return builder.format == "html" and not builder.name.startswith("epub")


def _find_mmdc(app):
    cmd = app.config.mermaid_cmd
    if isinstance(cmd, str) and cmd and os.sep not in cmd:
        found = shutil.which(cmd)
        if found:
            return found
        local = os.path.join(_DOCUMENTATION, "node_modules", ".bin", cmd)
        if os.access(local, os.X_OK):
            return os.path.abspath(local)
        return None
    return cmd or None


def _probe_mmdc(app, mmdc, fmt):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "probe.mmd")
        out = os.path.join(tmp, "probe." + fmt)
        with open(src, "w", encoding="utf-8") as f:
            f.write(_PROBE)
        args = [mmdc] + list(app.config.mermaid_params) + ["-i", src, "-o", out]
        try:
            p = subprocess.run(args, capture_output=True, text=True, timeout=180)
        except (OSError, subprocess.TimeoutExpired) as e:
            return str(e)
        if p.returncode != 0 or not os.path.isfile(out):
            return (p.stderr or p.stdout or "no output file").strip()[-2000:]
    return None


def _builder_inited(app):
    app.natron_mermaid_fallback = False
    if _is_browser_builder(app.builder):
        if os.environ.get("READTHEDOCS") == "True":
            # Read the Docs builds the PDF and ePub after the HTML (and pull
            # request builds only the HTML): say in the log whether they will
            # get pictures.
            mmdc = _find_mmdc(app)
            error = "mmdc not found" if mmdc is None else _probe_mmdc(app, mmdc, "pdf")
            logger.info("mermaid: mermaid-cli check for the PDF and ePub: %s"
                        % ("works (%s)" % mmdc if error is None else "fails: " + error))
        return
    fmt = "pdf" if app.builder.format == "latex" else "png"
    mmdc = _find_mmdc(app)
    error = "mmdc not found" if mmdc is None else _probe_mmdc(app, mmdc, fmt)
    if error is None:
        app.config.mermaid_cmd = mmdc
        if app.builder.name.startswith("epub"):
            app.config.mermaid_output_format = "png"
        logger.info("mermaid: diagrams rendered by %s", mmdc)
        return
    logger.info(
        "mermaid: mermaid-cli is not usable (%s); the %s output shows the "
        "source of each diagram instead (run 'npm install' in Documentation/ "
        "to render them)" % (error, app.builder.name)
    )
    app.natron_mermaid_fallback = True


def _doctree_resolved(app, doctree, docname):
    if not getattr(app, "natron_mermaid_fallback", False):
        return
    for node in list(doctree.findall(mermaid_node)
                     if hasattr(doctree, "findall")
                     else doctree.traverse(mermaid_node)):
        code = node["code"]
        block = nodes.literal_block(code, code)
        block["language"] = "text"
        note = nodes.paragraph(
            "", "(Mermaid diagram: the HTML documentation shows it as a figure.)")
        container = nodes.container("", note, block, classes=["mermaid-source"])
        node.replace_self(container)


def setup(app):
    app.setup_extension("sphinxcontrib.mermaid")
    app.connect("builder-inited", _builder_inited)
    app.connect("doctree-resolved", _doctree_resolved)
    return {"version": "1.0", "parallel_read_safe": True,
            "parallel_write_safe": True}
