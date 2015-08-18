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

#ifndef _Gui_DockablePanelPrivate_h_
#define _Gui_DockablePanelPrivate_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <map>

#include "Global/Macros.h"

#include <QtCore/QMutex>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QTabWidget>
#include <QDialog>

#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"
#include "Gui/DockablePanel.h"

class QHBoxLayout;
class QVBoxLayout;
class QUndoStack;
class QUndoCommand;
class QGridLayout;
class QTabWidget;

class KnobI;
class KnobGui;
class KnobHolder;
class NodeGui;
class Gui;
class Page_Knob;
class Button;
class RotoPanel;
class MultiInstancePanel;
class Group_Knob;
class LineEdit;
class FloatingWidget;

class VerticalColorBar;
class TabGroup;

namespace Natron {
    class Label;
}

struct Page
{
    QWidget* tab;
    int currentRow;
    TabGroup* groupAsTab; //< to gather group knobs that are set as a tab

    Page()
    : tab(0), currentRow(0),groupAsTab(0)
    {
    }

    Page(const Page & other)
    : tab(other.tab), currentRow(other.currentRow), groupAsTab(other.groupAsTab)
    {
    }
};

typedef std::map<QString,Page> PageMap;

struct DockablePanelPrivate
{
    DockablePanel* _publicInterface;
    Gui* _gui;
    QVBoxLayout* _container; /*!< ptr to the layout containing this DockablePanel*/

    /*global layout*/
    QVBoxLayout* _mainLayout;

    /*Header related*/
    QFrame* _headerWidget;
    QHBoxLayout *_headerLayout;
    LineEdit* _nameLineEdit; /*!< if the name is editable*/
    Natron::Label* _nameLabel; /*!< if the name is read-only*/

    QHBoxLayout* _horizLayout;
    QWidget* _horizContainer;
    VerticalColorBar* _verticalColorBar;

    /*Tab related*/
    QTabWidget* _tabWidget;
    Button* _centerNodeButton;
    Button* _enterInGroupButton;
    Button* _helpButton;
    Button* _minimize;
    Button* _hideUnmodifiedButton;
    Button* _floatButton;
    Button* _cross;
    mutable QMutex _currentColorMutex;
    QColor _overlayColor;
    bool _hasOverlayColor;
    Button* _colorButton;
    Button* _overlayButton;
    Button* _undoButton;
    Button* _redoButton;
    Button* _restoreDefaultsButton;
    bool _minimized; /*!< true if the panel is minimized*/
    boost::shared_ptr<QUndoStack> _undoStack; /*!< undo/redo stack*/
    bool _floating; /*!< true if the panel is floating*/
    FloatingWidget* _floatingWidget;

    /*a map storing for each knob a pointer to their GUI.*/
    std::map<boost::weak_ptr<KnobI>,KnobGui*> _knobs;

    ///THe visibility of the knobs before the hide/show unmodified button is clicked
    ///to show only the knobs that need to afterwards
    std::map<KnobGui*,bool> _knobsVisibilityBeforeHideModif;
    KnobHolder* _holder;

    /* map<tab name, pair<tab , row count> >*/
    PageMap _pages;
    QString _defaultPageName;
    bool _useScrollAreasForTabs;
    DockablePanel::HeaderModeEnum _mode;
    mutable QMutex _isClosedMutex;
    bool _isClosed; //< accessed by serialization thread too

    QString _helpToolTip;
    QString _pluginID;
    unsigned _pluginVersionMajor,_pluginVersionMinor;


    bool _pagesEnabled;

    Natron::Label* _iconLabel;

    DockablePanelPrivate(DockablePanel* publicI,
                         Gui* gui,
                         KnobHolder* holder,
                         QVBoxLayout* container,
                         DockablePanel::HeaderModeEnum headerMode,
                         bool useScrollAreasForTabs,
                         const QString & defaultPageName,
                         const QString& helpToolTip,
                         const boost::shared_ptr<QUndoStack>& stack);
    
    /*inserts a new page to the dockable panel.*/
    PageMap::iterator addPage(Page_Knob* page,const QString & name);

    boost::shared_ptr<Page_Knob> ensureDefaultPageKnobCreated() ;


    void initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> > & knobs,
                              QWidget* lastRowWidget);

    KnobGui* createKnobGui(const boost::shared_ptr<KnobI> &knob);

    /*Search an existing knob GUI in the map, otherwise creates
     the gui for the knob.*/
    KnobGui* findKnobGuiOrCreate( const boost::shared_ptr<KnobI> &knob,
                                 bool makeNewLine,
                                 QWidget* lastRowWidget,
                                 const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine = std::vector< boost::shared_ptr< KnobI > >() );

    PageMap::iterator getDefaultPage(const boost::shared_ptr<KnobI> &knob);
};

/**
 * @class This is to overcome an issue in QTabWidget : switching tab does not resize the QTabWidget.
 * This class resizes the QTabWidget to the current tab size.
 **/
class DockablePanelTabWidget
    : public QTabWidget
{
    Gui* _gui;
public:

    DockablePanelTabWidget(Gui* gui,QWidget* parent = 0);

    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    
private:
    
    virtual void keyPressEvent(QKeyEvent* event) OVERRIDE FINAL;
};



struct ManageUserParamsDialogPrivate;
class ManageUserParamsDialog : public QDialog
{
    Q_OBJECT
    
public:
    
    
    ManageUserParamsDialog(DockablePanel* panel,QWidget* parent);
    
    virtual ~ManageUserParamsDialog();
    
public Q_SLOTS:
    
    void onAddClicked();
    
    void onPickClicked();
    
    void onDeleteClicked();
    
    void onEditClicked();
    
    void onUpClicked();
    
    void onDownClicked();
    
    void onCloseClicked();
    
    void onSelectionChanged();
    
private:
    
    boost::scoped_ptr<ManageUserParamsDialogPrivate> _imp;
};

struct PickKnobDialogPrivate;
class PickKnobDialog : public QDialog
{
    Q_OBJECT
    
public:
    
    PickKnobDialog(DockablePanel* panel, QWidget* parent);
    
    virtual ~PickKnobDialog();
    
    KnobGui* getSelectedKnob(bool* useExpressionLink) const;
    
public Q_SLOTS:
    
    void onNodeComboEditingFinished();
    
private:
    
    boost::scoped_ptr<PickKnobDialogPrivate> _imp;
};

struct AddKnobDialogPrivate;
class AddKnobDialog : public QDialog
{
    Q_OBJECT
public:
    
    AddKnobDialog(DockablePanel* panel,const boost::shared_ptr<KnobI>& knob,QWidget* parent);
    
    virtual ~AddKnobDialog();
    
    boost::shared_ptr<KnobI> getKnob() const;

public Q_SLOTS:
    
    void onPageCurrentIndexChanged(int index);
    
    void onTypeCurrentIndexChanged(int index);
    
    void onOkClicked();
private:
    
    boost::scoped_ptr<AddKnobDialogPrivate> _imp;
};

class VerticalColorBar : public QWidget
{
    Q_OBJECT
    
    QColor _color;
    
public:
    
    VerticalColorBar(QWidget* parent);
    
public Q_SLOTS:
    
    void setColor(const QColor& color);
    
private:
    
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
};
#endif // _Gui_DockablePanelPrivate_h_
