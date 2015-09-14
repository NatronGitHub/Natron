/**************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Installer Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
**************************************************************************/

function Component()
{
    // constructor
}

Component.prototype.isDefault = function()
{
    // select the component by default
    return true;
}

Component.prototype.beginInstallation = function()
{
    installer.setValue("RunProgram", "@TargetDir@\\bin\\Natron.exe");
}

Component.prototype.createOperations = function()
{
    try {
        // call default implementation to actually install the registeredfile
        component.createOperations();
	component.addElevatedOperation("CreateShortcut", "@TargetDir@/bin/Natron.exe", "@StartMenuDir@/Natron.lnk",
            "workingDirectory=@TargetDir@");


    component.addElevatedOperation("CreateShortcut", "@TargetDir@/bin/Natron.exe", "@DesktopDir@/Natron.lnk",
            "workingDirectory=@TargetDir@");

    var natron =  installer.value("TargetDir") + "\\bin\\Natron.exe";
    natron = natron.replace(/\//g, "\\");
    component.addElevatedOperation("RegisterFileType",
                            "ntp",
                            natron + " \"%1\"",
                            "Natron Project file",
                            "application/vnd.natron.project",
                            "@TargetDir@/share/pixmaps/natronProjectIcon_windows.ico",
			    "ProgId=Inria.Natron.ntp");

    component.addElevatedOperation("RegisterFileType",
                            "nl",
                            "",
                            "Natron Layout file",
                            "application/vnd.natron.layout",
                            "@TargetDir@/share/pixmaps/natronProjectIcon_windows.ico",
	    		    "ProgId=Inria.Natron.nl");

    component.addElevatedOperation("RegisterFileType",
                            "nps",
                            "",
                            "Natron Node Presets file",
                            "application/vnd.natron.nodepresets",
                            "@TargetDir@/share/pixmaps/natronProjectIcon_windows.ico",
			    "ProgId=Inria.Natron.nps");

    } catch (e) {
        print(e);
    }
}
