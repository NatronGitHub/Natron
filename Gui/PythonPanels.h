/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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


#ifndef PYTHONPANELS_H
#define PYTHONPANELS_H

#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

#include "Engine/NodeWrapper.h"
#include "Engine/ScriptObject.h"
#include "Engine/Knob.h"

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"


struct DialogParamHolderPrivate;
class DialogParamHolder : public NamedKnobHolder
{
    
public:
    
    DialogParamHolder(const std::string& uniqueID,AppInstance* app);
    
    virtual ~DialogParamHolder();
    
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL;
    
    void setParamChangedCallback(const std::string& callback);
    
    
private:
    
    virtual void initializeKnobs() OVERRIDE FINAL {}
    
    virtual void evaluate(KnobI* /*knob*/,bool /*isSignificant*/,Natron::ValueChangedReasonEnum /*reason*/) OVERRIDE FINAL {}
    
    virtual void onKnobValueChanged(KnobI* /*k*/,
                                    Natron::ValueChangedReasonEnum /*reason*/,
                                    double /*time*/,
                                    bool /*originatedFromMainThread*/) OVERRIDE FINAL;

    boost::scoped_ptr<DialogParamHolderPrivate> _imp;
};




struct PyModalDialogPrivate;
class PyModalDialog : public QDialog, public UserParamHolder
{
    Q_OBJECT
    
public:
    
    PyModalDialog(Gui* gui);
    
    virtual ~PyModalDialog();
    
    Param* getParam(const std::string& scriptName) const;
        
    void setParamChangedCallback(const std::string& callback);
    
    void insertWidget(int index, QWidget* widget);
    
    void addWidget(QWidget* widget);
    
private:
    
    boost::scoped_ptr<PyModalDialogPrivate> _imp;

    
};


struct PyPanelPrivate;
class PyPanel : public QWidget, public UserParamHolder, public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    PyPanel(const std::string& scriptName,const std::string& label,bool useUserParameters,GuiApp* app);
    
    virtual ~PyPanel();
        
    std::string save_serialization_thread() const;
    
    virtual void restore(const std::string& /*data*/) {}
    
    std::string getPanelScriptName() const;
    
    void setPanelLabel(const std::string& label);
    
    std::string getPanelLabel() const;
    
    Param* getParam(const std::string& scriptName) const;
    
    std::list<Param*> getParams() const;
    
    void setParamChangedCallback(const std::string& callback);
    
    void insertWidget(int index, QWidget* widget);
    
    void addWidget(QWidget* widget);
    
protected:
    
    virtual std::string save() { return std::string(); }
    
    void onUserDataChanged();
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE;
    virtual void enterEvent(QEvent* e) OVERRIDE ;
    virtual void leaveEvent(QEvent* e) OVERRIDE ;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE ;
private:
    boost::scoped_ptr<PyPanelPrivate> _imp;
    
};


class PyTabWidget {

    TabWidget* _tab;
    
public :
    
    PyTabWidget(TabWidget* pane);
    
    ~PyTabWidget();
    
    TabWidget* getInternalTabWidget() const {
        return _tab;
    }
    
    bool appendTab(PyPanel* tab);
    
    void insertTab(int index,PyPanel* tab);
    
    void removeTab(QWidget* tab);
    
    void removeTab(int index);
    
    void closeTab(int index);
    
    std::string getTabLabel(int index) const;
    
    int count();
    
    QWidget* currentWidget();
    
    void setCurrentIndex(int index);
    
    int getCurrentIndex() const;
    
    PyTabWidget* splitHorizontally();
    
    PyTabWidget* splitVertically();
    
    void closePane();
    
    void floatPane();
    
    void setNextTabCurrent();
    
    void floatCurrentTab();
    
    void closeCurrentTab();
    
    std::string getScriptName() const;
};

#endif // PYTHONPANELS_H
