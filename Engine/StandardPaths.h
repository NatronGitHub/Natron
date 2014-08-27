#ifndef STANDARDPATHS_H
#define STANDARDPATHS_H

#include <QString>

namespace Natron {
class StandardPaths
{
public:

    enum StandardLocation
    {
        DesktopLocation = 0,
        DocumentsLocation,
        FontsLocation,
        ApplicationsLocation,
        MusicLocation,
        MoviesLocation,
        PicturesLocation,
        TempLocation,
        HomeLocation,
        DataLocation,
        CacheLocation,
        GenericDataLocation,
        RuntimeLocation,
        ConfigLocation,
        DownloadLocation,
        GenericCacheLocation
    };

    static QString writableLocation(StandardLocation type);

private:

    StandardPaths();
};
}

#endif // STANDARDPATHS_H
