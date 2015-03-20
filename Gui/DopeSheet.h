#ifndef DOPESHEET_H
#define DOPESHEET_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

#include "Engine/ScriptObject.h"

class DopeSheetPrivate;
class Gui;
class KnobI;
class KnobGui;
class DopeSheetKeyframeSetPrivate;
class DopeSheetNodePrivate;
class NodeGui;
class QTreeWidget;
class QTreeWidgetItem;
class TimeLine;

class DopeSheetKeyframeSet : public QObject
{
    Q_OBJECT

public:
    DopeSheetKeyframeSet(QTreeWidgetItem *parentItem,
                         const boost::shared_ptr<KnobI> knob,
                         KnobGui *knobGui,
                         int dimension,
                         bool isMultiDim);
    ~DopeSheetKeyframeSet();

    QTreeWidgetItem *getItem() const;

public Q_SLOTS:
    void checkVisibleState();

private:
    boost::shared_ptr<DopeSheetKeyframeSetPrivate> _imp;
};

class DopeSheetNode : public QObject
{
    Q_OBJECT

public:
    DopeSheetNode(QTreeWidget *hierarchyView,
                         const boost::shared_ptr<NodeGui> &node);
    ~DopeSheetNode();

    boost::shared_ptr<NodeGui> getNode() const;

    void checkVisibleState();

public Q_SLOTS:
    void onNodeNameChanged(const QString &name);

private:
    boost::scoped_ptr<DopeSheetNodePrivate> _imp;
};

class DopeSheet : public QWidget, public ScriptObject
{
    Q_OBJECT

public:
    DopeSheet(Gui *gui,
              QWidget *parent = 0);
    ~DopeSheet();

    void frame();

    void addNode(boost::shared_ptr<NodeGui> node);
    void removeNode(NodeGui *node);

public Q_SLOTS:
    void onItemSelectionChanged();

private:
    boost::scoped_ptr<DopeSheetPrivate> _imp;
};

#endif // DOPESHEET_H
