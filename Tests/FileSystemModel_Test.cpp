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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <QDir>

#include "Engine/FileSystemModel.h"

NATRON_NAMESPACE_USING

namespace {

class MockSortableView : public SortableViewI {
 public:
  MockSortableView() = default;
  ~MockSortableView() override {}

  // SortableViewI methods
  Qt::SortOrder sortIndicatorOrder() const override { return _sortOrder; }

  int sortIndicatorSection() const override { return _sortSection; }

  void onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order) override {
    _sortSection = logicalIndex;
    _sortOrder = order;
  }

 private:
  int _sortSection = 0;
  Qt::SortOrder _sortOrder = Qt::AscendingOrder;
};

}  // namespace

TEST(FileSystemModelTest, DriveName) {
  MockSortableView sortableView;
  auto model = std::make_shared<FileSystemModel>();
  model->initialize(&sortableView);

  struct DriveNameTestCase {
    const char* input;
    bool isWindowsDriveName;
    bool startsWithWindowsDriveName;
    bool isUnixDriveName;
    bool startsWithUnixDriveName;
  };
  std::vector<DriveNameTestCase> testCases({
      {"", false, false, false, false},
      {"filename.txt", false, false, false, false},
      {"somedir/", false, false, false, false},
      {"somedir/filename.txt", false, false, false, false},
      {"/", false, false, true, true},
      {"/filename.txt", false, false, false, true},
      {"/somedir/", false, false, false, true},
      {"/somedir/filename.txt", false, false, false, true},
      {"c:", false, false, false, false},
      {"c:/", true, true, false, false},
      {"c:\\", true, true, false, false},
      {"c:/filename.txt", false, true, false, false},
      {"c:\\filename.txt", false, true, false, false},
      {"c:/somedir/", false, true, false, false},
      {"c:/somedir/filename.txt", false, true, false, false},
      {"c:\\somedir\\", false, true, false, false},
      {"c:\\somedir\\filename.txt", false, true, false, false},
      {"//", false, false, false, true},
      {"//somehost", false, false, false, true},
      {"//somehost/", true, true, false, true},
      {"//somehost/somedir/", false, true, false, true},
      {"//somehost/somedir/filename.txt", false, true, false, true},
  });

  for (const auto& testCase : testCases) {
    const QString input = QString::fromUtf8(testCase.input);
#ifdef __NATRON_WIN32__
    const bool expectedIsDriveName = testCase.isWindowsDriveName;
    const bool expectedStartsWithDriveName =
        testCase.startsWithWindowsDriveName;
#else
    const bool expectedIsDriveName = testCase.isUnixDriveName;
    const bool expectedStartsWithDriveName = testCase.startsWithUnixDriveName;
#endif

    EXPECT_EQ(expectedIsDriveName, FileSystemModel::isDriveName(input))
        << " input '" << testCase.input << "'";
    EXPECT_EQ(expectedStartsWithDriveName,
              FileSystemModel::startsWithDriveName(input))
        << " input '" << testCase.input << "'";
  }
}

TEST(FileSystemModelTest, GetSplitPath) {
  MockSortableView sortableView;
  auto model = std::make_shared<FileSystemModel>();
  model->initialize(&sortableView);

  struct SplitPathTestCase {
    const char* input;
    std::vector<const char*> expectedWindowsOutput;
    std::vector<const char*> expectedUnixOutput;
  };
  std::vector<SplitPathTestCase> testCases({
      {"", {}, {}},
      {"filename.txt", {"filename.txt"}, {"filename.txt"}},
      {"somedir/", {"somedir"}, {"somedir"}},
      {"somedir/filename.txt",
       {"somedir", "filename.txt"},
       {"somedir", "filename.txt"}},
      {"/", {""}, {"/", ""}},
      {"/filename.txt", {"", "filename.txt"}, {"/", "filename.txt"}},
      {"/somedir/", {"", "somedir"}, {"/", "somedir"}},
      {"/somedir/filename.txt",
       {"", "somedir", "filename.txt"},
       {"/", "somedir", "filename.txt"}},
      {"c:", {"c:"}, {"c:"}},
      {"c:/", {"c:/", ""}, {"c:"}},
      {"c:\\", {"c:\\", ""}, {"c:\\"}},
      {"c:/filename.txt", {"c:/", "filename.txt"}, {"c:", "filename.txt"}},
      {"c:\\filename.txt", {"c:\\", "filename.txt"}, {"c:\\filename.txt"}},
      {"c:/somedir/", {"c:/", "somedir"}, {"c:", "somedir"}},
      {"c:/somedir/filename.txt",
       {"c:/", "somedir", "filename.txt"},
       {"c:", "somedir", "filename.txt"}},
      {"c:\\somedir\\", {"c:\\", "somedir\\"}, {"c:\\somedir\\"}},
      {"c:\\somedir\\filename.txt",
       {"c:\\", "somedir\\filename.txt"},
       {"c:\\somedir\\filename.txt"}},
      {"//", {"", ""}, {"/", ""}},
      {"//somehost",
       {"", "", "somehost"},  // Not considered a network path because
                              // of missing slash after hostname.
       {"/", "", "somehost"}},
      {"//somehost/", {"//somehost/", ""}, {"/", "", "somehost"}},
      {"//somehost/somedir/",
       {"//somehost/", "somedir"},
       {"/", "", "somehost", "somedir"}},
      {"//somehost/somedir/filename.txt",
       {"//somehost/", "somedir", "filename.txt"},
       {"/", "", "somehost", "somedir", "filename.txt"}},
  });

  for (const auto& testCase : testCases) {
    const QString input = QString::fromUtf8(testCase.input);

#ifdef __NATRON_WIN32__
    const auto& expectedOutput = testCase.expectedWindowsOutput;
#else
    const auto& expectedOutput = testCase.expectedUnixOutput;
#endif

    const int expectedSize = static_cast<int>(expectedOutput.size());
    const auto output = FileSystemModel::getSplitPath(input);
    EXPECT_EQ(expectedSize, output.size()) << "input '" << testCase.input
                                           << "'";

    for (int i = 0; i < expectedSize && i < output.size(); ++i) {
      EXPECT_EQ(expectedOutput[i], output[i].toStdString())
          << "input '" << testCase.input << "' i " << i;
    }
  }
}

TEST(FileSystemModelTest, CleanPath) {
  MockSortableView sortableView;
  auto model = std::make_shared<FileSystemModel>();
  model->initialize(&sortableView);

  struct CleanPathTestCase {
    const char* input;
    const char* expectedWindowsOutput;  // nullptr if the expectation is the
                                        // same as input.
    const char* expectedUnixOutput;  // nullptr if the expectation is the same
                                     // as expectedWindowsOutput.
  };

  std::vector<CleanPathTestCase> testCases({
      // Paths that behave the same on all platforms.
      // - Verify trailing slashes are removed.
      {"", nullptr, nullptr},
      {"filename.txt", nullptr, nullptr},
      {"somedir/", "somedir", nullptr},
      {"somedir/filename.txt", nullptr, nullptr},
      {"/", nullptr, nullptr},
      {"/filename.txt", nullptr, nullptr},
      {"/somedir/", "/somedir", nullptr},
      {"/somedir/filename.txt", nullptr, nullptr},
      {"c:", nullptr, nullptr},
      // Platform specific path behaviors.
      // Paths with Windows drive letters and backslashes.
      // - Verify drive letters become capitalized on Windows
      // - Verify drives keep their trailing slash.
      // - Verify backslashes are converted to forward slashes on Windows.
      // - Verify Unix platforms do not treat forward slashes and drive
      //   letters in special way.
      {"c:/", "C:/", "c:"},
      {"c:\\", "C:/", "c:\\"},
      {"c:/filename.txt", "C:/filename.txt", "c:/filename.txt"},
      {"c:\\filename.txt", "C:/filename.txt", "c:\\filename.txt"},
      {"c:/somedir/", "C:/somedir", "c:/somedir"},
      {"c:/somedir/filename.txt", "C:/somedir/filename.txt", "c:/somedir/filename.txt"},
      {"c:\\somedir\\", "C:/somedir", "c:\\somedir\\"},
      {"c:\\somedir\\filename.txt", "C:/somedir/filename.txt",
       "c:\\somedir\\filename.txt"},
      // Windows Network Shares
      // - Verify that hostname is capitalized on Windows as long as it
      //   is followed by a /.
      // - Verify Unix just collapses the '//' to a single slash.
      {"//", "/", nullptr},
      {"//somehost", "//somehost", "/somehost"},
      {"//somehost/", "//SOMEHOST/", "/somehost"},
      {"//somehost/somedir/", "//SOMEHOST/somedir", "/somehost/somedir"},
      {"//somehost/somedir/filename.txt",
        "//SOMEHOST/somedir/filename.txt",
        "/somehost/somedir/filename.txt"},
  });

  for (const auto& testCase : testCases) {
    const QString input = QString::fromUtf8(testCase.input);
    std::string expectedOutput;

#ifdef __NATRON_WIN32__
    if (testCase.expectedWindowsOutput != nullptr) {
      expectedOutput = testCase.expectedWindowsOutput;
    } else {
      expectedOutput = testCase.input;
    }
#else
    if (testCase.expectedUnixOutput != nullptr) {
      expectedOutput = testCase.expectedUnixOutput;
    } else if (testCase.expectedWindowsOutput != nullptr) {
      expectedOutput = testCase.expectedWindowsOutput;
    } else {
      expectedOutput = testCase.input;
    }
#endif
    const QString output = FileSystemModel::cleanPath(input);
    ASSERT_TRUE(!output.isNull());
    EXPECT_EQ(expectedOutput, output.toStdString()) << " input '" << testCase.input << "'";
  }
}

// A file name typed in a save dialog is resolved against the directory shown by the dialog,
// not against the working directory of the process (#825).
TEST(FileSystemModelTest, AbsoluteFilePath) {
  struct AbsoluteFilePathTestCase {
    const char* directory;
    const char* filePath;
    const char* expectedOutput;
  };

  const QStringList variableNames({QString::fromUtf8("OCIO"), QString::fromUtf8("Project")});

  std::vector<AbsoluteFilePathTestCase> testCases({
      // Relative names are resolved against the directory, with or without a
      // trailing slash.
      {"/somedir", "project.ntp", "/somedir/project.ntp"},
      {"/somedir/", "project.ntp", "/somedir/project.ntp"},
      {"/", "project.ntp", "/project.ntp"},
      {"/somedir/", "sub/out####.exr", "/somedir/sub/out####.exr"},
      {"/somedir/", "../out_%04d.exr", "/out_%04d.exr"},
      // Absolute paths and project variables are unchanged.
      {"/somedir/", "/otherdir/project.ntp", "/otherdir/project.ntp"},
      {"/somedir/", "[Project]/out.exr", "[Project]/out.exr"},
      {"/somedir/", "[OCIO]/config.ocio", "[OCIO]/config.ocio"},
      // A leading "[Name]" that is not a project variable is part of a bare name.
      {"/somedir/", "[v2] shot.ntp", "/somedir/[v2] shot.ntp"},
      {"/somedir/", "[]out.exr", "/somedir/[]out.exr"},
      {"/somedir/", "[Project", "/somedir/[Project"},
      // Nothing to resolve.
      {"/somedir/", "", ""},
      // No directory (the "Computer" level): unchanged.
      {"", "project.ntp", "project.ntp"},
#ifdef __NATRON_WIN32__
      {"C:/somedir/", "project.ntp", "C:/somedir/project.ntp"},
      {"C:/somedir/", "D:/otherdir/project.ntp", "D:/otherdir/project.ntp"},
      {"C:/somedir/", "//somehost/share/project.ntp", "//somehost/share/project.ntp"},
      {"//SOMEHOST/share/", "project.ntp", "//SOMEHOST/share/project.ntp"},
#endif
  });

  for (const auto& testCase : testCases) {
    const QString output = FileSystemModel::absoluteFilePath(QString::fromUtf8(testCase.directory),
                                                             QString::fromUtf8(testCase.filePath),
                                                             variableNames);
    EXPECT_EQ(std::string(testCase.expectedOutput), output.toStdString())
        << " directory '" << testCase.directory << "' filePath '" << testCase.filePath << "'";
  }

#ifdef Q_OS_UNIX
  // "~" is the home directory.
  EXPECT_EQ((QDir::homePath() + QString::fromUtf8("/project.ntp")).toStdString(),
            FileSystemModel::absoluteFilePath(QString::fromUtf8("/somedir/"),
                                              QString::fromUtf8("~/project.ntp"),
                                              variableNames).toStdString());
#endif
}

// The text a save dialog may show after the extension is removed, only in the last path
// component (#825).
TEST(FileSystemModelTest, RemoveTextAfterExtension) {
  struct RemoveTextAfterExtensionTestCase {
    const char* input;
    const char* expectedOutput;
  };

  std::vector<RemoveTextAfterExtensionTestCase> testCases({
      {"", ""},
      {"out", "out"},
      {"out.exr", "out.exr"},
      {"out.####.exr 1-10", "out.####.exr"},
      {"/data/out.####.exr 1-10", "/data/out.####.exr"},
      // Dots and spaces in directories are kept.
      {"/data/a.b c/out", "/data/a.b c/out"},
      {"/Users/me/v1.2 final/curve", "/Users/me/v1.2 final/curve"},
      {"/data/a.b c/out.exr 1-10", "/data/a.b c/out.exr"},
      // A space in the name before the extension is kept.
      {"/data/my out.exr", "/data/my out.exr"},
  });

  for (const auto& testCase : testCases) {
    const QString output = FileSystemModel::removeTextAfterExtension(QString::fromUtf8(testCase.input));
    EXPECT_EQ(std::string(testCase.expectedOutput), output.toStdString()) << " input '" << testCase.input << "'";
  }
}
