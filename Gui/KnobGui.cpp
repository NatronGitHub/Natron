//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobGui.h"

#ifdef __NATRON_UNIX__
#include <dlfcn.h>
#endif
#include <climits>
#include <cfloat>
#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QTextEdit>
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QKeyEvent>
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>


#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"

#include "Engine/Node.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Settings.h"
#include "Engine/Knob.h"

#include "Gui/Button.h"
#include "Gui/DockablePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/TimeLineGui.h"
#include "Gui/Gui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/SpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/LineEdit.h"
#include "Gui/ScaleSlider.h"

#include "Readers/Reader.h"

using namespace Natron;
using std::make_pair;

KnobGui::KnobGui(Knob* knob,DockablePanel* container):
_knob(knob),
_triggerNewLine(true),
_spacingBetweenItems(0),
_widgetCreated(false),
_container(container)
{
    QObject::connect(knob,SIGNAL(valueChanged(const Variant&)),this,SLOT(onInternalValueChanged(const Variant&)));
    QObject::connect(this,SIGNAL(valueChanged(const Variant&)),knob,SLOT(onValueChanged(const Variant&)));
    QObject::connect(knob,SIGNAL(visible(bool)),this,SLOT(setVisible(bool)));
    QObject::connect(knob,SIGNAL(enabled(bool)),this,SLOT(setEnabled(bool)));
    QObject::connect(knob,SIGNAL(deleted()),this,SLOT(deleteKnob()));
    QObject::connect(this, SIGNAL(knobUndoneChange()), knob, SLOT(onKnobUndoneChange()));
    QObject::connect(this, SIGNAL(knobRedoneChange()), knob, SLOT(onKnobRedoneChange()));
}

KnobGui::~KnobGui(){
    
    emit deleted(this);
}

void KnobGui::pushUndoCommand(QUndoCommand* cmd){
    if(_knob->canBeUndone() && !_knob->isInsignificant()){
        _container->pushUndoCommand(cmd);
    }else{
        cmd->redo();
    }
}



void KnobGui::moveToLayout(QVBoxLayout* layout){
    QWidget* container = new QWidget(layout->parentWidget());
    QHBoxLayout* containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    addToLayout(containerLayout);
    layout->addWidget(container);
}

//================================================================

void KnobUndoCommand::undo(){
    _knob->setValue(_oldValue);
    emit knobUndoneChange();
    setText(QObject::tr("Change %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
}
void KnobUndoCommand::redo(){
    _knob->setValue(_newValue);
    emit knobRedoneChange();
    setText(QObject::tr("Change %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
}
bool KnobUndoCommand::mergeWith(const QUndoCommand *command){
    const KnobUndoCommand *knobCommand = static_cast<const KnobUndoCommand*>(command);
    KnobGui* knob = knobCommand->_knob;
    if(_knob != knob)
        return false;
    _newValue = knob->getKnob()->getValueAsVariant();
    setText(QObject::tr("Change %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
    return true;
}

//===========================FILE_KNOB_GUI=====================================
File_KnobGui::File_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container){
    File_Knob* fileKnob = dynamic_cast<File_Knob*>(knob);
    QObject::connect(fileKnob,SIGNAL(shouldOpenFile()),this,SLOT(open_file()));
}
File_KnobGui::~File_KnobGui(){
    delete _lineEdit;
    delete _descriptionLabel;
    delete _openFileButton;
}
void File_KnobGui::createWidget(QGridLayout* layout,int row){
    
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0,Qt::AlignRight);

    _lineEdit = new LineEdit(layout->parentWidget());
    _lineEdit->setPlaceholderText("File path...");
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _lineEdit->setToolTip(_knob->getHintToolTip().c_str());
    QObject::connect(_lineEdit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
	
    _openFileButton = new Button(layout->parentWidget());
    
    QImage img(NATRON_IMAGES_PATH"open-file.png");
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(15,15);
    _openFileButton->setIcon(QIcon(pix));
    _openFileButton->setFixedSize(20, 20);
    QObject::connect(_openFileButton,SIGNAL(clicked()),this,SLOT(open_file()));
    
    
    QWidget* container = new QWidget(layout->parentWidget());
    QHBoxLayout* containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);
    
    layout->addWidget(container,row,1);
}

void File_KnobGui::updateGUI(const Variant& variant){
    std::string pattern = SequenceFileDialog::patternFromFilesList(variant.toStringList()).toStdString();
    _lineEdit->setText(pattern.c_str());
}

void File_KnobGui::open_file(){
    
    QStringList oldList,filesList;
    oldList = SequenceFileDialog::filesListFromPattern(_lineEdit->text());
    std::vector<std::string> filters = appPTR->getCurrentSettings()._readersSettings.supportedFileTypes();
    
    SequenceFileDialog dialog(_lineEdit->parentWidget(),filters,true,SequenceFileDialog::OPEN_DIALOG,_lastOpened.toStdString());
    if(dialog.exec()){
        filesList = dialog.selectedFiles();
    }
    if(!filesList.isEmpty()){
        updateLastOpened(filesList.at(0));
        pushUndoCommand(new KnobUndoCommand(this,Variant(oldList),Variant(filesList)));
    }
}

void File_KnobGui::onReturnPressed(){
    QString str= _lineEdit->text();
    if(str.isEmpty())
        return;
    QStringList newList = SequenceFileDialog::filesListFromPattern(str);
    if(newList.isEmpty())
        return;
    QStringList oldList = dynamic_cast<File_Knob*>(_knob)->value<QStringList>();
    pushUndoCommand(new KnobUndoCommand(this,Variant(oldList),Variant(newList)));
}
void File_KnobGui::updateLastOpened(const QString& str){
    QString withoutPath = SequenceFileDialog::removePath(str);
    int pos = str.indexOf(withoutPath);
    _lastOpened = str.left(pos);
}

void File_KnobGui::hide(){
    _openFileButton->hide();
    _descriptionLabel->hide();
    _lineEdit->hide();
}

void File_KnobGui::show(){
    _openFileButton->show();
    _descriptionLabel->show();
    _lineEdit->show();
}
void File_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_lineEdit);
    layout->addWidget(_openFileButton);
}
//============================OUTPUT_FILE_KNOB_GUI====================================
OutputFile_KnobGui::OutputFile_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container){
    OutputFile_Knob* fileKnob = dynamic_cast<OutputFile_Knob*>(knob);
    QObject::connect(fileKnob,SIGNAL(shouldOpenFile()),this,SLOT(open_file()));
}

OutputFile_KnobGui::~OutputFile_KnobGui(){
    delete _descriptionLabel;
    delete _lineEdit;
    delete _openFileButton;
}
void OutputFile_KnobGui::createWidget(QGridLayout *layout, int row){
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);

    _lineEdit = new LineEdit(layout->parentWidget());
    _lineEdit->setPlaceholderText(QString("File path..."));
    _lineEdit->setToolTip(_knob->getHintToolTip().c_str());
	_lineEdit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    _openFileButton = new Button(layout->parentWidget());
    
    QImage img(NATRON_IMAGES_PATH"open-file.png");
    QPixmap pix=QPixmap::fromImage(img);
    pix = pix.scaled(15,15);
    _openFileButton->setIcon(QIcon(pix));
    _openFileButton->setFixedSize(20,20);
    QObject::connect(_openFileButton,SIGNAL(clicked()),this,SLOT(open_file()));
    
    
    QWidget* container = new QWidget(layout->parentWidget());
    QHBoxLayout* containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);
    
    layout->addWidget(container,row,1);
}



void OutputFile_KnobGui::updateGUI(const Variant& variant){
    _lineEdit->setText(variant.toString());
}
void OutputFile_KnobGui::open_file(){
    std::vector<std::string> filters = appPTR->getCurrentSettings()._readersSettings.supportedFileTypes();
    SequenceFileDialog dialog(_lineEdit->parentWidget(),filters,true,SequenceFileDialog::SAVE_DIALOG,_lastOpened.toStdString());
    if(dialog.exec()){
        QString oldPattern = _knob->value<QString>();
        QString newPattern = dialog.filesToSave();
        updateLastOpened(SequenceFileDialog::removePath(oldPattern));
        pushUndoCommand(new KnobUndoCommand(this,Variant(oldPattern),Variant(newPattern)));
    }
}



void OutputFile_KnobGui::onReturnPressed(){
    QString oldPattern = _knob->value<QString>();
    QString newPattern= _lineEdit->text();
    pushUndoCommand(new KnobUndoCommand(this,Variant(oldPattern),Variant(newPattern)));
}


void OutputFile_KnobGui::updateLastOpened(const QString& str){
    QString withoutPath = SequenceFileDialog::removePath(str);
    int pos = str.indexOf(withoutPath);
    _lastOpened = str.left(pos);
}
void OutputFile_KnobGui::hide(){
    _openFileButton->hide();
    _descriptionLabel->hide();
    _lineEdit->hide();
}

void OutputFile_KnobGui::show(){
    _openFileButton->show();
    _descriptionLabel->show();
    _lineEdit->show();
}
void OutputFile_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_lineEdit);
    layout->addWidget(_openFileButton);
}
//==========================INT_KNOB_GUI======================================
Int_KnobGui::Int_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container),_slider(0){
    Int_Knob* intKnob = dynamic_cast<Int_Knob*>(_knob);
    assert(intKnob);
    QObject::connect(intKnob, SIGNAL(minMaxChanged(int,int,int)), this, SLOT(onMinMaxChanged(int, int,int)));
    QObject::connect(intKnob, SIGNAL(incrementChanged(int,int)), this, SLOT(onIncrementChanged(int,int)));
}
Int_KnobGui::~Int_KnobGui(){
    delete _descriptionLabel;
    for(U32 i  = 0 ; i < _spinBoxes.size(); ++i){
        delete _spinBoxes[i].first;
        delete _spinBoxes[i].second;
    }
    if(_slider){
        delete _slider;
    }
    
}
void Int_KnobGui::createWidget(QGridLayout *layout, int row){
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);
    
    int dim = _knob->getDimension();
    
    QWidget* container = new QWidget(layout->parentWidget());
    QHBoxLayout* containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    Int_Knob* intKnob = dynamic_cast<Int_Knob*>(_knob);
    assert(intKnob);
    
    QString subLabels_XYZW[] = {"x:","y:","z:","w:"};
    
    const std::vector<int>& maximums = intKnob->getMaximums();
    const std::vector<int>& minimums = intKnob->getMinimums();
    const std::vector<int>& increments = intKnob->getIncrements();
    const std::vector<int>& displayMins = intKnob->getDisplayMinimums();
    const std::vector<int>& displayMaxs = intKnob->getDisplayMaximums();
    for (int i = 0; i < dim; ++i) {
        QWidget* boxContainer = new QWidget(layout->parentWidget());
        QHBoxLayout* boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* subDesc = 0;
        if(dim != 1){
            subDesc = new QLabel(subLabels_XYZW[i],boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox* box = new SpinBox(layout->parentWidget(),SpinBox::INT_SPINBOX);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        if(maximums.size() > (U32)i)
            box->setMaximum(maximums[i]);
        if(minimums.size() > (U32)i)
            box->setMinimum(minimums[i]);
        if(increments.size() > (U32)i)
            box->setIncrement(increments[i]);
        box->setToolTip(_knob->getHintToolTip().c_str());
        boxContainerLayout->addWidget(box);
        if(_knob->getDimension() == 1){
            int min=0,max=99;
            if(displayMins.size() >(U32)i && displayMins[i] != INT_MIN)
                min = displayMins[i];
            else if(minimums.size() > (U32)i)
                min = minimums[i];
            if(displayMaxs.size() >(U32)i && displayMaxs[i] != INT_MAX)
                max = displayMaxs[i];
            else if(maximums.size() > (U32)i)
                max = maximums[i];
            _slider = new ScaleSlider(min,max,
                                      100,
                                      _knob->value<int>());
            _slider->setToolTip(_knob->getHintToolTip().c_str());
            QObject::connect(_slider, SIGNAL(positionChanged(double)), this, SLOT(onSliderValueChanged(double)));
            boxContainerLayout->addWidget(_slider);
            
        }
        
        containerLayout->addWidget(boxContainer);
        _spinBoxes.push_back(make_pair(box,subDesc));
    }
    
    layout->addWidget(container,row,1,Qt::AlignLeft);
}
void Int_KnobGui::onMinMaxChanged(int mini,int maxi,int index){
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
}
void Int_KnobGui::onIncrementChanged(int incr,int index){
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setIncrement(incr);
}
void Int_KnobGui::updateGUI(const Variant& variant){
    if(_knob->getDimension() > 1){
        QList<QVariant> list = variant.toList();
        if(list.isEmpty()){
            return;
        }
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            _spinBoxes[i].first->setValue(list.at(i).toInt());
        }
    }else{
        int value = variant.toInt();
        _spinBoxes[0].first->setValue(value);
        _slider->seekScalePosition(value);
    }
}
void Int_KnobGui::onSliderValueChanged(double d){
    assert(_knob->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    Variant v;
    v.setValue(d);
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),v));
}
void Int_KnobGui::onSpinBoxValueChanged(){
    Variant v;
    if(_knob->getDimension() > 1){
        QList<QVariant> list;
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            list << QVariant(_spinBoxes[i].first->value());
        }
        v.setValue(list);
    }else{
        v.setValue(_spinBoxes[0].first->value());
    }
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),v));
}
void Int_KnobGui::hide(){
    _descriptionLabel->hide();
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->hide();
        if(_spinBoxes[i].second)
            _spinBoxes[i].second->hide();
    }
    if(_slider){
        _slider->hide();
    }
}

void Int_KnobGui::show(){
    _descriptionLabel->show();
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->show();
        if(_spinBoxes[i].second)
            _spinBoxes[i].second->show();
    }
    if(_slider){
        _slider->show();
    }
}

void Int_KnobGui::setEnabled(bool b){
    _descriptionLabel->setEnabled(b);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if(_spinBoxes[i].second)
            _spinBoxes[i].second->setEnabled(b);
    }
    if(_slider){
        _slider->setEnabled(b);
    }
    
}
void Int_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        layout->addWidget(_spinBoxes[i].first);
        if(_spinBoxes[i].second)
            layout->addWidget(_spinBoxes[i].second);
    }
    if(_slider){
        layout->addWidget(_slider);
    }
    
}
//==========================BOOL_KNOB_GUI======================================

void Bool_KnobGui::createWidget(QGridLayout *layout, int row){
    _descriptionLabel = new ClickableLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);

    _checkBox = new QCheckBox(layout->parentWidget());
    _checkBox->setToolTip(_knob->getHintToolTip().c_str());
    QObject::connect(_checkBox,SIGNAL(clicked(bool)),this,SLOT(onCheckBoxStateChanged(bool)));
    QObject::connect(_descriptionLabel,SIGNAL(clicked(bool)),this,SLOT(onCheckBoxStateChanged(bool)));
    layout->addWidget(_checkBox,row,1,Qt::AlignLeft);
}
Bool_KnobGui::~Bool_KnobGui(){
    delete _descriptionLabel;
    delete _checkBox;
}
void Bool_KnobGui::updateGUI(const Variant& variant){
    bool b = variant.toBool();
    _checkBox->setChecked(b);
    _descriptionLabel->setClicked(b);
}


void Bool_KnobGui::onCheckBoxStateChanged(bool b){
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),Variant(b)));
}
void Bool_KnobGui::hide(){
    _descriptionLabel->hide();
    _checkBox->hide();
}

void Bool_KnobGui::show(){
    _descriptionLabel->show();
    _checkBox->show();
}
void Bool_KnobGui::setEnabled(bool b){
    _descriptionLabel->setEnabled(b);
    _checkBox->setEnabled(b);
}
void Bool_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_checkBox);
}
//=============================DOUBLE_KNOB_GUI===================================
Double_KnobGui::Double_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container),_slider(0){
    Double_Knob* dbl_knob = dynamic_cast<Double_Knob*>(_knob);
    assert(dbl_knob);
    QObject::connect(dbl_knob, SIGNAL(minMaxChanged(double,double,int)), this, SLOT(onMinMaxChanged(double, double,int)));
    QObject::connect(dbl_knob, SIGNAL(incrementChanged(double,int)), this, SLOT(onIncrementChanged(double,int)));
    QObject::connect(dbl_knob, SIGNAL(decimalsChanged(int,int)), this, SLOT(onDecimalsChanged(int,int)));
}

Double_KnobGui::~Double_KnobGui(){
    delete _descriptionLabel;
    for(U32 i  = 0 ; i < _spinBoxes.size(); ++i){
        delete _spinBoxes[i].first;
        delete _spinBoxes[i].second;
    }
    if(_slider){
        delete _slider;
    }
}

bool compareDoubles(double a,double b){
    return fabs(a-b) < 0.001;//std::numeric_limits<double>::epsilon();
}

void Double_KnobGui::createWidget(QGridLayout *layout, int row){
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);
    
    QWidget* container = new QWidget(layout->parentWidget());
    QHBoxLayout* containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    Double_Knob* dbl_knob = dynamic_cast<Double_Knob*>(_knob);
    assert(dbl_knob);
    int dim = _knob->getDimension();
    
    QString subLabels_XYZW[] = {"x:","y:","z:","w:"};
    
    const std::vector<double>& maximums = dbl_knob->getMaximums();
    const std::vector<double>& minimums = dbl_knob->getMinimums();
    const std::vector<double>& increments = dbl_knob->getIncrements();
    const std::vector<double>& displayMins = dbl_knob->getDisplayMinimums();
    const std::vector<double>& displayMaxs = dbl_knob->getDisplayMaximums();
    
    const std::vector<int>& decimals = dbl_knob->getDecimals();
    for (int i = 0; i < dim; ++i) {
        QWidget* boxContainer = new QWidget(layout->parentWidget());
        QHBoxLayout* boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* subDesc = 0;
        if(dim != 1){
            subDesc = new QLabel(subLabels_XYZW[i],boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox* box = new SpinBox(layout->parentWidget(),SpinBox::DOUBLE_SPINBOX);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        if((int)maximums.size() > i)
            box->setMaximum(maximums[i]);
        if((int)minimums.size() > i)
            box->setMinimum(minimums[i]);
        if((int)decimals.size() > i)
            box->decimals(decimals[i]);
        if((int)increments.size() > i){
            double incr = increments[i];
            if(incr > 0)
                box->setIncrement(incr);
        }
        box->setToolTip(_knob->getHintToolTip().c_str());
        boxContainerLayout->addWidget(box);
        if(_knob->getDimension() == 1){
            double min=0.,max=99.;
            if(displayMins.size() >(U32)i)
                min = displayMins[i];
            else if(minimums.size() > (U32)i)
                min = minimums[i];
            if(displayMaxs.size() >(U32)i)
                max = displayMaxs[i];
            else if(maximums.size() > (U32)i)
                max = maximums[i];
            _slider = new ScaleSlider(min,max,
                                      100,
                                      _knob->value<double>());
            _slider->setToolTip(_knob->getHintToolTip().c_str());
            QObject::connect(_slider, SIGNAL(positionChanged(double)), this, SLOT(onSliderValueChanged(double)));
            boxContainerLayout->addWidget(_slider);
        }
        
        containerLayout->addWidget(boxContainer);
        //        layout->addWidget(boxContainer,row,i+1+columnOffset);
        _spinBoxes.push_back(make_pair(box,subDesc));
    }
    layout->addWidget(container,row,1,Qt::AlignLeft);
}
void Double_KnobGui::onMinMaxChanged(double mini,double maxi,int index){
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
}
void Double_KnobGui::onIncrementChanged(double incr,int index){
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setIncrement(incr);
}
void Double_KnobGui::onDecimalsChanged(int deci,int index){
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->decimals(deci);
}

void Double_KnobGui::updateGUI(const Variant& variant){
    if(_knob->getDimension() > 1){
        QList<QVariant> list = variant.toList();
        if(list.isEmpty()){
            return;
        }
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            _spinBoxes[i].first->setValue(list.at(i).toDouble());
        }
    }else{
        double value = variant.toDouble();
        _spinBoxes[0].first->setValue(value);
        _slider->seekScalePosition(value);
    }
}

void Double_KnobGui::onSliderValueChanged(double d){
    assert(_knob->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    Variant v;
    v.setValue(d);
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),v));
}
void Double_KnobGui::onSpinBoxValueChanged(){
    Variant v;
    if(_knob->getDimension() > 1){
        QList<QVariant> list;
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            list << QVariant(_spinBoxes[i].first->value());
        }
        v.setValue(list);
    }else{
        v.setValue(_spinBoxes[0].first->value());
    }
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),v));
}
void Double_KnobGui::hide(){
    _descriptionLabel->hide();
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->hide();
        if(_spinBoxes[i].second)
            _spinBoxes[i].second->hide();
    }
    if(_slider){
        _slider->hide();
    }
    
}

void Double_KnobGui::show(){
    _descriptionLabel->show();
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->show();
        if(_spinBoxes[i].second)
            _spinBoxes[i].second->show();
    }
    if(_slider){
        _slider->show();
    }
    
}
void Double_KnobGui::setEnabled(bool b){
    _descriptionLabel->setEnabled(b);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if(_spinBoxes[i].second)
            _spinBoxes[i].second->setEnabled(b);
    }
    if(_slider){
        _slider->setEnabled(b);
    }
    
}
void Double_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        layout->addWidget(_spinBoxes[i].first);
        if(_spinBoxes[i].second)
            layout->addWidget(_spinBoxes[i].second);
    }
    if(_slider){
        layout->addWidget(_slider);
    }
}

//=============================BUTTON_KNOB_GUI===================================
void Button_KnobGui::createWidget(QGridLayout *layout, int row){
    _button = new Button(QString(QString(_knob->getDescription().c_str())),layout->parentWidget());
    QObject::connect(_button, SIGNAL(pressed()),this,SLOT(emitValueChanged()));
    _button->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_button,row,0,Qt::AlignRight);
}

Button_KnobGui::~Button_KnobGui(){
    delete _button;
}

void Button_KnobGui::emitValueChanged(){
    emit valueChanged(Variant());
}
void Button_KnobGui::hide(){
    _button->hide();
}

void Button_KnobGui::show(){
    _button->show();
}
void Button_KnobGui::setEnabled(bool b){
    _button->setEnabled(b);
}
void Button_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_button);
}

//=============================COMBOBOX_KNOB_GUI===================================
ComboBox_KnobGui::ComboBox_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container){
    ComboBox_Knob* cbKnob = dynamic_cast<ComboBox_Knob*>(knob);
    _entries = cbKnob->getEntries();
    QObject::connect(cbKnob,SIGNAL(populated()),this,SLOT(onEntriesPopulated()));
}
ComboBox_KnobGui::~ComboBox_KnobGui(){
    delete _comboBox;
    delete _descriptionLabel;
}
void ComboBox_KnobGui::createWidget(QGridLayout *layout, int row) {
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);

    _comboBox = new ComboBox(layout->parentWidget());
    
    const std::vector<std::string>& help =  dynamic_cast<ComboBox_Knob*>(_knob)->getEntriesHelp();
    for (U32 i = 0; i < _entries.size(); ++i) {
        std::string helpStr = help.empty() ? "" : help[i];
        _comboBox->addItem(_entries[i].c_str(),QIcon(),QKeySequence(),QString(helpStr.c_str()));
    }
    if(_entries.size() > 0)
        _comboBox->setCurrentText(_entries[0].c_str());
    _comboBox->setToolTip(_knob->getHintToolTip().c_str());
    QObject::connect(_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
    layout->addWidget(_comboBox,row,1,Qt::AlignLeft);
}
void ComboBox_KnobGui::onCurrentIndexChanged(int i){
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),Variant(i)));
    
}
void ComboBox_KnobGui::onEntriesPopulated(){
    int i = _comboBox->activeIndex();
    _comboBox->clear();
    const std::vector<std::string> entries = dynamic_cast<ComboBox_Knob*>(_knob)->getEntries();
    _entries = entries;
    for (U32 j = 0; j < _entries.size(); ++j) {
        _comboBox->addItem(_entries[j].c_str());
    }
    _comboBox->setCurrentText(_entries[i].c_str());
}

void ComboBox_KnobGui::updateGUI(const Variant& variant){
    int i = variant.toInt();
    assert(i < (int)_entries.size());
    _comboBox->setCurrentText(_entries[i].c_str());
}
void ComboBox_KnobGui::hide(){
    _descriptionLabel->hide();
    _comboBox->hide();
}

void ComboBox_KnobGui::show(){
    _descriptionLabel->show();
    _comboBox->show();
}
void ComboBox_KnobGui::setEnabled(bool b){
    _descriptionLabel->setEnabled(b);
    _comboBox->setEnabled(b);
}
void ComboBox_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_comboBox);
    
}

//=============================SEPARATOR_KNOB_GUI===================================
void Separator_KnobGui::createWidget(QGridLayout* layout,int row){
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);

    _line = new QFrame(layout->parentWidget());
    _line->setFrameShape(QFrame::HLine);
    _line->setFrameShadow(QFrame::Sunken);
    _line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(_line,row,1,Qt::AlignLeft);
}

Separator_KnobGui::~Separator_KnobGui(){
    delete _descriptionLabel;
    delete _line;
}
void Separator_KnobGui::hide(){
    _descriptionLabel->hide();
    _line->hide();
}

void Separator_KnobGui::show(){
    _descriptionLabel->show();
    _line->show();
}


void Separator_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_line);
    
}
//=============================RGBA_KNOB_GUI===================================

RGBA_KnobGui::~RGBA_KnobGui(){
    delete mainContainer;
}

void RGBA_KnobGui::createWidget(QGridLayout* layout,int row) {
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);

    mainContainer = new QWidget(layout->parentWidget());
    mainLayout = new QHBoxLayout(mainContainer);
    mainContainer->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    boxContainers = new QWidget(mainContainer);
    boxLayout = new QHBoxLayout(boxContainers);
    boxContainers->setLayout(boxLayout);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setSpacing(0);
    
    
    _rBox = new SpinBox(boxContainers,SpinBox::DOUBLE_SPINBOX);
    QObject::connect(_rBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    _gBox = new SpinBox(boxContainers,SpinBox::DOUBLE_SPINBOX);
    QObject::connect(_gBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    _bBox = new SpinBox(boxContainers,SpinBox::DOUBLE_SPINBOX);
    QObject::connect(_bBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    _aBox = new SpinBox(boxContainers,SpinBox::DOUBLE_SPINBOX);
    QObject::connect(_aBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    
    
    _rBox->setMaximum(1.);
    _rBox->setMinimum(0.);
    _rBox->setIncrement(0.1);

    _gBox->setMaximum(1.);
    _gBox->setMinimum(0.);
    _gBox->setIncrement(0.1);

    _bBox->setMaximum(1.);
    _bBox->setMinimum(0.);
    _bBox->setIncrement(0.1);

    _aBox->setMaximum(1.);
    _aBox->setMinimum(0.);
    _aBox->setIncrement(0.1);

    _rBox->setToolTip(_knob->getHintToolTip().c_str());
    _gBox->setToolTip(_knob->getHintToolTip().c_str());
    _bBox->setToolTip(_knob->getHintToolTip().c_str());
    _aBox->setToolTip(_knob->getHintToolTip().c_str());
    
    _rLabel = new QLabel("r:",boxContainers);
    _rLabel->setToolTip(_knob->getHintToolTip().c_str());
    _gLabel = new QLabel("g:",boxContainers);
    _gLabel->setToolTip(_knob->getHintToolTip().c_str());
    _bLabel = new QLabel("b:",boxContainers);
    _bLabel->setToolTip(_knob->getHintToolTip().c_str());
    _aLabel = new QLabel("a:",boxContainers);
    _aLabel->setToolTip(_knob->getHintToolTip().c_str());

    boxLayout->addWidget(_rLabel);
    boxLayout->addWidget(_rBox);
    boxLayout->addWidget(_gLabel);
    boxLayout->addWidget(_gBox);
    boxLayout->addWidget(_bLabel);
    boxLayout->addWidget(_bBox);
    boxLayout->addWidget(_aLabel);
    boxLayout->addWidget(_aBox);
    
    colorContainer = new QWidget(mainContainer);
    colorLayout = new QHBoxLayout(colorContainer);
    colorContainer->setLayout(colorLayout);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(0);
    
    _colorLabel = new QLabel(colorContainer);
    _colorLabel->setToolTip(_knob->getHintToolTip().c_str());
    colorLayout->addWidget(_colorLabel);
    
    QImage buttonImg(NATRON_IMAGES_PATH"colorwheel.png");
    QPixmap buttonPix = QPixmap::fromImage(buttonImg);
    buttonPix = buttonPix.scaled(25, 20);
    QIcon buttonIcon(buttonPix);
    _colorDialogButton = new Button(buttonIcon,"",colorContainer);
    QObject::connect(_colorDialogButton, SIGNAL(pressed()), this, SLOT(showColorDialog()));
    colorLayout->addWidget(_colorDialogButton);
    
    mainLayout->addWidget(boxContainers);
    mainLayout->addWidget(colorContainer);
    
    layout->addWidget(mainContainer,row,1,Qt::AlignLeft);
    
    if (!dynamic_cast<RGBA_Knob*>(_knob)->isAlphaEnabled()) {
        disablePermantlyAlpha();
    }
    
}
void RGBA_KnobGui::setEnabled(bool b){
    _rBox->setEnabled(b);
    _gBox->setEnabled(b);
    _bBox->setEnabled(b);
    _aBox->setEnabled(b);
    _colorDialogButton->setEnabled(b);
}
void RGBA_KnobGui::updateGUI(const Variant& variant){
    QVector4D v = variant.value<QVector4D>();
    _rBox->setValue(v.x());
    _gBox->setValue(v.y());
    _bBox->setValue(v.z());
    _aBox->setValue(v.w());
    
    uchar r = (uchar)std::min(v.x()*256., 255.);
    uchar g = (uchar)std::min(v.y()*256., 255.);
    uchar b = (uchar)std::min(v.z()*256., 255.);
    uchar a = (uchar)std::min(v.w()*256., 255.);
    QColor color(r, g, b, a);
    updateLabel(color);
}
void RGBA_KnobGui::showColorDialog(){
    QColorDialog dialog(_rBox->parentWidget());
    if(dialog.exec()){
        QColor color = dialog.getColor();
        updateLabel(color);
        _rBox->setValue(color.redF());
        _gBox->setValue(color.greenF());
        _bBox->setValue(color.blueF());
        if(_alphaEnabled)
            _aBox->setValue(color.alphaF());
        else
            _aBox->setValue(1.0);
        
        onColorChanged();
    }
}

void RGBA_KnobGui::onColorChanged(){
    QColor color;
    color.setRedF(_rBox->value());
    color.setGreenF(_gBox->value());
    color.setBlueF(_bBox->value());
    color.setAlphaF(_aBox->value());
    QVector4D oldValues = _knob->getValueAsVariant().value<QVector4D>();
    QVector4D newValues;
    newValues.setX(color.redF());
    newValues.setY(color.greenF());
    newValues.setZ(color.blueF());
    if(!_alphaEnabled){
        newValues.setW(1.0);
    }else{
        newValues.setW(color.alphaF());
    }
    pushUndoCommand(new KnobUndoCommand(this,Variant(oldValues),Variant(newValues)));
}


void RGBA_KnobGui::updateLabel(const QColor& color){
    QImage img(20,20,QImage::Format_RGB32);
    img.fill(color.rgb());
    QPixmap pix=QPixmap::fromImage(img);
    _colorLabel->setPixmap(pix);
}

void RGBA_KnobGui::disablePermantlyAlpha(){
    _alphaEnabled = false;
    boxLayout->removeWidget(_aLabel);
    boxLayout->removeWidget(_aBox);
    boxLayout->update();
    _aLabel->setParent(NULL);
    _aBox->setParent(NULL);
    _aLabel->setVisible(false);
    _aLabel->setEnabled(false);
    _aBox->setVisible(false);
    _aBox->setEnabled(false);
}
void RGBA_KnobGui::hide(){
    _descriptionLabel->hide();
    _rBox->hide();
    _rLabel->hide();
    _gBox->hide();
    _gLabel->hide();
    _bBox->hide();
    _bLabel->hide();
    _aBox->hide();
    _aLabel->hide();
    
    _colorLabel->hide();
    _colorDialogButton->hide();
}

void RGBA_KnobGui::show(){
    _descriptionLabel->show();
    
    _rBox->show();
    _rLabel->show();
    _gBox->show();
    _gLabel->show();
    _bBox->show();
    _bLabel->show();
    if(_alphaEnabled){
        _aBox->show();
        _aLabel->show();
    }
    
    _colorLabel->show();
    _colorDialogButton->show();
    
}
void RGBA_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    
    layout->addWidget(_rLabel);
    layout->addWidget(_rBox);
    layout->addWidget(_gLabel);
    layout->addWidget(_gBox);
    layout->addWidget(_bLabel);
    layout->addWidget(_bBox);
    if(_alphaEnabled){
        layout->addWidget(_aLabel);
        layout->addWidget(_aBox);
    }
    
    layout->addWidget(_colorLabel);
    layout->addWidget(_colorDialogButton);
}


//=============================STRING_KNOB_GUI===================================
void String_KnobGui::createWidget(QGridLayout *layout, int row) {
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);

    _lineEdit = new LineEdit(layout->parentWidget());
    _lineEdit->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_lineEdit,row,1,Qt::AlignLeft);
    QObject::connect(_lineEdit, SIGNAL(textEdited(QString)), this, SLOT(onStringChanged(QString)));
    
}

String_KnobGui::~String_KnobGui(){
    delete _descriptionLabel;
    delete _lineEdit;
}

void String_KnobGui::onStringChanged(const QString& str){
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),Variant(str)));
}
void String_KnobGui::updateGUI(const Variant& variant){
    _lineEdit->setText(variant.toString());
}
void String_KnobGui::hide(){
    _descriptionLabel->hide();
    _lineEdit->hide();
}

void String_KnobGui::show(){
    _descriptionLabel->show();
    _lineEdit->show();
}
void String_KnobGui::setEnabled(bool b){
    _descriptionLabel->setEnabled(b);
    _lineEdit->setEnabled(b);
}

void String_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_lineEdit);
    
}
//=============================GROUP_KNOB_GUI===================================
GroupBoxLabel::GroupBoxLabel(QWidget* parent):
QLabel(parent),
_checked(false){
    
    QObject::connect(this, SIGNAL(checked(bool)), this, SLOT(setChecked(bool)));
}

void GroupBoxLabel::setChecked(bool b){
    _checked = b;
    if(b){
        QImage img(NATRON_IMAGES_PATH"groupbox_unfolded.png");
        QPixmap pix = QPixmap::fromImage(img);
        pix = pix.scaled(20, 20);
        setPixmap(pix);
    }else{
        QImage img(NATRON_IMAGES_PATH"groupbox_folded.png");
        QPixmap pix = QPixmap::fromImage(img);
        pix = pix.scaled(20, 20);
        setPixmap(pix);
    }
}

Group_KnobGui::~Group_KnobGui(){
    delete _button;
    delete _descriptionLabel;
    for(U32 i  = 0 ; i < _children.size(); ++i){
        delete _children[i].first;
    }
    
}

void Group_KnobGui::createWidget(QGridLayout* layout,int row){
    _layout = layout;
    _button = new GroupBoxLabel(layout->parentWidget());
    _button->setToolTip(_knob->getHintToolTip().c_str());
    QWidget* header = new QWidget(layout->parentWidget());
    QHBoxLayout* headerLay = new QHBoxLayout(header);
    header->setLayout(headerLay);
    QObject::connect(_button, SIGNAL(checked(bool)), this, SLOT(setChecked(bool)));
    headerLay->addWidget(_button);
    headerLay->setSpacing(1);
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    headerLay->addWidget(_descriptionLabel);
    
    layout->addWidget(header,row,0,1,2,Qt::AlignLeft);
    
}

void Group_KnobGui::setChecked(bool b){
    if(b == _checked){
        return;
    }
    _checked = b;
    for(U32 i = 0 ; i < _children.size() ;++i) {
        if(!b){
            _children[i].first->hide();
            
        }else{
            _children[i].first->show();
        }
    }
}
void Group_KnobGui::updateGUI(const Variant& variant){
    bool b = variant.toBool();
    setChecked(b);
    _button->setChecked(b);
}
void Group_KnobGui::hide(){
    _button->hide();
    _descriptionLabel->hide();
    for (U32 i = 0; i < _children.size(); ++i) {
        _children[i].first->hide();
    }
}

void Group_KnobGui::show(){
    _button->show();
    _descriptionLabel->show();
    for (U32 i = 0; i < _children.size(); ++i) {
        _children[i].first->show();
    }
}
void Group_KnobGui::addToLayout(QHBoxLayout* layout){
    QWidget* mainWidget = new QWidget(_layout->parentWidget());
    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
    mainWidget->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    QWidget* header = new QWidget(mainWidget);
    QHBoxLayout* headerLayout = new QHBoxLayout(mainWidget);
    header->setLayout(headerLayout);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addWidget(_button);
    headerLayout->addWidget(_descriptionLabel);
    
    mainLayout->addWidget(header);
    for (U32 i = 0; i < _children.size(); ++i) {
        QWidget* container = new QWidget(_layout->parentWidget());
        QHBoxLayout* containerLayout = new QHBoxLayout(container);
        container->setLayout(containerLayout);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->addStretch();
        _children[i].first->addToLayout(containerLayout);
        mainLayout->addWidget(container);
    }
    layout->addWidget(mainWidget);
}

//=============================RICH_TEXT_KNOBGUI===================================

void RichText_KnobGui::createWidget(QGridLayout* layout,int row){
    _descriptionLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),layout->parentWidget());
    _descriptionLabel->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel,row,0,Qt::AlignRight);

    _textEdit = new QTextEdit(layout->parentWidget());
    _textEdit->setToolTip(_knob->getHintToolTip().c_str());
    layout->addWidget(_textEdit,row,1,Qt::AlignLeft);
    QObject::connect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));

}

RichText_KnobGui::~RichText_KnobGui(){
    delete _descriptionLabel;
    delete _textEdit;

}

void RichText_KnobGui::hide(){
    _descriptionLabel->hide();
    _textEdit->hide();
}

void RichText_KnobGui::show(){
    _descriptionLabel->show();
    _textEdit->show();
}
void RichText_KnobGui::setEnabled(bool b){
    _descriptionLabel->setEnabled(b);
    _textEdit->setEnabled(b);
}

void RichText_KnobGui::addToLayout(QHBoxLayout* layout){
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_textEdit);
}

void RichText_KnobGui::onTextChanged(){
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),Variant(_textEdit->toPlainText())));
}


void RichText_KnobGui::updateGUI(const Variant& variant){
    QObject::disconnect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
    QTextCursor cursor = _textEdit->textCursor();
    int pos = cursor.position();
    _textEdit->clear();
    QString txt = variant.toString();
    _textEdit->setPlainText(txt);
    cursor.setPosition(pos);
    _textEdit->setTextCursor(cursor);
    QObject::connect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));

}

