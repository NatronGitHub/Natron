#ifndef NATRON_GLOBAL_QTCOMPAT_H_
#define NATRON_GLOBAL_QTCOMPAT_H_


#if QT_VERSION < 0x050000
#include <QtCore/QDir>
#include <QtCore/QString>

namespace Natron {

inline bool removeRecursively(const QString& dirName) {
    bool result = false;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeRecursively(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}
    
}
#endif

#include <QtCore/QString>

namespace Natron {

/*Removes the . and the extension from the filename and also
 *returns the extension as a string.*/
inline QString removeFileExtension(QString& filename) {
    int i = filename.size() -1;
    QString extension;
    while(i>=0 && filename.at(i) != QChar('.')) {
        extension.prepend(filename.at(i));
        --i;
    }
    filename = filename.left(i);
    return extension;
}

}

#endif // NATRON_GLOBAL_QTCOMPAT_H_
