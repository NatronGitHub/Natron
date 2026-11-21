# TestsOfxCacheCorrupt (Tests/CMakeLists.txt): Natron discards an OFX plug-in cache that it cannot read,
# scans all the plug-ins again and rewrites the cache.
# The cache that the Tests run wrote (in SRC_CACHE_DIR/OFXLoadCache, or in NATRON_DISK_CACHE_PATH if set)
# is copied to WORK_DIR, with an element that a plug-in cache never has, which makes HostSupport's
# readCache() throw (openfx #218; OfxHost::loadOFXPlugins() catches it). The Tests binary (TESTS) then
# runs on that copy, keeping the OFX cache (NATRON_TESTS_KEEP_OFX_CACHE). It must pass, and the cache
# must be the one of a full scan again: without the bad element, with as many plug-ins as before.
foreach(v TESTS SRC_CACHE_DIR WORK_DIR)
    if(NOT DEFINED ${v})
        message(FATAL_ERROR "OfxCacheCorrupt.cmake: ${v} is not set")
    endif()
endforeach()

# NATRON_DISK_CACHE_PATH, if set, takes precedence over NATRON_TESTS_CACHE_DIR (Tests/wmain.cpp): Tests
# then wrote its cache there
if(NOT "$ENV{NATRON_DISK_CACHE_PATH}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{NATRON_DISK_CACHE_PATH}" SRC_CACHE_DIR)
endif()
file(GLOB caches "${SRC_CACHE_DIR}/OFXLoadCache/OFXCache_*.xml")
list(LENGTH caches ncaches)
if(NOT ncaches EQUAL 1)
    message(FATAL_ERROR "expected one OFX plug-in cache in ${SRC_CACHE_DIR}/OFXLoadCache, found ${ncaches}")
endif()
get_filename_component(name "${caches}" NAME)
file(READ "${caches}" original)
string(REGEX MATCHALL "<plugin " plugins "${original}")
list(LENGTH plugins nplugins)
string(FIND "${original}" "<apiproperties>" pos)
if(nplugins EQUAL 0 OR pos EQUAL -1)
    message(FATAL_ERROR "${caches} lists no plug-in")
endif()
string(REPLACE "<apiproperties>" "<apiproperties><corrupt />" corrupt "${original}")

file(REMOVE_RECURSE "${WORK_DIR}")
file(WRITE "${WORK_DIR}/OFXLoadCache/${name}" "${corrupt}")

set(ENV{NATRON_DISK_CACHE_PATH} "")
set(ENV{NATRON_TESTS_CACHE_DIR} "${WORK_DIR}")
set(ENV{NATRON_TESTS_KEEP_OFX_CACHE} "1")
execute_process(COMMAND "${TESTS}" "--gtest_filter=OfxTestPlugin*:OfxPluginObsolete.*"
    RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Tests failed with a corrupt OFX plug-in cache (${rc})")
endif()

file(READ "${WORK_DIR}/OFXLoadCache/${name}" rewritten)
string(FIND "${rewritten}" "<corrupt" bad)
string(REGEX MATCHALL "<plugin " plugins "${rewritten}")
list(LENGTH plugins nrewritten)
if(NOT bad EQUAL -1)
    message(FATAL_ERROR "the corrupt OFX plug-in cache was not rewritten")
endif()
if(NOT nrewritten EQUAL nplugins)
    message(FATAL_ERROR "the rewritten OFX plug-in cache lists ${nrewritten} plug-ins instead of ${nplugins}")
endif()
message(STATUS "the corrupt OFX plug-in cache was discarded, and ${nrewritten} plug-ins were scanned again")
