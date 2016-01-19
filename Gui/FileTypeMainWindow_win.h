// ————————————————————————————————-
/**
 * @file
 * @brief
 * @author Gerolf Reinwardt
 * @date 30. march 2011
 *
 * Copyright © 2011, Gerolf Reinwardt. All rights reserved.
 *
 * Simplified BSD License
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Gerolf Reinwardt.
 */
// ————————————————————————————————-

#ifndef FILETYPEMAINWINDOW_WIN_H
#define FILETYPEMAINWINDOW_WIN_H

#include "Global/Macros.h"
#ifdef __NATRON_WIN32__

// —— general includes —————————————————————————
#include <windows.h>
#include <QMainWindow>

// —— local includes —————————————————————————-
// —— pre defines ——————————————————————————-

// —— class definition —————————————————————————
/**
 * @short This class implements a main window with the ability to register file types on MS Windows and
 * react on the corresponding DDE events to open / print the files in an Windows MDI typical manner.
 *
 * The usage is fairly easy. Derive your own MainWindow class from DocumentWindow instead of QMainWindow.
 * Inside your constructor, call registerFileType and enableShellOpen. The axample is build on top of the
 * Qt MDI Example (http://doc.qt.nokia.com/4.7/mainwindows-mdi.html)
 *
 * @code
 MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) :
 DocumentWindow(parent, flags)
 {
 …
 
 registerFileType("GiMdi.Document", "MDI Text Editor Document", ".gidoc", 0, true);
 enableShellOpen();
 
 setWindowTitle(tr("MDI"));
 setUnifiedTitleAndToolBarOnMac(true);
 }
 
 *
 * Aditionally, the actions must be performed by overwriting one or more of:
 * @li ddeOpenFile
 * @li ddeNewFile
 * @li ddePrintFile
 * @li executeUnknownDdeCommand
 *
 *
 
 void MainWindow::ddeOpenFile(const QString& filePath)
 {
 MdiChild *child = createMdiChild();
 if (child->loadFile(filePath))
 {
 statusBar()->showMessage(tr("File loaded"), 2000);
 child->show();
 }
 else
 {
 child->close();
 }
 }
 
 *
 */
class DocumentWindow : public QMainWindow
{
    
public:
    // —— enums ———————————————————————————
    /**
     * Known DDE command. These commands are typically used
     */
    enum DdeCommand {
        DDEOpen = 0x0001, /**< open a file via explorer*/
        DDENew = 0x0002, /**< create a new file via explorer*/
        DDEPrint = 0x0004, /**< print a file via explorer*/
    };
    Q_DECLARE_FLAGS(DdeCommands, DdeCommand)
    
    // —— construction —————————————————————————
    /**
     * Constructor.
     *
     * Creates a DocumentWindow with a given @arg parent and @arg flags.
     */
    explicit DocumentWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    
    /**
     * Destructor
     */
    virtual ~DocumentWindow();
    
    // —— operators ——————————————————————————
    // —— methods ——————————————————————————-
    // —— accessors ——————————————————————————
    // —— members ——————————————————————————-
    
protected:
    // —— events ———————————————————————————
    /**
     * reimpl as DDE events come as windows events and are not translated by Qt.
     */
    virtual bool winEvent(MSG *message, long *result);
    
    // —— helpers for the file registration ——————————————————
    /**
     * virtual function that must be implemented by the derived class to react to the open command
     * from Windows.
     *
     * @param filePath file that was selected in the explorer
     */
    virtual void ddeOpenFile(const QString& filePath);
    
    /**
     * virtual function that must be implemented by the derived class to react to the new command
     * from Windows.
     *
     * @param filePath file that was selected in the explorer
     */
    virtual void ddeNewFile(const QString& filePath);
    
    /**
     * virtual function that must be implemented by the derived class to react to the print command
     * from Windows.
     *
     * @param filePath file that was selected in the explorer
     */
    virtual void ddePrintFile(const QString& filePath);
    
    /**
     * virtual function that must be implemented by get custom dde commands from the explorer. If, e.g.
     * a printo or copy command should be available in explorer and they are registered via registerCommand,
     * this function is called.
     *
     * @param command name of the command
     * @param params parameter string, containing the hole stuff, also " and commas.
     *
     * @note a command like this [compare("%1")] will result in the parameters:
     command = "compare";
     params = "quot;<filepath>quot;";
     */
    virtual void executeUnknownDdeCommand(const QString& command, const QString& params);
    
    /**
     * Call this method to register the file type in Windows. It creates the default command
     * which means the app is started with the file to open as parameter. If @arg registerForDDE
     * is true, the dde events are also created so the opening of a file is done in typical MDI
     * behavior (all files open in the same app).
     *
     * @param documentId id of the document, typically <Application>.Document —> see in registry, e.g. "GiMdi.Document"
     * @param fileTypeName free name of the file type, e.g. "MDI Text Editor Document"
     * @param fileExtension File extension, including the dot (e.g. ".gidoc")
     * @param appIconIndex index of the app icon to use for the file in the windows explorer, typically the application icon
     * @param registerForDDE true if DDE should be used (typical for MDI apps), typically false for SDI apps.
     * @param commands a combination of the commands to install.
     *
     * @note If more then the default commands are needed, then subsequent calls of registerCommand are needed.
     *
     * @note DDEOpen leads to the DDE command: [open("%1)]
     DDENew leads to the DDE command: [new("%1)]
     DDEPrint leads to the DDE command: [print("%1)]
     */
    void registerFileType(const QString& documentId,
                          const QString& fileTypeName,
                          const QString& fileExtension,
                          qint32 appIconIndex = 0,
                          bool registerForDDE = false,
                          DdeCommands commands = DDEOpen);
    
    /**
     * registeres one command for a given file type. It is called for the pre defined DDE command
     * types from registerFileType. if more then the normal commands are needed, it can be called
     * in addition to registerFileType.
     *
     * @code
     MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) :
     DocumentWindow(parent, flags)
     {
     …
     
     registerFileType("GiMdi.Document", "MDI Text Editor Document", ".gidoc", 0, true);
     registerCommand("fancyCommand", "GiMdi.Document", "-fancy %1", "[fancy(quot;%1quot;)]");
     enableShellOpen();
     
     …
     }
     @endcode
     */
    void registerCommand(const QString& command,
                         const QString& documentId,
                         const QString cmdLineArg = QString::null,
                         const QString ddeCommand = QString::null);
    
    /**
     * Call this method to enable the user to open data files associated with your application
     * by double-clicking the files in the windows file manager.
     *
     * Use it together with registerFileType to register the fspezified file types or provida a
     * registry file (*.reg) which does this.
     */
    void enableShellOpen();
    
    
private:
    // —— privat helpers ————————————————————————
    /**
     * implementation of the WM_DDE_INITIATE windows message
     */
    bool ddeInitiate(MSG* message, long* result);
    
    /**
     * implementation of the WM_DDE_EXECUTE windows message
     */
    bool ddeExecute(MSG* message, long* result);
    
    /**
     * implementation of the WM_DDE_TERMINATE windows message
     */
    bool ddeTerminate(MSG* message, long* result);
    
    /**
     * Sets specified value in the registry under HKCU\Software\Classes, which is mapped to HKCR then.
     */
    bool SetHkcrUserRegKey(QString key, const QString& value, const QString& valueName = QString::null);
    
    /**
     * this method is called to do the DDE command handling. It does argument splitting and then calls
     * ddeOpenFile, ddeNewFile, ddePrintFile or executeUnknownDdeCommand.
     */
    void executeDdeCommand(const QString& command, const QString& params);
    
    // —— members —————————a
    bool m_registerForDDE; /**< used to identfy, if the dde commands should be written to the registry*/
        QString m_appAtomName; /**< the name of the application, without file extension*/
    QString m_systemTopicAtomName; /**< the name of the system topic atom, typically "System"*/
    ATOM m_appAtom; /**< The windows atom needed for DDE communication*/
    ATOM m_systemTopicAtom; /**< The windows system topic atom needed for DDE communication*/
    // —— not allowed members ——————————————————————-
};

NATRON_NAMESPACE_EXIT;

Q_DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::DocumentWindow::DdeCommands)

#endif // __NATRON_WIN32__
#endif // FILETYPEMAINWINDOW_WIN_H
