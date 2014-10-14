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

#ifndef NATRON_GUI_SETTINGSPANEL_H_
#define NATRON_GUI_SETTINGSPANEL_H_

#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QTabWidget>
#include "Global/GlobalDefines.h"

class KnobI;
class KnobGui;
class KnobHolder;
class NodeGui;
class Gui;
class QVBoxLayout;
class Button;
class QUndoStack;
class NodeBackDrop;
class QUndoCommand;
class RotoPanel;
class MultiInstancePanel;

/**
 * @class This is to overcome an issue in QTabWidget : switching tab does not resize the QTabWidget.
 * This class resizes the QTabWidget to the current tab size.
 **/
class DockablePanelTabWidget
    : public QTabWidget
{
public:

    DockablePanelTabWidget(QWidget* parent = 0);

    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
};

class RightClickableWidget : public QWidget
{
    Q_OBJECT
public:
    
    
    RightClickableWidget(QWidget* parent)
    : QWidget(parent)
    {
        
    }
    
    virtual ~RightClickableWidget() {}
    
signals:
    
    void rightClicked(const QPoint& p);

private:
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    
};

/**
 * @brief An abstract class that defines a dockable properties panel that can be found in the Property bin pane.
 **/
struct DockablePanelPrivate;
class DockablePanel
    : public QFrame
{
    Q_OBJECT

public:

    enum HeaderMode
    {
        FULLY_FEATURED = 0,
        READ_ONLY_NAME,
        NO_HEADER
    };

    explicit DockablePanel(Gui* gui
                           ,
                           KnobHolder* holder
                           ,
                           QVBoxLayout* container
                           ,
                           HeaderMode headerMode
                           ,
                           bool useScrollAreasForTabs
                           ,
                           const QString & initialName = QString()
                           ,
                           const QString & helpToolTip = QString()
                           ,
                           bool createDefaultPage = false
                           ,
                           const QString & defaultPageName = QString("Default")
                           ,
                           QWidget *parent = 0);

    virtual ~DockablePanel() OVERRIDE;

    bool isMinimized() const;

    const std::map<boost::shared_ptr<KnobI>,KnobGui*> & getKnobs() const;
    QVBoxLayout* getContainer() const;
    QUndoStack* getUndoStack() const;

    bool isClosed() const;
    bool isFloating() const;

    /*Creates a new button and inserts it in the header
       at position headerPosition. You can then take
       the pointer to the Button to customize it the
       way you want. The ownership of the Button remains
       to the DockablePanel.*/
    Button* insertHeaderButton(int headerPosition);


    void pushUndoCommand(QUndoCommand* cmd);


    const QUndoCommand* getLastUndoCommand() const;
    Gui* getGui() const;

    void insertHeaderWidget(int index,QWidget* widget);

    void appendHeaderWidget(QWidget* widget);

    QWidget* getHeaderWidget() const;
    KnobGui* getKnobGui(const boost::shared_ptr<KnobI> & knob) const;

    ///MT-safe
    QColor getCurrentColor() const;

    ///MT-safe
    void setCurrentColor(const QColor & c);

    virtual boost::shared_ptr<MultiInstancePanel> getMultiInstancePanel() const
    {
        return boost::shared_ptr<MultiInstancePanel>();
    }

    KnobHolder* getHolder() const;

    void onGuiClosing();

public slots:

    /*Internal slot, not meant to be called externally.*/
    void closePanel();

    /*Internal slot, not meant to be called externally.*/
    void minimizeOrMaximize(bool toggled);

    /*Internal slot, not meant to be called externally.*/
    void showHelp();

    /*You can connect to this when you want to change
       the name externally.*/
    void onNameChanged(const QString & str);

    /*initializes the knobs GUI and also the roto context if any*/
    void initializeKnobs();

    /*Internal slot, not meant to be called externally.*/
    void onKnobDeletion();

    /*Internal slot, not meant to be called externally.*/
    void onUndoClicked();

    /*Internal slot, not meant to be called externally.*/
    void onRedoPressed();

    void onRestoreDefaultsButtonClicked();

    void onLineEditNameEditingFinished();

    void floatPanel();

    void onColorButtonClicked();

    void onColorDialogColorChanged(const QColor & color);

    void setClosed(bool closed);

    void onRightClickMenuRequested(const QPoint & pos);
    
    void setKeyOnAllParameters();
    void removeAnimationOnAllParameters();

    void onCenterButtonClicked();

signals:

    /*emitted when the panel is clicked*/
    void selected();

    /*emitted when the name changed on the line edit*/
    void nameChanged(QString);

    /*emitted when the user pressed undo*/
    void undoneChange();

    /*emitted when the user pressed redo*/
    void redoneChange();

    /*emitted when the panel is minimized*/
    void minimized();

    /*emitted when the panel is maximized*/
    void maximized();

    void closeChanged(bool closed);

    void colorChanged(QColor);
    

protected:
    
    /**
     * @brief Called when the "center on..." button is clicked
     **/
    virtual void centerOnItem() {}

    virtual RotoPanel* initializeRotoPanel()
    {
        return NULL;
    }

    virtual void initializeExtraGui(QVBoxLayout* /*layout*/)
    {
    }

private:

    void initializeKnobsInternal( const std::vector< boost::shared_ptr<KnobI> > & knobs);
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        emit selected();
        QFrame::mousePressEvent(e);
    }

    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<DockablePanelPrivate> _imp;
};


class NodeSettingsPanel
    : public DockablePanel
{
    Q_OBJECT Q_PROPERTY( bool _selected READ isSelected WRITE setSelected)

    /*Pointer to the node GUI*/
    boost::shared_ptr<NodeGui> _nodeGUI;
    bool _selected;
    Button* _settingsButton;
    boost::shared_ptr<MultiInstancePanel> _multiPanel;

public:

    explicit NodeSettingsPanel(const boost::shared_ptr<MultiInstancePanel> & multiPanel,
                               Gui* gui,
                               boost::shared_ptr<NodeGui> NodeUi,
                               QVBoxLayout* container,
                               QWidget *parent = 0);

    virtual ~NodeSettingsPanel();

    void setSelected(bool s);

    bool isSelected() const
    {
        return _selected;
    }

    boost::shared_ptr<NodeGui> getNode() const
    {
        return _nodeGUI;
    }

    virtual boost::shared_ptr<MultiInstancePanel> getMultiInstancePanel() const
    {
        return _multiPanel;
    }

private:

    virtual RotoPanel* initializeRotoPanel();
    virtual void initializeExtraGui(QVBoxLayout* layout) OVERRIDE FINAL;
    virtual void centerOnItem() OVERRIDE FINAL;

public slots:
    
    void onSettingsButtonClicked();
    
    void onImportPresetsActionTriggered();
    
    void onExportPresetsActionTriggered();
};

class NodeBackDropSettingsPanel : public DockablePanel
{
    
public:
    
    NodeBackDropSettingsPanel(NodeBackDrop* backdrop,
                              Gui* gui,
                              QVBoxLayout* container,
                              const QString& name,
                              QWidget* parent);
    
    virtual ~NodeBackDropSettingsPanel();
    
private:
    
    virtual void centerOnItem() OVERRIDE FINAL;
    
    NodeBackDrop* _backdrop;
};

#endif // NATRON_GUI_SETTINGSPANEL_H_
