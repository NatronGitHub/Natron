# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

set(tess_HEADERS
        dict-list.h
        dict.h
        geom.h
        memalloc.h
        mesh.h
        normal.h
        priorityq-heap.h
        priorityq-sort.h
        priorityq.h
        render.h
        sweep.h
        tess.h
        tessmono.h
        libtess.h
        libtess_internal.h
)
set(tess_SOURCES
        dict.c
        geom.c
        memalloc.c
        mesh.c
        normal.c
        priorityq-heap.c
        priorityq.c
        render.c
        sweep.c
        tess.c
        tessmono.c
)
add_library(tess STATIC ${tess_HEADERS} ${tess_SOURCES})
target_include_directories(tess SYSTEM PUBLIC .)
target_compile_options(tess PRIVATE ${NATRON_THIRD_PARTY_NO_WARNINGS})
set_target_properties(tess PROPERTIES POSITION_INDEPENDENT_CODE ON)
# No floating-point contraction (fused multiply-add): the sweep's geometric
# predicates (EdgeSign(), EdgeEval(), __gl_edgeIntersect()) must be evaluated
# the same way everywhere. clang contracts a*b+c by default on arm64 (and GCC
# with -ffp-contract=fast wherever FMA is available), x86-64 without -mfma
# can't, so the same shape was tessellated differently (or dropped) on macOS
# arm64 and on Linux/Windows x86-64 (#1510). This comes after any -ffast-math
# (NATRON_FAST_MATH), which enables contraction. MSVC doesn't contract by
# default (/fp:precise, no /fp:contract).
if(NOT MSVC)
    target_compile_options(tess PRIVATE -ffp-contract=off)
endif()
