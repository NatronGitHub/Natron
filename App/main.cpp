//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include <clocale>
#include <iostream>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QApplication>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QMetaType>
#include <QAbstractSocket>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"
#include "Engine/AppManager.h"

#include "Engine/Knob.h"
#include "Engine/ChannelSet.h"
#include "Engine/Log.h"
#include "Engine/Settings.h"
#include "Engine/Node.h"
#include "Engine/Format.h"

#include "Gui/CurveWidget.h"
#include "Gui/GuiApplicationManager.h"

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QAbstractSocket::SocketState)
#endif

void registerMetaTypes(){
    qRegisterMetaType<Variant>();
    qRegisterMetaType<Natron::ChannelSet>();
    qRegisterMetaType<Format>();
    qRegisterMetaType<SequenceTime>("SequenceTime");
    qRegisterMetaType<Knob*>();
    qRegisterMetaType<Natron::Node*>();
    qRegisterMetaType<CurveWidget*>();
    qRegisterMetaType<Natron::StandardButtons>();
#if QT_VERSION < 0x050000
    qRegisterMetaType<QAbstractSocket::SocketState>("SocketState");
#endif
}



void printUsage(){
    std::cout << NATRON_APPLICATION_NAME << " usage: " << std::endl;
    std::cout << "./" NATRON_APPLICATION_NAME "    <project file path>" << std::endl;
    std::cout << "[--background] or [-b] enables background mode rendering. No graphical interface will be shown." << std::endl;
    std::cout << "[--writer <Writer node name>] When in background mode, the renderer will only try to render with the node"
    " name following the --writer argument. If no such node exists in the project file, the process will abort."
    "Note that if you don't pass the --writer argument, it will try to start rendering with all the writers in the project's file."<< std::endl;

}


/**
 * @brief Extracts from argv[0] the path of the application's binary
 **/
static QString extractBinaryPath(const QString arg0) {
    int lastSepPos = arg0.lastIndexOf(QDir::separator());
    if (lastSepPos == -1) {
        return "";
    } else {
        return arg0.left(lastSepPos);
    }
}

int main(int argc, char *argv[])
{	

    /*Parse args to find a valid project file name. This is not fool-proof:
     any file with a matching extension will pass through...
     @TODO : use some magic number to identify uniquely the project file type.*/
    QString projectFile;
    bool isBackGround = false;
    QStringList writers;
    bool expectWriterNameOnNextArg = false;
    bool expectPipeFileNameOnNextArg = false;
    QString mainProcessServerName;
    QStringList args;

    QString binaryPath;
    for(int i = 0; i < argc ;++i){
        if (i == 0) {
            extractBinaryPath(argv[i]);
        }
        args.push_back(QString(argv[i]));
    }
    for (int i = 0 ; i < args.size(); ++i) {
        if(args.at(i).contains("." NATRON_PROJECT_FILE_EXT)){
            if(expectWriterNameOnNextArg || expectPipeFileNameOnNextArg) {
                printUsage();
                return 1;
            }
            projectFile = args.at(i);
            continue;
        }else if(args.at(i) == "--background" || args.at(i) == "-b"){
            if(expectWriterNameOnNextArg || expectPipeFileNameOnNextArg){
                printUsage();
                return 1;
            }
            isBackGround = true;
            continue;
        }else if(args.at(i) == "--writer"){
            if(expectWriterNameOnNextArg  || expectPipeFileNameOnNextArg){
                printUsage();
                return 1;
            }
            expectWriterNameOnNextArg = true;
            continue;
        }else if(args.at(i) == "--IPCpipe"){
            if (expectWriterNameOnNextArg || expectPipeFileNameOnNextArg) {
                printUsage();
                return 1;
            }
            expectPipeFileNameOnNextArg = true;
            continue;
        }
        
        if(expectWriterNameOnNextArg){
            assert(!expectPipeFileNameOnNextArg);
            writers << args.at(i);
            expectWriterNameOnNextArg = false;
        }
        if (expectPipeFileNameOnNextArg) {
            assert(!expectWriterNameOnNextArg);
            mainProcessServerName = args.at(i);
            expectPipeFileNameOnNextArg = false;
        }
    }

    QCoreApplication* app = NULL;
    if(!isBackGround){
        QApplication* guiApp = new QApplication(argc, argv);
        Q_INIT_RESOURCE(GuiResources);
        guiApp->setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_12));
        app = guiApp;
    }else{
        app = new QCoreApplication(argc,argv);
        
    }
    // Natron is not yet internationalized, so it is better for now to use the "C" locale,
    // until it is tested for robustness against locale choice.
    // The locale affects numerics printing and scanning, date and time.
    // Note that with other locales (e.g. "de" or "fr"), the floating-point numbers may have
    // a comma (",") as the decimal separator instead of a point (".").
    // There is also an OpenCOlorIO issue with non-C numeric locales:
    // https://github.com/imageworks/OpenColorIO/issues/297
    //
    // this must be done after initializing the QCoreApplication, see
    // https://qt-project.org/doc/qt-5/qcoreapplication.html#locale-settings
    //std::setlocale(LC_NUMERIC,"C"); // set the locale for LC_NUMERIC only
    std::setlocale(LC_ALL,"C"); // set the locale for everything


    app->setOrganizationName(NATRON_ORGANIZATION_NAME);
    app->setOrganizationDomain(NATRON_ORGANIZATION_DOMAIN);
    app->setApplicationName(NATRON_APPLICATION_NAME);

    registerMetaTypes();
    
    Natron::Log::instance();//< enable logging
    
    
    //the AppManager singleton
    AppManager* manager;
    if (isBackGround) {
        manager = new AppManager;
    } else {
        manager = new GuiApplicationManager;
    }
    
    
    //load and create data structures
    bool loaded = manager->load(binaryPath,mainProcessServerName,projectFile,writers);
    if (isBackGround) {
        int rVal = 0;
        delete manager;
        if (!loaded) {
            printUsage();
            rVal = 1;
        }
        return rVal;
    }

    int retCode =  app->exec();
    delete app;
    return retCode;

}

