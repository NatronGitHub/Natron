Natron binaries include components from the following libraries:

# Main components

* Python (Modified BSD) https://www.python.org/download/releases/2.7/license/
* PySide (LGPL 2.1) https://wiki.qt.io/About_PySide
* libshiboken (LGPL 2.1) https://github.com/qtproject/pyside-shiboken
* Qt (LGPL 2.1 with Qt LGPL Exception) http://doc.qt.io/qt-4.8/lgpl.html
* GLEW (Modified BSD) http://glew.sourceforge.net/credits.html
* Boost (Boost Software License) http://www.boost.org/users/license.html
* Cairo (LGPL 2.1) http://cairographics.org/
* openfx (Modified BSD) https://github.com/ofxa/openfx
* openfx-io (GPLv2) https://github.com/MrKepzie/openfx-io
* openfx-misc (GPLv2) https://github.com/devernay/openfx-misc
* openfx-arena (GPLv2) https://github.com/olear/openfx-arena

# Additional dependencies

## openfx-io

* OpenImageIO (Modified BSD) https://openimageio.org
* OpenColorIO (Modified BSD) http://opencolorio.org/License.html
* FFmpeg (LGPL 2.1) https://www.ffmpeg.org/legal.html
* SeExpr (modified Apache 2.0) http://www.disneyanimation.com/technology/seexpr.html

### Video codecs and formats

* x264 (GPLv2) http://www.videolan.org/developers/x264.html (used by openfx-io via FFmpeg) 
* libogg (3-clause BSD) https://xiph.org/ogg/ (used by openfx-io via FFmpeg)
* libvorbis (3-clause BSD) https://xiph.org/vorbis/ (used by openfx-io via FFmpeg)
* libtheora (3-clause BSD) http://www.theora.org/ (used by openfx-io via FFmpeg)
* libspeex (3-clause BSD) http://www.speex.org/ (used by openfx-io via FFmpeg)
* libopus (3-clause BSD) http://opus-codec.org/ (used by openfx-io via FFmpeg)
* libmodplug (Public Domain) http://modplug-xmms.sourceforge.net/ (used by openfx-io via FFmpeg) 
* libmp3lame (LGPL 2.1) http://lame.sourceforge.net/ (used by openfx-io via FFmpeg) 
* orc (BSD) http://code.entropywave.com/orc/ (used by openfx-io via FFmpeg and schroedinger) 
* Schroedinger (MIT) http://diracvideo.org/ (used by openfx-io via FFmpeg) 

### Image formats

* OpenEXR & IlmBase (Modified BSD)  http://www.openexr.com/license.html (used by openfx-io via OpenImageIO) 
* libtiff (Modified BSD) http://www.remotesensing.org/libtiff (used by openfx-io via OpenImageIO) 
* libpng (libpng/zlib License) http://www.libpng.org (used by openfx-io via OpenImageIO) 
* libjpeg (Modified BSD) http://ijg.org (used by openfx-io via OpenImageIO) 
* openjpeg (2-clauses BSD) http://www.openjpeg.org (used by openfx-io via OpenImageIO) 
* webp (Modified BSD) https://developers.google.com/speed/webp (used by openfx-io via OpenImageIO) 
* HDF5 (Modified BSD) https://www.hdfgroup.org/HDF5 (used by openfx-io via OpenImageIO) 
* Field3D (Modified BSD) https://github.com/imageworks/Field3D (used by openfx-io via OpenImageIO) 
* GIFLIB (X Consortium-like licence) http://giflib.sourceforge.net (used by openfx-io via OpenImageIO) 
* LibRaw (LGPL 2.1 or CDDL 1.0 license) http://www.libraw.org (used by openfx-io via OpenImageIO) 
* JasPer (MIT) http://www.ece.uvic.ca/~frodo/jasper/ (used by openfx-io via LibRaw) 

## openfx-arena

* ImageMagick (Apache 2.0 with attribution clause) http://www.imagemagick.org/script/license.php (used by openfx-arena)
* fontconfig (Modified BSD) http://www.freedesktop.org/wiki/Software/fontconfig/ (used by openfx-arena)

## Indirect dependencies

* TinyXML (libpng/zlib license) http://www.sourceforge.net/projects/tinyxml (used by openfx-io via OpenColorIO)
* Pango (LGPL 2.1) http://www.pango.org/ (used by openfx-arena via ImageMagick) 
* FreeType (Modified BSD) http://git.savannah.gnu.org/cgit/freetype/freetype2.git/plain/docs/FTL.TXT (used by Natron, openfx-io and openfx-arena via Cairo, ImageMagick and OpenImageIO) 
* Expat (MITX) http://expat.sourceforge.net/ (used by openfx) 
* librsvg (LGPL 2.1) http://live.gnome.org/LibRsvg (used by openfx-arena via ImageMagick) 
* libxml (MIT) http://www.xmlsoft.org/(used by openfx-arena via ImageMagick via librsvg)
* Pixman (MIT) http://www.pixman.org/ (used by Natron via Cairo)
* libmng (libpng/zlib License) http://www.libpng.org/pub/mng/ (used by Qt)
* OpenSSL (Apache 1.0 or 4-clause BSD) http://www.openssl.org (used by Qt and Python)
* gdk_pixbuf (LGPL 2.1) https://developer.gnome.org/gdk-pixbuf/stable/ (used by openfx-arena via librsvg)
* Little CMS (MIT) http://www.littlecms.com/ (used by Qt and openfx-io via libmng and LibRaw)
* libcroco (LGPL 2.1) https://github.com/GNOME/libcroco (used by openfx-arena via librsvg)
* GLib (LGPL 2.1) http://www.gtk.org (used by openfx-arena via libcroco, harfbuzz, Pango, librsvg)
* HarfBuzz (MIT) http://www.freedesktop.org/wiki/Software/HarfBuzz/ (used by Qt and openfx-arena via Pango)
* libffi (MIT) https://sourceware.org/libffi/ (used Python and openfx-arena via GLib)
* ICU (MIT) http://www.icu-project.org/ (used by Boost)
* Graphite (LGPL 2.1) http://graphite.sil.org (used by openfx-arena via HarfBuzz)
* libedit (BSD) http://thrysoee.dk/editline/ (used by Python) 
* Berkeley DB (AGPLv3) http://www.oracle.com/us/products/database/berkeley-db (used by Python) 
* SQLite (Public Domain) https://www.sqlite.org/ (used by Python)
* bzip2 (Modified BSD) http://www.bzip.org (used by Natron, openfx-io and openfx-arena via FreeType and FFmpeg) 
* liblzma (Public Domain) http://tukaani.org/xz/ (used by openfx-io and openfx-arena via FFmpeg and libxml) 
* zlib (libpng/zlib License) http://www.zlib.net/
* libiconv (LGPL 2.1) https://www.gnu.org/software/libiconv/
* libintl (LGPL 2.1) https://www.gnu.org/software/gettext/
* ncurses (LGPL 2.1) https://www.gnu.org/software/ncurses/

