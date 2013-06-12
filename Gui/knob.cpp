//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include <climits>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include "Gui/knob.h"
#include "Gui/knob_callback.h"
#include "Core/node.h"
#include "Core/outputnode.h"
#include "Gui/node_ui.h"
#include "Reader/Reader.h"
#include "Superviser/controler.h"
#include <QtWidgets/QtWidgets>
#include "Core/model.h"
#include "Core/VideoEngine.h"
#include "Gui/dockableSettings.h"
#include "Gui/framefiledialog.h"

std::vector<Knob_Flags> Knob_Mask_to_Knobs_Flags(Knob_Mask &m){
    unsigned int i=0x1;
    std::vector<Knob_Flags> flags;
    if(m!=0){
        while(i<0x4){
            if((m & i)==i){
                flags.push_back((Knob_Flags)i);
            }
            i*=2;
        }
    }
    return flags;
}

Knob::Knob(Knob_Types type, Knob_Callback *cb):QWidget()
{
    this->cb=cb;
    this->_type = type;
    layout=new QHBoxLayout(this);
    foreach(QWidget* ele,elements){
        layout->addWidget(ele);
    }
    setLayout(layout);
    //cb->addKnob(this);
    setVisible(true);
}

Knob::~Knob(){
    foreach(QWidget* el,elements){
        delete el;
    }
    elements.clear();
    delete layout;
    values.clear();
}

void Knob::validateEvent(bool initViewer){
    Node* node = getCallBack()->getNode();
    NodeGui* nodeUI = node->getNodeUi();
    NodeGui* viewer = NodeGui::hasViewerConnected(nodeUI);
    if(viewer){
        //Controler* ctrlPTR = viewer->getControler();
        ctrlPTR->getModel()->clearPlaybackCache();
        ctrlPTR->getModel()->setVideoEngineRequirements(static_cast<OutputNode*>(viewer->getNode()));
        ctrlPTR->getModel()->getVideoEngine()->videoEngine(true,1,initViewer,true,false);
    }
}

//================================================================
Knob* Knob::channels_Knob(ChannelMask *channels, Knob_Callback *cb, Knob_Mask flags){
    
    Channels_Knob* knob=new Channels_Knob(channels,cb,flags);
    cb->addKnob(knob);
    return knob;
    
    
}

Channels_Knob::Channels_Knob(ChannelMask* channels,Knob_Callback *cb,Knob_Mask flags):Knob(CHANNELS_KNOB,cb)
{
    this->channels=channels;
    QLabel* boxtext=new QLabel(QString("Channels:"));
    QComboBox* box=new QComboBox();
    ChannelSet chanset= *this->channels;
    foreachChannels( z,chanset){
        box->addItem(QString(getChannelName(z).c_str()));
    }
    
    layout->addWidget(boxtext);
    layout->addWidget(box);
    std::vector<Knob_Flags> f=Knob_Mask_to_Knobs_Flags(flags);
    foreach(Knob_Flags flag,f){
        if(flag==INVISIBLE){
            setVisible(false);
        }else if(flag==READ_ONLY){
            // nothing to do yet
        }
        
    }
}
void Channels_Knob::setValues(){
    values.clear();
}

//================================================================
IntQSpinBox::IntQSpinBox(Int_Knob *knob):QSpinBox(){
    this->knob=knob;
    lineEdit()->setReadOnly(false);
}
void IntQSpinBox::keyPressEvent(QKeyEvent *event){
    if(event->key()==Qt::Key_Return){
        int integer=this->value();
        knob->setInteger(&integer);
        knob->setValues();
        // std::cout << "Missing implementation: keypressevent IntQSpinBox, to validate the integer" << std::endl;
    }
    QSpinBox::keyPressEvent(event);
}

Knob* Knob::int_Knob(int *integer, Knob_Callback *cb, QString &description,Knob_Mask flags){
    Int_Knob* knob=new Int_Knob(integer,cb,description,flags);
    cb->addKnob(knob);
    return knob;
}

Int_Knob::Int_Knob(int *integer, Knob_Callback *cb,QString& description, Knob_Mask flags):Knob(INT_KNOB,cb){
    this->integer=integer;
    QLabel* desc=new QLabel(description);
    box=new IntQSpinBox(this);
    
    box->setMaximum(INT_MAX);
    box->setMinimum(INT_MIN);
    box->setValue(*integer);
    layout->addWidget(desc);
    layout->addWidget(box);
    std::vector<Knob_Flags> f=Knob_Mask_to_Knobs_Flags(flags);
    foreach(Knob_Flags flag,f){
        if(flag==INVISIBLE){
            setVisible(false);
        }else if(flag==READ_ONLY){
            box->setReadOnly(true);
        }
        
    }
}
void Int_Knob::setValues(){
    values.clear();
    values.push_back((U64)*integer);
}

//================================================================
FileQLineEdit::FileQLineEdit(File_Knob *knob):QLineEdit(){
    this->knob=knob;
}
void FileQLineEdit::keyPressEvent(QKeyEvent *e){
    if(e->key()==Qt::Key_Return){
        QString str=this->text();
		QStringList strlist(str);
		if(strlist!=knob->getStr()){
			knob->setStr(strlist);
			knob->setValues();
            std::string className=knob->getCallBack()->getNode()->className();
			if(className == std::string("Reader")){
                Node* node=knob->getCallBack()->getNode();
                static_cast<Reader*>(node)->showFilePreview();
                knob->validateEvent(true);
            }
		}
    }
	QLineEdit::keyPressEvent(e);
}

Knob* Knob::file_Knob( Knob_Callback *cb, QString &description, QStringList &filePath,Knob_Mask flags){
    File_Knob* knob=new File_Knob(cb,description,filePath,flags);
    cb->addKnob(knob);
    return knob;
}
void File_Knob::open_file(){
    str.clear(); 



//    QStringList strlist=QFileDialog::getOpenFileNames(this,QString("Open File")
//                                                      ,_lastOpened
//                                                      ,"Image Files (*.png *.jpg *.bmp *.exr *.pgm *.ppm *.pbm *.jpeg *.dpx)");
    QStringList strlist;
    FrameFileDialog dialog(this,QString("Open File"),_lastOpened,"Image Files (*.png *.jpg *.bmp *.exr *.pgm *.ppm *.pbm *.jpeg *.dpx)");
    if(dialog.exec()){
        strlist = dialog.selectedFiles();
    }

    if(!strlist.isEmpty()){
        updateLastOpened(strlist[0]);
        name->setText(strlist.at(0));
        setStr(strlist);
        setValues();
        std::string className=getCallBack()->getNode()->className();
        if(className == string("Reader")){
            Node* node=getCallBack()->getNode();
            static_cast<Reader*>(node)->showFilePreview();
            validateEvent(true);
        }
    }
    
}
void File_Knob::updateLastOpened(QString str){
    int index = str.lastIndexOf(QChar('/'));
    if(index==-1){
        index=str.lastIndexOf(QChar('\\'));
    }
    _lastOpened = str.left(index);
}

File_Knob::File_Knob(Knob_Callback *cb, QString &description, QStringList &filePath, Knob_Mask flags):Knob(FILE_KNOB,cb),str(filePath)
{
    
    setStyleSheet("color:rgb(200,200,200) ;QLineEdit{color:rgb(200,200,200);}");
    QLabel* desc=new QLabel(description);
    _lastOpened =QString(ROOT);
    name=new FileQLineEdit(this);
    name->setPlaceholderText(QString("File path..."));
	
    QPushButton* openFile=new QPushButton(name);
    QImage img(IMAGES_PATH"open-file.png");
    QPixmap pix=QPixmap::fromImage(img);
    pix.scaled(10,10);
    openFile->setIcon(QIcon(pix));
    QObject::connect(openFile,SIGNAL(clicked()),this,SLOT(open_file()));
    layout->addWidget(desc);
    layout->addWidget(name);
    layout->addWidget(openFile);
    
    //flags handling: no Knob_Flags makes sense (yet) for the File_Knob. We keep it in parameters in case in the future there're some changes to be made.
    
}
void File_Knob::setValues(){
    values.clear();
    // filenames should not be involved in hash key computation as it defeats all the purpose of the cache
}

//================================================================
Knob* Knob::bool_Knob(bool& boolean,Knob_Callback* cb,QString& description,Knob_Mask flags){
	Bool_Knob* knob=new Bool_Knob(boolean,cb,description,flags);
	cb->addKnob(knob);
	return knob;
    
}
void Bool_Knob::change_checkBox(int checkBoxState){
	if(checkBoxState==0){
		boolean=false;
	}else if(checkBoxState==2){
		boolean=true;
	}
	this->setValues();
}
void Bool_Knob::setValues(){
    values.clear();
	if(boolean){
		values.push_back(1);
	}else{
		values.push_back(0);
	}
}

Bool_Knob::Bool_Knob(bool& boolean,Knob_Callback *cb,QString& description,Knob_Mask flags/* =0 */):Knob(BOOL_KNOB,cb) ,boolean(boolean){
	
	checkbox=new QCheckBox(description,this);
	checkbox->setChecked(boolean);
	QObject::connect(checkbox,SIGNAL(stateChanged(int)),this,SLOT(change_checkBox(int)));
	layout->addWidget(checkbox);
}
