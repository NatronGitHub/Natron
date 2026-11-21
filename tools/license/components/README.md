Natron binaries include components from the following libraries:

# Main components

* Python (Modified BSD) https://www.python.org/download/releases/2.7/license/
* PySide (LGPL 2.1) https://wiki.qt.io/About_PySide
* libshiboken (LGPL 2.1) https://github.com/qtproject/pyside-shiboken
* Qt (LGPL 2.1 with Qt LGPL Exception) http://doc.qt.io/qt-4.8/lgpl.html
* Boost (Boost Software License) http://www.boost.org/users/license.html
* Cairo (LGPL 2.1) http://cairographics.org/
* GLAD (MIT) http://glad.dav1d.de/
* GLFW (libpng/zlib license) http://www.glfw.org/
* openfx (Modified BSD) https://github.com/ofxa/openfx
* openfx-io (GPLv2) https://github.com/NatronGitHub/openfx-io
* openfx-misc (GPLv2) https://github.com/NatronGitHub/openfx-misc
* openfx-arena (GPLv2) https://github.com/NatronGitHub/openfx-arena
* openfx-gmic (CeCILL-C, LGPL-like) https://github.com/NatronGitHub/openfx-gmic
* Eigen 3 (MPL 2.0) http://eigen.tuxfamily.org/
* Ceres Solver (New BSD) http://ceres-solver.org/
* gflags (New BSD) http://gflags.github.io/gflags/
* glog (New BSD) https://github.com/google/glog
* libmv (BSD) https://developer.blender.org/project/profile/59/
* openMVG (MPL 2.0) http://imagine.enpc.fr/~moulonp/openMVG/
* qhttpserver (BSD) https://github.com/nikhilm/qhttpserver
* md4c (MIT) https://github.com/mity/md4c
* libtess (SGI Free Software License B, X11-like) ftp://ftp.freedesktop.org/pub/mesa/glu/

# Additional dependencies

## openfx-misc

* Mesa (MIT) http://www.mesa3d.org
* TinyThread++ (libpng/zlib License) http://tinythreadpp.bitsnbites.eu/
* CImg (CeCILL-C, LGPL-like) http://cimg.eu/

## openfx-io

* OpenImageIO (Modified BSD) https://openimageio.org
* OpenColorIO (Modified BSD) http://opencolorio.org/License.html
* FFmpeg (GPL 2.0 or later in GPL builds, LGPL 2.1 or later in COMMERCIAL builds) https://www.ffmpeg.org/legal.html
* SeExpr (modified Apache 2.0) http://www.disneyanimation.com/technology/seexpr.html
* pstream (Boost Software License) http://pstreams.sourceforge.net

### Video codecs and formats

* x264 (GPLv2) http://www.videolan.org/developers/x264.html (used by openfx-io via FFmpeg) 
* x265 (GPLv2) http://x265.org/ (used by openfx-io via FFmpeg) 
* libogg (3-clause BSD) https://xiph.org/ogg/ (used by openfx-io via FFmpeg)
* libvorbis (3-clause BSD) https://xiph.org/vorbis/ (used by openfx-io via FFmpeg)
* libtheora (3-clause BSD) http://www.theora.org/ (used by openfx-io via FFmpeg)
* libspeex (3-clause BSD) http://www.speex.org/ (used by openfx-io via FFmpeg)
* libopus (3-clause BSD) http://opus-codec.org/ (used by openfx-io via FFmpeg)
* libmodplug (Public Domain) http://modplug-xmms.sourceforge.net/ (used by openfx-io via FFmpeg) 
* libmp3lame (LGPL 2.1) http://lame.sourceforge.net/ (used by openfx-io via FFmpeg) 
* openh264 (BSD) http://www.openh264.org/ (used by openfx-io via FFmpeg)
* orc (BSD) http://code.entropywave.com/orc/ (used by openfx-io via FFmpeg and schroedinger) 
* Snappy (3-clause BSD) http://diracvideo.org/ (used by openfx-io via FFmpeg) 

### Image formats

* OpenEXR & IlmBase (Modified BSD)  http://www.openexr.com/license.html (used by openfx-io via OpenImageIO) 
* libtiff (Modified BSD) http://www.remotesensing.org/libtiff (used by openfx-io via OpenImageIO) 
* JBIG-KIT (GPL 2.0 or later) https://www.cl.cam.ac.uk/~mgk25/jbigkit/ (Windows GPL builds only, used by libtiff; COMMERCIAL builds use a libtiff without it)
* libpng (libpng/zlib License) http://www.libpng.org (used by openfx-io via OpenImageIO and openfx-arena via ImageMagick)
* libjpeg (Modified BSD) http://ijg.org (used by openfx-io via OpenImageIO) 
* openjpeg (2-clauses BSD) http://www.openjpeg.org (used by openfx-io via OpenImageIO) 
* webp (Modified BSD) https://developers.google.com/speed/webp (used by openfx-io via OpenImageIO) 
* HDF5 (Modified BSD) https://www.hdfgroup.org/HDF5 (used by openfx-io via OpenImageIO) 
* Field3D (Modified BSD) https://github.com/imageworks/Field3D (used by openfx-io via OpenImageIO) 
* GIFLIB (X Consortium-like licence) http://giflib.sourceforge.net (used by openfx-io via OpenImageIO) 
* LibRaw (LGPL 2.1 or CDDL 1.0 license; GPL builds also include LibRaw's GPL 2.0 demosaic pack, with the AHD-Mod, AFD, VCD, Mixed and LMMSE methods, which makes that LibRaw GPL 2.0 or later; the GPL 3.0 pack is not included) https://www.libraw.org (used by openfx-io directly and via OpenImageIO) 
* JasPer (MIT) http://www.ece.uvic.ca/~frodo/jasper/ (used by openfx-io via LibRaw) 
* libheif (LGPL 3.0; GPL builds also include its x264 and x265 encoders, which are GPL 2.0, COMMERCIAL builds a libheif without them) https://github.com/strukturag/libheif (used by openfx-io via OpenImageIO)
* libde265 (LGPL 3.0) https://github.com/strukturag/libde265 (used by openfx-io via libheif)
* kvazaar (3-clause BSD) https://github.com/ultravideo/kvazaar (used by openfx-io via libheif)

## openfx-arena

* ImageMagick (Apache 2.0 with attribution clause) http://www.imagemagick.org/script/license.php
* fontconfig (Modified BSD) http://www.freedesktop.org/wiki/Software/fontconfig/
* Pango (LGPL 2.1) http://www.pango.org/
* Little CMS (MIT) http://www.littlecms.com/
* librsvg (LGPL 2.1) http://live.gnome.org/LibRsvg
* libzip (BSD) http://www.nih.at/libzip/
* lodepng (zlib) http://lodev.org/lodepng/
* Cairo (LGPL 2.1) http://cairographics.org/
* librevenge (MPL 2.0) https://sourceforge.net/p/libwpd/librevenge/
* libcdr (MPL 2.0) https://github.com/LibreOffice/libcdr
* libxml (MIT) http://www.xmlsoft.org/
* poppler (GPL 2.0) https://poppler.freedesktop.org/ (used by openfx-arena via ReadPDF) 

## openfx-gmic

* G'MIC (CeCILL-C, LGPL-like) http://gmic.eu/
* CImg (CeCILL-C, LGPL-like) http://cimg.eu/
* pocketfft (3-clause BSD) https://github.com/mreineck/pocketfft

## Indirect dependencies

* TinyXML (libpng/zlib license) http://www.sourceforge.net/projects/tinyxml (used by openfx-io via OpenColorIO and openfx-arena via OpenColorIO)
* FreeType (Modified BSD) http://git.savannah.gnu.org/cgit/freetype/freetype2.git/plain/docs/FTL.TXT (used by Natron, openfx-io and openfx-arena via Cairo, ImageMagick and OpenImageIO) 
* Expat (MITX) http://expat.sourceforge.net/ (used by openfx) 
* Pixman (MIT) http://www.pixman.org/ (used by Natron and openfx-arena via Cairo)
* libmng (libpng/zlib License) http://www.libpng.org/pub/mng/ (used by Qt)
* OpenSSL 3 (Apache 2.0) https://www.openssl.org (used by Qt and Python). The Apache 2.0 license is compatible with the GPL 3.0 but not with the GPL 2.0 only: since Natron and the openfx-io, openfx-misc and openfx-arena plug-ins are GPL 2.0 or later, the binaries that include OpenSSL 3 are distributed under the GPL 3.0.
* gdk_pixbuf (LGPL 2.1) https://developer.gnome.org/gdk-pixbuf/stable/ (used by openfx-arena via librsvg)
* Little CMS (MIT) http://www.littlecms.com/ (used by Qt and openfx-io via libmng and LibRaw)
* libcroco (LGPL 2.1) https://github.com/GNOME/libcroco (used by openfx-arena via librsvg)
* GLib (LGPL 2.1) http://www.gtk.org (used by Qt and openfx-arena via libcroco, harfbuzz, Pango, librsvg)
* HarfBuzz (MIT) http://www.freedesktop.org/wiki/Software/HarfBuzz/ (used by Qt and openfx-arena via Pango)
* libffi (MIT) https://sourceware.org/libffi/ (used Python and openfx-arena via GLib)
* ICU (MIT) http://www.icu-project.org/ (used by Boost and openfx-arena via libcdr)
* Graphite (LGPL 2.1) http://graphite.sil.org (used by openfx-arena via HarfBuzz)
* libedit (BSD) http://thrysoee.dk/editline/ (used by Python) 
* Berkeley DB (AGPLv3) http://www.oracle.com/us/products/database/berkeley-db (used by Python) 
* SQLite (Public Domain) https://www.sqlite.org/ (used by Python)
* bzip2 (Modified BSD) http://www.bzip.org (used by Natron, openfx-io and openfx-arena via FreeType and FFmpeg) 
* liblzma (Public Domain) http://tukaani.org/xz/ (used by openfx-io and openfx-arena via FFmpeg and libxml) 
* zlib (libpng/zlib License) http://www.zlib.net/
* libiconv (LGPL 2.1) https://www.gnu.org/software/libiconv/
* libintl (LGPL 2.1) https://www.gnu.org/software/gettext/
* ncurses (MIT-style X11 license) https://invisible-island.net/ncurses/ (used by Python via the readline and curses modules)
* GNU Readline (GPL 3.0 or later) https://tiswww.case.edu/php/chet/readline/rltop.html (used by Python via the readline module, Linux binaries)

