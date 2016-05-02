# patch __PREFIX__ in the Portfile post-patch stage

# Coerge CMake to use MacPorts' BZIP2
set(BZIP2_INCLUDE_DIR "__PREFIX__/include" CACHE PATH "The BZIP2 include directory" FORCE)
set(BZIP2_LIBRARY_RELEASE "__PREFIX__/lib/libbz2.dylib" CACHE FILEPATH "The BZIP2 Release Library" FORCE)
set(BZIP2_LIBRARY_DEBUG "BZIP2_LIBRARY_DEBUG-NOTFOUND" CACHE FILEPATH "The BZIP2 Debug Library" FORCE)

# Coerge CMake to use MacPorts' CURL
set(CURL_INCLUDE_DIR "__PREFIX__/include" CACHE PATH "The CURL include directory" FORCE)
set(CURL_LIBRARY "__PREFIX__/lib/libcurl.dylib" CACHE FILEPATH "The CURL Library" FORCE)

# Coerce CMake to use MacPorts' CURSES / NCURSES
set(CURSES_INCLUDE_PATH "__PREFIX__/include" CACHE PATH "The Curses Include Directory" FORCE)
set(CURSES_CURSES_LIBRARY "__PREFIX__/lib/libncurses.dylib" CACHE FILEPATH "The Curses Library" FORCE)
set(CURSES_FORM_LIBRARY "__PREFIX__/lib/libform.dylib" CACHE FILEPATH "The Form Library" FORCE)
set(CURSES_NCURSES_LIBRARY "__PREFIX__/lib/libncurses.dylib" CACHE FILEPATH "The NCurses Library" FORCE)

# Coerge CMake to use MacPorts' EXPAT
set(EXPAT_INCLUDE_DIR "__PREFIX__/include" CACHE PATH "The EXPAT include directory" FORCE)
set(EXPAT_LIBRARY "__PREFIX__/lib/libexpat.dylib" CACHE FILEPATH "The EXPAT Library" FORCE)

# Coerge CMake to use MacPorts' LibArchive
set(LibArchive_INCLUDE_DIR "__PREFIX__/include" CACHE PATH "The LibArchive include directory" FORCE)
set(LibArchive_LIBRARY "__PREFIX__/lib/libarchive.dylib" CACHE FILEPATH "The LibArchive Library" FORCE)

# Coerce CMake to use MacPorts' Python Executable
set(PYTHON_EXECUTABLE "__PREFIX__/bin/python__PYTHON_VERSION_WITH_DOT__" CACHE FILEPATH "The Python Executable to use" FORCE)

# Coerge CMake to use MacPorts' ZLIB
set(ZLIB_INCLUDE_DIR "__PREFIX__/include" CACHE PATH "The ZLIB include directory" FORCE)
set(ZLIB_LIBRARY_RELEASE "__PREFIX__/lib/libz.dylib" CACHE FILEPATH "The ZLIB Release Library" FORCE)
set(ZLIB_LIBRARY_DEBUG "ZLIB_LIBRARY_DEBUG-NOTFOUND" CACHE FILEPATH "The ZLIB Debug Library" FORCE)
