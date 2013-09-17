//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobGui.h"

#ifdef __POWITER_UNIX__
#include <dlfcn.h>
#endif
#include <climits>
#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
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


#include "Engine/Node.h"
#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/Settings.h"
#include "Engine/Knob.h"

#include "Gui/Button.h"
#include "Gui/SettingsPanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/Timeline.h"
#include "Gui/Gui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/LineEdit.h"

#include "Readers/Reader.h"

using namespace Powiter;
using namespace std;


KnobGui::KnobGui(Knob* knob,QWidget* parent):
QWidget(parent),
_knob(knob),
_triggerNewLine(true),
_spacingBetweenItems(0),
_widgetCreated(false)
{
    QObject::connect(knob,SIGNAL(valueChanged(const Variant&)),this,SLOT(onInternalValueChanged(const Variant&)));
    QObject::connect(this,SIGNAL(valueChanged(const Variant&)),knob,SLOT(onValueChanged(const Variant&)));
    setVisible(true);
}

KnobGui::~KnobGui(){
    
}

void KnobGui::pushUndoCommand(QUndoCommand* cmd){
    _knob->getNode()->pushUndoCommand(cmd);
}





//================================================================
class KnobUndoCommand : public QUndoCommand{
    
public:
    
    KnobUndoCommand(KnobGui* knob,const Variant& oldValue,const Variant& newValue,QUndoCommand *parent = 0):QUndoCommand(parent),
    _oldValue(oldValue),
    _newValue(newValue),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    Variant _oldValue;
    Variant _newValue;
    KnobGui* _knob;
};

void KnobUndoCommand::undo(){
    _knob->setValue(_oldValue);
    
    setText(QObject::tr("Change %1")
            .arg(_knob->getKnob()->getDescription().c_str()));
}
void KnobUndoCommand::redo(){
    _knob->setValue(_newValue);
    
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
File_KnobGui::File_KnobGui(Knob* knob):KnobGui(knob){
    File_Knob* fileKnob = dynamic_cast<File_Knob*>(knob);
    QObject::connect(fileKnob,SIGNAL(shouldOpenFile()),this,SLOT(open_file()));
}

void File_KnobGui::createWidget(QGridLayout* layout,int row,int columnOffset){
    
    QLabel* desc = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
    _lineEdit = new LineEdit(this);
    _lineEdit->setPlaceholderText("File path...");
    QObject::connect(_lineEdit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
	
    Button* openFile = new Button(this);
    
    QImage img(POWITER_IMAGES_PATH"open-file.png");
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(15,15);
    openFile->setIcon(QIcon(pix));
    
    QObject::connect(openFile,SIGNAL(clicked()),this,SLOT(open_file()));
        
    layout->addWidget(desc, row, 0+columnOffset);
    layout->addWidget(_lineEdit,row,1+columnOffset);
    layout->addWidget(openFile,row,2+columnOffset);
}

void File_KnobGui::updateGUI(const Variant& variant){
    std::string pattern = SequenceFileDialog::patternFromFilesList(variant.toStringList()).toStdString();
    _lineEdit->setText(pattern.c_str());
}

void File_KnobGui::open_file(){
    
    QStringList oldList,filesList;
    oldList = SequenceFileDialog::filesListFromPattern(_lineEdit->text());
    std::vector<std::string> filters = Settings::getPowiterCurrentSettings()->_readersSettings.supportedFileTypes();
    
    SequenceFileDialog dialog(this,filters,true,SequenceFileDialog::OPEN_DIALOG,_lastOpened.toStdString());
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

//============================OUTPUT_FILE_KNOB_GUI====================================
OutputFile_KnobGui::OutputFile_KnobGui(Knob* knob):KnobGui(knob){
    OutputFile_Knob* fileKnob = dynamic_cast<OutputFile_Knob*>(knob);
    QObject::connect(fileKnob,SIGNAL(shouldOpenFile()),this,SLOT(open_file()));
}

void OutputFile_KnobGui::createWidget(QGridLayout *layout, int row,int columnOffset){
    QLabel* desc = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
    _lineEdit = new LineEdit(this);
    _lineEdit->setPlaceholderText(QString("File path..."));
	
    QPushButton* openFile = new Button(this);
    
    QImage img(POWITER_IMAGES_PATH"open-file.png");
    QPixmap pix=QPixmap::fromImage(img);
    pix = pix.scaled(15,15);
    openFile->setIcon(QIcon(pix));
    
    QObject::connect(openFile,SIGNAL(clicked()),this,SLOT(open_file()));
    
    layout->addWidget(desc,row,0+columnOffset);
    layout->addWidget(_lineEdit,row,1+columnOffset);
    layout->addWidget(openFile,row,2+columnOffset);
}



void OutputFile_KnobGui::updateGUI(const Variant& variant){
    _lineEdit->setText(variant.toString());
}
void OutputFile_KnobGui::open_file(){
    std::vector<std::string> filters = Settings::getPowiterCurrentSettings()->_readersSettings.supportedFileTypes();
    SequenceFileDialog dialog(this,filters,true,SequenceFileDialog::SAVE_DIALOG,_lastOpened.toStdString());
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

//==========================INT_KNOB_GUI======================================


void Int_KnobGui::createWidget(QGridLayout *layout, int row,int columnOffset){
    QLabel* desc = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);    
    
    layout->addWidget(desc,row,0+columnOffset);
    
    Knob_Mask flags = _knob->getFlags();
    int dim = _knob->getDimension();
    
    QString subLabels_RGBA[] = {"r:","g:","b:","a:"};
    QString subLabels_XYZW[] = {"x:","y:","z:","w:"};
    
    QList<QVariant> maximums = _knob->getMaximum().toList();
    QList<QVariant> minimums = _knob->getMinimum().toList();
    for (int i = 0; i < dim; ++i) {
        QWidget* boxContainer = new QWidget(this);
        QHBoxLayout* boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        if(dim != 1){
            QString subLabelDesc;
            if(flags & Knob::RGBA_STYLE_SCALAR){
                subLabelDesc = subLabels_RGBA[i];
            }else{
                subLabelDesc = subLabels_XYZW[i];
            }
            QLabel* subDesc = new QLabel(subLabelDesc,boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        FeedbackSpinBox* box = new FeedbackSpinBox(this,false);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        if(_knob->getDimension() > 1){
            if(maximums.size() > i)
                box->setMaximum(maximums.at(i).toInt());
            if(minimums.size() > i)
                box->setMinimum(minimums.at(i).toInt());
        }else{
            box->setMaximum(_knob->getMaximum().toInt());
            box->setMinimum(_knob->getMinimum().toInt());
        }
        
        boxContainerLayout->addWidget(box);
        if(flags & Knob::READ_ONLY){
            box->setReadOnly(true);
        }
        if(flags & Knob::INVISIBLE){
            box->setVisible(false);
        }
        layout->addWidget(boxContainer,row,i+1+columnOffset);
        _spinBoxes.push_back(box);
    }
}
void Int_KnobGui::updateGUI(const Variant& variant){
    if(_knob->getDimension() > 1){
        QList<QVariant> list = variant.toList();
        if(list.isEmpty()){
            return;
        }
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            _spinBoxes[i]->setValue(list.at(i).toInt());
        }
    }else{
        int value = variant.toInt();
        _spinBoxes[0]->setValue(value);
    }
}

void Int_KnobGui::onSpinBoxValueChanged(){
    QList<QVariant> list;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        list << QVariant(_spinBoxes[i]->value());
    }
    pushUndoCommand(new KnobUndoCommand(this,Variant(_knob->getValueAsVariant().toStringList()),Variant(list)));
}

//==========================BOOL_KNOB_GUI======================================

void Bool_KnobGui::createWidget(QGridLayout *layout, int row,int columnOffset){
    QLabel* label = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
    _checkBox = new QCheckBox(this);
    QObject::connect(_checkBox,SIGNAL(clicked(bool)),this,SLOT(onCheckBoxStateChanged(bool)));    
    layout->addWidget(label,row,0+columnOffset);
    layout->addWidget(_checkBox,row,1+columnOffset);
    Knob_Mask flags = _knob->getFlags();
    if(flags & Knob::READ_ONLY){
        _checkBox->setEnabled(false);
    }
    if(flags & Knob::INVISIBLE){
        _checkBox->setVisible(false);
    }
}

void Bool_KnobGui::updateGUI(const Variant& variant){
    bool b = variant.toBool();
    _checkBox->setChecked(b);
}


void Bool_KnobGui::onCheckBoxStateChanged(bool b){
    pushUndoCommand(new KnobUndoCommand(this,Variant(_knob->getValueAsVariant()),Variant(b)));
}


//=============================DOUBLE_KNOB_GUI===================================
void Double_KnobGui::createWidget(QGridLayout *layout, int row,int columnOffset){
    QLabel* desc = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
        
    layout->addWidget(desc,row,0+columnOffset);
    
    Knob_Mask flags = _knob->getFlags();
    int dim = _knob->getDimension();
    
    QString subLabels_RGBA[] = {"r:","g:","b:","a:"};
    QString subLabels_XYZW[] = {"x:","y:","z:","w:"};
    
    QList<QVariant> maximums = _knob->getMaximum().toList();
    QList<QVariant> minimums = _knob->getMinimum().toList();
    QList<QVariant> increments = _knob->getIncrement().toList();
    for (int i = 0; i < dim; ++i) {
        QWidget* boxContainer = new QWidget(this);
        QHBoxLayout* boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        if(dim != 1){
            QString subLabelDesc;
            if(flags & Knob::RGBA_STYLE_SCALAR){
                subLabelDesc = subLabels_RGBA[i];
            }else{
                subLabelDesc = subLabels_XYZW[i];
            }
            QLabel* subDesc = new QLabel(subLabelDesc,boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        FeedbackSpinBox* box = new FeedbackSpinBox(this,true);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        if(_knob->getDimension() > 1){
            if(maximums.size() > i)
                box->setMaximum(maximums.at(i).toDouble());
            if(minimums.size() > i)
                box->setMinimum(minimums.at(i).toDouble());
            if(increments.size() > i){
                double incr = increments.at(i).toDouble();
                if(incr > 0)
                    box->setIncrement(incr);
            }
        }else{
            box->setMaximum(_knob->getMaximum().toDouble());
            box->setMinimum(_knob->getMinimum().toDouble());
            double incr = _knob->getIncrement().toDouble();
            if(incr > 0)
                box->setIncrement(incr);
        }
        
        boxContainerLayout->addWidget(box);
        if(flags & Knob::READ_ONLY){
            box->setReadOnly(true);
        }
        if(flags & Knob::INVISIBLE){
            box->setVisible(false);
        }
        int offset = _knob->determineHierarchySize();
        if(_knob->getParentKnob()){
            ++offset;
        }
        layout->addWidget(boxContainer,row,i+1+columnOffset);
        _spinBoxes.push_back(box);
    }
}
void Double_KnobGui::updateGUI(const Variant& variant){
    if(_knob->getDimension() > 1){
        QList<QVariant> list = variant.toList();
        if(list.isEmpty()){
            return;
        }
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            _spinBoxes[i]->setValue(list.at(i).toDouble());
        }
    }else{
        double value = variant.toDouble();
        _spinBoxes[0]->setValue(value);
    }
}

void Double_KnobGui::onSpinBoxValueChanged(){
    QList<QVariant> list;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        list << QVariant(_spinBoxes[i]->value());
    }
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),Variant(list)));
}

//=============================BUTTON_KNOB_GUI===================================
void Button_KnobGui::createWidget(QGridLayout *layout, int row,int columnOffset){
    _button = new Button(QString(QString(_knob->getDescription().c_str())+":"),this);
    QObject::connect(_button, SIGNAL(pressed()),this,SLOT(emitValueChanged()));
    
    layout->addWidget(_button,row,0+columnOffset);
    Knob_Mask flags = _knob->getFlags();
    if(flags & Knob::INVISIBLE){
        _button->setVisible(false);
    }
}

void Button_KnobGui::emitValueChanged(){
    emit valueChanged(Variant()); 
}

//=============================COMBOBOX_KNOB_GUI===================================
ComboBox_KnobGui::ComboBox_KnobGui(Knob* knob):KnobGui(knob){
    ComboBox_Knob* cbKnob = dynamic_cast<ComboBox_Knob*>(knob);
    _entries = cbKnob->getEntries();
}

void ComboBox_KnobGui::createWidget(QGridLayout *layout, int row,int columnOffset){
    _comboBox = new ComboBox(this);
    
    for (U32 i = 0; i < _entries.size(); ++i) {
        _comboBox->addItem(_entries[i].c_str());
    }
    if(_entries.size() > 0)
        _comboBox->setCurrentText(_entries[0].c_str());
    QLabel* desc = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
    QObject::connect(_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));    
    layout->addWidget(desc,row,0+columnOffset);
    layout->addWidget(_comboBox,row,1+columnOffset);
    Knob_Mask flags = _knob->getFlags();
    if(flags & Knob::INVISIBLE){
        _comboBox->setVisible(false);
    }
}
void ComboBox_KnobGui::onCurrentIndexChanged(int i){
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),Variant(i)));    
    
}

void ComboBox_KnobGui::updateGUI(const Variant& variant){
    int i = variant.toInt();
    assert(i <= (int)_entries.size());
    _comboBox->setCurrentText(_entries[i].c_str());
}

//=============================SEPARATOR_KNOB_GUI===================================
void Separator_KnobGui::createWidget(QGridLayout* layout,int row,int columnOffset){
    QLabel* name = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);    
    layout->addWidget(name,row,0+columnOffset);
    _line = new QFrame(this);
    _line->setFrameShape(QFrame::HLine);
    _line->setFrameShadow(QFrame::Sunken);
    _line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(_line,row,1+columnOffset);
}



//=============================RGBA_KNOB_GUI===================================
void RGBA_KnobGui::createWidget(QGridLayout* layout,int row,int columnOffset){
    _rBox = new FeedbackSpinBox(this,true);
    QObject::connect(_rBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    _gBox = new FeedbackSpinBox(this,true);
    QObject::connect(_gBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    _bBox = new FeedbackSpinBox(this,true);
    QObject::connect(_bBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    _aBox = new FeedbackSpinBox(this,true);
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
    
    
    _rLabel = new QLabel("r:",this);
    _gLabel = new QLabel("g:",this);
    _bLabel = new QLabel("b:",this);
    _aLabel = new QLabel("a:",this);
    
    QLabel* nameLabel = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(nameLabel,row,0+columnOffset);
    
    QWidget* boxContainers = new QWidget(this);
    QHBoxLayout* boxLayout = new QHBoxLayout(boxContainers);
    boxContainers->setLayout(boxLayout);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    
    boxLayout->addWidget(_rLabel);
    boxLayout->addWidget(_rBox);
    boxLayout->addWidget(_gLabel);
    boxLayout->addWidget(_gBox);
    boxLayout->addWidget(_bLabel);
    boxLayout->addWidget(_bBox);
    boxLayout->addWidget(_aLabel);
    boxLayout->addWidget(_aBox);
    
    _colorLabel = new QLabel(this);
    boxLayout->addWidget(_colorLabel);
    
    QImage buttonImg(POWITER_IMAGES_PATH"colorwheel.png");
    QPixmap buttonPix = QPixmap::fromImage(buttonImg);
    buttonPix = buttonPix.scaled(25, 20);
    QIcon buttonIcon(buttonPix);
    _colorDialogButton = new Button(buttonIcon,"",this);
    
    QObject::connect(_colorDialogButton, SIGNAL(pressed()), this, SLOT(showColorDialog()));
    boxLayout->addWidget(_colorDialogButton);
    
    layout->addWidget(boxContainers,row,1+columnOffset);
    
    
    Knob_Mask flags = _knob->getFlags();
    if (flags  & Knob::NO_ALPHA) {
        disablePermantlyAlpha();
    }
}
void RGBA_KnobGui::updateGUI(const Variant& variant){
    QVector4D v = variant.value<QVector4D>();
    _rBox->setValue(v.x());
    _gBox->setValue(v.y());
    _bBox->setValue(v.z());
    _aBox->setValue(v.w());
    QColor color(v.x()*255,v.y()*255,v.z()*255,v.w()*255);
    updateLabel(color);
}
void RGBA_KnobGui::showColorDialog(){
    QColorDialog dialog(this);
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
    _aLabel->setVisible(false);
    _aLabel->setEnabled(false);
    _aBox->setVisible(false);
    _aBox->setEnabled(false);
}

//=============================STRING_KNOB_GUI===================================
void String_KnobGui::createWidget(QGridLayout *layout, int row,int columnOffset){
    QLabel* name = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
    
    layout->addWidget(name,row,0+columnOffset);
    _lineEdit = new LineEdit(this);
    layout->addWidget(_lineEdit,row,1+columnOffset);
    QObject::connect(_lineEdit, SIGNAL(textEdited(QString)), this, SLOT(onStringChanged(QString)));
    Knob_Mask flags = _knob->getFlags();
    if(flags & Knob::READ_ONLY){
        _lineEdit->setReadOnly(true);
    }
    if(flags & Knob::INVISIBLE){
        _lineEdit->setVisible(false);
    }

}

void String_KnobGui::onStringChanged(const QString& str){
    pushUndoCommand(new KnobUndoCommand(this,_knob->getValueAsVariant(),Variant(str)));
}
void String_KnobGui::updateGUI(const Variant& variant){
    _lineEdit->setText(variant.toString());
}
#include <QSpacerItem>
//=============================GROUP_KNOB_GUI===================================
GroupBoxLabel::GroupBoxLabel(QWidget* parent):
QLabel(parent),
_checked(false){
    
    QObject::connect(this, SIGNAL(checked(bool)), this, SLOT(setChecked(bool)));
}

void GroupBoxLabel::setChecked(bool b){
    _checked = b;
    if(b){
        QImage img(POWITER_IMAGES_PATH"groupbox_unfolded.png");
        QPixmap pix = QPixmap::fromImage(img);
        pix = pix.scaled(20, 20);
        setPixmap(pix);
    }else{
        QImage img(POWITER_IMAGES_PATH"groupbox_folded.png");
        QPixmap pix = QPixmap::fromImage(img);
        pix = pix.scaled(20, 20);
        setPixmap(pix);
    }
}

void Group_KnobGui::createWidget(QGridLayout* layout,int row,int columnOffset){
    _layout = layout;
    _button = new GroupBoxLabel(this);
    
    QObject::connect(_button, SIGNAL(checked(bool)), this, SLOT(setChecked(bool)));
    layout->addWidget(_button,row,0+columnOffset);
    
    QLabel* name = new QLabel(QString(QString(_knob->getDescription().c_str())+":"),this);
    layout->addWidget(name, row, 1+columnOffset);
    
}

void Group_KnobGui::setChecked(bool b){
    if(b == _checked){
        return;
    }
    _checked = b;
    for(U32 i = 0 ; i < _children.size() ;++i) {
        if(!b){
            _children[i].first->hide();
            _layout->removeWidget(_children[i].first);
            _children[i].first->setParent(NULL);
        }else{
            _children[i].first->show();
            _layout->addWidget(_children[i].first, _children[i].second.first, _children[i].second.second);
            _children[i].first->setParent(_layout->parentWidget());
        }
        //        _children[i].first->setVisible(b);
        //   sa->viewport()->update();
    }
}
void Group_KnobGui::updateGUI(const Variant& variant){
    bool b = variant.toBool();
    setChecked(b);
    _button->setChecked(b);
}

//=============================TAB_KNOB_GUI===================================
void Tab_KnobGui::createWidget(QGridLayout* layout,int row,int columnOffset){
    /*not a pretty call... considering using QTabWidget instead*/
    _tabWidget = new TabWidget(_knob->getNode()->getModel()->getApp()->getGui(),
                               TabWidget::NONE,this);
    
    layout->addWidget(_tabWidget,row,0+columnOffset);
    
}
void Tab_KnobGui::addKnobs(const std::map<std::string,std::vector<KnobGui*> >& knobs){
    for (std::map<std::string,std::vector<KnobGui*> >::const_iterator it = knobs.begin();
         it!=knobs.end(); ++it) {
        std::string name = it->first;
        QWidget* newTab = new QWidget(_tabWidget);
        _tabWidget->appendTab(name.c_str(), newTab);
        QVBoxLayout* newLayout = new QVBoxLayout(newTab);
        newTab->setLayout(newLayout);
        vector<KnobGui*> knobs = it->second;
        for (U32 i = 0; i < knobs.size(); ++i) {
            knobs[i]->layout()->removeWidget(knobs[i]);
            knobs[i]->setParent(this);
            newLayout->addWidget(knobs[i]);
        }
    }

}
