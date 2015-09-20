# Coerce CMake to use the MacPorts ncurses library
# patch __PREFIX__ in the Portfile post-patch stage
set(CURSES_CURSES_LIBRARY "__PREFIX__/lib/libncurses.dylib" CACHE FILEPATH "The Curses Library" FORCE)

# Coerce CMake to use the correct Python Executable
set(PYTHON_EXECUTABLE "__PREFIX__/bin/python__PYTHON_VERSION_WITH_DOT__" CACHE FILEPATH "The Python Executable to use" FORCE)
