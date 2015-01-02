#ifndef STANDARDPATHS_H
#define STANDARDPATHS_H

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
