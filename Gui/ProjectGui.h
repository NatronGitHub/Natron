//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#ifndef PROJECTGUI_H
#define PROJECTGUI_H

#include <QObject>
#include <QDialog>
#include <boost/shared_ptr.hpp>
#include "Engine/Format.h"
class Button;
class QWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class ComboBox;
class SpinBox;
class LineEdit;
class DockablePanel;
class ProjectGuiSerialization;
class NodeGuiSerialization;
namespace Natron{
    class Project;
}

class ProjectGui : public QObject
{
    Q_OBJECT
    
public:
    
    ProjectGui();
        
    void create(boost::shared_ptr<Natron::Project> projectInternal,QVBoxLayout* container,QWidget* parent = NULL);
    
    void loadProject(const QString& path,const QString& name);
    
    void saveProject(const QString& path,const QString& filename,bool autoSave = false);
    
    bool isVisible() const;
    
    DockablePanel* getPanel() const { return _panel; }
    
    boost::shared_ptr<Natron::Project> getInternalProject() const { return _project; }
    
    void save(ProjectGuiSerialization* serializationObject) const;
    
    void load(const ProjectGuiSerialization& obj);
    
public slots:
    
    void createNewFormat();

    void setVisible(bool visible);

private:
    
        
    boost::shared_ptr<Natron::Project> _project;
    DockablePanel* _panel;
    
    bool _created;
};


class AddFormatDialog : public QDialog {
    
    Q_OBJECT
    
public:
    
    AddFormatDialog(Natron::Project* project,QWidget* parent = 0);
    
    virtual ~AddFormatDialog(){}
    
    Format getFormat() const ;
    
    public slots:
    
    void onCopyFromViewer();
    
private:
    Natron::Project* _project;
    
    QVBoxLayout* _mainLayout;
    
    QWidget* _fromViewerLine;
    QHBoxLayout* _fromViewerLineLayout;
    Button* _copyFromViewerButton;
    ComboBox* _copyFromViewerCombo;
    
    QWidget* _parametersLine;
    QHBoxLayout* _parametersLineLayout;
    QLabel* _widthLabel;
    SpinBox* _widthSpinBox;
    QLabel* _heightLabel;
    SpinBox* _heightSpinBox;
    QLabel* _pixelAspectLabel;
    SpinBox* _pixelAspectSpinBox;
    
    
    QWidget* _formatNameLine;
    QHBoxLayout* _formatNameLayout;
    QLabel* _nameLabel;
    LineEdit* _nameLineEdit;
    
    QWidget* _buttonsLine;
    QHBoxLayout* _buttonsLineLayout;
    Button* _cancelButton;
    Button* _okButton;
};



#endif // PROJECTGUI_H
