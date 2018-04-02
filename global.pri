# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

# libs may modify the config (eg openmp), so it must be included before
include(libs.pri)

CONFIG += exceptions warn_on no_keywords
DEFINES += OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_VEGAS OFX_SUPPORTS_PARAMETRIC OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_NATRON OFX_EXTENSIONS_RESOLVE OFX_SUPPORTS_OPENGLRENDER
DEFINES += OFX_SUPPORTS_MULTITHREAD
DEFINES += OFX_SUPPORTS_DIALOG
#Commented-out because many plug-in vendors do not implement it correctly
#DEFINES += OFX_SUPPORTS_DIALOG_V1

#Since in Natron and OpenFX all strings are supposed UTF-8 and that the constructor
#for QString(const char*) assumes ASCII strings, we may run into troubles
DEFINES += QT_NO_CAST_FROM_ASCII

# To run Natron without Python functionnalities (for debug purposes)
run-without-python {
    message("Natron will run (not build) without Python")
    DEFINES += NATRON_RUN_WITHOUT_PYTHON
} else {
    # from <https://docs.python.org/3/c-api/intro.html#include-files>:
    # "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
    CONFIG += python
    QMAKE_CFLAGS += -include Python.h
    QMAKE_CXXFLAGS += -include Python.h
}

*g++* | *clang* | *xcode* {
#See https://bugreports.qt.io/browse/QTBUG-35776 we cannot use
# QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_OBJECTIVE_CFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

    QMAKE_CFLAGS_WARN_ON += -Wall -Wextra -Wmissing-prototypes -Wmissing-declarations -Wno-multichar -Winit-self -Wno-long-long
    QMAKE_CXXFLAGS_WARN_ON += -Wall -Wextra -Wno-multichar -Winit-self -Wno-long-long
    #QMAKE_CFLAGS_WARN_ON += -pedantic
    #QMAKE_CXXFLAGS_WARN_ON += -pedantic
    #QMAKE_CXXFLAGS_WARN_ON += -Weffc++

    CONFIG(relwithdebinfo) {
        CONFIG += release
        DEFINES *= NDEBUG
        QMAKE_CXXFLAGS += -O2 -g
        QMAKE_CXXFLAGS -= -O3
        #Remove the -s flag passed in release mode by qmake so binaries don't get stripped
        QMAKE_LFLAGS_RELEASE =
    }
    CONFIG(fast) {
        QMAKE_CFLAGS_RELEASE += -Ofast
        QMAKE_CFLAGS_RELEASE -= -O -O2 -O3
        QMAKE_CXXFLAGS_RELEASE += -Ofast
        QMAKE_CXXFLAGS_RELEASE -= -O -O2 -O3
        QMAKE_CFLAGS_DEBUG += -Ofast
        QMAKE_CFLAGS_DEBUG -= -O -O2 -O3
        QMAKE_CXXFLAGS_DEBUG += -Ofast
        QMAKE_CXXFLAGS_DEBUG -= -O -O2 -O3
        QMAKE_CXXFLAGS -= -O -O2 -O3
        QMAKE_CFLAGS -= -O -O2 -O3
    }
}

win32-g++ {
    # on Mingw LD is extremely slow in  debug mode and can take hours. Optimizing the build seems to make it faster
    CONFIG(debug, debug|release){
        QMAKE_CXXFLAGS += -O -g
    }
}

win32-msvc* {
    CONFIG(relwithdebinfo) {
        CONFIG += release
        DEFINES *= NDEBUG
        QMAKE_CXXFLAGS_RELEASE += -Zi
        QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF
    }
}

CONFIG(debug, debug|release){
    DEFINES *= DEBUG
} else {
    DEFINES *= NDEBUG
}

enable-breakpad {
    include(breakpadclient.pri)
}

CONFIG(noassertions) {
#See http://doc.qt.io/qt-4.8/debug.html
   DEFINES *= NDEBUG QT_NO_DEBUG QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT
}

CONFIG(snapshot) {
   message("Compiling an official snapshot (should only be done on the Natron build farm).")
   DEFINES += NATRON_CONFIG_SNAPSHOT
   CONFIG_SET=1
}
CONFIG(alpha) {
	message("Compiling Natron in alpha version (should only be done on the Natron build farm).")
	DEFINES += NATRON_CONFIG_ALPHA
	CONFIG_SET=1
}
CONFIG(beta) {
	message("Compiling Natron in beta version (should only be done on the Natron build farm).")
	DEFINES += NATRON_CONFIG_BETA
	CONFIG_SET=1
}
CONFIG(RC) {
	message("Compiling Natron in release candidate version (should only be done on the Natron build farm).")
	DEFINES += NATRON_CONFIG_RC
	CONFIG_SET=1
}
CONFIG(stable) {
	message("Compiling Natron in stable version (should only be done on the Natron build farm).")
	DEFINES += NATRON_CONFIG_STABLE
	CONFIG_SET=1
}
CONFIG(custombuild) {
	message("Compiling Natron with a custom version for $$BUILD_USER_NAME")
	#BUILD_USER_NAME should be defined reflecting the user name that should appear in Natron.
        DEFINES += NATRON_CUSTOM_BUILD_USER_TOKEN=\"$$BUILD_USER_NAME\"
	CONFIG_SET=1
}

isEmpty(BUILD_NUMBER) {
	DEFINES += NATRON_BUILD_NUMBER=0
} else {
	DEFINES += NATRON_BUILD_NUMBER=$$BUILD_NUMBER
}

# https://qt.gitorious.org/qt-creator/qt-creator/commit/b48ba2c25da4d785160df4fd0d69420b99b85152
unix:LIBS += $$QMAKE_LIBS_DYNLOAD

*g++* {
  QMAKE_CXXFLAGS += -ftemplate-depth-1024
  GCCVer = $$system($$QMAKE_CXX --version)
  contains(GCCVer,[0-3]\\.[0-9]+.*) {
  } else {
    contains(GCCVer,4\\.7.*) {
      QMAKE_CXXFLAGS += -Wno-c++11-extensions
    }
    contains(GCCVer,[5-9]\\.[0-9]+.*) {
      # Eigen uses std::binder1st which is deprecated in C++11
      QMAKE_CXXFLAGS += -Wno-deprecated-declarations
    }
    contains(GCCVer,[6-9]\\.[0-9]+.*) {
      # older versions of boost (at least up to 1.65.1) fail to compile with:
      # /usr/include/boost/crc.hpp:350:9: error: right operand of shift expression '(18446744073709551615 << 64)' is >= than the precision of the left operand [-fpermissive]
      # see https://github.com/MrKepzie/Natron/issues/1659
      # -fpermissive turns it into a warning
      QMAKE_CXXFLAGS += -fpermissive
      # GCC 6 and later are C++14 by default, but Qt 4 is C++98
      # Note: disabled, because qmake should put the right flags anyway
      #lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=gnu++98

      # clear some Eigen3 warnings
      QMAKE_CFLAGS += -Wno-int-in-bool-context
      QMAKE_CXXFLAGS += -Wno-int-in-bool-context
    }
    contains(GCCVer,[7-9]\\.[0-9]+.*) {
      # clear a lot of boost warnings
      QMAKE_CFLAGS += -Wno-expansion-to-defined
      QMAKE_CXXFLAGS += -Wno-expansion-to-defined
    }
  }
  c++11 {
    # check for at least version 4.7
    contains(GCCVer,[0-3]\\.[0-9]+.*) {
      error("At least GCC 4.6 is required.")
    } else {
      contains(GCCVer,4\\.[0-5].*) {
        error("At least GCC 4.6 is required.")
      } else {
        contains(GCCVer,4\\.6.*) {
          QMAKE_CXXFLAGS += -std=c++0x
        } else {
          QMAKE_CXXFLAGS += -std=c++11
        }
      }
    }
  }
}

openmp {
  QMAKE_CXXFLAGS += -fopenmp
  QMAKE_CFLAGS += -fopenmp
  QMAKE_LFLAGS += -fopenmp
}

macx {
  # Set the pbuilder version to 46, which corresponds to Xcode >= 3.x
  # (else qmake generates an old pbproj on Snow Leopard)
  QMAKE_PBUILDER_VERSION = 46

  QMAKE_MACOSX_DEPLOYMENT_VERSION = $$split(QMAKE_MACOSX_DEPLOYMENT_TARGET, ".")
  QMAKE_MACOSX_DEPLOYMENT_MAJOR_VERSION = $$first(QMAKE_MACOSX_DEPLOYMENT_VERSION)
  QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION = $$last(QMAKE_MACOSX_DEPLOYMENT_VERSION)
  universal {
    message("Compiling for universal OSX $${QMAKE_MACOSX_DEPLOYMENT_MAJOR_VERSION}.$$QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION")
    contains(QMAKE_MACOSX_DEPLOYMENT_MAJOR_VERSION, 10) {
      contains(QMAKE_MACOSX_DEPLOYMENT_TARGET, 4)|contains(QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION, 5) {
        # OSX 10.4 (Tiger) and 10.5 (Leopard) are x86/ppc
        message("Compiling for universal ppc/i386")
        CONFIG += x86 ppc
      }
      contains(QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION, 6) {
        message("Compiling for universal i386/x86_64")
        # OSX 10.6 (Snow Leopard) may run on Intel 32 or 64 bits architectures
        CONFIG += x86 x86_64
      }
      # later OSX instances only run on x86_64, universal builds are useless
      # (unless a later OSX supports ARM)
    }
  } 

  #link against the CoreFoundation framework for the StandardPaths functionnality
  LIBS += -framework CoreServices
    
  #// Disable availability macros on macOS
  #// because we may be using libc++ on an older macOS,
  #// so that std::locale::numeric may be available
  #// even on macOS < 10.9.
  #// see _LIBCPP_AVAILABILITY_LOCALE_CATEGORY
  #// in /opt/local/libexec/llvm-5.0/include/c++/v1/__config
  #// and /opt/local/libexec/llvm-5.0/include/c++/v1/__locale
  DEFINES += _LIBCPP_DISABLE_AVAILABILITY
}

macx-clang-libc++ {
    # in Qt 4.8.7, objective-C misses the stdlib and macos version flags
    QMAKE_OBJECTIVE_CFLAGS += -stdlib=libc++ -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET
}

macx-clang {
    # in Qt 4.8.7, objective-C misses the stdlib and macos version flags
    QMAKE_OBJECTIVE_CFLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET
}

CONFIG(debug) {
    CONFIG += nopch
}

# CONFIG+=nopch disables precompiled headers
!nopch {
  !macx|!universal {
    # precompiled headers don't work with multiple archs
    CONFIG += precompile_header
  }
}

!macx {
  # c++11 build fails on Snow Leopard 10.6 (see the macx section below)
  #CONFIG += c++11
}

win32 {
  #ofx needs WINDOWS def
  #microsoft compiler needs _MBCS to compile with the multi-byte character set.
  #DEFINES += _MBCS
  DEFINES += WINDOWS COMPILED_FROM_DSP XML_STATIC  NOMINMAX
  DEFINES += _UNICODE UNICODE
 
  DEFINES += QHTTP_SERVER_STATIC 

  #System library is required on windows to map network share names from drive letters
  LIBS += -lmpr



  # Natron requires a link to opengl32.dll and Gdi32 for offscreen rendering
  LIBS += -lopengl32 -lGdi32


}

win32-g++ {
    # -municode is needed for linking because it uses wmain() instead of the traditional main()
    # https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/
    QMAKE_CFLAGS += -municode
    QMAKE_CXXFLAGS += -municode
    QMAKE_LFLAGS += -municode
}

win32-msvc* {
    CONFIG(64bit){
        message("Compiling for architecture x86 64 bits")
        Release:DESTDIR = x64/release
        Release:OBJECTS_DIR = x64/release/.obj
        Release:MOC_DIR = x64/release/.moc
        Release:RCC_DIR = x64/release/.rcc
        Release:UI_DIR = x64/release/.ui

        Debug:DESTDIR = x64/debug
        Debug:OBJECTS_DIR = x64/debug/.obj
        Debug:MOC_DIR = x64/debug/.moc
        Debug:RCC_DIR = x64/debug/.rcc
        Debug:UI_DIR = x64/debug/.ui
    } else {
        message("Compiling for architecture x86 32 bits")
        Release:DESTDIR = win32/release
        Release:OBJECTS_DIR = win32/release/.obj
        Release:MOC_DIR = win32/release/.moc
        Release:RCC_DIR = win32/release/.rcc
        Release:UI_DIR = win32/release/.ui

        Debug:DESTDIR = win32/debug
        Debug:OBJECTS_DIR = win32/debug/.obj
        Debug:MOC_DIR = win32/debug/.moc
        Debug:RCC_DIR = win32/debug/.rcc
        Debug:UI_DIR = win32/debug/.ui
    }
    #System library is required on windows to map network share names from drive letters
    LIBS += mpr.lib
}


win32-g++ {
   # On MingW everything is defined with pkgconfig except boost
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig

    expat:     PKGCONFIG += expat
    cairo:     PKGCONFIG += cairo
    equals(QT_MAJOR_VERSION, 5) {
        shiboken:  INCLUDEPATH += $$system(python2 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")/PySide2/include/shiboken
    	pyside:    INCLUDEPATH += $$system(python2 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")/PySide2/include/PySide2
   	pyside:    INCLUDEPATH += $$system(python2 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")/PySide2/include/PySide2/QtCore
    }
    equals(QT_MAJOR_VERSION, 4) {
        shiboken:  PKGCONFIG += shiboken-py2
    	pyside:    PKGCONFIG += pyside-py2
   	pyside:    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py2)/QtCore
        pyside:    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py2)/QtGui
    }
    python:    PKGCONFIG += python-2.7
    boost:     LIBS += -lboost_serialization-mt
    boost:     LIBS += -lboost_serialization-mt
	
    #See http://stackoverflow.com/questions/16596876/object-file-has-too-many-sections
    Debug:	QMAKE_CXXFLAGS += -Wa,-mbig-obj
}

unix {
     #  on Unix systems, only the "boost" option needs to be defined in config.pri
     QT_CONFIG -= no-pkg-config
     CONFIG += link_pkgconfig
     expat:     PKGCONFIG += expat

     # GLFW will require a link to X11 on linux and OpenGL framework on OS X
     linux-* {
          LIBS += -lGL -lX11
         # link with static cairo on linux, to avoid linking to X11 libraries in NatronRenderer
         cairo {
             PKGCONFIG += pixman-1 freetype2 fontconfig
             LIBS +=  $$system(pkg-config --variable=libdir cairo)/libcairo.a
         }
         LIBS += -ldl
         QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/../lib\',-z,origin'
     } else {
         LIBS += -framework OpenGL
         cairo:     PKGCONFIG += cairo
     }

     # User may specify an alternate python2-config from the command-line,
     # as in "qmake PYTHON_CONFIG=python2.7-config"
     isEmpty(PYTHON_CONFIG) {
         PYTHON_CONFIG = python2-config
     }
     python {
          #PKGCONFIG += python
          LIBS += -L$$system($$PYTHON_CONFIG --exec-prefix)/lib $$system($$PYTHON_CONFIG --ldflags)
          PYTHON_CFLAGS = $$system($$PYTHON_CONFIG --includes)
          PYTHON_INCLUDEPATH = $$find(PYTHON_CFLAGS, ^-I.*)
          PYTHON_INCLUDEPATH ~= s/^-I(.*)/\\1/g
          INCLUDEPATH *= $$PYTHON_INCLUDEPATH
     }

     equals(QT_MAJOR_VERSION, 5) {
         shiboken:  INCLUDEPATH += $$system(python2 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")/PySide2/include/shiboken
    	 pyside:    INCLUDEPATH += $$system(python2 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")/PySide2/include/PySide2
   	 pyside:    INCLUDEPATH += $$system(python2 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")/PySide2/include/PySide2/QtCore
     }

     equals(QT_MAJOR_VERSION, 4) {
         # There may be different pyside.pc/shiboken.pc for different versions of python.
         # pkg-config will probably give a bad answer, unless python2 is the system default.
         # See for example tools/travis/install_dependencies.sh for a solution that works on Linux,
         # using a custom config.pri
         shiboken: PKGCONFIG += shiboken
         pyside:   PKGCONFIG += pyside
         # The following hack also works with Homebrew if pyside is installed with option --with-python3
         macx {
           QMAKE_LFLAGS += '-Wl,-rpath,\'@loader_path/../Frameworks\''
           shiboken {
             PKGCONFIG -= shiboken
             PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --exec-prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
             INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir shiboken)
             # the sed stuff is to work around an Xcode generator bug
             LIBS += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --libs shiboken | sed -e s/-undefined\\ dynamic_lookup//)
           }
           pyside {
             PKGCONFIG -= pyside
             PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --exec-prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
             INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)
             INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
             equals(QT_MAJOR_VERSION, 4) {
               # QtGui include are needed because it looks for Qt::convertFromPlainText which is defined in
               # qtextdocument.h in the QtGui module.
               INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
               INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$${QMAKE_LIBDIR_QT}/pkgconfig pkg-config --variable=includedir QtGui)
             }
             LIBS += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --libs pyside)
           }
         }
     }
} #unix

*xcode* {
  # redefine cxx flags as qmake tends to automatically add -O2 to xcode projects
  QMAKE_CFLAGS -= -O2
  QMAKE_CXXFLAGS -= -O2
  QMAKE_CXXFLAGS += -ftemplate-depth-1024

  # all libraries in Natron are static, so visibility can be hidden by default.
  symbols_hidden_by_default.name = GCC_SYMBOLS_PRIVATE_EXTERN
  symbols_hidden_by_default.value = YES
  QMAKE_MAC_XCODE_SETTINGS += symbols_hidden_by_default
  c++11 {
    QMAKE_CXXFLAGS += -std=c++11
  }
}

*clang* {
  QMAKE_CXXFLAGS += -ftemplate-depth-1024
  QMAKE_CXXFLAGS_WARN_ON += -Wno-c++11-extensions
  c++11 {
    QMAKE_CXXFLAGS += -std=c++11
  }
}

# see http://clang.llvm.org/docs/AddressSanitizer.html and http://blog.qt.digia.com/blog/2013/04/17/using-gccs-4-8-0-address-sanitizer-with-qt/
addresssanitizer {
  *xcode* {
    enable_cxx_container_overflow_check.name = CLANG_ADDRESS_SANITIZER_CONTAINER_OVERFLOW
    enable_cxx_container_overflow_check.value = YES
    QMAKE_MAC_XCODE_SETTINGS += enable_cxx_container_overflow_check  
  }
  *g++* | *clang* {
  message("Compiling with AddressSanitizer (for gcc >= 4.8 and clang). Set the ASAN_SYMBOLIZER_PATH environment variable to point to the llvm-symbolizer binary, or make sure llvm-symbolizer in in your PATH.")
  message("To compile with clang, use a clang-specific spec, such as unsupported/linux-clang, unsupported/macx-clang, linux-clang or macx-clang.")
  message("For example, with Qt4 on OS X:")
  message("  sudo port install clang-3.4")
  message("  sudo port select clang mp-clang-3.4")
  message("  export ASAN_SYMBOLIZER_PATH=/opt/local/bin/llvm-symbolizer-mp-3.4")
  message("  qmake -spec unsupported/macx-clang CONFIG+=addresssanitizer ...")
  message("see http://clang.llvm.org/docs/AddressSanitizer.html")
  CONFIG += debug
  QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1
  QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1
  QMAKE_LFLAGS += -fsanitize=address -g

#  QMAKE_LFLAGS += -fsanitize-blacklist=../asan_blacklist.ignore
#  QMAKE_CLAGS += -fsanitize-blacklist=../asan_blacklist.ignore
#  QMAKE_CFLAGS += -fsanitize-blacklist=../asan_blacklist.ignore
  }
}

# see http://clang.llvm.org/docs/ThreadSanitizer.html
threadsanitizer {
  message("Compiling with ThreadSanitizer (for clang).")
  message("see http://clang.llvm.org/docs/ThreadSanitizer.html")
  CONFIG += debug
  QMAKE_CFLAGS += -fsanitize=thread -O1
  QMAKE_CFLAGS += -fsanitize=thread -O1
  QMAKE_LFLAGS += -fsanitize=thread -g
}

coverage {
  QMAKE_CFLAGS += -fprofile-arcs -ftest-coverage -O0
  QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage
  QMAKE_CLEAN += $(OBJECTS_DIR)/*.gcda $(OBJECTS_DIR)/*.gcno
}

# install targets on unix
unix:!macx {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
    target.path = $${PREFIX}/bin
    target_icons.path = $${PREFIX}/share/pixmaps
    target_icons.files = $PWD/../Gui/Resources/Images/natronIcon256_linux.png $PWD/../Gui/Resources/Images/natronProjectIcon_linux.png
    target_mime.path = $${PREFIX}/share/mime/packages
    target_mime.files = $PWD/../Gui/Resources/Mime/x-natron.xml
    target_desktop.path = $${PREFIX}/share/applications
    target_desktop.files = $PWD/../Gui/Resources/Applications/Natron.desktop
    INSTALLS += target_icons target_mime target_desktop
}

# and finally...
include(config.pri)
