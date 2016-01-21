/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include <gtest/gtest.h>

#include <QString>
#include <QDir>

#include "Engine/StandardPaths.h"
#include <SequenceParsing.h>

NATRON_NAMESPACE_USING
using namespace SequenceParsing;

TEST(SequenceParsing,TestHashCharacter) {
    ////////
    ////////
    /////// WARNING: If this test fails it *may* left some files on disk
    ////// which can cause this test to fail on next run. For safety, if the test fails,
    ////// please delete the files indicated by the location that should've been printed on stdout.

    QStringList fileNames;
    int sequenceItemsCount = 10;
    ///create temporary files as a sequence and try to read that sequence.
    QString tempPath = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);
    QDir dir(tempPath);

    dir.mkpath(".");
    dir.mkdir("NatronUnitTest");
    dir.cd("NatronUnitTest");
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QFile file( dir.absoluteFilePath("test_" + QString::number(i) + ".unittest") );
        fileNames << file.fileName();
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    SequenceFromPattern sequence;
    filesListFromPattern(dir.absoluteFilePath("test_#.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_##.unittest").toStdString(), &sequence);
    EXPECT_EQ( 0,(int)sequence.size() );

    ///test that a single file matching pattern would match a single file only.
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_0.unittest").toStdString(), &sequence);
    EXPECT_EQ( 1,(int)sequence.size() );

    ///delete files
    for (int i = 0; i < fileNames.size(); ++i) {
        QFile::remove(fileNames[i]);
    }

    fileNames.clear();
    ///re-create all 11 files withtout padding
    sequenceItemsCount = 11;
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QFile file( dir.absoluteFilePath("test_" + QString::number(i) + ".unittest") );
        fileNames << file.fileName();
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_##.unittest").toStdString(), &sequence);
    EXPECT_EQ( 1,(int)sequence.size() );

    ///delete files
    for (int i = 0; i < fileNames.size(); ++i) {
        QFile::remove(fileNames[i]);
    }


    fileNames.clear();
    ///re-create 11 files with padding
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        QFile file( dir.absoluteFilePath("test_" + number + ".unittest") );
        fileNames << file.fileName();
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_##.unittest").toStdString(), &sequence);
    EXPECT_EQ( 11,(int)sequence.size() );

    ///delete files
    for (int i = 0; i < fileNames.size(); ++i) {
        QFile::remove(fileNames[i]);
    }

    fileNames.clear();
    ///now test with mixed paddings
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        QFile file( dir.absoluteFilePath(QString::number(i) + "test_" + number + ".unittest") );
        fileNames << file.fileName();
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#test_##.unittest").toStdString(), &sequence);
    EXPECT_EQ( 11,(int)sequence.size() );

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("##test_##.unittest").toStdString(), &sequence);
    EXPECT_EQ( 1,(int)sequence.size() );

    ///delete files
    for (int i = 0; i < fileNames.size(); ++i) {
        QFile::remove(fileNames[i]);
    }

    fileNames.clear();

    ///now test with mixed false paddings
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        QFile file( dir.absoluteFilePath(QString::number(i + 1) + "test_" + number + ".unittest") );
        fileNames << file.fileName();
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#test_##.unittest").toStdString(), &sequence);
    EXPECT_EQ( 0,(int)sequence.size() );
    ///delete files
    for (int i = 0; i < fileNames.size(); ++i) {
        QFile::remove(fileNames[i]);
    }

    ///now test with filenames containing only digits
    fileNames.clear();
    {
        QFile file( dir.absoluteFilePath("12345.unittest") );
        fileNames << file.fileName();
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    {
        QFile file( dir.absoluteFilePath("93830.unittest") );
        fileNames << file.fileName();
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    {
        QFile file( dir.absoluteFilePath("03829.unittest") );
        fileNames << file.fileName();
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    {
        QFile file( dir.absoluteFilePath("003830.unittest") );
        fileNames << file.fileName();
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#####.unittest").toStdString(), &sequence);
    EXPECT_EQ( 3,(int)sequence.size() );

    for (int i = 0; i < fileNames.size(); ++i) {
        QFile::remove(fileNames[i]);
    }

    ///test with an empty pattern
    sequence.clear();
    filesListFromPattern("", &sequence);
    EXPECT_EQ( 0,(int)sequence.size() );
}

TEST(SequenceParsing,TestPrintfLikeSyntax) {
    int sequenceItemsCount = 10;
    ///create temporary files as a sequence and try to read that sequence.
    QString tempPath = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);
    QDir dir(tempPath);

    dir.mkpath(".");
    dir.mkdir("NatronUnitTest");
    dir.cd("NatronUnitTest");
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QFile file( dir.absoluteFilePath("test_" + QString::number(i) + ".unittest") );
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    SequenceFromPattern sequence;
    filesListFromPattern(dir.absoluteFilePath("test_%01d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_%02d.unittest").toStdString(), &sequence);
    EXPECT_EQ( 0,(int)sequence.size() );

    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        dir.remove("test_" + QString::number(i) + ".unittest");
    }


    ///re-create all 11 files with padding
    sequenceItemsCount = 11;
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 4) {
            number.prepend('0');
        }
        QFile file( dir.absoluteFilePath("test_" + number + ".unittest") );
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_%04d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );

    ///testing wrong pattern
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("%02dtest_%04d.unittest").toStdString(), &sequence);
    EXPECT_EQ( 0,(int)sequence.size() );

    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 4) {
            number.prepend('0');
        }

        dir.remove("test_" + number + ".unittest");
    }


    ///test with mixed paddings
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        QString shortNumber = number;
        while (number.size() < 4) {
            number.prepend('0');
        }
        while (shortNumber.size() < 2) {
            shortNumber.prepend('0');
        }

        QFile file( dir.absoluteFilePath(shortNumber + "test_" + number + ".unittest") );
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("%02dtest_%04d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );


    ///try mixing printf-like syntax with hashes characters (###)
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("##test_%04d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );

    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#test_%04d.unittest").toStdString(), &sequence);
    EXPECT_EQ( 1,(int)sequence.size() );

    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        QString shortNumber = number;
        while (number.size() < 4) {
            number.prepend('0');
        }
        while (shortNumber.size() < 2) {
            shortNumber.prepend('0');
        }

        dir.remove(shortNumber + "test_" + number + ".unittest");
    }
}

TEST(SequenceParsing,TestViews) {
    int sequenceItemsCount = 11;
    ///create temporary files as a sequence and try to read that sequence.
    QString tempPath = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);
    QDir dir(tempPath);

    dir.mkpath(".");
    dir.mkdir("NatronUnitTest");
    dir.cd("NatronUnitTest");

    ///test with long view name
    QStringList filesCreated;
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "left" : "right";
            QFile file( dir.absoluteFilePath("test_" + QString::number(i) + "."  + viewName + ".unittest") );
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }

    SequenceFromPattern sequence;
    filesListFromPattern(dir.absoluteFilePath("test_%01d.%V.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );
    for (SequenceFromPattern::iterator it = sequence.begin(); it != sequence.end(); ++it) {
        EXPECT_EQ( 2,(int)it->second.size() );
    }

    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove( filesCreated.at(i) );
    }
    filesCreated.clear();

    ///test with long view names in the middle of filenames
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "left" : "right";
            QFile file( dir.absoluteFilePath("weird." + viewName + ".test_" + QString::number(i)  + ".unittest") );
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird.%V.test_%01d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );
    for (SequenceFromPattern::iterator it = sequence.begin(); it != sequence.end(); ++it) {
        EXPECT_EQ( 2,(int)it->second.size() );
    }


    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove( filesCreated.at(i) );
    }
    filesCreated.clear();


    ///test with short view name
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "l" : "r";
            QFile file( dir.absoluteFilePath("test_" + QString::number(i) + "." + viewName + ".unittest") );
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_%01d.%v.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );
    for (SequenceFromPattern::iterator it = sequence.begin(); it != sequence.end(); ++it) {
        EXPECT_EQ( 2,(int)it->second.size() );
    }

    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove( filesCreated.at(i) );
    }
    filesCreated.clear();


    ///test with short view name placed weirdly
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "l" : "r";
            QFile file( dir.absoluteFilePath("weird" + viewName + "test_" + QString::number(i) + ".unittest") );
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird%vtest_%01d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );

    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove( filesCreated.at(i) );
    }
    filesCreated.clear();


    ///test with short view name placed in the middle of a filename
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "l" : "r";
            QFile file( dir.absoluteFilePath("weird." + viewName + ".test_" + QString::number(i) + ".unittest") );
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird.%v.test_%01d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );
    for (SequenceFromPattern::iterator it = sequence.begin(); it != sequence.end(); ++it) {
        EXPECT_EQ( 2,(int)it->second.size() );
    }

    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove( filesCreated.at(i) );
    }
    filesCreated.clear();


    ///test multi-view

    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 5; ++j) {
            QString viewName;
            switch (j) {
            case 0:
                viewName = "l";
                break;
            case 1:
                viewName = "r";
                break;
            case 2:
                viewName = "view2";
                break;
            case 3:
                viewName = "view3";
                break;
            case 4:
                viewName = "view4";
                break;
            default:
                break;
            }
            QFile file( dir.absoluteFilePath("weird." + viewName + ".test_" + QString::number(i) + ".unittest") );
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird.%v.test_%01d.unittest").toStdString(), &sequence);
    EXPECT_EQ( sequenceItemsCount,(int)sequence.size() );
    for (SequenceFromPattern::iterator it = sequence.begin(); it != sequence.end(); ++it) {
        EXPECT_EQ( 5, (int)it->second.size() );
    }

    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove( filesCreated.at(i) );
    }
    filesCreated.clear();
}

TEST(SequenceParsing,OutputSequence) {
    std::string pattern = "weird.%v.test_%01d.unittest";
    std::string filename = generateFileNameFromPattern(pattern, 120, 0);

    EXPECT_EQ("weird.l.test_120.unittest",filename);

    pattern = "####.jpg";
    filename = generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_EQ("0120.jpg",filename);

    pattern = "###lalala%04d.jpg";
    filename = generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_EQ("120lalala0120.jpg",filename);

    pattern = "####%V.jpg";
    filename = generateFileNameFromPattern(pattern, 120, 5);
    EXPECT_EQ("0120view5.jpg",filename);

    ///test with a non-pattern
    pattern = "mysequence10.png";
    filename = generateFileNameFromPattern(pattern, 120, 5);
    EXPECT_EQ(pattern,filename);
}


TEST(FileNameContent,GeneralTest) {
    std::string file1("/Users/Test/mysequence001.jpg");

    ///first off test-out the API of FileNameContent, see if it is working
    ///correctly for base functionnality
    FileNameContent file1Content(file1);

    ASSERT_TRUE(file1Content.fileName() == "mysequence001.jpg");
    ASSERT_TRUE(file1Content.getPath() == "/Users/Test/");
    ASSERT_TRUE(file1Content.absoluteFileName() == file1);

    ASSERT_TRUE(file1Content.getFilePattern(3) == "mysequence###0.jpg");
    std::string numberStr;
    ASSERT_TRUE( file1Content.getNumberByIndex(0, &numberStr) );
    ASSERT_TRUE(numberStr == "001");
    ASSERT_TRUE(QString( numberStr.c_str() ).toInt() == 1);

    ///now attempt to match it to a second filename
    std::string file2("/Users/Test/mysequence002.jpg");
    FileNameContent file2Content(file2);
    int frameNumberIndex;
    ASSERT_TRUE( file1Content.matchesPattern(file2Content, &frameNumberIndex) );
    ASSERT_TRUE(frameNumberIndex == 0);

    ///attempt to match it to a wrong second filename
    file2 = "/Users/Test/mysequence01.jpg";
    file2Content = FileNameContent(file2);
    ASSERT_FALSE( file1Content.matchesPattern(file2Content, &frameNumberIndex) );
}

TEST(SequenceFromFiles,SimpleTest) {
    ///test that a single file generates a pattern identical to the filename
    FileNameContent file1("/Users/Test/mysequence000.jpg");
    SequenceFromFiles sequence(file1,false);

    EXPECT_EQ( file1.absoluteFileName(), sequence.generateValidSequencePattern() );
    EXPECT_EQ( file1.fileName(), sequence.generateUserFriendlySequencePattern() );
    EXPECT_EQ( "jpg", sequence.fileExtension() );
    EXPECT_TRUE( sequence.isSingleFile() );
    EXPECT_EQ( 0, sequence.getFirstFrame() );
    EXPECT_EQ( 0, sequence.getLastFrame() );
    EXPECT_TRUE( sequence.contains( file1.absoluteFileName() ) );

    ///now add valid files
    for (int i = 0; i < 11; ++i) {
        QString number = QString::number(i);
        while (number.size() < 3) {
            number.prepend('0');
        }
        bool ok = sequence.tryInsertFile( FileNameContent( QString("/Users/Test/mysequence" + number + ".jpg").toStdString() ) );
        if (i == 0) {
            EXPECT_FALSE(ok);
        } else {
            EXPECT_TRUE(ok);
        }
    }

    EXPECT_FALSE( sequence.isSingleFile() );
    EXPECT_EQ( 0, sequence.getFirstFrame() );
    EXPECT_EQ( 10, sequence.getLastFrame() );
    EXPECT_EQ( 11, (int)sequence.getFrameIndexes().size() );
    EXPECT_EQ( "/Users/Test/mysequence###.jpg", sequence.generateValidSequencePattern() );
}

TEST(SequenceFromFiles,ComplexTest) {
    {
        FileNameContent file1("/Users/Test/00mysequence000.jpg");
        SequenceFromFiles sequence(file1,false);
        ///now add valid files
        for (int i = 1; i < 11; ++i) {
            QString number = QString::number(i);
            while (number.size() < 3) {
                number.prepend('0');
            }
            bool ok = sequence.tryInsertFile( FileNameContent( QString("/Users/Test/00mysequence" + number + ".jpg").toStdString() ) );
            EXPECT_TRUE(ok);
        }
        EXPECT_EQ( "/Users/Test/00mysequence###.jpg", sequence.generateValidSequencePattern() );
    }


    ///now test only filenames with digits
    {
        FileNameContent file1("/Users/Test/12345.jpg");
        SequenceFromFiles sequence(file1,false);
        EXPECT_TRUE( sequence.tryInsertFile( FileNameContent("/Users/Test/34567.jpg") ) );
        EXPECT_TRUE( sequence.tryInsertFile( FileNameContent("/Users/Test/04592.jpg") ) );
        EXPECT_TRUE( sequence.tryInsertFile( FileNameContent("/Users/Test/23489.jpg") ) );
        EXPECT_TRUE( sequence.tryInsertFile( FileNameContent("/Users/Test/00001.jpg") ) );
        EXPECT_FALSE( sequence.tryInsertFile( FileNameContent("/Users/Test/0001.jpg") ) );
        EXPECT_FALSE( sequence.tryInsertFile( FileNameContent("/Users/Test/122938.jpg") ) );
        EXPECT_FALSE( sequence.tryInsertFile( FileNameContent("/Users/Test/000002.jpg") ) );
        EXPECT_TRUE(sequence.generateValidSequencePattern() == "/Users/Test/#####.jpg");
    }
}
