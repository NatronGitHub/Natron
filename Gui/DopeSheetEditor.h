#ifndef DOPESHEETEDITOR_H
#define DOPESHEETEDITOR_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGui/QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ScriptObject.h"

class DopeSheetEditorPrivate;
class Gui;
class NodeGui;
class TimeLine;

/**
 * @brief The DopeSheetEditor class
 *
 *
 */
class DopeSheetEditor : public QWidget, public ScriptObject
{
    Q_OBJECT

public:
    DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent = 0);
    ~DopeSheetEditor();

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

    void centerOn(double xMin, double xMax);

public Q_SLOTS:
    void toggleTripleSync(bool enabled);

private:
    boost::scoped_ptr<DopeSheetEditorPrivate> _imp;
};

#endif // DOPESHEETEDITOR_H
