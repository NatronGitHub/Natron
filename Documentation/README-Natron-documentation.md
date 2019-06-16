Natron Documentation
====================

Documentation included in Natron and available on Read the Docs. Requires Sphinx and the [Sphinx RtD theme](https://sphinx-rtd-theme.readthedocs.io) to build. Theme is custom for Natron, and will not work as intended without the Natron Documentation Server.

**Note**: Don't edit index.rst or files prefixed with _ , or anything in the plugins folder.

The sources are in the "source" directory.

To generate the plugins documentation in rst format, for example on macOS from a snapshot installed in `/Applications/Natron.app`:
```
env FONTCONFIG_FILE=/Applications/Natron.app/Contents/Resources/etc/fonts/fonts.conf OCIO=/Applications/Natron.app/Contents/Resources/OpenColorIO-Configs/nuke-default/config.ocio OFX_PLUGIN_PATH=/Applications/Natron.app/Contents/Plugins ../tools/genStaticDocs.sh /Applications/Natron.app/Contents/MacOS/NatronRenderer  /var/tmp/natrondocs$$ .
```

To generate the html doc, with output in the `html` directory:
```
sphinx-build -b html source html
```

To generate the PDF doc, with output in the `pdf` directory (output is `pdf/Natron.pdf`):
```
sphinx-build -b latex source pdf
(cd pdf; pdflatex Natron)
```
