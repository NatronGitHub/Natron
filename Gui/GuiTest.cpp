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

#include "GuiTest.h"

#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QTextEdit>

#include "Global/Enums.h"

#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#define NATRON_GUI_TEST_PYTHON_MODULE_NAME "NatronGuiTest"

NATRON_NAMESPACE_ENTER

namespace {
struct DialogRecord
{
    std::string type;
    std::string title;
    std::string text;
    std::string answer;
};

struct GuiTestState
{
    std::deque<StandardButtonEnum> answers;
    std::vector<DialogRecord> dialogs;
    int unexpectedDialogs; // dialogs that the test did not answer
    Qt::MouseButtons pressedButtons;
    QPointer<QWidget> hovered; // the widget of the last synthetic pen event (for Enter/Leave)

    GuiTestState()
        : answers()
        , dialogs()
        , unexpectedDialogs(0)
        , pressedButtons(Qt::NoButton)
        , hovered()
    {
    }
};

GuiTestState&
state()
{
    static GuiTestState s;

    return s;
}

struct ButtonName
{
    StandardButtonEnum button;
    const char* name;
};

const ButtonName buttonNames[] = {
    { eStandardButtonOk, "ok" },
    { eStandardButtonSave, "save" },
    { eStandardButtonSaveAll, "saveall" },
    { eStandardButtonOpen, "open" },
    { eStandardButtonYes, "yes" },
    { eStandardButtonYesToAll, "yestoall" },
    { eStandardButtonNo, "no" },
    { eStandardButtonNoToAll, "notoall" },
    { eStandardButtonAbort, "abort" },
    { eStandardButtonRetry, "retry" },
    { eStandardButtonIgnore, "ignore" },
    { eStandardButtonClose, "close" },
    { eStandardButtonCancel, "cancel" },
    { eStandardButtonDiscard, "discard" },
    { eStandardButtonHelp, "help" },
    { eStandardButtonApply, "apply" },
    { eStandardButtonReset, "reset" },
    { eStandardButtonRestoreDefaults, "restoredefaults" },
    { eStandardButtonEscape, "escape" },
    { eStandardButtonNoButton, "none" },
};

std::string
buttonName(StandardButtonEnum b)
{
    for (const ButtonName& n : buttonNames) {
        if (n.button == b) {
            return n.name;
        }
    }

    return "unknown";
}

bool
buttonFromName(const QString& name,
               StandardButtonEnum* b)
{
    const QString lower = name.trimmed().toLower();
    for (const ButtonName& n : buttonNames) {
        if ( lower == QString::fromUtf8(n.name) ) {
            *b = n.button;

            return true;
        }
    }

    return false;
}

void
recordDialog(const char* type,
             const QString& title,
             const QString& text,
             const std::string& answer)
{
    DialogRecord r;
    r.type = type;
    r.title = title.toStdString();
    r.text = text.toStdString();
    r.answer = answer;
    state().dialogs.push_back(r);
    if ( !answer.empty() ) {
        std::cout << "NatronGuiTest: " << type << " \"" << r.title << "\" answered " << answer << std::endl;
    }
}

// The text of a dialog, for the log: the text of a QMessageBox, else the texts of its labels
QString
dialogText(QWidget* dialog)
{
    QMessageBox* box = qobject_cast<QMessageBox*>(dialog);

    if (box) {
        return box->text();
    }
    QStringList texts;
    QList<QLabel*> labels = dialog->findChildren<QLabel*>();
    for (QLabel* l : labels) {
        if ( !l->text().isEmpty() ) {
            texts << l->text();
        }
    }
    QList<QTextEdit*> edits = dialog->findChildren<QTextEdit*>();
    for (QTextEdit* e : edits) {
        texts << e->toPlainText();
    }

    return texts.join( QLatin1Char('\n') );
}

// A modal dialog is shown (Natron's error, warning, information and question dialogs, or any other).
// The dialogs are shown as usual, so that the tests run the real dialog code: the next scripted
// answer (setDialogAnswers()) clicks its button. A dialog that the test does not answer is
// unexpected: it is rejected, and the test fails.
void
handleModalDialog(QWidget* dialog)
{
    if ( !dialog || !dialog->isVisible() ) {
        return;
    }
    const QString title = dialog->windowTitle();
    const QString text = dialogText(dialog);
    std::deque<StandardButtonEnum>& answers = state().answers;
    if ( !answers.empty() ) {
        const StandardButtonEnum answer = answers.front();
        answers.pop_front();
        QDialogButtonBox* buttonBox = dialog->findChild<QDialogButtonBox*>();
        QPushButton* button = buttonBox ? buttonBox->button( (QDialogButtonBox::StandardButton)QtEnumConvert::toQtStandardButton(answer) ) : 0;
        if (button) {
            recordDialog( "dialog", title, text, buttonName(answer) );
            button->click();

            return;
        }
        std::cout << "NatronGuiTest: FAIL: the dialog \"" << title.toStdString() << "\" has no button "
                  << buttonName(answer) << std::endl;
    } else {
        std::cout << "NatronGuiTest: FAIL: unexpected dialog \"" << title.toStdString() << "\": "
                  << text.toStdString().substr(0, 300) << std::endl;
    }
    // rejecting is the safe answer, and the test must not block until its timeout
    ++state().unexpectedDialogs;
    recordDialog( "unexpected", title, text, "reject" );
    QDialog* d = qobject_cast<QDialog*>(dialog);
    if (d) {
        d->reject();
    } else {
        dialog->close();
    }
}

// Calls handleModalDialog() when a modal window is shown, once its event loop runs.
class ModalDialogWatcher
    : public QObject
{
public:
    ModalDialogWatcher(QObject* parent)
        : QObject(parent)
    {
    }

    virtual bool eventFilter(QObject* watched,
                             QEvent* event) OVERRIDE FINAL
    {
        if ( event->type() == QEvent::Show ) {
            QWidget* w = qobject_cast<QWidget*>(watched);
            if ( w && w->isWindow() && (w->windowModality() != Qt::NonModal) ) {
                QPointer<QWidget> dialog(w);
                QTimer::singleShot(0, this, [dialog]() {
                    handleModalDialog(dialog.data());
                });
            }
        }

        return QObject::eventFilter(watched, event);
    }
};

Gui*
getGui()
{
    GuiAppInstancePtr app = std::dynamic_pointer_cast<GuiAppInstance>( appPTR->getTopLevelInstance() );

    return app ? app->getGui() : 0;
}

// Sets a Python exception and returns 0 if there is no viewer with that node name.
ViewerGL*
findViewer(const char* name)
{
    Gui* gui = getGui();

    if (gui) {
        const std::list<ViewerTab*>& viewers = gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            ViewerInstance* viewer = (*it)->getInternalNode();
            if ( viewer && (viewer->getNode()->getScriptName_mt_safe() == name) ) {
                return (*it)->getViewer();
            }
        }
    }
    PyErr_Format(PyExc_ValueError, "no viewer named %s", name);

    return 0;
}

// The target of a key event: a viewer (by node name) or the node graph of the project ("NodeGraph").
QWidget*
findKeyTarget(const char* name)
{
    if ( std::string(name) == "NodeGraph" ) {
        Gui* gui = getGui();
        if ( gui && gui->getNodeGraph() ) {
            return gui->getNodeGraph();
        }
        PyErr_SetString(PyExc_ValueError, "no node graph");

        return 0;
    }

    return findViewer(name);
}

bool
parseModifiers(const char* str,
               Qt::KeyboardModifiers* mods)
{
    *mods = Qt::NoModifier;
    const QStringList parts = QString::fromUtf8(str).split( QLatin1Char('+') );
    for (const QString& p : parts) {
        const QString m = p.trimmed().toLower();
        if ( m.isEmpty() ) {
            continue;
        } else if ( m == QString::fromUtf8("shift") ) {
            *mods |= Qt::ShiftModifier;
        } else if ( ( m == QString::fromUtf8("ctrl") ) || ( m == QString::fromUtf8("control") ) ) {
            *mods |= Qt::ControlModifier;
        } else if ( m == QString::fromUtf8("alt") ) {
            *mods |= Qt::AltModifier;
        } else if ( m == QString::fromUtf8("meta") ) {
            *mods |= Qt::MetaModifier;
        } else {
            PyErr_Format(PyExc_ValueError, "unknown modifier %s", m.toStdString().c_str());

            return false;
        }
    }

    return true;
}

bool
parseButton(const char* str,
            Qt::MouseButton* button)
{
    const std::string b(str);

    if (b == "left") {
        *button = Qt::LeftButton;
    } else if (b == "middle") {
        *button = Qt::MiddleButton;
    } else if (b == "right") {
        *button = Qt::RightButton;
    } else if (b == "none") {
        *button = Qt::NoButton;
    } else {
        PyErr_Format(PyExc_ValueError, "unknown button %s (left, middle, right or none)", str);

        return false;
    }

    return true;
}

// Processes the events until no render runs for settleMs, or timeoutMs elapsed. Returns true when idle.
bool
waitIdleInternal(int timeoutMs,
                 int settleMs)
{
    QElapsedTimer total;

    total.start();
    QElapsedTimer idle;
    bool idleStarted = false;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance( qApp->thread() );
    for (;;) {
        QCoreApplication::sendPostedEvents();
        bool processed = dispatcher ? dispatcher->processEvents(QEventLoop::AllEvents) : false;
        // A render may be posting its results (e.g. a viewer texture upload): the posted events are
        // processed in the next iterations, while no render is running.
        if ( appPTR->hasRenderInProgress() ) {
            idleStarted = false;
        } else if (!idleStarted) {
            idle.start();
            idleStarted = true;
        } else if (idle.elapsed() >= settleMs) {
            return true;
        }
        if (total.elapsed() >= timeoutMs) {
            return false;
        }
        if (!processed) {
            QThread::msleep(5);
        }
    }
}

// ----------------------------------------------------------------- Python functions

PyObject*
py_waitIdle(PyObject* /*self*/,
            PyObject* args,
            PyObject* kwds)
{
    static const char* kwlist[] = { "timeout", "settle", 0 };
    double timeout = 30.;
    double settle = 0.2;

    if ( !PyArg_ParseTupleAndKeywords(args, kwds, "|dd", const_cast<char**>(kwlist), &timeout, &settle) ) {
        return 0;
    }
    bool ok;
    // The renders may need the GIL (expressions, Python callbacks): release it while waiting.
    Py_BEGIN_ALLOW_THREADS
    ok = waitIdleInternal( (int)(timeout * 1000.), (int)(settle * 1000.) );
    Py_END_ALLOW_THREADS

    return PyBool_FromLong(ok);
}

PyObject*
py_isRendering(PyObject* /*self*/,
               PyObject* /*args*/)
{
    return PyBool_FromLong( appPTR->hasRenderInProgress() );
}

// The widget that receives the mouse events of a pen event target, and the position in that widget of
// the target coordinates: canvas coordinates for a viewer, scene coordinates for "NodeGraph".
QWidget*
penEventWidget(const char* targetName,
               double x,
               double y,
               QPointF* pos)
{
    if ( std::string(targetName) == "NodeGraph" ) {
        NodeGraph* graph = static_cast<NodeGraph*>( findKeyTarget(targetName) );
        if (!graph) {
            return 0;
        }
        // a QGraphicsView receives the mouse events in its viewport, in whose coordinates mapFromScene() is
        *pos = QPointF( graph->mapFromScene( QPointF(x, y) ) );

        return graph->viewport();
    }
    ViewerGL* gl = findViewer(targetName);
    if (!gl) {
        return 0;
    }
    // canvas (OpenGL canonical) coordinates to widget coordinates
    *pos = gl->toWidgetCoordinates( QPointF(x, y) );

    return gl;
}

// Enter and Leave events, as the real mouse would send when moving from a widget to another
void
updateHovered(QWidget* w,
              const QPointF& pos)
{
    QPointer<QWidget>& hovered = state().hovered;

    if (hovered == w) {
        return;
    }
    if (hovered) {
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(hovered.data(), &leave);
    }
    hovered = w;
    const QPointF global( w->mapToGlobal( pos.toPoint() ) );
    QEnterEvent enter( pos, w->window()->mapFromGlobal( global.toPoint() ), global );
    QCoreApplication::sendEvent(w, &enter);
}

PyObject*
py_penEvent(PyObject* /*self*/,
            PyObject* args,
            PyObject* kwds)
{
    static const char* kwlist[] = { "target", "type", "x", "y", "button", "modifiers", 0 };
    const char* targetName = 0;
    const char* typeStr = 0;
    double x = 0., y = 0.;
    const char* buttonStr = "left";
    const char* modsStr = "";

    if ( !PyArg_ParseTupleAndKeywords(args, kwds, "ssdd|ss", const_cast<char**>(kwlist),
                                      &targetName, &typeStr, &x, &y, &buttonStr, &modsStr) ) {
        return 0;
    }
    QPointF pos;
    QWidget* w = penEventWidget(targetName, x, y, &pos);
    if (!w) {
        return 0;
    }
    Qt::MouseButton button;
    Qt::KeyboardModifiers mods;
    if ( !parseButton(buttonStr, &button) || !parseModifiers(modsStr, &mods) ) {
        return 0;
    }
    const std::string type(typeStr);
    QEvent::Type evType;
    Qt::MouseButtons& pressed = state().pressedButtons;
    if (type == "press") {
        evType = QEvent::MouseButtonPress;
        pressed |= button;
    } else if (type == "move") {
        evType = QEvent::MouseMove;
        button = Qt::NoButton;
    } else if (type == "release") {
        evType = QEvent::MouseButtonRelease;
        pressed &= ~Qt::MouseButtons(button);
    } else {
        PyErr_Format(PyExc_ValueError, "unknown pen event type %s (press, move or release)", typeStr);

        return 0;
    }
    const QPoint global = w->mapToGlobal( pos.toPoint() );
    updateHovered(w, pos);
    QMouseEvent e( evType, pos, QPointF(global), button, pressed, mods );
    QCoreApplication::sendEvent(w, &e);
    QCoreApplication::processEvents();

    Py_RETURN_NONE;
}

// Delivers a key as Qt does: a ShortcutOverride event, then a matching shortcut, else the key events.
void
sendKeyCombination(QWidget* w,
                   Qt::Key key,
                   Qt::KeyboardModifiers mods)
{
    QString text;

    if ( (key >= Qt::Key_Space) && (key <= Qt::Key_AsciiTilde) && !( mods & (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier) ) ) {
        text = QChar( (ushort)key );
        if ( !(mods & Qt::ShiftModifier) ) {
            text = text.toLower();
        }
    }
    QKeyEvent override(QEvent::ShortcutOverride, key, mods, text);
    override.setAccepted(false);
    QCoreApplication::sendEvent(w, &override);
    if ( !override.isAccepted() ) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QKeySequence seq( QKeyCombination(mods, key) );
#else
        const QKeySequence seq( (int)key | (int)mods );
#endif
        // the actions of the target and its parents, then the ones of the menu bar (as the Edit menu
        // has the undo/redo actions of the current undo stack)
        QList<QAction*> actions;
        for (QWidget* p = w; p; p = p->parentWidget()) {
            actions += p->actions();
        }
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>( w->window() );
        if (!mainWindow) {
            mainWindow = getGui(); // the target is in a floating window
        }
        if ( mainWindow && mainWindow->menuBar() ) {
            QList<QAction*> menuActions = mainWindow->menuBar()->actions();
            while ( !menuActions.isEmpty() ) {
                QAction* a = menuActions.takeFirst();
                actions.push_back(a);
                QMenu* menu = a->menu();
                if (menu) {
                    menuActions += menu->actions();
                }
            }
        }
        for (QAction* a : actions) {
            if ( !a->isEnabled() || !a->shortcuts().contains(seq) ) {
                continue;
            }
            std::cout << "NatronGuiTest: shortcut " << seq.toString(QKeySequence::PortableText).toStdString()
                      << " triggers \"" << a->text().toStdString() << "\"" << std::endl;
            a->trigger();
            QCoreApplication::processEvents();

            return;
        }
    }
    QKeyEvent press(QEvent::KeyPress, key, mods, text);
    QCoreApplication::sendEvent(w, &press);
    QKeyEvent release(QEvent::KeyRelease, key, mods, text);
    QCoreApplication::sendEvent(w, &release);
    QCoreApplication::processEvents();
}

PyObject*
py_sendKey(PyObject* /*self*/,
           PyObject* args)
{
    const char* targetName = 0;
    const char* keys = 0;

    if ( !PyArg_ParseTuple(args, "ss", &targetName, &keys) ) {
        return 0;
    }
    QWidget* w = findKeyTarget(targetName);
    if (!w) {
        return 0;
    }
    const QKeySequence seq = QKeySequence::fromString(QString::fromUtf8(keys), QKeySequence::PortableText);
    if ( seq.isEmpty() ) {
        PyErr_Format(PyExc_ValueError, "invalid key sequence %s", keys);

        return 0;
    }
    for (int i = 0; i < seq.count(); ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QKeyCombination c = seq[i];
        sendKeyCombination( w, c.key(), c.keyboardModifiers() );
#else
        const int c = seq[i];
        sendKeyCombination( w, Qt::Key(c & ~Qt::KeyboardModifierMask), Qt::KeyboardModifiers(c & Qt::KeyboardModifierMask) );
#endif
    }

    Py_RETURN_NONE;
}

PyObject*
py_getDisplayedPixel(PyObject* /*self*/,
                     PyObject* args)
{
    const char* viewerName = 0;
    double x = 0., y = 0.;

    if ( !PyArg_ParseTuple(args, "sdd", &viewerName, &x, &y) ) {
        return 0;
    }
    ViewerGL* gl = findViewer(viewerName);
    if (!gl) {
        return 0;
    }
    const QPointF pos = gl->toWidgetCoordinates( QPointF(x, y) );
    const QImage img = gl->grabFramebuffer();
    const double ratio = gl->devicePixelRatioF();
    const int px = (int)(pos.x() * ratio);
    const int py = (int)(pos.y() * ratio);
    if ( !img.valid(px, py) ) {
        PyErr_Format(PyExc_ValueError, "(%g, %g) is not visible in the viewer", x, y);

        return 0;
    }
    const QRgb c = img.pixel(px, py);

    return Py_BuildValue( "(iiii)", qRed(c), qGreen(c), qBlue(c), qAlpha(c) );
}

PyObject*
py_setDialogAnswers(PyObject* /*self*/,
                    PyObject* args)
{
    PyObject* list = 0;

    if ( !PyArg_ParseTuple(args, "O", &list) ) {
        return 0;
    }
    PyObject* seq = PySequence_Fast(list, "setDialogAnswers() takes a list of button names");
    if (!seq) {
        return 0;
    }
    std::deque<StandardButtonEnum> answers;
    const Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* item = PySequence_Fast_GET_ITEM(seq, i);
        const char* name = PyUnicode_Check(item) ? PyUnicode_AsUTF8(item) : 0;
        StandardButtonEnum b;
        if ( !name || !buttonFromName(QString::fromUtf8(name), &b) ) {
            Py_DECREF(seq);
            PyErr_SetString(PyExc_ValueError, "setDialogAnswers(): unknown button name (e.g. \"yes\", \"no\", \"ok\", \"cancel\")");

            return 0;
        }
        answers.push_back(b);
    }
    Py_DECREF(seq);
    state().answers = answers;

    Py_RETURN_NONE;
}

PyObject*
py_getDialogLog(PyObject* /*self*/,
                PyObject* /*args*/)
{
    const std::vector<DialogRecord>& dialogs = state().dialogs;
    PyObject* list = PyList_New(0);

    if (!list) {
        return 0;
    }
    for (std::vector<DialogRecord>::const_iterator it = dialogs.begin(); it != dialogs.end(); ++it) {
        PyObject* d = Py_BuildValue( "{s:s,s:s,s:s,s:s}", "type", it->type.c_str(), "title", it->title.c_str(),
                                     "text", it->text.c_str(), "answer", it->answer.c_str() );
        if ( !d || (PyList_Append(list, d) < 0) ) {
            Py_XDECREF(d);
            Py_DECREF(list);

            return 0;
        }
        Py_DECREF(d);
    }

    return list;
}

PyObject*
py_clearDialogLog(PyObject* /*self*/,
                  PyObject* /*args*/)
{
    state().dialogs.clear();

    Py_RETURN_NONE;
}

PyMethodDef guiTestMethods[] = {
    { "waitIdle", (PyCFunction)(void (*)(void))py_waitIdle, METH_VARARGS | METH_KEYWORDS,
      "waitIdle(timeout=30.0, settle=0.2) -> bool\n"
      "Process the events until no render has been running for settle seconds. "
      "Returns False if this did not happen within timeout seconds." },
    { "isRendering", py_isRendering, METH_NOARGS,
      "isRendering() -> bool\nTrue if a render is running or queued." },
    { "penEvent", (PyCFunction)(void (*)(void))py_penEvent, METH_VARARGS | METH_KEYWORDS,
      "penEvent(target, type, x, y, button='left', modifiers='')\n"
      "Send a mouse event to target: a viewer (its node script name), at the canvas coordinates (x, y), "
      "or 'NodeGraph', at the node graph coordinates (x, y) (as Effect.getPosition()). "
      "type is 'press', 'move' or 'release'; button is 'left', 'middle', 'right' or 'none' (the button "
      "of a press or release; a move has the pressed buttons); modifiers is e.g. 'Shift+Ctrl'. "
      "The real cursor is not moved." },
    { "sendKey", py_sendKey, METH_VARARGS,
      "sendKey(target, keys)\n"
      "Send keys (e.g. 'F', 'Ctrl+Z', 'Backspace', in QKeySequence portable text) to target: a viewer "
      "node name or 'NodeGraph'. A key that the target does not override triggers the matching shortcut." },
    { "getDisplayedPixel", py_getDisplayedPixel, METH_VARARGS,
      "getDisplayedPixel(viewer, x, y) -> (r, g, b, a)\n"
      "The 8-bit colour displayed by the viewer at the canvas coordinates (x, y), after the display LUT." },
    { "setDialogAnswers", py_setDialogAnswers, METH_VARARGS,
      "setDialogAnswers(answers)\n"
      "The answers (button names, e.g. ['yes', 'no', 'ok']) of the next modal dialogs, in order: each one "
      "clicks that button of the dialog. A dialog that gets no answer fails the test (it is rejected)." },
    { "getDialogLog", py_getDialogLog, METH_NOARGS,
      "getDialogLog() -> list of dict\n"
      "The modal dialogs shown so far: dicts with the keys type ('dialog' for an answered one, "
      "'unexpected' for one that was rejected), title, text and answer (the button name, or 'reject')." },
    { "clearDialogLog", py_clearDialogLog, METH_NOARGS,
      "clearDialogLog()\nClear the dialog log." },
    { 0, 0, 0, 0 }
};

// The Python part of the module: skip(), drag() and the test runner.
const char* guiTestPythonSource =
    "class Skip(Exception):\n"
    "    \"\"\"Raised by skip(): the test cannot run here (exit status 77).\"\"\"\n"
    "    pass\n"
    "\n"
    "def skip(reason):\n"
    "    \"\"\"Stop the test, which is reported as skipped (exit status 77).\"\"\"\n"
    "    raise Skip(reason)\n"
    "\n"    "def requireInteractive(reason='needs the real focus or mouse'):\n"
    "    \"\"\"Skip the test unless NATRON_TEST_INTERACTIVE=1 (Xvfb on CI, a VM, or an explicit opt-in).\"\"\"\n"
    "    import os\n"
    "    if os.environ.get('NATRON_TEST_INTERACTIVE') != '1':\n"
    "        raise Skip('interactive test (%s): set NATRON_TEST_INTERACTIVE=1 to run it' % reason)\n"
    "\n"
    "def drag(target, x0, y0, x1, y1, steps=10, button='left', modifiers=''):\n"
    "    \"\"\"Drag in target (see penEvent()) from (x0, y0) to (x1, y1).\"\"\"\n"
    "    penEvent(target, 'move', x0, y0, 'none', modifiers)\n"
    "    penEvent(target, 'press', x0, y0, button, modifiers)\n"
    "    for i in range(1, steps + 1):\n"
    "        t = float(i) / steps\n"
    "        penEvent(target, 'move', x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, 'none', modifiers)\n"
    "    penEvent(target, 'release', x1, y1, button, modifiers)\n"
    "\n"
    "def _run(path):\n"
    "    import os, sys, traceback\n"
    "    import NatronGui\n"
    "    saved = (sys.stdout, sys.stderr)\n"
    "    # the GUI redirects the output to the Script Editor: print to the terminal\n"
    "    if sys.__stdout__ is not None:\n"
    "        sys.stdout = sys.__stdout__\n"
    "    if sys.__stderr__ is not None:\n"
    "        sys.stderr = sys.__stderr__\n"
    "    status = 1\n"
    "    try:\n"
    "        d = os.path.dirname(path)\n"
    "        if d not in sys.path:\n"
    "            sys.path.insert(0, d)\n"
    "        g = {'__name__': '__main__', '__file__': path, '__builtins__': __builtins__,\n"
    "             'app': NatronGui.natron.getGuiInstance(0)}\n"
    "        with open(path, encoding='utf-8') as f:\n"
    "            code = compile(f.read(), path, 'exec')\n"
    "        exec(code, g)\n"
    "        status = 0\n"
    "    except Skip as e:\n"
    "        print('SKIP: %s' % e)\n"
    "        status = 77\n"
    "    except SystemExit as e:\n"
    "        status = e.code if isinstance(e.code, int) else (0 if e.code is None else 1)\n"
    "    except BaseException:\n"
    "        traceback.print_exc()\n"
    "        status = 1\n"
    "    finally:\n"
    "        sys.stdout.flush()\n"
    "        sys.stderr.flush()\n"
    "        (sys.stdout, sys.stderr) = saved\n"
    "    return status\n";

PyModuleDef guiTestModule = {
    PyModuleDef_HEAD_INIT,
    NATRON_GUI_TEST_PYTHON_MODULE_NAME,
    "Test-only functions for the GUI tests (Natron --test script.py).\n"
    "Not part of the Natron Python API: only available in test mode.",
    -1,
    guiTestMethods,
    0, 0, 0, 0
};

PyObject*
PyInit_NatronGuiTest()
{
    PyObject* module = PyModule_Create(&guiTestModule);

    if (!module) {
        return 0;
    }
    PyObject* dict = PyModule_GetDict(module); // borrowed
    PyObject* r = PyRun_String(guiTestPythonSource, Py_file_input, dict, dict);
    if (!r) {
        Py_DECREF(module);

        return 0;
    }
    Py_DECREF(r);

    return module;
}

void
runTestScript(const QString& path)
{
    int status = 1;
    {
        // the script processes events (waitIdle()...): natron.quit() and the close requests wait
        // until it returns
        AppManager::NestedEventLoopRAII nestedLoop;
        PythonGILLocker pgl;
        PyObject* module = PyImport_ImportModule(NATRON_GUI_TEST_PYTHON_MODULE_NAME);
        PyObject* r = module ? PyObject_CallMethod( module, "_run", "s", path.toStdString().c_str() ) : 0;
        if (r) {
            status = (int)PyLong_AsLong(r);
            Py_DECREF(r);
        }
        if ( PyErr_Occurred() ) {
            PyErr_Print();
            status = 1;
        }
        Py_XDECREF(module);
    }
    if ( (status == 0) && (state().unexpectedDialogs > 0) ) {
        std::cout << "NatronGuiTest: " << state().unexpectedDialogs << " unexpected dialog(s)" << std::endl;
        status = 1;
    }
    std::cout << "NatronGuiTest: " << path.toStdString() << ": "
              << ( (status == 0) ? "PASS" : (status == 77) ? "SKIP" : "FAIL" ) << std::endl;
    // as natron.quit(status): close the projects without asking to save them, and exit with status
    appPTR->requestQuit(status);
}
} // anonymous namespace

namespace GuiTest {
void
install()
{
    if (PyImport_AppendInittab(NATRON_GUI_TEST_PYTHON_MODULE_NAME, &PyInit_NatronGuiTest) == -1) {
        throw std::runtime_error("Failed to initialize built-in Python module.");
    }
    // from the start: the dialogs shown while Natron loads must not block the test either
    qApp->installEventFilter( new ModalDialogWatcher(qApp) );
}

void
scheduleTestScript(const QString& path)
{
    QTimer::singleShot(0, qApp, [path]() {
        runTestScript(path);
    });
}

} // namespace GuiTest

NATRON_NAMESPACE_EXIT
