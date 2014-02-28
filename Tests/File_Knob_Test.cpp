//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>

#include <QString>
#include <QDir>

#include "Engine/StandardPaths.h"
#include "Engine/SequenceParsing.h"

using namespace SequenceParsing;

TEST(SequenceParsing,TestHashCharacter) {
    
    ////////
    ////////
    /////// WARNING: If this test fails it *may* left some files on disk
    ////// which can cause this test to fail on next run. For safety, if the test fails,
    ////// please delete the files indicated by the location that should've been printed on stdout.
    int sequenceItemsCount = 10;
    ///create temporary files as a sequence and try to read that sequence.
    QString tempPath = Natron::StandardPaths::writableLocation(Natron::StandardPaths::TempLocation);
    QDir dir(tempPath);
    dir.mkpath(".");
    dir.mkdir("NatronUnitTest");
    dir.cd("NatronUnitTest");
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QFile file(dir.absoluteFilePath("test_" + QString::number(i) + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    SequenceFromPattern sequence;
    filesListFromPattern(dir.absoluteFilePath("test_#.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);
    
    ///test that a single file matching pattern would match a single file only.
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_0.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 1);
    
    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        dir.remove("test_" + QString::number(i) + ".unittest");
    }
    
    
    ///re-create all 11 files withtout padding
    sequenceItemsCount = 11;
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QFile file(dir.absoluteFilePath("test_" + QString::number(i) + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 1);
    
    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        dir.remove("test_" + QString::number(i) + ".unittest");
    }
    
    ///re-create 11 files with padding
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        QFile file(dir.absoluteFilePath("test_" + number + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 11);
    
    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }

        dir.remove("test_" + number + ".unittest");
    }
    
    
    ///now test with mixed paddings
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        QFile file(dir.absoluteFilePath(QString::number(i) + "test_" + number + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 11);
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("##test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 1);
    
    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        
        dir.remove(QString::number(i) + "test_" + number + ".unittest");
    }
    
    
    ///now test with mixed false paddings
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        QFile file(dir.absoluteFilePath(QString::number(i+1) + "test_" + number + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);
    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        
        dir.remove(QString::number(i+1) + "test_" + number + ".unittest");
    }
    
    ///now test with filenames containing only digits
    QStringList fileNames;
    fileNames << "12345.unittest";
    {
        QFile file(dir.absoluteFilePath("12345.unittest"));
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    fileNames << "93830.unittest";
    {
        QFile file(dir.absoluteFilePath("93830.unittest"));
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    fileNames << "03829.unittest";
    {
        QFile file(dir.absoluteFilePath("03829.unittest"));
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    fileNames << "003830.unittest";
    {
        QFile file(dir.absoluteFilePath("003830.unittest"));
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#####.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 3);
    
    for (int i = 0; i < fileNames.size(); ++i) {
        QFile::remove(fileNames[i]);
    }
    
    ///test with an empty pattern
    sequence.clear();
    filesListFromPattern("", &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);

}

TEST(SequenceParsing,TestPrintfLikeSyntax) {
    int sequenceItemsCount = 10;
    ///create temporary files as a sequence and try to read that sequence.
    QString tempPath = Natron::StandardPaths::writableLocation(Natron::StandardPaths::TempLocation);
    QDir dir(tempPath);
    dir.mkpath(".");
    dir.mkdir("NatronUnitTest");
    dir.cd("NatronUnitTest");
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QFile file(dir.absoluteFilePath("test_" + QString::number(i) + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    SequenceFromPattern sequence;
    filesListFromPattern(dir.absoluteFilePath("test_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_%02d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);
    
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
        QFile file(dir.absoluteFilePath("test_" + number + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);

    ///testing wrong pattern
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("%02dtest_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);
    
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
        
        QFile file(dir.absoluteFilePath(shortNumber + "test_" + number + ".unittest"));
        std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("%02dtest_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    
    ///try mixing printf-like syntax with hashes characters (###)
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("##test_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("#test_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 1);
    
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
    QString tempPath = Natron::StandardPaths::writableLocation(Natron::StandardPaths::TempLocation);
    QDir dir(tempPath);
    dir.mkpath(".");
    dir.mkdir("NatronUnitTest");
    dir.cd("NatronUnitTest");
    
    ///test with long view name
    QStringList filesCreated;
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "left" : "right";
            QFile file(dir.absoluteFilePath("test_" + QString::number(i) + "."  + viewName + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    
    SequenceFromPattern sequence;
    filesListFromPattern(dir.absoluteFilePath("test_%01d.%V.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (SequenceFromPattern::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        EXPECT_TRUE(it->second.size() == 2);
    }
    
    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
    }
    filesCreated.clear();
    
    ///test with long view names in the middle of filenames
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "left" : "right";
            QFile file(dir.absoluteFilePath("weird." + viewName + ".test_" + QString::number(i)  + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird.%V.test_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (SequenceFromPattern::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        EXPECT_TRUE(it->second.size() == 2);
    }

    
    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
    }
    filesCreated.clear();

    
    ///test with short view name
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "l" : "r";
            QFile file(dir.absoluteFilePath("test_" + QString::number(i) + "." + viewName + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("test_%01d.%v.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (SequenceFromPattern::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        EXPECT_TRUE(it->second.size() == 2);
    }
    
    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
    }
    filesCreated.clear();
    
    
    ///test with short view name placed weirdly
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "l" : "r";
            QFile file(dir.absoluteFilePath("weird" + viewName + "test_"+ QString::number(i) + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird%vtest_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);

    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
    }
    filesCreated.clear();

    
    ///test with short view name placed in the middle of a filename
    for (int i = 0; i < sequenceItemsCount; ++i) {
        for (int j = 0; j < 2; ++j) {
            QString viewName = j == 0 ? "l" : "r";
            QFile file(dir.absoluteFilePath("weird." + viewName + ".test_"+ QString::number(i) + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird.%v.test_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (SequenceFromPattern::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        EXPECT_TRUE(it->second.size() == 2);
    }
    
    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
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
            QFile file(dir.absoluteFilePath("weird." + viewName + ".test_"+ QString::number(i) + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    filesListFromPattern(dir.absoluteFilePath("weird.%v.test_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (SequenceFromPattern::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        EXPECT_TRUE(it->second.size() == 5);
    }
    
    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
    }
    filesCreated.clear();
}

TEST(SequenceParsing,OutputSequence) {
    QString pattern = "weird.%v.test_%01d.unittest";
    QString filename = generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_TRUE(QString("weird.l.test_120.unittest") == filename);
    
    pattern = "####.jpg";
    filename = generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_TRUE(QString("0120.jpg") == filename);
    
    pattern = "###lalala%04d.jpg";
    filename = generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_TRUE(QString("120lalala0120.jpg") == filename);
    
    pattern = "####%V.jpg";
    filename = generateFileNameFromPattern(pattern, 120, 5);
    EXPECT_TRUE(QString("0120view5.jpg") == filename);
    
    ///test with a non-pattern
    pattern = "mysequence10.png";
    filename = generateFileNameFromPattern(pattern, 120, 5);
    EXPECT_TRUE(filename == pattern);
}


TEST(FileNameContent,GeneralTest) {
    QString file1("/Users/Test/mysequence001.jpg");
    
    ///first off test-out the API of FileNameContent, see if it is working
    ///correctly for base functionnality
    FileNameContent file1Content(file1);
    ASSERT_TRUE(file1Content.fileName() == "mysequence001.jpg");
    ASSERT_TRUE(file1Content.getPath() == "/Users/Test/");
    ASSERT_TRUE(file1Content.absoluteFileName() == file1);
    
    ASSERT_TRUE(file1Content.hasSingleNumber());
    ASSERT_FALSE(file1Content.isFileNameComposedOnlyOfDigits());
    ASSERT_TRUE(file1Content.getFilePattern() == "mysequence###0.jpg");
    QString numberStr;
    ASSERT_TRUE(file1Content.getNumberByIndex(0, &numberStr));
    ASSERT_TRUE(numberStr == "001");
    ASSERT_TRUE(numberStr.toInt() == 1);
    
    ///now attempt to match it to a second filename
    QString file2("/Users/Test/mysequence002.jpg");
    FileNameContent file2Content(file2);
    std::vector<int> frameNumberIndex;
    ASSERT_TRUE(file1Content.matchesPattern(file2Content, &frameNumberIndex));
    ASSERT_TRUE(frameNumberIndex.size() == 1 && frameNumberIndex[0] == 0);
    
    ///attempt to match it to a wrong second filename
    file2 = "/Users/Test/mysequence01.jpg";
    file2Content = FileNameContent(file2);
    ASSERT_FALSE(file1Content.matchesPattern(file2Content, &frameNumberIndex));

    
}

TEST(SequenceFromFiles,SimpleTest) {
    ///test that a single file generates a pattern identical to the filename
    FileNameContent file1("/Users/Test/mysequence000.jpg");
    SequenceFromFiles sequence(file1,false);
    EXPECT_TRUE(file1.absoluteFileName() == sequence.generateValidSequencePattern());
    EXPECT_TRUE(file1.absoluteFileName() == sequence.generateUserFriendlySequencePattern());
    EXPECT_TRUE(sequence.fileExtension() == "jpg");
    EXPECT_TRUE(sequence.isSingleFile());
    EXPECT_TRUE(sequence.getFirstFrame() == INT_MIN && sequence.getLastFrame() == INT_MAX);
    EXPECT_TRUE(sequence.contains(file1.absoluteFileName()));
    
    ///now add valid files
    for (int i = 0; i < 11; ++i) {
        QString number = QString::number(i);
        while (number.size() < 3) {
            number.prepend('0');
        }
        bool ok = sequence.tryInsertFile(FileNameContent("/Users/Test/mysequence" + number + ".jpg"));
        if (i == 0) {
            EXPECT_FALSE(ok);
        } else {
            EXPECT_TRUE(ok);
        }
    }
    
    EXPECT_FALSE(sequence.isSingleFile());
    EXPECT_TRUE(sequence.getFirstFrame() == 0 && sequence.getLastFrame() == 10);
    EXPECT_TRUE((int)sequence.getFrameIndexes().size() == 11);
    EXPECT_TRUE(sequence.generateValidSequencePattern() == "/Users/Test/mysequence###.jpg");
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
            bool ok = sequence.tryInsertFile(FileNameContent("/Users/Test/00mysequence" + number + ".jpg"));
            EXPECT_TRUE(ok);
        }
        EXPECT_TRUE(sequence.generateValidSequencePattern() == "/Users/Test/00mysequence###.jpg");
    }
    {
        FileNameContent file1("/Users/Test/00my000sequence000.jpg");
        SequenceFromFiles sequence(file1,false);
        ///now add valid files
        for (int i = 1; i < 11; ++i) {
            QString number = QString::number(i);
            while (number.size() < 3) {
                number.prepend('0');
            }
            bool ok = sequence.tryInsertFile(FileNameContent("/Users/Test/00my"+ number + "sequence" + number + ".jpg"));
            EXPECT_TRUE(ok);
        }
        EXPECT_TRUE(sequence.generateValidSequencePattern() == "/Users/Test/00my###sequence###.jpg");
    }
    
    {
        FileNameContent file1("/Users/Test/00my0sequence000.jpg");
        SequenceFromFiles sequence(file1,false);
        ///now add valid files
        for (int i = 1; i < 11; ++i) {
            QString number = QString::number(i);
            QString originalNumber = number;
            while (number.size() < 3) {
                number.prepend('0');
            }
            bool ok = sequence.tryInsertFile(FileNameContent("/Users/Test/00my"+ originalNumber + "sequence" + number + ".jpg"));
            EXPECT_TRUE(ok);
        }
        EXPECT_TRUE(sequence.generateValidSequencePattern() == "/Users/Test/00my#sequence###.jpg");
        EXPECT_FALSE(sequence.generateValidSequencePattern() == "/Users/Test/00my##sequence###.jpg");

    }
    
    ///now test only filenames with digits
    {
        FileNameContent file1("/Users/Test/12345.jpg");
        SequenceFromFiles sequence(file1,false);
        EXPECT_TRUE(sequence.tryInsertFile(FileNameContent("/Users/Test/34567.jpg")));
        EXPECT_TRUE(sequence.tryInsertFile(FileNameContent("/Users/Test/04592.jpg")));
        EXPECT_TRUE(sequence.tryInsertFile(FileNameContent("/Users/Test/23489.jpg")));
        EXPECT_TRUE(sequence.tryInsertFile(FileNameContent("/Users/Test/00001.jpg")));
        EXPECT_FALSE(sequence.tryInsertFile(FileNameContent("/Users/Test/0001.jpg")));
        EXPECT_TRUE(sequence.tryInsertFile(FileNameContent("/Users/Test/122938.jpg")));
        EXPECT_FALSE(sequence.tryInsertFile(FileNameContent("/Users/Test/000002.jpg")));
        EXPECT_TRUE(sequence.generateValidSequencePattern() == "/Users/Test/#####.jpg");

    }
}
