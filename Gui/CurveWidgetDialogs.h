/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef _Gui_CurveGuiDialogs_h
#define _Gui_CurveGuiDialogs_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <set>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMetaType>
#include <QDialog>
#include <QByteArray>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include "Gui/CurveEditorUndoRedo.h" // KeyPtr
#include "Gui/CurveGui.h" // CurveGui

class CurveSelection;
class QFont;
class Variant;
class TimeLine;
class KnobGui;
class CurveWidget;
class Button;
class LineEdit;
class SpinBox;
class Gui;
class Bezier;
class RotoContext;
class QVBoxLayout;
class QHBoxLayout;
namespace Natron {
    class Label;
}



class ImportExportCurveDialog
    : public QDialog
{
    Q_OBJECT

public:

    ImportExportCurveDialog(bool isExportDialog,
                            const std::vector<boost::shared_ptr<CurveGui> > & curves,
                            Gui* gui,
                            QWidget* parent = 0);

    virtual ~ImportExportCurveDialog();
    QString getFilePath();

    double getXStart() const;

    double getXIncrement() const;

    double getXEnd() const;

    void getCurveColumns(std::map<int,boost::shared_ptr<CurveGui> >* columns) const;

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
    Natron::Label* _fileLabel;
    LineEdit* _fileLineEdit;
    Button* _fileBrowseButton;

    //////x start value
    QWidget* _startContainer;
    QHBoxLayout* _startLayout;
    Natron::Label* _startLabel;
    SpinBox* _startSpinBox;

    //////x increment
    QWidget* _incrContainer;
    QHBoxLayout* _incrLayout;
    Natron::Label* _incrLabel;
    SpinBox* _incrSpinBox;

    //////x end value
    QWidget* _endContainer;
    QHBoxLayout* _endLayout;
    Natron::Label* _endLabel;
    SpinBox* _endSpinBox;


    /////Columns
    struct CurveColumn
    {
        boost::shared_ptr<CurveGui> _curve;
        QWidget* _curveContainer;
        QHBoxLayout* _curveLayout;
        Natron::Label* _curveLabel;
        SpinBox* _curveSpinBox;
    };

    std::vector< CurveColumn > _curveColumns;

    ////buttons
    QWidget* _buttonsContainer;
    QHBoxLayout* _buttonsLayout;
    Button* _okButton;
    Button* _cancelButton;
};


struct EditKeyFrameDialogPrivate;
class EditKeyFrameDialog : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    enum EditModeEnum {
        eEditModeKeyframePosition,
        eEditModeLeftDerivative,
        eEditModeRightDerivative
    };
    
    EditKeyFrameDialog(EditModeEnum mode,CurveWidget* curveWidget, const KeyPtr& key,QWidget* parent);
    
    virtual ~EditKeyFrameDialog();
    
Q_SIGNALS:
    
    void valueChanged(int dimension,double value);
    
    
public Q_SLOTS:
    
    void onXSpinBoxValueChanged(double d);
    void onYSpinBoxValueChanged(double d);
    
private:
    
    void moveKeyTo(double newX,double newY);
    void moveDerivativeTo(double d);
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<EditKeyFrameDialogPrivate> _imp;
};

#endif // _Gui_CurveGuiDialogs_h
