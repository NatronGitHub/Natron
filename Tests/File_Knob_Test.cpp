//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>

#include <QString>
#include <QDir>

#include "Engine/KnobFile.h"
#include "Engine/StandardPaths.h"


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
    File_Knob::FileSequence sequence;
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_#.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);
    
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_##.unittest"), &sequence);
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_##.unittest"), &sequence);
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("#test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 11);
    
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("##test_##.unittest"), &sequence);
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("#test_##.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);
    ///delete files
    for (int i = 0; i < sequenceItemsCount; ++i) {
        QString number = QString::number(i);
        while (number.size() < 2) {
            number.prepend('0');
        }
        
        dir.remove(QString::number(i+1) + "test_" + number + ".unittest");
    }
    
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
    File_Knob::FileSequence sequence;
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_%02d.unittest"), &sequence);
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);

    ///testing wrong pattern
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("%02dtest_%04d.unittest"), &sequence);
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("%02dtest_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    
    ///try mixing printf-like syntax with hashes characters (###)
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("##test_%04d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("#test_%04d.unittest"), &sequence);
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
            QFile file(dir.absoluteFilePath("test_" + QString::number(i) + viewName + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    
    File_Knob::FileSequence sequence;
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_%01d%V.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (File_Knob::FileSequence::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
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
            QFile file(dir.absoluteFilePath("weird" + viewName + "test_" + QString::number(i)  + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("weird%Vtest_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (File_Knob::FileSequence::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
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
            QFile file(dir.absoluteFilePath("test_" + QString::number(i) + viewName + ".unittest"));
            std::cout << "Creating file: " << file.fileName().toStdString() << std::endl;
            filesCreated << file.fileName();
            file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
    }
    sequence.clear();
    File_Knob::filesListFromPattern(dir.absoluteFilePath("test_%01d%v.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (File_Knob::FileSequence::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        EXPECT_TRUE(it->second.size() == 2);
    }
    
    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
    }
    filesCreated.clear();
    
    
    ///test with short view name placed wrongly
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("weird%vtest_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == 0);

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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("weird.%v.test_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (File_Knob::FileSequence::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
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
    File_Knob::filesListFromPattern(dir.absoluteFilePath("weird.%v.test_%01d.unittest"), &sequence);
    EXPECT_TRUE((int)sequence.size() == sequenceItemsCount);
    for (File_Knob::FileSequence::iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        EXPECT_TRUE(it->second.size() == 5);
    }
    
    for (int i = 0; i < filesCreated.size(); ++i) {
        QFile::remove(filesCreated.at(i));
    }
    filesCreated.clear();
}

TEST(SequenceParsing,OutputSequence) {
    QString pattern = "weird.%v.test_%01d.unittest";
    QString filename = OutputFile_Knob::generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_TRUE(QString("weird.l.test_120.unittest") == filename);
    
    pattern = "####.jpg";
    filename = OutputFile_Knob::generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_TRUE(QString("0120.jpg") == filename);
    
    pattern = "###lalala%04d.jpg";
    filename = OutputFile_Knob::generateFileNameFromPattern(pattern, 120, 0);
    EXPECT_TRUE(QString("120lalala0120.jpg") == filename);
    
    pattern = "####%V.jpg";
    filename = OutputFile_Knob::generateFileNameFromPattern(pattern, 120, 5);
    EXPECT_TRUE(QString("0120.view5.jpg") == filename);
    
    
}
