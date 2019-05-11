.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
.. _writeDocumentation:

Writing documentation
=====================

This quick tutorial will guide you through the creation/modification of documentation for Natron and the plugins.

Plugins
-------

Editing/adding documentation for the Natron plugins requires you to edit the CPP file for each plugin. Usually the plugin(s) has a **kPluginDescription** define where you can edit the description found when hovering or clicking the **?** button in Natron. 

Let's start with an example, you want to edit the description in the Checkerboard plugin.

1. Fork the https://github.com/NatronGitHub/openfx-misc repository on Github.
2. Open the file *Checkerboard/Checkerboard.cpp* in your favorite (plain) text-editor

Navigate to the line **#define  kPluginDescription**, where you can edit the description. Line breaks are added with *\\n*. 

You will also notice that each parameter has a hint define, for example the Checkerboard has **#define kParamBoxSizeHint**, **#define kParamColor0Hint** etc. These describe each parameter in the plugin and shows up when you hover the parameter in Natron, or access the HTML documentation online or through Natron.

To test your modification you must build the plugin(s) and load them in Natron, refer to each plugin bundle on Github on how to build the plugin(s).

When you are done do a pull request on the master repository on Github.

Markdown
~~~~~~~~

The plugin description and parameters supports `Markdown <https://daringfireball.net/projects/markdown/syntax>`_ format. This enables you to have more control over how the information is displayed.

Enabling Markdown on a plugin requires some modifications, as the plugin must tell the host (Natron) that it supports Markdown on the description and/or parameters. See the `Shadertoy <https://github.com/NatronGitHub/openfx-misc/blob/master/Shadertoy/Shadertoy.cpp>`_ plugin for an example of how this works.

Basically you need to add **desc.setPluginDescription(kPluginDescriptionMarkdown, true);** in the **describe** function for each plugin. If you are not comfortable with this, contact the repository maintainer(s) and ask them to enable Markdown for you.

Natron
------

Contributing to the Natron documentation is a bit easier than contributing to the plugins. First fork the Natron repository on Github, https://github.com/NatronGitHub/Natron. The source for the documentation is located in the *Documentation/source* folder. 

The documentation is generated using `Sphinx <http://www.sphinx-doc.org>`_, and the source files are in `reStructuredText <http://docutils.sourceforge.net/docs/user/rst/quickref.html>`_ format.

Most likely you will want to contribute to the User Guide. The guide is located in *Documentation/source/guide*. If you want to contribute to an already existing document just open the file in your favorite (plain) text-editor and do your modifications.

If you prefer editing with `LibreOffice <https://libreoffice.org>`_ (or even `MSWord <https://fr.wikipedia.org/wiki/Microsoft_Word>`_), just keep the document simple (use styles for section headers, don't try to format too much, etc.), and use `pandoc <https://pandoc.org/>`_ to get a first working version in `reStructuredText <http://docutils.sourceforge.net/docs/user/rst/quickref.html>`_ format. This file will probably require a few touch-ups afterwards, but it is usually a good starting point.

To send your contributions, you will need to:

- Fork `https://github.com/NatronGitHub/Natron <https://github.com/NatronGitHub/Natron>`_ using your github account.
- On your fork, create a branch from the RB-2.3 branch (do not use the master branch), and give it a name like "documentation-keying" if you are going to write the keying doc (which we really need).
- To add your doc, you can either:

  - clone the repository to your computer, edit and add files, commit your changes locally (the github desktop application is easy to use), and then push your changes,
  - or edit the files directly on github. see `tutorials-hsvtool.rst <https://github.com/NatronGitHub/Natron/blob/RB-2.3/Documentation/source/guide/tutorials-hsvtool.rst>`_ for example (you will probably need to fork the repository first, see below, and browse to that file on your fork): click on the pencil icon on the top right. You get an editable the text view and can get a preview by clicking on the preview tab on top.

- Then, submit a `pull request <https://help.github.com/articles/about-pull-requests/>`_ to the RB-2.3 branch on the main repository from your branch (there is a button to submit a pull request when you view your fork on github), and the Natron maintainers can either accept it as it is, or ask for a few modifications.

You can view the formatted documentation on your github repository, as explained above, but you can also preview your modifications by using `pandoc <https://pandoc.org/>`_ to convert it to another format, or install `Sphinx <http://sphinx-doc.org>`_ and recompile the whole documentation. On Linux and Mac you can install Sphinx through your package manager (using MacPorts type ``sudo port install py27-sphinx``, on HomeBrew type ``pip install sphinx``), on Windows refer to the `Sphinx documentation <http://www.sphinx-doc.org/en/stable/install.html#windows-install-python-and-sphinx>`_.

When you have Sphinx installed go to the Documentation folder and launch the following command:

    sphinx-build -b html source html

The Natron documentation has now been generated in the *Documentation/html* folder. Open *Documentation/html/index.html* in your web browser to review your changes.

When your are satisfied with your modifications do a pull request against the master repository on Github.

.. note:: do not modify the files in *Documentation/source/plugins*, these are automatically generated by Natron and updated when needed.
