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
#include <QTreeWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

#include "Engine/ScriptObject.h"

class DopeSheetEditorPrivate;
class DSKnobPrivate;
class DopeSheetEditor;
class DSNodePrivate;
class Gui;
class KnobI;
class KnobGui;
class NodeGui;
class TimeLine;


//
class DSNode;
class DSKnob;

typedef std::list<DSNode *> DSNodeList;
typedef std::list<DSKnob *> DSKnobList;
typedef std::list<QTreeWidgetItem *> QTreeWidgetItemList;
// typedefs


const int DSNODE_TYPE_USERROLE = Qt::UserRole + 1;


class DSKnob : public QObject
{
    Q_OBJECT

public:
    friend class DopeSheetEditor;

    DSKnob(DopeSheetEditor *dopeSheetEditor,
           QTreeWidgetItem *nameItem,
           KnobGui *knobGui);
    ~DSKnob();

    QRectF getNameItemRect() const;
    QRectF getNameItemRectForDim(int dim) const;

    KnobGui *getKnobGui() const;

    bool isMultiDim() const;
    bool isExpanded() const;
    bool isHidden() const;

    bool nodeItemIsCollapsed() const;

Q_SIGNALS:
    void needNodesVisibleStateChecking();

public Q_SLOTS:
    void checkVisibleState();

private:
    QTreeWidgetItem *getNameItem() const;

private:
    boost::scoped_ptr<DSKnobPrivate> _imp;
};

class DSNode : public QObject
{
    Q_OBJECT

public:
    friend class DopeSheetEditor;

    enum DSNodeType
    {
        CommonNodeType = 1001,
        ReaderNodeType,
        GroupNodeType
    };

    DSNode(DopeSheetEditor *dopeSheetEditor,
           const boost::shared_ptr<NodeGui> &nodeGui);
    ~DSNode();

    QRectF getNameItemRect() const;
    boost::shared_ptr<NodeGui> getNodeGui() const;

    DSKnobList getDSKnobs() const;

    bool isExpanded() const;
    bool isHidden() const;

    bool isCommonNode() const;
    bool isReaderNode() const;
    bool isTimeNode() const;
    bool isGroupNode() const;

public Q_SLOTS:
    void onNodeNameChanged(const QString &name);

    void checkVisibleState();

private:
    QTreeWidgetItem *getNameItem() const;

private:
    boost::scoped_ptr<DSNodePrivate> _imp;
};

class DopeSheetEditor : public QWidget, public ScriptObject
{
    Q_OBJECT

public:
    DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent = 0);
    ~DopeSheetEditor();

    QTreeWidget *getHierarchyView() const;

    const DSNodeList &getDSNodeItems() const;

    QRect getNameItemRect(const QTreeWidgetItem *item) const;
    QRect getSectionRect(const QTreeWidgetItem *item) const;

    boost::shared_ptr<TimeLine> getTimeline() const;

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

public Q_SLOTS:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

    void refreshDopeSheetView();

private:
    boost::scoped_ptr<DopeSheetEditorPrivate> _imp;
};

class HierarchyView : public QTreeWidget
{
public:
    friend class HierarchyViewItemDelegate;

    explicit HierarchyView(QWidget *parent = 0);
};


#endif // DOPESHEET_H
