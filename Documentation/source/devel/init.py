#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#Created by Alexandre GAUTHIER-FOICHAT on 01/27/2015.


#To import the variable "natron"
import NatronEngine


def addFormats(app):

    app.addFormat ("720p 1280x720 1.0")
    app.addFormat ("2k_185 2048x1108 1.0")


def afterNodeCreatedCallback(thisNode, app, userEdited):
    
    #Turn-off the Clamp black for new grade nodes
    if thisNode.getPluginID() == "net.sf.openfx.GradePlugin":
        thisNode.clampBlack.setDefaultValue(False)
    
    #Set the blur size to (3,3) upon creation
    elif thisNode.getPluginID() == "net.sf.cimg.CImgBlur":
        thisNode.size.setDefaultValue(3,0)
        thisNode.size.setDefaultValue(3,1)


#This will set the After Node Created callback on the project to tweek default values for parameters
def setNodeDefaults(app):
    app.afterNodeCreated.set("afterNodeCreatedCallback")

    
def setProjectDefaults(app):
    app.getProjectParam('autoPreviews').setValue(False)
    app.getProjectParam('outputFormat').setValue("2k_185")
    app.getProjectParam('frameRate').setValue(24)
    app.getProjectParam('frameRange').setValue(1, 0)
    app.getProjectParam('frameRange').setValue(30, 1)
    app.getProjectParam('lockRange').setValue(True)


def myCallback(app):
    addFormats(app)
    setNodeDefaults(app)
    setProjectDefaults(app)



#Set the After Project Created/Loaded callbacks
NatronEngine.natron.setOnProjectCreatedCallback("init.myCallback")
NatronEngine.natron.setOnProjectLoadedCallback("init.myCallback")

#Add this path to the Natron search paths so that our PyPlug can be found.
#Note that we could also set this from the NATRON_PLUGIN_PATH environment variable
#or even in the Preferences panel, Plug-ins tab, with the "Pyplugs search path"
NatronEngine.natron.appendToNatronPath("/Library/Natron/PyPlugs")

