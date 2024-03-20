// ————————————————————————————————-
/**
 * @file
 * @brief
 * @author Gerolf Reinwardt
 * @date 30.01.2011
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

// See https://gitlab.com/qtdevnet-wiki-mvc/qtdevnet-registereditorfiletype/tree/master
// and https://wiki.qt.io/Assigning_a_file_type_to_an_Application_on_Windows

#include "FileTypeMainWindow_win.h"

#ifdef __NATRON_WIN32__

#include <stdexcept>

// —— general includes —————————————————————————
#include <windows.h>
#include <QMessageBox>
#include <QApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>

NATRON_NAMESPACE_ENTER

// —— construction ——————————————————————————
DocumentWindow::DocumentWindow(QWidget* parent,
                               Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_registerForDDE(false)
    , m_appAtomName()
    , m_systemTopicAtomName( QString::fromUtf8("system") )
    , m_appAtom(0)
    , m_systemTopicAtom(0)
{
    QFileInfo fi( qApp->applicationFilePath() );

    m_appAtomName = fi.baseName();
}

DocumentWindow::~DocumentWindow()
{
    if (0 != m_appAtom) {
        ::GlobalDeleteAtom(m_appAtom);
        m_appAtom = 0;
    }
    if (0 != m_systemTopicAtom) {
        ::GlobalDeleteAtom(m_systemTopicAtom);
        m_systemTopicAtom = 0;
    }
}

// —— operators ———————————————————————————
// —— methods ————————————————————————————
// —— accessors ———————————————————————————
// —— public slots ——————————————————————————
// —— protected slots —————————————————————————
// —— events ————————————————————————————
bool
DocumentWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    MSG* msg = static_cast<MSG*>(message);
    switch (msg->message) {
    case WM_DDE_INITIATE:

        return ddeInitiate(msg, result);
        break;
    case WM_DDE_EXECUTE:

        return ddeExecute(msg, result);
        break;
    case WM_DDE_TERMINATE:

        return ddeTerminate(msg, result);
        break;
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}

void
DocumentWindow::ddeOpenFile(const QString&)
{
    // this method will be overwritten, if the application uses the dde open command
}

void
DocumentWindow::ddeNewFile(const QString&)
{
    // this method will be overwritten, if the application uses the dde new command
}

void
DocumentWindow::ddePrintFile(const QString&)
{
    // this method will be overwritten, if the application uses the dde print command
}

void
DocumentWindow::executeUnknownDdeCommand(const QString&,
                                         const QString&)
{
    // this method will be overwritten, if the application uses other commands,
    // then open, new and print via DDE and Explorer
}

void
DocumentWindow::registerFileType(const QString& documentId,
                                 const QString& fileTypeName,
                                 const QString& fileExtension,
                                 qint32 appIconIndex,
                                 bool registerForDDE,
                                 DdeCommands commands)
{
    // first register the type ID of our server
    if ( !SetHkcrUserRegKey(documentId, fileTypeName) ) {
        return;
    }

    if ( !SetHkcrUserRegKey( QString::fromUtf8("%1\\DefaultIcon").arg(documentId),
                             QString::fromUtf8("\"%1\",%2").arg( QDir::toNativeSeparators( qApp->applicationFilePath() ) ).arg(appIconIndex) ) ) {
        return;
    }

    m_registerForDDE = registerForDDE;

    if (commands & DDEOpen) {
        registerCommand( QString::fromUtf8("Open"), documentId, QString::fromUtf8(" %1"), QString::fromUtf8("[open(\"%1\")]") );
    }
    if (commands & DDENew) {
        registerCommand( QString::fromUtf8("New"), documentId, QString::fromUtf8("-new %1"), QString::fromUtf8("[new(\"%1\")]") );
    }
    if (commands & DDEPrint) {
        registerCommand( QString::fromUtf8("Print"), documentId, QString::fromUtf8("-print %1"), QString::fromUtf8("[print(\"%1\")]") );
    }

    LONG lSize = _MAX_PATH * 2;
    wchar_t szTempBuffer[_MAX_PATH * 2];
    LONG lResult = ::RegQueryValueW(HKEY_CLASSES_ROOT,
                                    (const wchar_t*)fileExtension.utf16(),
                                    szTempBuffer,
                                    &lSize);
    QString temp = QString::fromUtf16( (unsigned short*)szTempBuffer );

    if ( (lResult != ERROR_SUCCESS) || temp.isEmpty() || (temp == documentId) ) {
        // no association for that suffix
        if ( !SetHkcrUserRegKey(fileExtension, documentId) ) {
            return;
        }

        SetHkcrUserRegKey( QString::fromUtf8("%1\\ShellNew").arg(fileExtension), QLatin1String(""), QLatin1String("NullFile") );
    }
}

void
DocumentWindow::registerCommand(const QString& command,
                                const QString& documentId,
                                const QString cmdLineArg,
                                const QString ddeCommand)
{
    QString commandLine = QDir::toNativeSeparators( qApp->applicationFilePath() );

    commandLine.prepend( QLatin1String("\"") );
    commandLine.append( QLatin1String("\"") );

    if ( !cmdLineArg.isEmpty() ) {
        commandLine.append( QLatin1Char(' ') );
        commandLine.append(cmdLineArg);
    }

    if ( !SetHkcrUserRegKey(QString::fromUtf8("%1\\shell\\%2\\command").arg(documentId).arg(command), commandLine) ) {
        return;       // just skip it
    }
    if (m_registerForDDE) {
        if ( !SetHkcrUserRegKey(QString::fromUtf8("%1\\shell\\%2\\ddeexec").arg(documentId).arg(command), ddeCommand) ) {
            return;
        }

        if ( !SetHkcrUserRegKey(QString::fromUtf8("%1\\shell\\%2\\ddeexec\\application").arg(documentId).arg(command), m_appAtomName) ) {
            return;
        }

        if ( !SetHkcrUserRegKey(QString::fromUtf8("%1\\shell\\%2\\ddeexec\\topic").arg(documentId).arg(command), m_systemTopicAtomName) ) {
            return;
        }
    }
}

void
DocumentWindow::enableShellOpen()
{
    if ( (0 != m_appAtom) || (0 != m_systemTopicAtom) ) {
        return;
    }

    m_appAtom = ::GlobalAddAtomW( (const wchar_t*)m_appAtomName.utf16() );
    m_systemTopicAtom = ::GlobalAddAtomW( (const wchar_t*)m_systemTopicAtomName.utf16() );
}

// —— private slots ——————————————————————————
// —— private helpers —————————————————————————
bool
DocumentWindow::ddeInitiate(MSG* message,
                            long* result)
{
    if ( ( 0 != LOWORD(message->lParam) ) &&
         ( 0 != HIWORD(message->lParam) ) &&
         (LOWORD(message->lParam) == m_appAtom) &&
         (HIWORD(message->lParam) == m_systemTopicAtom) ) {
        // make duplicates of the incoming atoms (really adding a reference)
#     ifndef QT_NO_DEBUG
        wchar_t atomName[_MAX_PATH];
        Q_ASSERT(::GlobalGetAtomNameW(m_appAtom, atomName, _MAX_PATH - 1) != 0);
        Q_ASSERT(::GlobalAddAtomW(atomName) == m_appAtom);
        Q_ASSERT(::GlobalGetAtomNameW(m_systemTopicAtom, atomName, _MAX_PATH - 1) != 0);
        Q_ASSERT(::GlobalAddAtomW(atomName) == m_systemTopicAtom);
#     endif

        // send the WM_DDE_ACK (caller will delete duplicate atoms)
        ::SendMessage( (HWND)message->wParam, WM_DDE_ACK, (WPARAM)winId(), MAKELPARAM(m_appAtom, m_systemTopicAtom) );
    }
    *result = 0;

    return true;
}

bool
DocumentWindow::ddeExecute(MSG* message,
                           long* result)
{
    // unpack the DDE message
    UINT_PTR unused = 0;
    HGLOBAL hData = 0;

    //IA64: Assume DDE LPARAMs are still 32-bit
    Q_ASSERT( ::UnpackDDElParam(WM_DDE_EXECUTE, message->lParam, &unused, (UINT_PTR*)&hData) );
    Q_UNUSED(unused);

    QString command = QString::fromWCharArray( (LPCWSTR) ::GlobalLock(hData) );
    ::GlobalUnlock(hData);

    // acknowledge now - before attempting to execute
    ::PostMessage( (HWND)message->wParam, WM_DDE_ACK, (WPARAM)winId(),
                   //IA64: Assume DDE LPARAMs are still 32-bit
                   ReuseDDElParam(message->lParam, WM_DDE_EXECUTE, WM_DDE_ACK, (UINT)0x8000, (UINT_PTR)hData) );

    // don't execute the command when the window is disabled
    if ( !isEnabled() ) {
        *result = 0;

        return true;
    }

    QRegExp regCommand( QString::fromUtf8("^\\[(\\w+)\\((.*)\\)\\]$") );
    if ( regCommand.exactMatch(command) ) {
        executeDdeCommand( regCommand.cap(1), regCommand.cap(2) );
    }

    *result = 0;

    return true;
}

bool
DocumentWindow::ddeTerminate(MSG* message,
                             long* result)
{
    Q_UNUSED(result);
    // The client or server application should respond by posting a WM_DDE_TERMINATE message.
    ::PostMessageW( (HWND)message->wParam, WM_DDE_TERMINATE, (WPARAM)winId(), message->lParam );

    return true;
}

bool
DocumentWindow::SetHkcrUserRegKey(QString key,
                                  const QString& value,
                                  const QString& valueName)
{
    HKEY hKey;

    key.prepend( QString::fromUtf8("Software\\Classes\\") );

    LONG lRetVal = RegCreateKeyW(HKEY_CURRENT_USER,
                                 (const wchar_t*)key.utf16(),
                                 &hKey);

    if (ERROR_SUCCESS == lRetVal) {
        LONG lResult = ::RegSetValueExW( hKey,
                                         valueName.isEmpty() ? 0 : (const wchar_t*)valueName.utf16(),
                                         0,
                                         REG_SZ,
                                         (CONST BYTE*)value.utf16(),
                                         (value.length() + 1) * sizeof(wchar_t) );

        if ( (::RegCloseKey(hKey) == ERROR_SUCCESS) && (lResult == ERROR_SUCCESS) ) {
            return true;
        }

        QMessageBox::warning( 0, QString::fromUtf8("Error in setting Registry values"),
                              QString::fromUtf8("registration database update failed for key '%s'.").arg(key) );
    } else {
        wchar_t buffer[4096];
        ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, lRetVal, 0, buffer, 4096, 0);
        QString szText = QString::fromUtf16( (const ushort*)buffer );
        QMessageBox::warning(this, QString::fromUtf8("Error in setting Registry values"), szText);
    }

    return false;
}

void
DocumentWindow::executeDdeCommand(const QString& command,
                                  const QString& params)
{
    QRegExp regCommand( QString::fromUtf8("^\"(.*)\"$") );
    bool singleCommand = regCommand.exactMatch(params);

    if ( ( 0 == command.compare(QString::fromUtf8("open"), Qt::CaseInsensitive) ) && singleCommand ) {
        ddeOpenFile( regCommand.cap(1) );
    } else if ( ( 0 == command.compare(QString::fromUtf8("new"), Qt::CaseInsensitive) ) && singleCommand ) {
        ddeNewFile( regCommand.cap(1) );
    } else if ( ( 0 == command.compare(QString::fromUtf8("print"), Qt::CaseInsensitive) ) && singleCommand ) {
        ddePrintFile( regCommand.cap(1) );
    } else {
        executeUnknownDdeCommand(command, params);
    }
}

NATRON_NAMESPACE_EXIT

#endif //__NATRON_WIN32__
