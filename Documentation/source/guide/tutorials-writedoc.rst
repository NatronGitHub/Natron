.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Writing documentation
=====================

This quick tutorial will guide you through the creation/modification of documentation for Natron and the plugins.

.. _writing-natron-manual:

Natron Manual
-------------

Writing contributions
~~~~~~~~~~~~~~~~~~~~~

Contributing to the Natron documentation is rather easy. The source for the documentation is located in the *Documentation/source* folder. 

The documentation is generated using `Sphinx <http://sphinx-doc.org>`_, and the source files are in `reStructuredText <http://docutils.sourceforge.net/docs/user/rst/quickref.html>`_ format.

Most likely you will want to contribute to the :ref:`user-guide`. The source files for the guide are located in the directory named ``Documentation/source/guide``. If you want to contribute to an already existing document just open the file in your favorite (plain) text-editor and do your modifications.

.. note:: The following files are generated automatically and can thus not be edited:

   - The `_group.rst` file, and any file with a name starting with `_group`.
   - The `_prefs.rst`.
   - The documentation for each individual plugin, which can be found in the `Documentation/source/plugins` directory (see :ref:`writing-plugins-manual`).

If you prefer editing with `LibreOffice <https://libreoffice.org>`_ (or even `MSWord <https://fr.wikipedia.org/wiki/Microsoft_Word>`_), just keep the document simple (use styles for section headers, don't try to format too much, etc.), and use `pandoc <https://pandoc.org/>`_ to get a first working version in `reStructuredText <http://docutils.sourceforge.net/docs/user/rst/quickref.html>`_ format: ``pandoc your_document.docx -t rst -o output_doc.rst``

This `reStructuredText <http://docutils.sourceforge.net/docs/user/rst/quickref.html>`_ file will probably require a few touch-ups afterwards, but it is usually a good starting point.

Submitting contributions
~~~~~~~~~~~~~~~~~~~~~~~~

To send your contributions, the best way is to follow the procedure below. However, if you wrote a nice piece of documentation, in any standard format, and have difficulties following that procedure, do not hesitate to ask for assistance on the `Natron forum <https://discuss.pixls.us/c/software/natron>`_, or to `file a GitHub issue <https://github.com/NatronGitHub/Natron/issues/new>`_, with your document attached to your message.

The standard procedure is the following:

- Fork `https://github.com/NatronGitHub/Natron <https://github.com/NatronGitHub/Natron>`_ using your github account.
- On your fork, `create a branch <https://help.github.com/en/articles/creating-and-deleting-branches-within-your-repository>`_ from the RB-2.4 branch (do not use the master branch), and give it a name like "documentation-keying" if you are going to write the keying doc (which we really need).
- To add your doc, you can either:

  - Clone the repository to your computer, edit and add files, commit your changes locally (the github desktop application is easy to use), and then push your changes,
  - Or edit the files directly on github. See `tutorials-hsvtool.rst <https://github.com/NatronGitHub/Natron/blob/RB-2.4/Documentation/source/guide/tutorials-hsvtool.rst>`_ for en example (you will probably need to fork the repository first, see below, and browse to that file on your fork). Click on the pencil icon on the top right. You get an editable the text view and can get a preview by clicking on the preview tab on top.

- Then, submit a `pull request <https://help.github.com/articles/about-pull-requests/>`_ to the RB-2.4 branch on the main repository from your branch (there is a button to submit a pull request when you view your fork on github). Give an accurate description of the pull request, and remember to follow the `Contributor Covenant Code of Conduct <https://www.contributor-covenant.org/version/1/4/code-of-conduct>`_, as with all contributions to Natron or the plugins. The Natron maintainers can either accept it as it is, or ask for a few modifications.

You can view the formatted documentation on your github repository, as explained above, but you can also preview your modifications by using `pandoc <https://pandoc.org/>`_ to convert it to another format, or install `Sphinx <http://sphinx-doc.org>`_ and recompile the whole documentation. On Linux and Mac you can install Sphinx through your package manager (using MacPorts type ``sudo port install py27-sphinx py27-sphinx_rtd_theme``, on HomeBrew type ``brew install sphinx-doc; /usr/local/opt/sphinx-doc/libexec/bin/pip3 install sphinx_rtd_theme```, on Linux type ``pip install sphinx sphinx_rtd_theme``), on Windows refer to the `Sphinx documentation <http://www.sphinx-doc.org/en/stable/install.html#windows-install-python-and-sphinx>`_.

When you have Sphinx installed go to the Documentation folder and launch the following command:

    sphinx-build -b html source html

The Natron documentation has now been generated in the *Documentation/html* folder. Open *Documentation/html/index.html* in your web browser to review your changes.

When your are satisfied with your modifications do a pull request against the master repository on GitHub.

.. note::  If you want to preview your files interactively you can use dedicated file editors. `RstPad <https://github.com/ShiraNai7/rstpad/releases>`_ for example is available on Mac and Windows 


.. _writing-plugins-manual:

Plugins Manual
--------------

The documentation for each plugin contains two parts:

- The main documentation, including the short description, and the documentation for individual parameters. This part of the documentation is available in the C++ source file of each plugin.
- An extra documentation, in the form of a Markdown file in the plugin bundle, named ``Contents/Resources/pluginId.md`` (in the same directory as the plugin icon files), where *pluginId* is the full plugin identifier (e.g. ``net.sf.openfx.MergePlugin``). The extra documentation is inserted after the *Description* section and before the *Inputs* section of the generated documentation.

Main Plugin Documentation
~~~~~~~~~~~~~~~~~~~~~~~~~

Editing or adding the main documentation for the Natron plugins requires you to edit the C++ source file for each plugin. Usually the plugin(s) has a **kPluginDescription** define where you can edit the description found when hovering or clicking the **?** button of the plugin properties panel in Natron. 

Let us say you want to edit the description in the Checkerboard plugin.

1. Fork the https://github.com/NatronGitHub/openfx-misc repository on GitHub.
2. Open the file *Checkerboard/Checkerboard.cpp* in your favorite (plain) text-editor

Navigate to the line **#define  kPluginDescription**, where you can edit the description. Line breaks are added with *\\n*. 

You will also notice that each parameter has a hint define, for example the Checkerboard has **#define kParamBoxSizeHint**, **#define kParamColor0Hint** etc. These describe each parameter in the plugin and shows up when you hover the parameter in Natron, or access the HTML documentation online or through Natron.

To test your modifications, you must build the plugin(s) and load them in Natron, refer to each plugin bundle on GitHub on how to build the plugin(s). Click the **?** button of the plugin properties panel in Natron to check your modifications.

Markdown
~~~~~~~~

The plugin description and parameters optionally supports `Markdown <https://daringfireball.net/projects/markdown/syntax>`_ format. This enables you to have more control over how the information is displayed.

Enabling Markdown on a plugin requires some modifications, as the plugin must tell the host (Natron) that it supports Markdown on the description and/or parameters. See the `Shadertoy <https://github.com/NatronGitHub/openfx-misc/blob/master/Shadertoy/Shadertoy.cpp>`_ plugin for an example of how this works.

Basically you need to add **desc.setPluginDescription(kPluginDescriptionMarkdown, true);** in the **describe** function for each plugin. If you are not comfortable with this, contact the repository maintainer(s) and ask them to enable Markdown for you.

Submitting contributions
~~~~~~~~~~~~~~~~~~~~~~~~

As with the :ref:`writing-natron-manual`, the standard way of submitting your contributions is by forking the relevant plugins repo on GitHub (`openfx-misc <https://github.com/NatronGitHub/openfx-misc>`_, `openfx-io <https://github.com/NatronGitHub/openfx-io>`_, `openfx-arena <https://github.com/NatronGitHub/openfx-arena>`_ or `openfx-gmic <https://github.com/NatronGitHub/openfx-gmic>`_) and submitting a `pull request <https://help.github.com/articles/about-pull-requests/>`_ to the *master* branch of that repo.

However, if you wrote a nice piece of documentation, in any standard format, and have difficulties following that procedure, do not hesitate to ask for assistance on the `Natron forum <https://discuss.pixls.us/c/software/natron>`_, or to `file a GitHub issue <https://github.com/NatronGitHub/Natron/issues/new>`_, with your document attached to your message.
