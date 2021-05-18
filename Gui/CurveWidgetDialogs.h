/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef Gui_CurveGuiDialogs_h
#define Gui_CurveGuiDialogs_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMetaType>
#include <QDialog>
#include <QtCore/QByteArray>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/CurveEditorUndoRedo.h" // KeyPtr
#include "Gui/CurveGui.h" // CurveGui
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class ImportExportCurveDialog
    : public QDialog
{
    Q_OBJECT

public:

    ImportExportCurveDialog(bool isExportDialog,
                            const std::vector<CurveGuiPtr> & curves,
                            Gui* gui,
                            QWidget* parent = 0);

    virtual ~ImportExportCurveDialog();
    QString getFilePath();

    double getXStart() const;

    double getXIncrement() const;

    int getXCount() const;

    double getXEnd() const;

    void getCurveColumns(std::map<int, CurveGuiPtr>* columns) const;

public Q_SLOTS:

    void open_file();

private:

    QByteArray saveState();

    void restoreState(const QByteArray& state);

    Gui* _gui;
    bool _isExportDialog;
    QVBoxLayout* _mainLayout;

    //////File
    QWidget* _fileContainer;
    QHBoxLayout* _fileLayout;
    Label* _fileLabel;
    LineEdit* _fileLineEdit;
    Button* _fileBrowseButton;

    //////x start value
    QWidget* _startContainer;
    QHBoxLayout* _startLayout;
    Label* _startLabel;
    LineEdit* _startLineEdit;

    //////x increment
    QWidget* _incrContainer;
    QHBoxLayout* _incrLayout;
    Label* _incrLabel;
    LineEdit* _incrLineEdit;

    //////x end value
    QWidget* _endContainer;
    QHBoxLayout* _endLayout;
    Label* _endLabel;
    LineEdit* _endLineEdit;


    /////Columns
    struct CurveColumn
    {
        CurveGuiPtr _curve;
        QWidget* _curveContainer;
        QHBoxLayout* _curveLayout;
        Label* _curveLabel;
        SpinBox* _curveSpinBox;
    };

    std::vector<CurveColumn > _curveColumns;

    ////buttons
    DialogButtonBox* _buttonBox;
};


struct EditKeyFrameDialogPrivate;
class EditKeyFrameDialog
    : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum EditModeEnum
    {
        eEditModeKeyframePosition,
        eEditModeLeftDerivative,
        eEditModeRightDerivative
    };

    EditKeyFrameDialog(EditModeEnum mode,
                       CurveWidget* curveWidget,
                       const KeyPtr& key,
                       QWidget* parent);

    virtual ~EditKeyFrameDialog();

Q_SIGNALS:

    void valueChanged(int dimension, double value);

public Q_SLOTS:

    void onXSpinBoxValueChanged(double d);
    void onYSpinBoxValueChanged(double d);
    void onEditingFinished();

private:

    void moveKeyTo(double newX, double newY);
    void moveDerivativeTo(double d);

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<EditKeyFrameDialogPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_CurveGuiDialogs_h
