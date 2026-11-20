.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Writing documentation
=====================

This tutorial explains how to write or change the documentation of Natron and
of its plug-ins.

.. _writing-natron-manual:

Natron Manual
-------------

Writing contributions
~~~~~~~~~~~~~~~~~~~~~

The sources of this documentation are in the ``Documentation/source`` folder
of the Natron repository. They are written in
`reStructuredText <https://docutils.sourceforge.io/docs/user/rst/quickref.html>`_
and built with `Sphinx <https://www.sphinx-doc.org/>`_.

Most contributions go to the :ref:`user-guide`, in
``Documentation/source/guide``. To change a page, open its ``.rst`` file in
any plain-text editor.

.. note:: Some files are generated from Natron and the plug-ins, by
   ``tools/genStaticDocs.sh``, and must not be edited by hand:

   - the files whose name starts with ``_``, such as ``_group.rst`` and the
     other ``_group*.rst`` files (the Reference Guide), ``_nodes.rst``,
     ``_prefs.rst`` and ``_environment.rst``;
   - the documentation of each plug-in, in ``Documentation/source/plugins``
     (see :ref:`writing-plugins-manual`).

   Don't edit ``index.rst`` either, except to add a new guide to its table of
   contents.

If you prefer to write in a word processor such as
`LibreOffice <https://www.libreoffice.org/>`_, keep the document simple (use
styles for the headings, avoid manual formatting), and convert it with
`pandoc <https://pandoc.org/>`_:

.. code-block:: bash

   pandoc your_document.docx -t rst -o output_doc.rst

The result usually needs a few touch-ups, but it is a good start.

Building the documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~

Install Python 3, then create a virtual environment with the Sphinx version
used by the documentation, from the root of the Natron repository:

.. code-block:: bash

   python3 -m venv venv
   venv/bin/pip install -r Documentation/source/requirements.txt

(On Windows, the programs are in ``venv\Scripts`` instead of ``venv/bin``.)
Then build the HTML documentation:

.. code-block:: bash

   cd Documentation
   ../venv/bin/sphinx-build -b html source html

Open ``Documentation/html/index.html`` in a web browser to check your
changes. Sphinx prints a warning for each problem it finds, such as a broken
link or a bad indentation: fix the warnings that come from the pages you
changed.

Submitting contributions
~~~~~~~~~~~~~~~~~~~~~~~~

If you wrote documentation in any format and can't follow the procedure below,
ask for help on the `Natron forum <https://discuss.pixls.us/c/software/natron>`_
or `open a GitHub issue <https://github.com/NatronGitHub/Natron/issues/new/choose>`_
with your document attached.

The standard procedure is:

#. Fork https://github.com/NatronGitHub/Natron with your GitHub account.
#. On your fork,
   `create a branch <https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-and-deleting-branches-within-your-repository>`_
   from the ``RB-2.6`` branch (the current release branch, not ``master``),
   with a name such as ``docs-keying``.
#. Make your changes, either:

   - in a clone of your fork on your computer: edit the files, commit, and
     push the branch to your fork;
   - or directly on GitHub: browse to the file on your branch, click the
     pencil icon (**Edit this file**), and use the **Preview** tab to check
     the result. GitHub's preview of reStructuredText is only approximate:
     Sphinx-specific markup such as ``:ref:`` is not rendered.

#. Open a
   `pull request <https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-pull-requests>`_
   from your branch to the ``RB-2.6`` branch of NatronGitHub/Natron, and
   describe what you changed. The maintainers may ask for changes before
   merging it.

As for all contributions to Natron, please follow the
`Code of Conduct <https://github.com/NatronGitHub/Natron/blob/RB-2.6/CODE_OF_CONDUCT.md>`_.


.. _writing-plugins-manual:

Plugins Manual
--------------

The documentation of each plug-in has two parts:

- The main documentation: the description of the plug-in and of each of its
  parameters. It is in the C++ source file of the plug-in.
- Optional extra documentation, in a Markdown file named ``pluginId.md``
  (where *pluginId* is the full plug-in identifier, for example
  ``net.sf.openfx.MergePlugin.md``), placed next to the plug-in icon
  ``pluginId.png`` in the ``Contents/Resources`` folder of the plug-in
  bundle. It is inserted after the *Description* section of the generated
  page.

Main plug-in documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~

To change the main documentation of a plug-in, edit its C++ source file. Most
plug-ins define the description in a ``kPluginDescription`` macro: this is the
text shown when you click the **?** button of the properties panel in Natron.

For example, to change the description of the CheckerBoard plug-in:

#. Fork the https://github.com/NatronGitHub/openfx-misc repository on GitHub.
#. Open ``CheckerBoard/CheckerBoard.cpp`` in a plain-text editor.
#. Edit the text of ``#define kPluginDescription``. Line breaks are written
   ``\n``.

Each parameter also has a hint, for example ``kParamBoxSizeHint`` or
``kParamColor0Hint`` in CheckerBoard. The hint is the tooltip of the
parameter, and it is also used in the generated documentation.

To test your changes, build the plug-ins (see the README of each plug-in
repository), load them in Natron, and click the **?** button of the
properties panel.

Markdown
~~~~~~~~

The plug-in description and the parameter hints can be written in
Markdown instead of plain text, which gives more control over the layout. The
extra documentation file (``pluginId.md``, see above), for OpenFX plug-ins as
well as PyPlugs, is also Markdown.

Since Natron 2.6, Markdown follows the
`CommonMark <https://spec.commonmark.org/0.31.2/>`_ specification, as on GitHub
or GitLab: Natron converts it to HTML with
`md4c <https://github.com/mity/md4c>`_. These extensions are enabled: tables,
footnotes (``[^1]``), ``~~strikethrough~~``, ``==highlight==``,
``^superscript^``, LaTeX math (``$...$`` and ``$$...$$``, shown with MathJax in
the HTML documentation), and URLs, ``www.`` links and e-mail addresses as
links. Raw HTML is passed through. To write one of the characters ``^``, ``~``,
``$`` or ``=`` literally where it would start one of these, escape it with a
backslash (``\^``). The `CommonMark tutorial <https://commonmark.org/help/>`_
is a quick reference. Natron 2.5 and earlier used hoedown, which followed the
older rules of the original Markdown, so a few constructs render differently:
``_text_`` is emphasis (it was underlined), ``^superscript`` needs a closing
``^``, ``"text"`` is no longer a quotation (``<q>``), and some lists follow the
CommonMark rules.

The plug-in must tell Natron that its description is in Markdown, and keep a
plain-text description for other OpenFX hosts. See the
`Shadertoy <https://github.com/NatronGitHub/openfx-misc/blob/master/Shadertoy/Shadertoy.cpp>`_
plug-in for an example. In its ``describe`` function:

.. code-block:: c++

   if ( desc.getPropertySet().propGetDimension(kNatronOfxPropDescriptionIsMarkdown, false) ) {
       desc.setPluginDescription(kPluginDescriptionMarkdown, false);
       desc.setDescriptionIsMarkdown(true);
   } else {
       desc.setPluginDescription(kPluginDescription);
   }

If you are not comfortable with this, ask the maintainers of the repository
to do it.

Submitting plug-in contributions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As for the :ref:`Natron manual <writing-natron-manual>`, fork the plug-in
repository on GitHub
(`openfx-misc <https://github.com/NatronGitHub/openfx-misc>`_,
`openfx-io <https://github.com/NatronGitHub/openfx-io>`_,
`openfx-arena <https://github.com/NatronGitHub/openfx-arena>`_ or
`openfx-gmic <https://github.com/NatronGitHub/openfx-gmic>`_) and open a pull
request against its ``master`` branch.

If you can't follow that procedure, ask for help on the
`Natron forum <https://discuss.pixls.us/c/software/natron>`_ or
`open a GitHub issue <https://github.com/NatronGitHub/Natron/issues/new/choose>`_
with your document attached.
