//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef _Gui_CurveGuiDialogs_h
#define _Gui_CurveGuiDialogs_h

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
