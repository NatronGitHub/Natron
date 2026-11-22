/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// Tests of the text (XML) and binary project formats (#1056): the portable binary archive, the
// detection of the format, the round trips between the formats, and the error messages.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <vector>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTemporaryDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Global/FStreamsSupport.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CLArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/PortableBinaryArchive.h"
#include "Engine/Project.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_USING

namespace {
std::string
toHex(const std::string& s)
{
    static const char digits[] = "0123456789abcdef";
    std::string ret;

    for (std::size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (i) {
            ret += ' ';
        }
        ret += digits[c >> 4];
        ret += digits[c & 0xf];
    }

    return ret;
}

QByteArray
readFile(const QString& path)
{
    QFile f(path);

    if ( !f.open(QIODevice::ReadOnly) ) {
        return QByteArray();
    }

    return f.readAll();
}

void
writeFile(const QString& path,
          const QByteArray& data)
{
    QFile f(path);

    ASSERT_TRUE( f.open(QIODevice::WriteOnly | QIODevice::Truncate) );
    ASSERT_EQ( (qint64)data.size(), f.write(data) );
}

// Saves the project to dir/name in the given format, without changing the project (as saveTemp()):
// the save date is not updated, so that two saves of the same project are identical.
bool
saveCopy(const ProjectPtr& project,
         const QString& dir,
         const QString& name,
         ProjectFileFormatEnum format)
{
    return project->saveProject_imp(dir, name, false /*autoSave*/, false /*updateProjectProperties*/, 0, 0, format);
}

// The bundled google-test predates GTEST_SKIP(): say clearly that the test did not run.
// TODO: Convert to GTEST_SKIP() when gtest updated.
void
reportSkipped(const char* reason)
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": " << reason << std::endl;
}

// The directory of the committed project files (Tests/data), or an empty string if it isn't there
// (e.g. Tests run from an installed bundle).
QString
dataDir()
{
#ifdef NATRON_TESTS_DATA_DIR
    QString d = QString::fromUtf8(NATRON_TESTS_DATA_DIR) + QLatin1Char('/');
    if ( QFile::exists( d + QString::fromUtf8("ProjectFileFormat-2.5.ntp") ) ) {
        return d;
    }
#endif

    return QString();
}

// Copies a committed project file to dir, so that the paths saved in the project (the [Project] variable) are those of dir.
bool
copyData(const QString& name,
         const QString& dir)
{
    return QFile::copy( dataDir() + name, dir + name );
}
// A project with built-in nodes only (no OpenFX plug-in is needed), with knobs of most types,
// animation, links, and strings that need escaping in XML.
void
createTestProject(const AppInstancePtr& app)
{
    const std::string a = app->getAppIDString();
    std::string script =
        "g = " + a + ".createNode('fr.inria.built-in.Group')\n"
        "g.setLabel('Group <&> \\xc3\\xa9')\n"
        "d = " + a + ".createNode('fr.inria.built-in.Dot')\n"
        "d.connectInput(0, g)\n"
        "b = " + a + ".createNode('fr.inria.built-in.BackDrop')\n"
        "b.setPosition(-1234.5, 678.25)\n"
        "p = g.createDoubleParam('myDouble', 'My double')\n"
        "p.setValueAtTime(0.1, 1)\n"
        "p.setValueAtTime(-1e300, 2)\n"
        "p.setValueAtTime(3.141592653589793, 30)\n"
        "i = g.createIntParam('myInt', 'My int')\n"
        "i.setValue(-2147483647)\n"
        "s = g.createStringParam('myString', 'My string')\n"
        "s.setValue('line 1\\nline 2 <tag> & \"quotes\" \\xe2\\x82\\xac')\n"
        "c = g.createColorParam('myColor', 'My color', True)\n"
        "c.setValue(0.25, 0)\n"
        "c.setValue(1e-30, 3)\n"
        "e = g.createBooleanParam('myBool', 'My bool')\n"
        "e.setValue(True)\n"
        "c.setExpression('thisNode.myDouble.get() * 2', False, 1)\n"
        "g.refreshUserParamsGUI()\n"
        + a + ".getProjectParam('frameRange').set(-5, 123)\n"
        // an additional project format: a Format is serialized (see SerializedTypeVersions)
        + a + ".addFormat('PR1056Format 1234x567 1.5')\n";
    std::string error, output;
    EXPECT_TRUE( ::Natron::Python::interpretPythonScript(script, &error, &output) ) << error << output;
}

// Loads the project file, and returns what loadProject() printed (headless, the error dialogs are printed).
bool
loadAndCaptureOutput(const ProjectPtr& project,
                     const QString& dir,
                     const QString& name,
                     std::string* output)
{
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    bool ok = project->loadProject(dir, name);
    *output = testing::internal::GetCapturedStdout();
    *output += testing::internal::GetCapturedStderr();

    return ok;
}
} // anonymous namespace

// The encoding doesn't depend on the platform (type sizes, endianness): these bytes are the same
// on every OS and CPU, so a file written on one loads on all the others. Integers are a signed
// byte count followed by the little-endian magnitude, floating point numbers their IEEE 754 bits.
TEST(PortableBinaryArchive, EncodingIsFixed)
{
    std::ostringstream ss;
    {
        PortableBinaryOArchive oa(ss, boost::archive::no_header);
        int zero = 0;
        int one = 1;
        int minusOne = -1;
        unsigned short u16 = 0xBEEF;
        long l = 0x12345678L;
        long long ll = -0x123456789AB;
        unsigned long long ull = 0xFFFFFFFFFFFFFFFFULL; // larger than INTMAX_MAX
        bool t = true;
        double d = 1.5;
        float f = -2.f;
        std::string s("ab");
        oa << zero << one << minusOne << u16 << l << ll << ull << t << d << f << s;
    }
    const std::string expected =
        "80 "                         // flags: little-endian (written even without the header)
        "00 "                         // 0
        "01 01 "                      // 1
        "ff 01 "                      // -1
        "02 ef be "                   // 0xBEEF
        "04 78 56 34 12 "             // 0x12345678, also if long is 64-bit
        "fa ab 89 67 45 23 01 "       // -0x123456789AB
        "ff 01 "                      // 0xFFFFFFFFFFFFFFFF is stored as -1
        "01 01 "                      // true
        "00 00 00 00 00 00 f8 3f "    // 1.5
        "00 00 00 c0 "                // -2.f
        "01 02 61 62";                // "ab": size, then the characters
    EXPECT_EQ( expected, toHex( ss.str() ) );

    // read back, also into types of other sizes
    std::istringstream is( ss.str() );
    PortableBinaryIArchive ia(is, boost::archive::no_header);
    long long zero, one, minusOne, u16, l, ll;
    unsigned long long ull;
    bool t;
    double d;
    float f;
    std::string s;
    ia >> zero >> one >> minusOne >> u16 >> l >> ll >> ull >> t >> d >> f >> s;
    EXPECT_EQ(0, zero);
    EXPECT_EQ(1, one);
    EXPECT_EQ(-1, minusOne);
    EXPECT_EQ(0xBEEF, u16);
    EXPECT_EQ(0x12345678L, l);
    EXPECT_EQ(-0x123456789AB, ll);
    EXPECT_EQ(0xFFFFFFFFFFFFFFFFULL, ull);
    EXPECT_TRUE(t);
    EXPECT_EQ(1.5, d);
    EXPECT_EQ(-2.f, f);
    EXPECT_EQ(std::string("ab"), s);
}

// A value that doesn't fit in the type it is read into is an error, not a truncated value.
TEST(PortableBinaryArchive, IntegerTooLargeIsAnError)
{
    std::ostringstream ss;
    {
        PortableBinaryOArchive oa(ss, boost::archive::no_header);
        long long big = 0x10000000000LL;
        oa << big;
    }
    std::istringstream is( ss.str() );
    PortableBinaryIArchive ia(is, boost::archive::no_header);
    int small;
    EXPECT_THROW(ia >> small, boost::archive::archive_exception);
}

// The header: the Boost signature, the version of the Boost archive library, then the flags.
// Project::detectFileFormat() relies on NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC.
TEST(PortableBinaryArchive, Header)
{
    std::ostringstream ss;
    {
        PortableBinaryOArchive oa(ss);
    }
    const std::string data = ss.str();
    ASSERT_EQ( (std::size_t)NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE + 3, data.size() );
    EXPECT_EQ( 0, std::memcmp(data.data(), NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC, NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE) );
    EXPECT_EQ( 0x01, (unsigned char)data[NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE] );     // 1 byte
    EXPECT_GE( (unsigned char)data[NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE + 1], 17 ); // Boost >= 1.68
    EXPECT_EQ( 0x80, (unsigned char)data[NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE + 2] );

    // A file written by a more recent Boost (a higher archive version) is accepted, as by xml_iarchive:
    // the same Natron version is built with different Boost versions on different platforms.
    std::string newer = data;
    newer[NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE + 1] = 100;
    std::istringstream is(newer);
    EXPECT_NO_THROW( PortableBinaryIArchive ia(is) );

    // Another encoding (e.g. big-endian, which this archive never writes) is refused.
    std::string bigEndian = data;
    bigEndian[NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE + 2] = 0x40;
    std::istringstream is2(bigEndian);
    EXPECT_THROW(PortableBinaryIArchive ia2(is2), boost::archive::archive_exception);
}

// Every type serialized in a project has the same class version in every translation unit that
// instantiates its (de)serializer, i.e. its BOOST_CLASS_VERSION is seen through the serialization header of
// its owner. This file only includes ProjectSerialization.h: without FormatSerialization.h there, Format had
// version 0 here and 3 in Project.cpp, and the MinGW linker kept this file's iserializer, which refused
// the Formats saved by Project.cpp ("Unrecognized or damaged binary project file: class version N6Natron6FormatE").
TEST(ProjectFileFormat, SerializedTypeVersions)
{
#ifdef FORMAT_SERIALIZATION_VERSION
    EXPECT_EQ( (unsigned int)FORMAT_SERIALIZATION_VERSION, (unsigned int)boost::serialization::version<Format>::value );
    EXPECT_NE( 0u, (unsigned int)boost::serialization::version<Format>::value );
#else
    ADD_FAILURE() << "ProjectSerialization.h serializes Format but doesn't include FormatSerialization.h (BOOST_CLASS_VERSION)";
#endif
}

// The format is detected from the content of the file, not from its extension.
TEST(ProjectFileFormat, Detection)
{
    const std::string xml("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n<!DOCTYPE boost_serialization>");
    EXPECT_EQ( eProjectFileFormatText, Project::detectFileFormat( xml.data(), xml.size() ) );
    const std::string xmlWithBOM = "\xEF\xBB\xBF \r\n\t" + xml;
    EXPECT_EQ( eProjectFileFormatText, Project::detectFileFormat( xmlWithBOM.data(), xmlWithBOM.size() ) );

    std::ostringstream ss;
    {
        PortableBinaryOArchive oa(ss);
        int i = 3;
        oa << i;
    }
    EXPECT_EQ( eProjectFileFormatBinary, Project::detectFileFormat( ss.str().data(), ss.str().size() ) );

    // the non-portable boost::archive::binary_oarchive is not a project format
    std::ostringstream native;
    {
        boost::archive::binary_oarchive oa(native);
        int i = 3;
        oa << i;
    }
    EXPECT_EQ( eProjectFileFormatNone, Project::detectFileFormat( native.str().data(), native.str().size() ) );

    const std::string garbage("Natron project?");
    EXPECT_EQ( eProjectFileFormatNone, Project::detectFileFormat( garbage.data(), garbage.size() ) );
    EXPECT_EQ( eProjectFileFormatNone, Project::detectFileFormat( garbage.data(), 0 ) );
    EXPECT_EQ( eProjectFileFormatNone, Project::detectFileFormat( xml.data(), 4 ) ); // "<?xm"
    EXPECT_EQ( eProjectFileFormatNone, Project::detectFileFormat( QString::fromUtf8("/nonexistent/project.ntp") ) );
}

// Text -> binary -> text: the binary file goes through the same serialize() functions (and class
// versions) as the text one, so the project saved again as text after loading the binary file is
// identical to the original text file. Each project also keeps the format of its file.
TEST(ProjectFileFormat, RoundTrip)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    EXPECT_EQ( eProjectFileFormatNone, project->getFileFormat() );

    createTestProject(app);
    // A new project is saved in the format of the preference, which becomes its format.
    ASSERT_TRUE( appPTR->getCurrentSettings()->saveAsBinary() ) << "binary is the default";
    EXPECT_EQ( eProjectFileFormatBinary, project->getFileFormatForSave() );
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("new.ntp"), 0 ) );
    EXPECT_EQ( eProjectFileFormatBinary, Project::detectFileFormat(dir + QString::fromUtf8("new.ntp")) );
    EXPECT_EQ( eProjectFileFormatBinary, project->getFileFormat() );

    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("original.ntp"), eProjectFileFormatText) );
    const QByteArray original = readFile( dir + QString::fromUtf8("original.ntp") );
    ASSERT_EQ( eProjectFileFormatText, Project::detectFileFormat(dir + QString::fromUtf8("original.ntp")) );
    ASSERT_TRUE( original.contains("myDouble") );

    // load the text file: it is saved as text
    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("original.ntp") ) );
    EXPECT_EQ( eProjectFileFormatText, project->getFileFormat() );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("binary.ntp"), eProjectFileFormatBinary) );
    const QByteArray binary = readFile( dir + QString::fromUtf8("binary.ntp") );
    ASSERT_EQ( eProjectFileFormatBinary, Project::detectFileFormat(dir + QString::fromUtf8("binary.ntp")) );
    std::cout << "text: " << original.size() << " bytes, binary: " << binary.size() << " bytes" << std::endl;
    EXPECT_LT( binary.size(), original.size() );

    // load the binary file: it is saved as binary, and it is the same project
    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("binary.ntp") ) );
    EXPECT_EQ( eProjectFileFormatBinary, project->getFileFormat() );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("roundtrip.ntp"), eProjectFileFormatText) );
    const QByteArray roundTrip = readFile( dir + QString::fromUtf8("roundtrip.ntp") );
    EXPECT_TRUE( original == roundTrip ) << "the text file saved after loading the binary file differs from the original";

    // the binary file of a project loaded from a binary file is identical too
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("binary2.ntp"), eProjectFileFormatBinary) );
    EXPECT_TRUE( binary == readFile( dir + QString::fromUtf8("binary2.ntp") ) );

    // "Save" keeps the format of the project, whatever the preference
    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("original.ntp") ) );
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("original.ntp"), 0 ) );
    EXPECT_EQ( eProjectFileFormatText, Project::detectFileFormat(dir + QString::fromUtf8("original.ntp")) );

    project->resetProject();
    EXPECT_EQ( eProjectFileFormatNone, project->getFileFormat() );
}

// Auto-saves are always binary, and record the format of the project: a text project restored
// from its auto-save is saved as text.
TEST(ProjectFileFormat, AutoSaveKeepsTheFormatOfTheProject)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);

    for (int i = 0; i < 2; ++i) {
        const ProjectFileFormatEnum format = (i == 0) ? eProjectFileFormatText : eProjectFileFormatBinary;
        const QString name = QString::fromUtf8( (i == 0) ? "text.ntp" : "binary.ntp" );
        project->setFileFormat(format);
        ASSERT_TRUE( project->saveProject(dir, name, 0) );
        ASSERT_EQ( format, Project::detectFileFormat(dir + name) );

        // the auto-save, as written by Project::autoSave()
        QString autoSavePath;
        ASSERT_TRUE( project->saveProject_imp(dir, name, true /*autoSave*/, true /*updateProjectProperties*/, &autoSavePath) );
        // not "*.ntp.autosave", which Natron 2.5 would offer to restore (and fail to load, then delete)
        ASSERT_EQ( name + QString::fromUtf8(".binautosave"), QFileInfo(autoSavePath).fileName() );
        EXPECT_EQ( eProjectFileFormatBinary, Project::detectFileFormat(autoSavePath) );

        // Restore it, as loadProject() does in GUI mode when the user chooses to use it: it is the project
        // that was auto-saved, with its name and its format, and "Save" writes that format.
        project->resetProject();
        ASSERT_TRUE( project->restoreAutoSave( dir, name + QString::fromUtf8(".binautosave") ) );
        EXPECT_EQ( format, project->getFileFormat() ) << "the format of the project was not restored from its auto-save";
        EXPECT_EQ( name, project->getProjectFilename() );
        EXPECT_TRUE( project->hasProjectBeenSavedByUser() );
        ASSERT_TRUE( project->saveProject( project->getProjectPath(), project->getProjectFilename(), 0 ) );
        EXPECT_EQ( format, Project::detectFileFormat(dir + name) );

        // the save removed the auto-save: write it again
        EXPECT_FALSE( QFile::exists(autoSavePath) );
        ASSERT_TRUE( project->saveProject_imp(dir, name, true /*autoSave*/, true /*updateProjectProperties*/, &autoSavePath) );

        // loaded directly (File > Open), it is a binary project
        project->resetProject();
        ASSERT_TRUE( project->loadProject( dir, name + QString::fromUtf8(".binautosave") ) );
        EXPECT_EQ( eProjectFileFormatBinary, project->getFileFormat() );
        project->resetProject();
        createTestProject(app);
    }

    // the auto-save of a new project, never saved: it has no format yet, the first save will choose it
    project->resetProject();
    createTestProject(app);
    ASSERT_EQ( eProjectFileFormatNone, project->getFileFormat() );
    QString autoSavePath;
    ASSERT_TRUE( project->saveProject_imp(dir, QString::fromUtf8("untitled.ntp"), true /*autoSave*/, true /*updateProjectProperties*/, &autoSavePath) );
    EXPECT_EQ( eProjectFileFormatBinary, Project::detectFileFormat(autoSavePath) );
    project->resetProject();
    ASSERT_TRUE( project->loadProject( dir, QFileInfo(autoSavePath).fileName(), true /*isUntitledAutosave*/ ) );
    EXPECT_EQ( eProjectFileFormatNone, project->getFileFormat() );
    project->resetProject();
}

// Binary auto-saves end with .ntp.binautosave: Natron 2.5 and earlier look for ".ntp.autosave" (RB-2.5
// Project::findAutoSaveForProject() and GuiAppInstance::findAndTryLoadUntitledAutoSave()), so they never
// offer to restore a binary auto-save, which they couldn't load. (2.5's Project::clearAutoSavesDir() still
// removes the untitled ones of the Autosaves directory.) 2.6 finds both.
TEST(ProjectFileFormat, AutoSaveNames)
{
    const QString natron25Pattern = QString::fromUtf8(".ntp.autosave");
    const char* binaryNames[] = { "foo.ntp.binautosave", "Untitled.ntp.binautosave.123456789" };
    for (const char* n : binaryNames) {
        const QString name = QString::fromUtf8(n);
        EXPECT_EQ( -1, name.indexOf(natron25Pattern) ) << n;
        int suffixSize = 0;
        EXPECT_EQ( name.indexOf( QString::fromUtf8(".ntp") ), Project::findAutoSaveSuffix(name, &suffixSize) ) << n;
        EXPECT_EQ( 12, suffixSize ) << n;
    }
    const char* textNames[] = { "foo.ntp.autosave", "Untitled.ntp.autosave.123456789" };
    for (const char* n : textNames) {
        const QString name = QString::fromUtf8(n);
        int suffixSize = 0;
        EXPECT_EQ( name.indexOf( QString::fromUtf8(".ntp") ), Project::findAutoSaveSuffix(name, &suffixSize) ) << n;
        EXPECT_EQ( 9, suffixSize ) << n;
    }
    EXPECT_EQ( -1, Project::findAutoSaveSuffix( QString::fromUtf8("foo.ntp") ) );
    EXPECT_EQ( -1, Project::findAutoSaveSuffix( QString::fromUtf8("foo.ntp.~1~") ) );
    // the temporary files of a save (QSaveFile: <name>.XXXXXX), e.g. left by a crash, are not auto-saves (#1590)
    const char* notAutoSaves[] = { "foo.ntp.binautosave.Ab3xYz", "Untitled.ntp.binautosave.123456789.Ab3xYz", "foo.ntp.autosave.Ab3xYz", "foo.ntp.binautosave." };
    for (const char* n : notAutoSaves) {
        EXPECT_EQ( -1, Project::findAutoSaveSuffix( QString::fromUtf8(n) ) ) << n;
    }

    // findAutoSaveForProject() finds the binary auto-save of a project, and also a text one by 2.5
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    writeFile( dir + QString::fromUtf8("a.ntp"), QByteArray("<?xml") );
    writeFile( dir + QString::fromUtf8("a.ntp.binautosave"), QByteArray("x") );
    writeFile( dir + QString::fromUtf8("b.ntp"), QByteArray("<?xml") );
    writeFile( dir + QString::fromUtf8("b.ntp.autosave"), QByteArray("x") );
    QString found;
    EXPECT_TRUE( project->findAutoSaveForProject( dir, QString::fromUtf8("a.ntp"), &found ) );
    EXPECT_EQ( QString::fromUtf8("a.ntp.binautosave"), found );
    EXPECT_TRUE( project->findAutoSaveForProject( dir, QString::fromUtf8("b.ntp"), &found ) );
    EXPECT_EQ( QString::fromUtf8("b.ntp.autosave"), found );

    // Both: the most recent one is offered, the other one is returned too (No removes both).
    const QDateTime now = QDateTime::currentDateTime();
    auto setTime = [](const QString& path, const QDateTime& t) {
        QFile f(path);
        return f.open(QIODevice::ReadWrite) && f.setFileTime(t, QFileDevice::FileModificationTime);
    };
    writeFile( dir + QString::fromUtf8("c.ntp"), QByteArray("<?xml") );
    writeFile( dir + QString::fromUtf8("c.ntp.autosave"), QByteArray("x") );
    writeFile( dir + QString::fromUtf8("c.ntp.binautosave"), QByteArray("x") );
    ASSERT_TRUE( setTime(dir + QString::fromUtf8("c.ntp"), now.addSecs(-100)) );
    ASSERT_TRUE( setTime(dir + QString::fromUtf8("c.ntp.binautosave"), now.addSecs(-50)) );
    ASSERT_TRUE( setTime(dir + QString::fromUtf8("c.ntp.autosave"), now.addSecs(-10)) ); // by 2.5, after 2.6 crashed
    QStringList others;
    EXPECT_TRUE( project->findAutoSaveForProject( dir, QString::fromUtf8("c.ntp"), &found, false, &others ) );
    EXPECT_EQ( QString::fromUtf8("c.ntp.autosave"), found );
    EXPECT_EQ( QStringList() << QString::fromUtf8("c.ntp.binautosave"), others );

    // An auto-save older than the project (left by a crash of 2.6, then the project was saved by 2.5, which
    // ignores the binary auto-saves) is not offered, and not removed by findAutoSaveForProject() (the next save or
    // auto-save of the project removes it, see removeLastAutosave()). It still counts for --no-dialogs.
    ASSERT_TRUE( setTime(dir + QString::fromUtf8("c.ntp.autosave"), now.addSecs(-200)) );
    ASSERT_TRUE( setTime(dir + QString::fromUtf8("c.ntp.binautosave"), now.addSecs(-150)) );
    EXPECT_FALSE( project->findAutoSaveForProject( dir, QString::fromUtf8("c.ntp"), &found ) );
    EXPECT_TRUE( QFile::exists( dir + QString::fromUtf8("c.ntp.binautosave") ) );
    EXPECT_TRUE( project->findAutoSaveForProject( dir, QString::fromUtf8("c.ntp"), &found, true /*includeOlderThanProject*/ ) );

    // the actual temporary file of an auto-save, while it is written (none on Linux, which has unnamed temporary files)
    for (const char* n : { "d.ntp.binautosave", "Untitled.ntp.binautosave.123456789" }) {
        const QString autoSave = QString::fromUtf8(n);
        QDir d(dir + QString::fromUtf8("d"));
        ASSERT_TRUE( d.mkpath( d.path() ) );
        QSaveFile f( d.filePath(autoSave) );
        ASSERT_TRUE( f.open(QIODevice::WriteOnly) );
        ASSERT_EQ( (qint64)1, f.write("x", 1) );
        ASSERT_TRUE( f.flush() );
        Q_FOREACH(const QString &entry, d.entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)) {
            EXPECT_EQ( -1, Project::findAutoSaveSuffix(entry) ) << entry.toStdString();
        }
        f.cancelWriting();
    }
}

// The text auto-save of a project, written by Natron 2.5, is restored as a text project.
TEST(ProjectFileFormat, RestoreNatron25AutoSave)
{
    if ( dataDir().isEmpty() ) {
        reportSkipped("Tests/data not found");

        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    ASSERT_TRUE( QFile::copy( dataDir() + QString::fromUtf8("ProjectFileFormat-2.5.ntp"), dir + QString::fromUtf8("p.ntp") ) );
    ASSERT_TRUE( QFile::copy( dataDir() + QString::fromUtf8("ProjectFileFormat-2.5.ntp"), dir + QString::fromUtf8("p.ntp.autosave") ) );
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    ASSERT_TRUE( project->restoreAutoSave( dir, QString::fromUtf8("p.ntp.autosave") ) );
    EXPECT_EQ( eProjectFileFormatText, project->getFileFormat() );
    EXPECT_EQ( QString::fromUtf8("p.ntp"), project->getProjectFilename() );
    EXPECT_TRUE( bool( project->getNodeByName("Group1") ) );
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("p.ntp"), 0 ) );
    EXPECT_EQ( eProjectFileFormatText, Project::detectFileFormat( dir + QString::fromUtf8("p.ntp") ) );
    EXPECT_FALSE( QFile::exists( dir + QString::fromUtf8("p.ntp.autosave") ) ) << "the save removes the restored auto-save";
    project->resetProject();
}

// Damaged files: the error says which format the file has, and what is wrong with it.
TEST(ProjectFileFormat, DamagedFiles)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("text.ntp"), eProjectFileFormatText) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("binary.ntp"), eProjectFileFormatBinary) );
    const QByteArray text = readFile( dir + QString::fromUtf8("text.ntp") );
    const QByteArray binary = readFile( dir + QString::fromUtf8("binary.ntp") );
    std::string output;

    writeFile( dir + QString::fromUtf8("damaged-text.ntp"), text.left(text.size() / 2) );
    EXPECT_FALSE( loadAndCaptureOutput(project, dir, QString::fromUtf8("damaged-text.ntp"), &output) );
    EXPECT_NE( std::string::npos, output.find("Unrecognized or damaged text (XML) project file") ) << output;

    writeFile( dir + QString::fromUtf8("damaged-binary.ntp"), binary.left(binary.size() / 2) );
    EXPECT_FALSE( loadAndCaptureOutput(project, dir, QString::fromUtf8("damaged-binary.ntp"), &output) );
    EXPECT_NE( std::string::npos, output.find("Unrecognized or damaged binary project file") ) << output;

    writeFile( dir + QString::fromUtf8("garbage.ntp"), QByteArray("this is not a project") );
    EXPECT_FALSE( loadAndCaptureOutput(project, dir, QString::fromUtf8("garbage.ntp"), &output) );
    EXPECT_NE( std::string::npos, output.find("it is neither a text (XML) nor a binary project") ) << output;

    // What Natron 2.5 and earlier do when they open a binary project: their xml_iarchive fails on
    // the first byte. They report "Unrecognized or damaged project file: " followed by this message.
    {
        FStreamsSupport::ifstream ifile;
        FStreamsSupport::open( &ifile, ( dir + QString::fromUtf8("binary.ntp") ).toStdString() );
        ASSERT_TRUE( bool(ifile) );
        try {
            boost::archive::xml_iarchive ia(ifile);
            ADD_FAILURE() << "xml_iarchive read a binary project";
        } catch (const std::exception& e) {
            // Checked with the official Natron 2.5.0 (macOS): "Error while loading project: Unrecognized or
            // damaged project file: unrecognized XML syntax"
            EXPECT_EQ( std::string("unrecognized XML syntax"), std::string( e.what() ) );
        }
    }

    // A binary project by a later version, with a more recent layout: said as is, not as a damaged file.
    {
        std::ofstream ofile( ( dir + QString::fromUtf8("future.ntp") ).toStdString().c_str(), std::ios_base::out | std::ios_base::binary );
        PortableBinaryOArchive oa(ofile);
        unsigned int version = 1000;
        int format = eProjectFileFormatBinary;
        oa << version << format;
    }
    EXPECT_FALSE( loadAndCaptureOutput(project, dir, QString::fromUtf8("future.ntp"), &output) );
    EXPECT_NE( std::string::npos, output.find("This binary project was saved with a more recent version") ) << output;
    EXPECT_EQ( std::string::npos, output.find("damaged") ) << output;
    project->resetProject();
}

namespace {
// Replaces every occurrence of a pattern (there may be several: a user knob is also saved in its page).
QByteArray
replaceAll(const QByteArray& data,
           const QByteArray& before,
           const QByteArray& after)
{
    EXPECT_TRUE( data.contains(before) ) << "not found: " << toHex( before.toStdString() );
    QByteArray ret = data;

    return ret.replace(before, after);
}

// The encoding of an int in the binary format: a signed byte count, then the little-endian magnitude.
QByteArray
binaryInt(const char* bytes,
          int size)
{
    return QByteArray(bytes, size);
}

// The encoding of a short string in the binary format: its size, then its characters.
QByteArray
binaryString(const char* s)
{
    const int n = (int)std::strlen(s);
    QByteArray ret;
    ret += (char)1;
    ret += (char)n;
    ret += QByteArray(s, n);

    return ret;
}
} // anonymous namespace

// Knob data from the file is checked, in Release builds too (#1612): a damaged or hand-edited
// project gives the "damaged project" error, or a warning, instead of crashing or allocating
// gigabytes. The Name, Type and Dimension of a knob are consecutive in both formats.
TEST(ProjectFileFormat, InvalidKnobData)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    {
        // A choice with a negative index: Python can set it, and a file can contain it.
        const std::string script =
            "ch = " + app->getAppIDString() + ".Group1.createChoiceParam('myChoice', 'My choice')\n"
            "ch.addOption('a', '')\n"
            "ch.addOption('b', '')\n"
            "ch.setValue(-1)\n";
        std::string error, output;
        ASSERT_TRUE( ::Natron::Python::interpretPythonScript(script, &error, &output) ) << error << output;
    }
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("text.ntp"), eProjectFileFormatText) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("binary.ntp"), eProjectFileFormatBinary) );
    const QByteArray text = readFile( dir + QString::fromUtf8("text.ntp") );
    const QByteArray binary = readFile( dir + QString::fromUtf8("binary.ntp") );
    std::string output;

    // The choice index -1 is replaced by 0, with a warning.
    for (const char* name : { "text.ntp", "binary.ntp" }) {
        SCOPED_TRACE(name);
        EXPECT_TRUE( loadAndCaptureOutput(project, dir, QString::fromUtf8(name), &output) ) << output;
        EXPECT_NE( std::string::npos, output.find("myChoice: Warning: invalid choice index -1 in the project, using 0") ) << output;
        NodePtr group = project->getNodeByName("Group1");
        ASSERT_TRUE( bool(group) );
        KnobChoicePtr choice = std::dynamic_pointer_cast<KnobChoice>( group->getKnobByName("myChoice") );
        ASSERT_TRUE( bool(choice) );
        EXPECT_EQ( 0, choice->getValue() );
        EXPECT_EQ( std::string("a"), choice->getActiveEntry().id );
    }

    // the knob "myInt" of the text file, with another type and dimension
    auto damageText = [&](const char* type,
                          const char* dimension) {
        const QRegularExpression knob( QString::fromUtf8("(<Name>myInt</Name>\\s*<Type>)Int(</Type>\\s*<Dimension>)1(</Dimension>)") );
        QString s = QString::fromUtf8(text);
        EXPECT_TRUE( s.contains(knob) );
        s.replace( knob, QString::fromUtf8("\\1%1\\2%2\\3").arg( QString::fromUtf8(type), QString::fromUtf8(dimension) ) );

        return s.toUtf8();
    };
    const QByteArray binaryKnob = binaryString("myInt") + binaryString("Int") + binaryInt("\x01\x01", 2);
    struct Damage
    {
        const char* file;
        const char* type;
        const char* dimension;
        QByteArray binaryTypeAndDimension;
        const char* error;
    };
    const Damage damages[] = {
        { "dimension-negative", "Int", "-1", binaryString("Int") + binaryInt("\xff\x01", 2),
          "parameter \"myInt\": invalid number of dimensions -1" },
        { "dimension-zero", "Int", "0", binaryString("Int") + binaryInt("\x00", 1),
          "parameter \"myInt\": invalid number of dimensions 0" },
        { "dimension-huge", "Int", "100000000", binaryString("Int") + binaryInt("\x04\x00\xe1\xf5\x05", 5),
          "parameter \"myInt\": invalid number of dimensions 100000000" },
        { "unknown-type", "NoSuchType", "1", binaryString("NoSuchType") + binaryInt("\x01\x01", 2),
          "parameter \"myInt\": unknown parameter type \"NoSuchType\"" },
    };
    for (const Damage& d : damages) {
        SCOPED_TRACE(d.file);
        const QString textName = QString::fromUtf8(d.file) + QString::fromUtf8("-text.ntp");
        writeFile( dir + textName, damageText(d.type, d.dimension) );
        EXPECT_FALSE( loadAndCaptureOutput(project, dir, textName, &output) );
        EXPECT_NE( std::string::npos, output.find( std::string("Unrecognized or damaged text (XML) project file: ") + d.error ) ) << output;

        const QString binaryName = QString::fromUtf8(d.file) + QString::fromUtf8("-binary.ntp");
        writeFile( dir + binaryName, replaceAll( binary, binaryKnob, binaryString("myInt") + d.binaryTypeAndDimension ) );
        EXPECT_FALSE( loadAndCaptureOutput(project, dir, binaryName, &output) );
        EXPECT_NE( std::string::npos, output.find( std::string("Unrecognized or damaged binary project file: ") + d.error ) ) << output;
    }

    // the undamaged files still load
    EXPECT_TRUE( project->loadProject( dir, QString::fromUtf8("text.ntp") ) );
    EXPECT_TRUE( project->loadProject( dir, QString::fromUtf8("binary.ntp") ) );
    project->resetProject();
}

// A damaged size (of a string or a collection) is an error, not a huge allocation.
TEST(PortableBinaryArchive, DamagedSizes)
{
    for (int i = 0; i < 2; ++i) {
        std::ostringstream ss;
        {
            PortableBinaryOArchive oa(ss, boost::archive::no_header);
            std::size_t huge = std::size_t(1) << 40; // 1 TB
            oa << huge;
            std::string rest("abc");
            oa << rest;
        }
        std::istringstream is( ss.str() );
        PortableBinaryIArchive ia(is, boost::archive::no_header);
        try {
            if (i == 0) {
                std::string s;
                ia >> s;
            } else {
                std::vector<double> v;
                ia >> v;
            }
            ADD_FAILURE() << "a 1 TB size was accepted";
        } catch (const PortableBinaryArchiveException& e) {
            EXPECT_EQ( std::string("size larger than the rest of the file"), std::string( e.what() ) );
        } catch (const std::exception& e) {
            ADD_FAILURE() << "unexpected error: " << e.what();
        }
    }
}

// A project saved by Natron 2.5.0 (Tests/data/ProjectFileFormat-2.5.ntp, made by ProjectFileFormat-2.5.py):
// it loads, and text -> binary -> text gives the same text as text -> text. The 2.5 class versions
// go through the binary archive.
TEST(ProjectFileFormat, Natron25Project)
{
    if ( dataDir().isEmpty() ) {
        reportSkipped("Tests/data not found");

        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    ASSERT_TRUE( copyData(QString::fromUtf8("ProjectFileFormat-2.5.ntp"), dir) );
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();

    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("ProjectFileFormat-2.5.ntp") ) );
    EXPECT_EQ( eProjectFileFormatText, project->getFileFormat() );
    EXPECT_TRUE( bool( project->getNodeByName("Group1") ) );
    EXPECT_TRUE( bool( project->getNodeByName("Dot1") ) );
    EXPECT_TRUE( bool( project->getNodeByName("BackDrop1") ) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("text.ntp"), eProjectFileFormatText) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("binary.ntp"), eProjectFileFormatBinary) );
    const QByteArray text = readFile( dir + QString::fromUtf8("text.ntp") );
    ASSERT_TRUE( text.contains("myString") );

    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("binary.ntp") ) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("roundtrip.ntp"), eProjectFileFormatText) );
    EXPECT_TRUE( text == readFile( dir + QString::fromUtf8("roundtrip.ntp") ) ) << "text -> binary -> text changed the 2.5 project";

    // NATRON_TEST_WRITE_DATA=1 regenerates Tests/data/ProjectFileFormat-2.6.binary.ntp from the 2.5 project
    if ( std::getenv("NATRON_TEST_WRITE_DATA") ) {
        const QString path = dataDir() + QString::fromUtf8("ProjectFileFormat-2.6.binary.ntp");
        QFile::remove(path);
        EXPECT_TRUE( QFile::copy(dir + QString::fromUtf8("binary.ntp"), path) );
        std::cout << "wrote " << path.toStdString() << std::endl;
    }
    project->resetProject();
}

// A committed binary project (Tests/data/ProjectFileFormat-2.6.binary.ntp, written on macOS arm64 from the
// 2.5 project) loads on every platform of the CI, and in later versions, and is the same project as its text twin.
TEST(ProjectFileFormat, CommittedBinaryProject)
{
    if ( dataDir().isEmpty() ) {
        reportSkipped("Tests/data not found");

        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    ASSERT_TRUE( copyData(QString::fromUtf8("ProjectFileFormat-2.5.ntp"), dir) );
    ASSERT_TRUE( copyData(QString::fromUtf8("ProjectFileFormat-2.6.binary.ntp"), dir) );
    ASSERT_EQ( eProjectFileFormatBinary, Project::detectFileFormat( dir + QString::fromUtf8("ProjectFileFormat-2.6.binary.ntp") ) );
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();

    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("ProjectFileFormat-2.5.ntp") ) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("from-text.ntp"), eProjectFileFormatText) );
    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("ProjectFileFormat-2.6.binary.ntp") ) );
    EXPECT_EQ( eProjectFileFormatBinary, project->getFileFormat() );
    EXPECT_TRUE( bool( project->getNodeByName("Group1") ) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("from-binary.ntp"), eProjectFileFormatText) );
    EXPECT_TRUE( readFile( dir + QString::fromUtf8("from-text.ntp") ) == readFile( dir + QString::fromUtf8("from-binary.ntp") ) )
        << "the committed binary project differs from its text twin";
    project->resetProject();
}

// The Python API: app.saveProject(), saveProjectAs() and saveTempProject() take an optional format,
// and app.getProjectFormat()/setProjectFormat() give the format of the project.
TEST(ProjectFileFormat, PythonAPI)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const std::string dir = ( tmpDir.path() + QLatin1Char('/') ).toStdString();
    AppInstancePtr app = appPTR->getTopLevelInstance();
    app->getProject()->resetProject();
    const std::string a = app->getAppIDString();
    std::string script =
        "import NatronEngine\n"
        "F = NatronEngine.Natron.ProjectFileFormatEnum\n"
        "def fmt(p):\n"
        "    d = open(p, 'rb').read(64)\n"
        "    return 'binary' if d.startswith(b'\\x01\\x16serialization::archive') else ('text' if d.startswith(b'<?xml') else 'unknown')\n"
        "a = " + a + "\n"
        "a.createNode('fr.inria.built-in.Dot')\n"
        "assert a.getProjectFormat() == F.eProjectFileFormatNone\n"
        // the preference (binary) for a new project
        "assert a.saveProjectAs('" + dir + "default.ntp')\n"
        "assert fmt('" + dir + "default.ntp') == 'binary', fmt('" + dir + "default.ntp')\n"
        "assert a.getProjectFormat() == F.eProjectFileFormatBinary\n"
        // an explicit format becomes the format of the project
        "assert a.saveProjectAs('" + dir + "text.ntp', F.eProjectFileFormatText)\n"
        "assert fmt('" + dir + "text.ntp') == 'text'\n"
        "assert a.getProjectFormat() == F.eProjectFileFormatText\n"
        "assert a.saveProject('')\n"
        "assert fmt('" + dir + "text.ntp') == 'text'\n"
        // a temporary copy in another format doesn't change the format of the project
        "assert a.saveTempProject('" + dir + "temp.ntp', F.eProjectFileFormatBinary)\n"
        "assert fmt('" + dir + "temp.ntp') == 'binary'\n"
        "assert a.getProjectFormat() == F.eProjectFileFormatText\n"
        "a.setProjectFormat(F.eProjectFileFormatBinary)\n"
        "assert a.saveProject('')\n"
        "assert fmt('" + dir + "text.ntp') == 'binary'\n";
    std::string error, output;
    EXPECT_TRUE( ::Natron::Python::interpretPythonScript(script, &error, &output) ) << error << output;
    app->getProject()->resetProject();
}

namespace {
// Waits until n threads reach it (std::barrier is C++20).
class Barrier
{
    std::mutex _mutex;
    std::condition_variable _cond;
    const int _n;
    int _count;
    int _generation;

public:
    explicit Barrier(int n)
        : _n(n)
        , _count(0)
        , _generation(0)
    {
    }

    void wait()
    {
        std::unique_lock<std::mutex> l(_mutex);
        const int generation = _generation;
        if (++_count == _n) {
            _count = 0;
            ++_generation;
            _cond.notify_all();
        } else {
            _cond.wait(l, [&]() { return generation != _generation; });
        }
    }
};

// The files of dir that are neither one of the names nor one of their backups (<name>.~N~):
// the temporary files left by the saves.
QStringList
strayFiles(const QString& dir,
           const QStringList& names)
{
    QStringList ret;
    const QRegularExpression backup( QString::fromUtf8("\\.~\\d+~\\z") );

    Q_FOREACH(const QString &entry, QDir(dir).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)) {
        QString name = entry;
        name.remove(backup);
        if ( !names.contains(name) ) {
            ret.append(entry);
        }
    }

    return ret;
}

// Sets the frame range of the project of the top-level instance, so that its file changes.
void
setFrameRange(int first,
              int last)
{
    std::string error, output;
    EXPECT_TRUE( ::Natron::Python::interpretPythonScript(appPTR->getTopLevelInstance()->getAppIDString()
                                                          + ".getProjectParam('frameRange').set(" + std::to_string(first) + ", " + std::to_string(last) + ")\n",
                                                          &error, &output) ) << error << output;
}

// Sets the "Save versions" preference (the number of backups) while it exists.
class SaveVersions
{
    KnobIntPtr _knob;
    int _old;

public:
    explicit SaveVersions(int n)
        : _knob( std::dynamic_pointer_cast<KnobInt>( appPTR->getCurrentSettings()->getKnobByName("saveVersions") ) )
        , _old(0)
    {
        EXPECT_TRUE( bool(_knob) );
        if (_knob) {
            _old = _knob->getValue();
            _knob->setValue(n);
        }
        EXPECT_EQ( n, appPTR->getCurrentSettings()->saveVersions() );
    }

    ~SaveVersions()
    {
        if (_knob) {
            _knob->setValue(_old);
        }
    }
};

// Sets the save commit hook (see Project::setSaveCommitHookForTesting()) while it exists.
class SaveCommitHook
{
public:
    explicit SaveCommitHook(const std::function<bool()>& hook)
    {
        Project::setSaveCommitHookForTesting(hook);
    }

    ~SaveCommitHook()
    {
        Project::setSaveCommitHookForTesting( std::function<bool()>() );
    }
};

// True if the current user can write to path, although it is read-only (root).
bool
canWriteReadOnly(const QString& path)
{
    if ( QFileInfo(path).isDir() ) {
        QFile probe( path + QString::fromUtf8("/probe") );
        if ( probe.open(QIODevice::WriteOnly) ) {
            probe.close();
            probe.remove();

            return true;
        }

        return false;
    }
    QFile probe(path);

    return probe.open(QIODevice::Append);
}
} // anonymous namespace

// Two projects saved at the same time (two windows, or two processes, e.g. on a render farm) used the same
// temporary file, $TMPDIR/<milliseconds since epoch>, then renamed it to the project file: a save failed, or
// wrote a corrupt file (#1590). The saves start at the same time, one in text while the other is in binary.
TEST(ProjectSave, ConcurrentSaves)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    AppInstancePtr app = appPTR->getTopLevelInstance();
    app->getProject()->resetProject();
    createTestProject(app);
    // a second project: an internal instance, as the one of a Precomp node
    AppInstancePtr app2 = appPTR->newBackgroundInstance(CLArgs(), true);
    ASSERT_TRUE( bool(app2) );

    struct Target
    {
        ProjectPtr project;
        QString name;
        QByteArray reference[2];
        int failures;
        int mismatches;
    };
    const ProjectFileFormatEnum formats[2] = { eProjectFileFormatText, eProjectFileFormatBinary };
    Target targets[2];
    targets[0].project = app->getProject();
    targets[0].name = QString::fromUtf8("a.ntp");
    targets[1].project = app2->getProject();
    targets[1].name = QString::fromUtf8("b.ntp");
    for (Target& t : targets) {
        t.failures = t.mismatches = 0;
        // the contents of the files, saved one at a time
        for (int f = 0; f < 2; ++f) {
            const QString name = QString::fromUtf8("reference-%1-").arg(f) + t.name;
            ASSERT_TRUE( saveCopy(t.project, dir, name, formats[f]) );
            t.reference[f] = readFile(dir + name);
            ASSERT_FALSE( t.reference[f].isEmpty() );
        }
    }
    ASSERT_TRUE( targets[0].reference[0] != targets[1].reference[0] );

    const int n = 100;
    Barrier barrier(2);
    auto run = [&](int index) {
        Target& t = targets[index];
        for (int i = 0; i < n; ++i) {
            const int f = (i + index) % 2;
            barrier.wait();
            if ( !saveCopy(t.project, dir, t.name, formats[f]) ) {
                ++t.failures;
            } else if ( readFile(dir + t.name) != t.reference[f] ) {
                ++t.mismatches;
            }
        }
    };
    std::thread thread0(run, 0);
    std::thread thread1(run, 1);
    thread0.join();
    thread1.join();

    for (const Target& t : targets) {
        EXPECT_EQ( 0, t.failures ) << "saves of " << t.name.toStdString() << " failed, out of " << n;
        EXPECT_EQ( 0, t.mismatches ) << "saves of " << t.name.toStdString() << " wrote a wrong file, out of " << n;
    }
    QStringList names;
    for (const Target& t : targets) {
        names << t.name << QString::fromUtf8("reference-0-") + t.name << QString::fromUtf8("reference-1-") + t.name;
    }
    EXPECT_EQ( QStringList(), strayFiles(dir, names) );

    app2->quitNow();
    app->getProject()->resetProject();
}

// A save that fails leaves the previous project file as it was, and no temporary file, with an error
// that names the file.
TEST(ProjectSave, FailedSaveKeepsThePreviousFile)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("p.ntp");
    const QString path = dir + name;
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    SaveVersions saveVersions(1);
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    const QByteArray previous = readFile(path);
    ASSERT_FALSE( previous.isEmpty() );
    // change the project, so that a save changes the file
    setFrameRange(7, 89);

    auto failedSave = [&](const char* what) {
        QString errorMessage;
        EXPECT_FALSE( project->saveProject_imp(dir, name, false /*autoSave*/, false /*updateProjectProperties*/, 0, &errorMessage, eProjectFileFormatBinary) ) << what;
        EXPECT_TRUE( errorMessage.contains(path) ) << what << ": " << errorMessage.toStdString();
        EXPECT_TRUE( errorMessage.contains( QString::fromUtf8("Save As") ) ) << what << ": " << errorMessage.toStdString();
        EXPECT_TRUE( readFile(path) == previous ) << what << ": the project file changed";
        EXPECT_EQ( QStringList(), strayFiles( dir, QStringList(name) ) ) << what;
        EXPECT_FALSE( QFile::exists( path + QString::fromUtf8(".~1~") ) ) << what;
    };

    // a read-only file is not replaced
    const QFileDevice::Permissions permissions = QFile::permissions(path);
    ASSERT_TRUE( QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::ReadUser | QFileDevice::ReadGroup | QFileDevice::ReadOther) );
    if ( canWriteReadOnly(path) ) {
        reportSkipped("the read-only file is writable (root?)");
    } else {
        failedSave("read-only file");
    }
    ASSERT_TRUE( QFile::setPermissions(path, permissions) );

#ifndef Q_OS_WIN
    // a read-only directory: the temporary file can't be created (Windows has no read-only directories)
    const QFileDevice::Permissions dirPermissions = QFile::permissions(tmpDir.path());
    ASSERT_TRUE( QFile::setPermissions(tmpDir.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner | QFileDevice::ReadUser | QFileDevice::ExeUser) );
    if ( canWriteReadOnly( tmpDir.path() ) ) {
        reportSkipped("the read-only directory is writable (root?)");
    } else {
        failedSave("read-only directory");
    }
    ASSERT_TRUE( QFile::setPermissions(tmpDir.path(), dirPermissions) );
#endif

    // and then it saves
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    EXPECT_FALSE( readFile(path) == previous );
    EXPECT_TRUE( readFile( path + QString::fromUtf8(".~1~") ) == previous );
    project->resetProject();
}

// The new file is written, the backups are rotated, then the new file can't replace the project file (on Windows,
// another process has it open; or closing the file fails): the save tries again, then puts the project file and
// its backups back.
TEST(ProjectSave, FailedCommitKeepsThePreviousFile)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("p.ntp");
    const QString path = dir + name;
    const QString backup1 = path + QString::fromUtf8(".~1~");
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    SaveVersions saveVersions(1);
    setFrameRange(1, 10);
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    setFrameRange(1, 20);
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    const QByteArray b = readFile(path);
    ASSERT_FALSE( b == readFile(backup1) );

    // Fails the next count commits.
    std::shared_ptr<int> failures = std::make_shared<int>(0);
    SaveCommitHook hook([failures]() {
        return (*failures)-- <= 0;
    });
    auto failCommits = [failures](int count) {
        *failures = count;
    };

    // one failure: the save tries again
    setFrameRange(1, 30);
    failCommits(1);
    EXPECT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    const QByteArray c = readFile(path);
    EXPECT_FALSE( c == b );
    EXPECT_TRUE( readFile(backup1) == b );
    EXPECT_EQ( QStringList(), strayFiles( dir, QStringList(name) ) );
    EXPECT_FALSE( QFile::exists( path + QString::fromUtf8(".~2~") ) );

    // every attempt fails
    setFrameRange(1, 40);
    failCommits(3);
    QString errorMessage;
    EXPECT_FALSE( project->saveProject_imp(dir, name, false /*autoSave*/, false /*updateProjectProperties*/, 0, &errorMessage, eProjectFileFormatBinary) );
    failCommits(0);
    EXPECT_TRUE( errorMessage.contains(path) ) << errorMessage.toStdString();
    EXPECT_TRUE( readFile(path) == c ) << "the project file was not put back";
    EXPECT_TRUE( readFile(backup1) == b ) << "the backup was not put back";
    EXPECT_FALSE( QFile::exists( path + QString::fromUtf8(".~2~") ) );
    EXPECT_EQ( QStringList(), strayFiles( dir, QStringList(name) ) );
    project->resetProject();
}

#ifndef Q_OS_WIN
// The new file can't replace the project file, and the project file can't be put back either (e.g. a network
// share went away): the error says where the previous version is.
TEST(ProjectSave, FailedRollbackSaysWhereThePreviousVersionIs)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("p.ntp");
    const QString path = dir + name;
    const QString backup1 = path + QString::fromUtf8(".~1~");
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    SaveVersions saveVersions(1);
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    const QByteArray previous = readFile(path);
    setFrameRange(5, 55);

    const QFileDevice::Permissions readOnly = QFileDevice::ReadOwner | QFileDevice::ExeOwner | QFileDevice::ReadUser | QFileDevice::ExeUser;
    {
        // root can write to a read-only folder: check it in another folder, before the save
        QTemporaryDir scratch;
        ASSERT_TRUE( scratch.isValid() );
        const QFileDevice::Permissions scratchPermissions = QFile::permissions( scratch.path() );
        ASSERT_TRUE( QFile::setPermissions(scratch.path(), readOnly) );
        const bool rootCanWrite = canWriteReadOnly( scratch.path() );
        ASSERT_TRUE( QFile::setPermissions(scratch.path(), scratchPermissions) );
        if (rootCanWrite) {
            reportSkipped("a read-only directory is writable (root?)");
            project->resetProject();

            return;
        }
    }

    // the folder becomes read-only after the rotation: the commit and the rollback fail
    const QFileDevice::Permissions dirPermissions = QFile::permissions( tmpDir.path() );
    const QString dirPath = tmpDir.path();
    int calls = 0;
    QString errorMessage;
    bool saved;
    {
        SaveCommitHook hook([&]() {
            ++calls;
            QFile::setPermissions(dirPath, readOnly);

            return false;
        });
        saved = project->saveProject_imp(dir, name, false /*autoSave*/, false /*updateProjectProperties*/, 0, &errorMessage, eProjectFileFormatBinary);
    }
    ASSERT_TRUE( QFile::setPermissions(dirPath, dirPermissions) );
    EXPECT_GE(calls, 1) << "the save failed before the rotation: " << errorMessage.toStdString();
    EXPECT_FALSE(saved);
    EXPECT_EQ(1, calls) << "a save whose rollback failed must not try again";
    EXPECT_TRUE( errorMessage.contains(path) ) << errorMessage.toStdString();
    EXPECT_TRUE( errorMessage.contains( QString::fromUtf8("The previous version of the project is in %1").arg(backup1) ) ) << errorMessage.toStdString();
    EXPECT_FALSE( QFile::exists(path) );
    EXPECT_TRUE( readFile(backup1) == previous );
    project->resetProject();
}
#endif

// The "Save versions" preference: the number of backups (.~1~ is the previous version), none with 0.
TEST(ProjectSave, Backups)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("p.ntp");
    const QString path = dir + name;
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    {
        // more than 9: .~10~ is older than .~9~ (they were sorted alphabetically)
        SaveVersions saveVersions(11);
        std::vector<QByteArray> versions;
        for (int k = 0; k < 14; ++k) {
            setFrameRange(1, 100 + k);
            ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
            versions.push_back( readFile(path) );
        }
        EXPECT_TRUE( readFile(path) == versions[13] );
        for (int n = 1; n <= 11; ++n) {
            EXPECT_TRUE( readFile( path + QString::fromUtf8(".~%1~").arg(n) ) == versions[13 - n] ) << "backup " << n;
        }
        EXPECT_FALSE( QFile::exists( path + QString::fromUtf8(".~12~") ) );
    }
    {
        // 0: no backups, the old ones are removed
        SaveVersions saveVersions(0);
        setFrameRange(1, 200);
        ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatText) );
        EXPECT_EQ( eProjectFileFormatText, Project::detectFileFormat(path) );
        EXPECT_EQ( QStringList(name), QDir(dir).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) );
    }
    project->resetProject();
}

#ifndef Q_OS_WIN
// A project saved through a symbolic link (latest.ntp -> v012.ntp): the link is kept, its target is saved,
// with its backups next to it. (On Windows, QFile::link() makes a .lnk shortcut.)
TEST(ProjectSave, SymbolicLink)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString target = dir + QString::fromUtf8("v012.ntp");
    const QString link = dir + QString::fromUtf8("latest.ntp");
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    SaveVersions saveVersions(1);
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("v012.ntp"), eProjectFileFormatBinary) );
    const QByteArray previous = readFile(target);
    // a relative link, and a link to it
    ASSERT_TRUE( QFile::link( QString::fromUtf8("v012.ntp"), link ) );
    ASSERT_TRUE( QFile::link( link, dir + QString::fromUtf8("current.ntp") ) );

    for (const char* n : { "latest.ntp", "current.ntp" }) {
        setFrameRange( 1, (int)std::strlen(n) * 10 );
        const QByteArray before = readFile(target);
        ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8(n), eProjectFileFormatBinary) ) << n;
        EXPECT_TRUE( QFileInfo(link).isSymLink() ) << n;
        EXPECT_EQ( QFileInfo(target).canonicalFilePath(), QFileInfo(link).canonicalFilePath() ) << n;
        EXPECT_FALSE( readFile(target) == before ) << n;
        EXPECT_TRUE( readFile(link) == readFile(target) ) << n;
        EXPECT_TRUE( readFile( target + QString::fromUtf8(".~1~") ) == before ) << n;
        EXPECT_FALSE( QFile::exists( link + QString::fromUtf8(".~1~") ) ) << n;
        EXPECT_FALSE( QFile::exists( dir + QString::fromUtf8("current.ntp.~1~") ) ) << n;
    }
    EXPECT_TRUE( QFileInfo( dir + QString::fromUtf8("current.ntp") ).isSymLink() );
    project->resetProject();
}

// A loop of symbolic links: a clean error, nothing changes on disk.
TEST(ProjectSave, SymbolicLinkLoop)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    SaveVersions saveVersions(1);
    ASSERT_TRUE( QFile::link( QString::fromUtf8("b.ntp"), dir + QString::fromUtf8("a.ntp") ) );
    ASSERT_TRUE( QFile::link( QString::fromUtf8("a.ntp"), dir + QString::fromUtf8("b.ntp") ) );
    const QStringList before = QDir(dir).entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
    QString errorMessage;
    EXPECT_FALSE( project->saveProject_imp(dir, QString::fromUtf8("a.ntp"), false /*autoSave*/, false /*updateProjectProperties*/, 0, &errorMessage, eProjectFileFormatBinary) );
    EXPECT_TRUE( errorMessage.contains( QString::fromUtf8("too many levels of symbolic links") ) ) << errorMessage.toStdString();
    EXPECT_TRUE( errorMessage.contains( dir + QString::fromUtf8("a.ntp") ) ) << errorMessage.toStdString();
    EXPECT_EQ( before, QDir(dir).entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot) );
    EXPECT_TRUE( QFileInfo( dir + QString::fromUtf8("a.ntp") ).isSymLink() );
    EXPECT_TRUE( QFileInfo( dir + QString::fromUtf8("b.ntp") ).isSymLink() );
    project->resetProject();
}

// A dangling symbolic link: its target is created. If the folder of the target doesn't exist: a clean error.
TEST(ProjectSave, DanglingSymbolicLink)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString link = dir + QString::fromUtf8("latest.ntp");
    const QString target = dir + QString::fromUtf8("v013.ntp");
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    SaveVersions saveVersions(1);
    ASSERT_TRUE( QFile::link( QString::fromUtf8("v013.ntp"), link ) );
    ASSERT_FALSE( QFile::exists(target) );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("latest.ntp"), eProjectFileFormatBinary) );
    EXPECT_TRUE( QFileInfo(link).isSymLink() );
    EXPECT_EQ( eProjectFileFormatBinary, Project::detectFileFormat(target) );
    EXPECT_FALSE( QFile::exists( target + QString::fromUtf8(".~1~") ) );
    EXPECT_FALSE( QFile::exists( link + QString::fromUtf8(".~1~") ) );

    const QString link2 = dir + QString::fromUtf8("missing.ntp");
    ASSERT_TRUE( QFile::link( QString::fromUtf8("no-such-folder/v.ntp"), link2 ) );
    const QStringList before = QDir(dir).entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
    QString errorMessage;
    EXPECT_FALSE( project->saveProject_imp(dir, QString::fromUtf8("missing.ntp"), false /*autoSave*/, false /*updateProjectProperties*/, 0, &errorMessage, eProjectFileFormatBinary) );
    EXPECT_TRUE( errorMessage.contains(link2) ) << errorMessage.toStdString();
    EXPECT_EQ( before, QDir(dir).entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot) );
    EXPECT_TRUE( QFileInfo(link2).isSymLink() );
    project->resetProject();
}
#endif

#ifdef Q_OS_WIN
// Another process has the project file open without FILE_SHARE_DELETE (a render, an antivirus): it can't
// be replaced. The save fails with an error, and the file is as it was.
TEST(ProjectSave, FileOpenByAnotherProcess)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("p.ntp");
    const QString path = dir + name;
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    SaveVersions saveVersions(1);
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    const QByteArray previous = readFile(path);
    setFrameRange(3, 33);

    const std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
    HANDLE h = CreateFileW(nativePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ASSERT_NE(INVALID_HANDLE_VALUE, h);
    QString errorMessage;
    const bool saved = project->saveProject_imp(dir, name, false /*autoSave*/, false /*updateProjectProperties*/, 0, &errorMessage, eProjectFileFormatBinary);
    CloseHandle(h);
    EXPECT_FALSE(saved);
    EXPECT_TRUE( errorMessage.contains(path) ) << errorMessage.toStdString();
    EXPECT_TRUE( readFile(path) == previous );
    // the copy of the project file to .~1~ (it can't be renamed while open) was removed by the rollback
    EXPECT_FALSE( QFile::exists( path + QString::fromUtf8(".~1~") ) );
    EXPECT_EQ( QStringList(), strayFiles( dir, QStringList(name) ) );

    // then it saves
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatBinary) );
    EXPECT_FALSE( readFile(path) == previous );
    EXPECT_TRUE( readFile( path + QString::fromUtf8(".~1~") ) == previous );
    project->resetProject();
}
#endif

#ifndef Q_OS_WIN
// Saving keeps the permissions of the project file (it was replaced by a new file with the default permissions).
TEST(ProjectSave, KeepsPermissions)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("p.ntp");
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    project->resetProject();
    createTestProject(app);
    ASSERT_TRUE( saveCopy(project, dir, name, eProjectFileFormatText) );
    const QFileDevice::Permissions permissions = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ReadGroup;
    ASSERT_TRUE( QFile::setPermissions(dir + name, permissions) );
    for (ProjectFileFormatEnum format : { eProjectFileFormatText, eProjectFileFormatBinary }) {
        ASSERT_TRUE( saveCopy(project, dir, name, format) );
        EXPECT_EQ( format, Project::detectFileFormat(dir + name) );
        EXPECT_EQ( permissions, QFile::permissions(dir + name) ) << "format " << (int)format;
    }
    project->resetProject();
}
#endif

// Sizes and times of a large project in both formats, and the same text -> binary -> text check
// with a real project, which may be saved by an old version and use OpenFX plug-ins. Runs only if
// NATRON_TEST_LARGE_PROJECT is the path of a text project, e.g. Natron-Tests/Spaceship/Spaceship_Natron_23.ntp.
TEST(ProjectFileFormat, LargeProject)
{
    const char* largeProject = std::getenv("NATRON_TEST_LARGE_PROJECT");
    if (!largeProject) {
        reportSkipped("NATRON_TEST_LARGE_PROJECT is not set");

        return;
    }
    QFileInfo info( QString::fromUtf8(largeProject) );
    ASSERT_TRUE( info.exists() );
    ASSERT_EQ( eProjectFileFormatText, Project::detectFileFormat( info.absoluteFilePath() ) );
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    const int n = 5;
    QElapsedTimer timer;

    // parse only (ProjectSerialization), without creating the nodes
    auto parse = [&](const QString& path, ProjectFileFormatEnum format) {
        qint64 best = -1;
        for (int i = 0; i < n; ++i) {
            timer.start();
            FStreamsSupport::ifstream ifile;
            FStreamsSupport::open( &ifile, path.toStdString(), std::ios_base::in | std::ios_base::binary );
            bool bg;
            ProjectSerialization obj(app);
            if (format == eProjectFileFormatText) {
                boost::archive::xml_iarchive ia(ifile);
                ia >> boost::serialization::make_nvp("Background_project", bg);
                ia >> boost::serialization::make_nvp("Project", obj);
            } else {
                PortableBinaryIArchive ia(ifile);
                unsigned int version;
                int projectFormat;
                ia >> version >> projectFormat;
                ia >> boost::serialization::make_nvp("Background_project", bg);
                ia >> boost::serialization::make_nvp("Project", obj);
            }
            qint64 t = timer.nsecsElapsed() / 1000;
            if ( (best < 0) || (t < best) ) {
                best = t;
            }
        }

        return best;
    };
    auto load = [&](const QString& path) {
        timer.start();
        EXPECT_TRUE( project->loadProject( QFileInfo(path).absolutePath() + QLatin1Char('/'), QFileInfo(path).fileName() ) );

        return timer.nsecsElapsed() / 1000;
    };
    auto save = [&](const QString& name, ProjectFileFormatEnum format) {
        qint64 best = -1;
        for (int i = 0; i < n; ++i) {
            timer.start();
            EXPECT_TRUE( saveCopy(project, dir, name, format) );
            qint64 t = timer.nsecsElapsed() / 1000;
            if ( (best < 0) || (t < best) ) {
                best = t;
            }
        }

        return best;
    };

    const qint64 loadOriginal = load( info.absoluteFilePath() );
    const qint64 saveText = save(QString::fromUtf8("large.ntp"), eProjectFileFormatText);
    const qint64 saveBinary = save(QString::fromUtf8("large-binary.ntp"), eProjectFileFormatBinary);
    const QByteArray text = readFile( dir + QString::fromUtf8("large.ntp") );
    const QByteArray binary = readFile( dir + QString::fromUtf8("large-binary.ntp") );
    const qint64 parseText = parse(dir + QString::fromUtf8("large.ntp"), eProjectFileFormatText);
    const qint64 parseBinary = parse(dir + QString::fromUtf8("large-binary.ntp"), eProjectFileFormatBinary);
    const qint64 loadText = load( dir + QString::fromUtf8("large.ntp") );
    // text -> text, for reference: loading and saving a project may change it (e.g. a plug-in that changes its parameters)
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("large-text2.ntp"), eProjectFileFormatText) );
    const QByteArray text2 = readFile( dir + QString::fromUtf8("large-text2.ntp") );
    const qint64 loadBinary = load( dir + QString::fromUtf8("large-binary.ntp") );
    ASSERT_TRUE( saveCopy(project, dir, QString::fromUtf8("large-roundtrip.ntp"), eProjectFileFormatText) );
    const QByteArray roundTrip = readFile( dir + QString::fromUtf8("large-roundtrip.ntp") );
    EXPECT_TRUE( text2 == roundTrip ) << "text -> binary -> text and text -> text differ";
    std::cout << "text -> text: " << (text == text2 ? "identical" : "different") << std::endl;
    const char* keepDir = std::getenv("NATRON_TEST_KEEP_DIR");
    if (keepDir) {
        const QString keep = QString::fromUtf8(keepDir) + QLatin1Char('/');
        const char* names[] = { "large.ntp", "large-text2.ntp", "large-binary.ntp", "large-roundtrip.ntp" };
        for (const char* n : names) {
            QFile::remove( keep + QString::fromUtf8(n) );
            QFile::copy( dir + QString::fromUtf8(n), keep + QString::fromUtf8(n) );
        }
    }

    std::cout << "project: " << largeProject << " (" << info.size() << " bytes, loaded in " << loadOriginal / 1000 << " ms)\n"
              << "size: text " << text.size() << " bytes, binary " << binary.size() << " bytes\n"
              << "save (best of " << n << "): text " << saveText / 1000. << " ms, binary " << saveBinary / 1000. << " ms\n"
              << "parse (best of " << n << "): text " << parseText / 1000. << " ms, binary " << parseBinary / 1000. << " ms\n"
              << "load (parse and create the nodes): text " << loadText / 1000. << " ms, binary " << loadBinary / 1000. << " ms" << std::endl;
    project->resetProject();
}
