#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#Created by Alexandre GAUTHIER-FOICHAT on 01/27/2015.


from PySide.QtGui import *
from PySide.QtCore import *

#A small panel to load and visualize icons/images
class IconViewer(NatronGui.PyPanel):

    #Register a custom signal
    userFileChanged = QtCore.Signal()

    #Slots should be decorated:
    #http://qt-project.org/wiki/Signals_and_Slots_in_PySide
    
    #This is called upon a user click on the button
    @QtCore.Slot()
    def onButtonClicked(self):
        location = self.currentApp.getFilenameDialog(("jpg","png","bmp","tif"))
        if location:
            self.locationEdit.setText(location)
        self.userFileChanged.emit()
    
    #This is called when the user finish editing of the line edit (when return is pressed or focus out)
    @QtCore.Slot()
    def onLocationEditEditingFinished(self):
        self.userFileChanged.emit()
    
    #This is called when our custom userFileChanged signal is emitted
    @QtCore.Slot()
    def onFileChanged(self):
        self.label.setPixmap(QPixmap(self.locationEdit.text()))
    
    
    def __init__(self,label,app):
        
        #Init base class, important! otherwise signals/slots won't work.
        NatronGui.PyPanel.__init__(self, label, False, app)
        
        #Store the current app as it might no longer be pointing to the app at the time being called
        #when a slot will be invoked later on
        self.currentApp = app
        
        #Set the layout
        self.setLayout( QVBoxLayout())
        
        #Create a widget container for the line edit + button
        fileContainer = QWidget(self)
        fileLayout = QHBoxLayout()
        fileContainer.setLayout(fileLayout)
        
        #Create the line edit, make it expand horizontally
        self.locationEdit = QLineEdit(fileContainer)
        self.locationEdit.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        
        #Create a pushbutton
        self.button = QPushButton(fileContainer)
        #Decorate it with the open-file pixmap built-in into Natron
        buttonPixmap = natron.getIcon(NatronEngine.Natron.PixmapEnum.NATRON_PIXMAP_OPEN_FILE)
        self.button.setIcon(QIcon(buttonPixmap))
        
        #Add widgets to the layout
        fileLayout.addWidget(self.locationEdit)
        fileLayout.addWidget(self.button)
        
        #Use a QLabel to display the images
        self.label = QLabel(self)
        
        #Init the label with the icon of Natron
        natronPixmap = natron.getIcon(NatronEngine.Natron.PixmapEnum.NATRON_PIXMAP_APP_ICON)
        self.label.setPixmap(natronPixmap)
        #Built-in icons of Natron are in the resources
        self.locationEdit.setText(":/Resources/Images/natronIcon256_linux.png")
        
        #Make it expand in both directions so it takes all space
        self.label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        #Add widgets to the layout
        self.layout().addWidget(fileContainer)
        self.layout().addWidget(self.label)
        
        #Make signal/slot connections
        self.button.clicked.connect(self.onButtonClicked)
        self.locationEdit.editingFinished.connect(self.onLocationEditEditingFinished)
        self.userFileChanged.connect(self.onFileChanged)


#To be called to create a new icon viewer panel:
#Note that *app* should be defined. Generally when called from onProjectCreatedCallback
#this is set, but when called from the Script Editor you should set it yourself beforehand:
#app = app1
#See http://natron.readthedocs.org/en/python/natronobjects.html for more info
def createIconViewer():
    
    if hasattr(app,"p"):
        #The icon viewer already exists, it we override the app.p variable, then it will destroy the previous widget
        #and create a new one but we don't really need it
        
        #The warning will be displayed in the Script Editor
        print("Note for us developers: this widget already exists!")
        return
    
    #Create our icon viewer
    app.p = IconViewer("Icon viewer",app)
    
    #Add it to the "Pane2" tab widget
    pane1 = app.getTabWidget("Pane2");
    pane1.appendTab(app.p);
    
    #Register the tab to the application, so it is saved into the layout of the project
    #and can appear in the Panes sub-menu of the "Manage layout" button (in top left-hand corner of each tab widget)
    app.registerPythonPanel(app.p,"createIconViewer")

def onProjectCreatedCallback():
    #Always create our icon viewer on project creation, you must register this call-back in the
    #"After project created callback" parameter of the Preferences-->Python tab.
    createIconViewer();

#Add a custom menu entry with a shortcut to create our icon viewer
natron.addMenuCommand("Inria/Scripts/IconViewer","createIconViewer",QtCore.Qt.Key.Key_L,QtCore.Qt.KeyboardModifier.ShiftModifier)
