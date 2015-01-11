#ifndef STANDARDPATHS_H
#define STANDARDPATHS_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <QString>

namespace Natron {
class StandardPaths
{
public:

    enum StandardLocationEnum
    {
        eStandardLocationDesktop = 0,
        eStandardLocationDocuments,
        eStandardLocationFonts,
        eStandardLocationApplications,
        eStandardLocationMusic,
        eStandardLocationMovies,
        eStandardLocationPictures,
        eStandardLocationTemp,
        eStandardLocationHome,
        eStandardLocationData,
        eStandardLocationCache,
        eStandardLocationGenericData,
        eStandardLocationRuntime,
        eStandardLocationConfig,
        eStandardLocationDownload,
        eStandardLocationGenericCache
    };

    static QString writableLocation(StandardLocationEnum type);

private:

    StandardPaths();
};
}

#endif // STANDARDPATHS_H
