#ifndef DOPESHEETEDITORUNDOREDO_H
#define DOPESHEETEDITORUNDOREDO_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Curve.h"

class DopeSheetView;
class DSKnob;
class QTreeWidgetItem;

class DSNode;

/**
 * @brief The DSSelectedKey struct
 *
 *
 */
struct DSSelectedKey
{
    DSSelectedKey(DSKnob *knob, KeyFrame kf, int dim) :
        dsKnob(knob),
        key(kf),
        dimension(dim)
    {}

    DSSelectedKey(const DSSelectedKey &other) :
        dsKnob(other.dsKnob),
        key(other.key),
        dimension(other.dimension)
    {}

    friend bool operator==(const DSSelectedKey &key1, const DSSelectedKey &key2)
    {
        if (key1.dsKnob != key2.dsKnob) {
            return false;
        }

        if (key1.key != key2.key) {
            return false;
        }

        if (key1.dimension != key2.dimension) {
            return false;
        }

        return true;
    }

    DSKnob *dsKnob;
    KeyFrame key;
    int dimension;
};


typedef boost::shared_ptr<DSSelectedKey> DSKeyPtr;
typedef std::list<DSKeyPtr> DSKeyPtrList;


/**
 * @brief The DSMoveKeysCommand class
 *
 *
 */
class DSMoveKeysCommand : public QUndoCommand
{
public:
    DSMoveKeysCommand(const DSKeyPtrList &keys,
                      double dt,
                      DopeSheetView *view,
                      QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void moveSelectedKeyframes(double dt);

private:
    DSKeyPtrList _keys;
    double _dt;
    DopeSheetView *_view;
};

/**
 * @brief The DSLeftTrimReaderCommand class
 *
 *
 */
class DSLeftTrimReaderCommand : public QUndoCommand
{
public:
    DSLeftTrimReaderCommand(DSNode *dsNodeReader,
                            double oldTime,
                            double newTime,
                            DopeSheetView *view,
                            QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void trimLeft(double time);

private:
    DSNode *_dsNodeReader;
    double _oldTime;
    double _newTime;
    DopeSheetView *_view;
};

/**
 * @brief The DSRightTrimReaderCommand class
 *
 *
 */
class DSRightTrimReaderCommand : public QUndoCommand
{
public:
    DSRightTrimReaderCommand(DSNode *dsNodeReader,
                             double oldTime,
                             double newTime,
                             DopeSheetView *view,
                             QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void trimRight(double time);

private:
    DSNode *_dsNodeReader;
    double _oldTime;
    double _newTime;
    DopeSheetView *_view;
};

class DSMoveReaderCommand : public QUndoCommand
{
public:
    DSMoveReaderCommand(DSNode *dsNodeReader,
                        double oldTime, double newTime,
                        DopeSheetView *view,
                        QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void moveClip(double time);

private:
    DSNode *_dsNodeReader;
    double _oldTime;
    double _newTime;
    DopeSheetView *_view;
};

/**
 * @brief The DSRemoveKeysCommand class
 *
 *
 */
class DSRemoveKeysCommand : public QUndoCommand
{
public:
    DSRemoveKeysCommand(const std::vector<DSSelectedKey> &keys,
                        DopeSheetView *view,
                        QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void addOrRemoveKeyframe(bool add);

private:
    std::vector<DSSelectedKey> _keys;
    DopeSheetView *_view;
};

/**
 * @brief The DSMoveGroupCommand class
 *
 *
 */
class DSMoveGroupCommand : public QUndoCommand
{
public:
    DSMoveGroupCommand(DSNode *dsNodeGroup,
                       double dt,
                       DopeSheetView *view,
                       QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void moveGroupKeyframes(double dt);

private:
    DSNode *_dsNodeGroup;
    double _dt;
    DopeSheetView *_view;
};

class DSChangeNodeLabelCommand : public QUndoCommand
{
public:
    DSChangeNodeLabelCommand(DSNode *dsNode,
                      const QString &oldLabel,
                      const QString &newLabel,
                      QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void changeNodeLabel(const QString &label);

private:
    DSNode *_dsNode;
    QString _oldLabel;
    QString _newLabel;
};



#endif // DOPESHEETEDITORUNDOREDO_H
