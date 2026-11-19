/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// Tests of the bookkeeping of the non-tiled disk cache (the cache used by the DiskCache node):
// - #1112: the restore cleanup removes the files that are not in the table of contents;
// - #1128: the size of the cache is counted correctly, and a cache hit does not drop the other entries of the hash;
// - #1265: NATRON_DISK_CACHE_PATH takes precedence over the "Disk cache path" preference;
// - #192: the disk space of the cache files is allocated before they are written;
// - the unreferenced files are not deleted while another Natron process uses the cache.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>

#ifndef _WIN32
#include <sys/file.h> // flock
#include <sys/stat.h> // stat
#endif

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QSysInfo>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/CacheSerialization.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/FrameEntry.h"
#include "Engine/FrameKey.h"
#include "Engine/FrameParams.h"
#include "Engine/Image.h"
#include "Engine/ImageKey.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/MemoryFile.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TextureRect.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

namespace {
// The bundled google-test predates GTEST_SKIP(): the tests return early and are reported as passed,
// so say clearly that they did not run, as RotoPaint_Test does.
// TODO: Convert to GTEST_SKIP() when gtest updated.
void
reportSkipped(const std::string& why)
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": " << why << std::endl;
}

typedef Cache<Image> ImageCache;
typedef std::shared_ptr<ImageCache> ImageCachePtr;

// A 16x16 RGBA 8-bit image: 1024 bytes of data
const std::size_t kImageBytes = 16 * 16 * 4;

NodePtr
createDot()
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args(PLUGINID_NATRON_DOT, app->getProject());
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);

    return app->createNode(args);
}

// The folder of a cache, with the 256 sub-folders 00..ff of a non-tiled cache (see AppManagerPrivate::cleanUpCacheDiskStructure)
void
createCacheFolder(const QString& cachePath)
{
    QDir cacheFolder(cachePath);
    cacheFolder.removeRecursively();
    QDir().mkpath(cachePath);
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            const char name[3] = { hex[i], hex[j], 0 };
            cacheFolder.mkdir( QString::fromUtf8(name) );
        }
    }
}

void
createFile(const QString& path)
{
    QFile f(path);
    if ( f.open(QIODevice::WriteOnly) ) {
        f.write("orphan");
    }
}

// A non-tiled cache whose entries are stored on disk, like the DiskCache (see AppManager::initializeCaches())
class DiskCacheTest
    : public testing::Test
{
protected:
    virtual void SetUp() OVERRIDE
    {
        _holder = createDot();
        ASSERT_TRUE( bool(_holder) );
        _cacheName = std::string("TestDiskCache") + QString::number( QCoreApplication::applicationPid() ).toStdString();
        _cache = makeCache();
        createCacheFolder( _cache->getCachePath() );
    }

    virtual void TearDown() OVERRIDE
    {
        QString cachePath;
        if (_cache) {
            cachePath = _cache->getCachePath();
            _cache->waitForDeleterThread();
            _cache.reset();
        }
        if ( !cachePath.isEmpty() ) {
            QDir(cachePath).removeRecursively();
        }
        if (_holder) {
            _holder->destroyNode(true, false);
        }
    }

    ImageCachePtr makeCache() const
    {
        return std::make_shared<ImageCache>(_cacheName, NATRON_CACHE_VERSION, 1024ULL * 1024ULL * 1024ULL, 0.);
    }

    ImageKey makeKey(U64 nodeHash) const
    {
        return ImageKey(_holder.get(), nodeHash, false, 0., ViewIdx(0), 1., false, false);
    }

    // Creates an image stored on disk in the cache, and moves it to the disk portion of the cache
    void createImageOnDisk(const ImageCachePtr& cache,
                           const ImageKey& key,
                           unsigned int mipmapLevel) const
    {
        // The rod is 16x16 pixels at the given mipmap level
        const double s = 16. * (1 << mipmapLevel);
        ImageParamsPtr params = Image::makeParams(RectD(0., 0., s, s), 1., mipmapLevel, false,
                                                  ImagePlaneDesc::getRGBAComponents(), eImageBitDepthByte,
                                                  eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, eStorageModeDisk);
        ImagePtr image;
        cache->getOrCreate(key, params, NULL, &image);
        ASSERT_TRUE( bool(image) );
        image->allocateMemory();
        ASSERT_TRUE( image->isStoredOnDisk() );
        image.reset();
        // Unmaps the file and moves the entry to the disk portion
        cache->clearInMemoryPortion(false);
    }

    std::list<ImagePtr> allEntries(const ImageCachePtr& cache) const
    {
        std::list<ImagePtr> entries;
        cache->getCopy(&entries);

        return entries;
    }

    NodePtr _holder;
    std::string _cacheName;
    ImageCachePtr _cache;
};
} // anonymous namespace

// #1112: when the cache is restored, the files that are not referenced by the table of contents are removed,
// in every sub-folder (including those whose name contains a letter), and the referenced files are kept.
TEST_F(DiskCacheTest, RestoreRemovesUnreferencedFiles)
{
    const ImageKey key = makeKey(0x123456789abcdefULL);
    createImageOnDisk(_cache, key, 0);
    std::list<ImagePtr> entries = allEntries(_cache);
    ASSERT_EQ( 1U, entries.size() );
    const QString usedFile = QString::fromUtf8( entries.front()->getFilePath().c_str() );
    entries.clear();
    ASSERT_TRUE( QFile::exists(usedFile) );

    const QString cachePath = _cache->getCachePath();
    const QStringList orphans = QStringList()
                                << cachePath + QString::fromUtf8("/00/orphan." NATRON_CACHE_FILE_EXT)
                                << cachePath + QString::fromUtf8("/0f/orphan." NATRON_CACHE_FILE_EXT)
                                << cachePath + QString::fromUtf8("/a0/orphan." NATRON_CACHE_FILE_EXT)
                                << cachePath + QString::fromUtf8("/ff/orphan." NATRON_CACHE_FILE_EXT);
    Q_FOREACH(const QString &orphan, orphans) {
        createFile(orphan);
        ASSERT_TRUE( QFile::exists(orphan) );
    }

    ImageCache::CacheTOC toc;
    _cache->save(&toc);
    ASSERT_EQ( 1U, toc.size() );
    _cache.reset(); // as when Natron quits

    _cache = makeCache();
    _cache->restore(toc);

    Q_FOREACH(const QString &orphan, orphans) {
        EXPECT_FALSE( QFile::exists(orphan) ) << orphan.toStdString();
    }
    EXPECT_TRUE( QFile::exists(usedFile) );
    EXPECT_EQ( 1U, allEntries(_cache).size() );
}

// #1128: the restored entries count in the size of the disk cache.
TEST_F(DiskCacheTest, RestoredEntriesCountInDiskSize)
{
    createImageOnDisk( _cache, makeKey(0x1111111111111111ULL), 0 );
    createImageOnDisk( _cache, makeKey(0x2222222222222222ULL), 0 );
    EXPECT_EQ(2 * kImageBytes, _cache->getDiskCacheSize());

    ImageCache::CacheTOC toc;
    _cache->save(&toc);
    ASSERT_EQ( 2U, toc.size() );
    _cache.reset();

    _cache = makeCache();
    _cache->restore(toc);
    EXPECT_EQ( 2U, allEntries(_cache).size() );
    EXPECT_EQ(2 * kImageBytes, _cache->getDiskCacheSize());
}

// #1128: removing the backing file of an entry subtracts its size once, even if it is called again when the entry
// is destroyed (entries evicted to make room are deleted by the deleter thread, which removes the file again).
TEST_F(DiskCacheTest, RemovingBackingFileSubtractsSizeOnce)
{
    createImageOnDisk( _cache, makeKey(0x1111111111111111ULL), 0 );
    createImageOnDisk( _cache, makeKey(0x2222222222222222ULL), 0 );
    ASSERT_EQ(2 * kImageBytes, _cache->getDiskCacheSize());

    std::list<ImagePtr> entries = allEntries(_cache);
    ASSERT_EQ( 2U, entries.size() );
    entries.front()->removeAnyBackingFile();
    EXPECT_EQ(kImageBytes, _cache->getDiskCacheSize());
    entries.front()->removeAnyBackingFile();
    EXPECT_EQ(kImageBytes, _cache->getDiskCacheSize());
}

// #1128: an entry moved from the disk portion to the memory portion by a cache hit counts its size once, and
// evicting it again moves it back.
TEST_F(DiskCacheTest, DiskHitAccounting)
{
    const ImageKey key = makeKey(0x3333333333333333ULL);
    createImageOnDisk(_cache, key, 0);
    ASSERT_EQ(kImageBytes, _cache->getDiskCacheSize());
    ASSERT_EQ(0U, _cache->getMemoryCacheSize());
    {
        std::list<ImagePtr> found;
        ASSERT_TRUE( _cache->get(key, &found) );
        ASSERT_EQ( 1U, found.size() );
        EXPECT_EQ(0U, _cache->getDiskCacheSize());
        EXPECT_EQ(kImageBytes, _cache->getMemoryCacheSize());
    }
    _cache->clearInMemoryPortion(false);
    EXPECT_EQ(kImageBytes, _cache->getDiskCacheSize());
    EXPECT_EQ(0U, _cache->getMemoryCacheSize());
}

// #1128: ImageKey has no mipmap level, so the images of a frame at several mipmap levels have the same hash and
// are in the same bucket. A cache hit on one of them must not drop the others from the cache.
TEST_F(DiskCacheTest, DiskHitKeepsEntriesWithTheSameHash)
{
    const ImageKey key = makeKey(0x4444444444444444ULL);
    createImageOnDisk(_cache, key, 0);
    createImageOnDisk(_cache, key, 1);
    std::list<ImagePtr> entries = allEntries(_cache);
    ASSERT_EQ( 2U, entries.size() );
    std::list<std::string> files;
    for (std::list<ImagePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
        files.push_back( (*it)->getFilePath() );
    }
    entries.clear();

    {
        std::list<ImagePtr> found;
        ASSERT_TRUE( _cache->get(key, &found) );
        ASSERT_FALSE( found.empty() );
    }
    EXPECT_EQ( 2U, allEntries(_cache).size() );

    // Both entries are still known to the cache, so clearing it removes both files
    _cache->clear();
    for (std::list<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
        EXPECT_FALSE( QFile::exists( QString::fromUtf8( it->c_str() ) ) ) << *it;
    }
}

// #1265: NATRON_DISK_CACHE_PATH takes precedence over the "Disk cache path" preference (see its documentation).
TEST(DiskCacheLocation, EnvironmentVariableOverridesPreference)
{
    const QByteArray oldEnv = qgetenv(NATRON_DISK_CACHE_PATH_ENV_VAR);
    const QString oldLocation = appPTR->getDiskCacheLocation();
    KnobPathPtr prefKnob = std::dynamic_pointer_cast<KnobPath>( appPTR->getCurrentSettings()->getKnobByName("diskCachePath") );
    ASSERT_TRUE( bool(prefKnob) );
    const std::string oldPref = prefKnob->getValue();

    const QString base = QDir::tempPath() + QString::fromUtf8("/NatronTest1265-%1").arg( QCoreApplication::applicationPid() );
    const QString envPath = base + QString::fromUtf8("/env");
    const QString prefPath = base + QString::fromUtf8("/pref");
    QDir().mkpath(envPath);
    QDir().mkpath(prefPath);

    qputenv( NATRON_DISK_CACHE_PATH_ENV_VAR, envPath.toUtf8() );
    appPTR->refreshDiskCacheLocation();
    EXPECT_EQ( envPath.toStdString(), appPTR->getDiskCacheLocation().toStdString() );

    // Setting the preference must neither change the location nor the environment variable
    prefKnob->setValue( prefPath.toStdString() );
    EXPECT_EQ( envPath.toStdString(), appPTR->getDiskCacheLocation().toStdString() );
    EXPECT_EQ( envPath.toStdString(), std::string( qgetenv(NATRON_DISK_CACHE_PATH_ENV_VAR).constData() ) );

    // Without the environment variable, the preference is used
    qunsetenv(NATRON_DISK_CACHE_PATH_ENV_VAR);
    appPTR->refreshDiskCacheLocation();
    EXPECT_EQ( prefPath.toStdString(), appPTR->getDiskCacheLocation().toStdString() );

    // Restore the previous state
    prefKnob->setValue(oldPref);
    if ( oldEnv.isEmpty() ) {
        qunsetenv(NATRON_DISK_CACHE_PATH_ENV_VAR);
    } else {
        qputenv(NATRON_DISK_CACHE_PATH_ENV_VAR, oldEnv);
    }
    appPTR->refreshDiskCacheLocation();
    EXPECT_EQ( oldLocation.toStdString(), appPTR->getDiskCacheLocation().toStdString() );
    QDir(base).removeRecursively();
}

// #192: resize() creates a sparse file on Unix, reserve() allocates its disk space so that writing to it is safe.
TEST(MemoryFileTest, ReserveAllocatesDiskSpace)
{
    const std::string path = ( QDir::tempPath() + QString::fromUtf8("/NatronTest192-%1").arg( QCoreApplication::applicationPid() ) ).toStdString();
    const std::size_t size = 1024 * 1024;
    MemoryFile f(path, MemoryFile::eFileOpenModeEnumIfExistsTruncateElseCreate);
    f.resize(size);
    EXPECT_NO_THROW( f.reserve(0, size) );
    EXPECT_NO_THROW( f.reserve(size / 2, size / 4) ); // already allocated
    ASSERT_TRUE( f.data() != NULL );
#ifndef _WIN32
    struct stat st;
    ASSERT_EQ( 0, ::stat(path.c_str(), &st) );
    EXPECT_GE( (unsigned long long)st.st_blocks * 512ULL, (unsigned long long)size );
#endif
    std::memset(f.data(), 1, size);
    EXPECT_EQ(1, f.data()[size - 1]);
    EXPECT_TRUE( f.flush(MemoryFile::eFlushTypeSync, NULL, 0) );
#ifndef _WIN32
    // The reserved blocks are those of the data (they back the start of the file), not more blocks
    ASSERT_EQ( 0, ::stat(path.c_str(), &st) );
    EXPECT_LT( (unsigned long long)st.st_blocks * 512ULL, 2ULL * size );
#endif
    f.remove();
}

// #192: when the disk is full, reserve() throws, instead of letting the writes to the mapping crash Natron (SIGBUS).
// Needs a small volume, e.g. on macOS a 32 MB disk image:
//   hdiutil create -size 32m -fs APFS -volname NatronTest /tmp/small.dmg
//   hdiutil attach -nobrowse -mountpoint /tmp/small /tmp/small.dmg
//   NATRON_TEST_SMALL_VOLUME=/tmp/small Tests --gtest_filter=MemoryFileTest.*
TEST(MemoryFileTest, ReserveFailsWhenTheDiskIsFull)
{
    const QString dir = QString::fromUtf8( qgetenv("NATRON_TEST_SMALL_VOLUME") );
    if ( dir.isEmpty() ) {
        reportSkipped("set NATRON_TEST_SMALL_VOLUME to a directory on a small volume");

        return;
    }
    const qint64 available = QStorageInfo(dir).bytesAvailable();
    if ( (available <= 0) || ( available > 512LL * 1024 * 1024 ) ) {
        reportSkipped("NATRON_TEST_SMALL_VOLUME must have less than 512 MB free");

        return;
    }
    const std::string path = ( dir + QString::fromUtf8("/NatronTest192-%1").arg( QCoreApplication::applicationPid() ) ).toStdString();
    // More than the free space
    const std::size_t size = (std::size_t)available + 64 * 1024 * 1024;
    MemoryFile f(path, MemoryFile::eFileOpenModeEnumIfExistsTruncateElseCreate);
    try {
        f.resize(size);
    } catch (const std::exception& e) {
        // A file system without sparse files (e.g. HFS+) already fails here, which is fine
        std::cerr << "resize() failed: " << e.what() << std::endl;
        f.remove();

        return;
    }
    EXPECT_THROW( f.reserve(0, size), std::runtime_error );
    // reserve() must not have filled the disk: on a full APFS volume, a file can't even be deleted
    f.remove();
    EXPECT_FALSE( QFile::exists( QString::fromUtf8( path.c_str() ) ) );
}

// #1112: the restore cleanup of a tiled cache (the viewer cache) removes the files of its folder that are not
// referenced by the table of contents, and keeps the tile files that are.
TEST_F(DiskCacheTest, TiledRestoreRemovesUnreferencedFiles)
{
    typedef Cache<FrameEntry> TileCache;
    const std::string name = _cacheName + "Tiled";
    const RectI bounds(0, 0, 16, 16);
    const std::size_t tileBytes = 16 * 16 * 4;
    std::shared_ptr<TileCache> cache = std::make_shared<TileCache>(name, NATRON_CACHE_VERSION, 1024ULL * 1024ULL * 1024ULL, 0.);
    cache->setTiled(true, tileBytes);
    const QString cachePath = cache->getCachePath();
    QDir(cachePath).removeRecursively();
    QDir().mkpath(cachePath);

    FrameKey key(_holder.get(), 1, 0x9999999999999999ULL, 1., 1., (int)eViewerColorSpaceSRGB, (int)eImageBitDepthByte,
                 (int)eDisplayChannelsRGB, ViewIdx(0), TextureRect(0, 0, 16, 16, 1, 1.), 0, "Input1",
                 ImagePlaneDesc::getRGBAComponents(), std::string(), false, false);
    QString usedFile;
    {
        FrameParamsPtr params = std::make_shared<FrameParams>(bounds, (int)eImageBitDepthByte, bounds, ImagePtr());
        FrameEntryPtr entry;
        cache->getOrCreate(key, params, NULL, &entry);
        ASSERT_TRUE( bool(entry) );
        entry->allocateMemory();
        ASSERT_TRUE( entry->isStoredOnDisk() );
        usedFile = QString::fromUtf8( entry->getFilePath().c_str() );
    }
    ASSERT_TRUE( QFile::exists(usedFile) );
    const QStringList orphans = QStringList()
                                << cachePath + QString::fromUtf8("/CachePart99")
                                << cachePath + QString::fromUtf8("/stray.txt");
    Q_FOREACH(const QString &orphan, orphans) {
        createFile(orphan);
    }

    TileCache::CacheTOC toc;
    cache->save(&toc);
    ASSERT_EQ( 1U, toc.size() );
    cache->waitForDeleterThread();
    cache.reset();

    cache = std::make_shared<TileCache>(name, NATRON_CACHE_VERSION, 1024ULL * 1024ULL * 1024ULL, 0.);
    cache->setTiled(true, tileBytes);
    cache->restore(toc);
    Q_FOREACH(const QString &orphan, orphans) {
        EXPECT_FALSE( QFile::exists(orphan) ) << orphan.toStdString();
    }
    EXPECT_TRUE( QFile::exists(usedFile) );
    {
        std::list<FrameEntryPtr> found;
        EXPECT_TRUE( cache->get(key, &found) );
    }
    cache->waitForDeleterThread();
    cache.reset();
    QDir(cachePath).removeRecursively();
}

// Each process keeps a file locked in <disk cache location>/Instances: the file of another running process means
// that the caches are shared (and they are then not restored nor wiped), and the file of a process that is gone is
// removed. On a local file system the lock is trusted whatever the host name (it changes with the network on macOS);
// on a network file system (not tested here), the files of other hosts are in use until they are 2 hours old.
TEST(DiskCacheLocation, DetectsOtherProcesses)
{
    if ( appPTR->isDiskCacheUsedByAnotherProcess() ) {
        reportSkipped("another Natron process uses the disk cache location");

        return;
    }
    const QString folder = appPTR->getDiskCacheLocation() + QString::fromUtf8("/Instances");
    ASSERT_TRUE( QDir(folder).exists() );
    // Files of other processes (simulated in this process: the lock of another open file conflicts), with the
    // current name format, from this host and from another host, and with the format of earlier builds
    const QStringList names = QStringList()
                              << QSysInfo::machineHostName() + QString::fromUtf8("@1-00000000-0000-0000-0000-000000000001.lock")
                              << QString::fromUtf8("other-host.invalid@1-00000000-0000-0000-0000-000000000002.lock")
                              << QString::fromUtf8("other-host.invalid-1.lock");
    Q_FOREACH(const QString &name, names) {
        const QString otherPath = folder + QLatin1Char('/') + name;
        {
            QFile other(otherPath);
            ASSERT_TRUE( other.open(QIODevice::ReadWrite) );
#ifndef _WIN32
            ASSERT_EQ( 0, ::flock(other.handle(), LOCK_EX | LOCK_NB) );
#endif
            EXPECT_TRUE( appPTR->isDiskCacheUsedByAnotherProcess() ) << name.toStdString();
            EXPECT_TRUE( QFile::exists(otherPath) ) << name.toStdString();
        } // closing the file releases the lock, as when the process exits
        EXPECT_FALSE( appPTR->isDiskCacheUsedByAnotherProcess() ) << name.toStdString();
        EXPECT_FALSE( QFile::exists(otherPath) ) << name.toStdString(); // the stale file was removed
    }

    // While another process uses the caches, wiping them doesn't delete the cache folders
    const QString otherPath = folder + QLatin1Char('/') + names.front();
    const QString marker = appPTR->getDiskCacheLocation() + QString::fromUtf8("/DiskCache/ab/NatronTest-other-process." NATRON_CACHE_FILE_EXT);
    QDir().mkpath( QFileInfo(marker).absolutePath() );
    createFile(marker);
    {
        QFile other(otherPath);
        ASSERT_TRUE( other.open(QIODevice::ReadWrite) );
#ifndef _WIN32
        ASSERT_EQ( 0, ::flock(other.handle(), LOCK_EX | LOCK_NB) );
#endif
        EXPECT_FALSE( appPTR->wipeAndCreateDiskCacheStructure() );
        EXPECT_TRUE( QFile::exists(marker) );
    }
    EXPECT_TRUE( appPTR->wipeAndCreateDiskCacheStructure() );
    EXPECT_FALSE( QFile::exists(marker) );
}
