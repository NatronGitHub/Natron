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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

#include <QByteArray>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

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
    const ProjectFileFormatEnum previous = project->getFileFormat();

    project->setFileFormat(format);
    bool ok = project->saveProject_imp(dir, name, false /*autoSave*/, false /*updateProjectProperties*/);
    project->setFileFormat(previous);

    return ok;
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
        + a + ".getProjectParam('frameRange').set(-5, 123)\n";
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

        // Restore it. In GUI mode, loadProject() asks whether to use the auto-save of a project;
        // headless, load it as the auto-save of an untitled project: it is also loaded as an auto-save.
        project->resetProject();
        ASSERT_TRUE( project->loadProject( dir, name + QString::fromUtf8(".binautosave"), true /*isUntitledAutosave*/ ) );
        EXPECT_EQ( format, project->getFileFormat() ) << "the format of the project was not restored from its auto-save";

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
// offer to restore a binary auto-save, which they couldn't load, and never delete it. 2.6 finds both.
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

namespace {
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
} // anonymous namespace

// A project saved by Natron 2.5.0 (Tests/data/ProjectFileFormat-2.5.ntp, made by ProjectFileFormat-2.5.py):
// it loads, and text -> binary -> text gives the same text as text -> text. The 2.5 class versions
// go through the binary archive.
TEST(ProjectFileFormat, Natron25Project)
{
    if ( dataDir().isEmpty() ) {
        std::cout << "Tests/data not found: skipped" << std::endl;

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
        std::cout << "Tests/data not found: skipped" << std::endl;

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

// Sizes and times of a large project in both formats, and the same text -> binary -> text check
// with a real project, which may be saved by an old version and use OpenFX plug-ins. Runs only if
// NATRON_TEST_LARGE_PROJECT is the path of a text project, e.g. Natron-Tests/Spaceship/Spaceship_Natron_23.ntp.
TEST(ProjectFileFormat, LargeProject)
{
    const char* largeProject = std::getenv("NATRON_TEST_LARGE_PROJECT");
    if (!largeProject) {
        std::cout << "NATRON_TEST_LARGE_PROJECT is not set: skipped" << std::endl;

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
