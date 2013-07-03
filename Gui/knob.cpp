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
#include <QtCore/QString>
#include "Gui/button.h"

std::vector<Knob::Knob_Flags> Knob_Mask_to_Knobs_Flags(Knob_Mask &m){
    unsigned int i=0x1;
    std::vector<Knob::Knob_Flags> flags;
    if(m!=0){
        while(i<0x4){
            if((m & i)==i){
                flags.push_back((Knob::Knob_Flags)i);
            }
            i*=2;
        }
    }
    return flags;
}


KnobFactory::KnobFactory(){
    loadKnobPlugins();
}

KnobFactory::~KnobFactory(){
    for ( std::map<std::string,PluginID*>::iterator it = _loadedKnobs.begin(); it!=_loadedKnobs.end() ; it++) {
        delete it->second;
    }
    _loadedKnobs.clear();
}

void KnobFactory::loadKnobPlugins(){
    QDir d(PLUGINS_PATH);
    if (d.isReadable())
    {
        QStringList filters;
#ifdef __POWITER_WIN32__
        filters << "*.dll";
#elif defined(__POWITER_OSX__)
        filters << "*.dylib";
#elif defined(__POWITER_LINUX__)
        filters << "*.so";
#endif
        d.setNameFilters(filters);
		QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ;i ++)
        {
            QString filename = fileList.at(i);
            if(filename.contains(".dll") || filename.contains(".dylib") || filename.contains(".so")){
                QString className;
                int index = filename.size() -1;
                while(filename.at(index) != QChar('.')) index--;
                className = filename.left(index);
                PluginID* plugin = 0;
#ifdef __POWITER_WIN32__
                HINSTANCE lib;
                string dll;
                dll.append(PLUGINS_PATH);
                dll.append(className.toStdString());
                dll.append(".dll");
                lib=LoadLibrary((LPCWSTR)dll.c_str());
                if(lib==NULL){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    KnobBuilder builder=(KnobBuilder)GetProcAddress(lib,"BuildRead");
                    if(builder!=NULL){
                        std::string str("");
                        Knob* knob=builder(NULL,str,0);
                        plugin = new PluginID((HINSTANCE)builder,knob->name().c_str());
                        _loadedKnobs.insert(make_pair(knob->name(),plugin));
                        delete knob;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildKnob" << endl;
                        continue;
                    }
                    
                }
                
#elif defined(__POWITER_UNIX__)
                string dll;
                dll.append(PLUGINS_PATH);
                dll.append(className.toStdString());
#ifdef __POWITER_OSX__
                dll.append(".dylib");
#elif defined(__POWITER_LINUX__)
                dll.append(".so");
#endif
                void* lib=dlopen(dll.c_str(),RTLD_LAZY);
                if(!lib){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }
                else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    KnobBuilder builder=(KnobBuilder)dlsym(lib,"BuildKnob");
                    if(builder!=NULL){
                        std::string str("");
                        Knob* knob=builder(NULL,str,0);
                        plugin = new PluginID((void*)builder,knob->name().c_str());
                        _loadedKnobs.insert(make_pair(knob->name(),plugin));
                        delete knob;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildKnob" << endl;
                        continue;
                    }
                }
#endif
            }else{
                continue;
            }
        }
    }
    loadBultinKnobs();
    
}

void KnobFactory::loadBultinKnobs(){
    std::string stub;
    Knob* fileKnob = File_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *FILEKnobPlugin = new PluginID((HINSTANCE)&File_Knob::BuildKnob,fileKnob->name().c_str());
#else
    PluginID *FILEKnobPlugin = new PluginID((void*)&File_Knob::BuildKnob,fileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(fileKnob->name(),FILEKnobPlugin));
    delete fileKnob;
    
    Knob* intKnob = Int_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *INTKnobPlugin = new PluginID((HINSTANCE)&Int_Knob::BuildKnob,intKnob->name().c_str());
#else
    PluginID *INTKnobPlugin = new PluginID((void*)&Int_Knob::BuildKnob,intKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(intKnob->name(),INTKnobPlugin));
    delete intKnob;
    
    Knob* doubleKnob = Double_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *DOUBLEKnobPlugin = new PluginID((HINSTANCE)&Double_Knob::BuildKnob,doubleKnob->name().c_str());
#else
    PluginID *DOUBLEKnobPlugin = new PluginID((void*)&Double_Knob::BuildKnob,doubleKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(doubleKnob->name(),DOUBLEKnobPlugin));
    delete doubleKnob;
    
    Knob* boolKnob = Bool_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *BOOLKnobPlugin = new PluginID((HINSTANCE)&Bool_Knob::BuildKnob,boolKnob->name().c_str());
#else
    PluginID *BOOLKnobPlugin = new PluginID((void*)&Bool_Knob::BuildKnob,boolKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(boolKnob->name(),BOOLKnobPlugin));
    delete boolKnob;
    
    Knob* buttonKnob = Button_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *BUTTONKnobPlugin = new PluginID((HINSTANCE)&Button_Knob::BuildKnob,buttonKnob->name().c_str());
#else
    PluginID *BUTTONKnobPlugin = new PluginID((void*)&Button_Knob::BuildKnob,buttonKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(buttonKnob->name(),BUTTONKnobPlugin));
    delete buttonKnob;
    
    Knob* outputFileKnob = OutputFile_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *OUTPUTFILEKnobPlugin = new PluginID((HINSTANCE)&OutputFile_Knob::BuildKnob,outputFileKnob->name().c_str());
#else
    PluginID *OUTPUTFILEKnobPlugin = new PluginID((void*)&OutputFile_Knob::BuildKnob,outputFileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(outputFileKnob->name(),OUTPUTFILEKnobPlugin));
    delete outputFileKnob;
    
    Knob* comboBoxKnob = ComboBox_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *ComboBoxKnobPlugin = new PluginID((HINSTANCE)&ComboBox_Knob::BuildKnob,comboBoxKnob->name().c_str());
#else
    PluginID *ComboBoxKnobPlugin = new PluginID((void*)&ComboBox_Knob::BuildKnob,comboBoxKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(comboBoxKnob->name(),ComboBoxKnobPlugin));
    delete comboBoxKnob;
    
    
    Knob* separatorKnob = Separator_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *SeparatorKnobPlugin = new PluginID((HINSTANCE)&Separator_Knob::BuildKnob,separatorKnob->name().c_str());
#else
    PluginID *SeparatorKnobPlugin = new PluginID((void*)&Separator_Knob::BuildKnob,separatorKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(separatorKnob->name(),SeparatorKnobPlugin));
    delete separatorKnob;
}

/*Calls the unique instance of the KnobFactory and
 calls the appropriate pointer to function to create a knob.*/
Knob* KnobFactory::createKnob(std::string name,Knob_Callback* callback,std::string& description,Knob_Mask flags){
    const std::map<std::string,PluginID*>& loadedPlugins = KnobFactory::instance()->getLoadedKnobs();
    std::map<std::string,PluginID*>::const_iterator it = loadedPlugins.find(name);
    if(it == loadedPlugins.end()){
        return NULL;
    }else{
        KnobBuilder builder = (KnobBuilder)(it->second->first);
        if(builder){
            Knob* ret = builder(callback,description,flags);
            return ret;
        }else{
            return NULL;
        }
    }
}

Knob::Knob( Knob_Callback *cb):QWidget()
{
    this->cb=cb;
    layout=new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
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

void Knob::enqueueForDeletion(){
    cb->removeAndDeleteKnob(this);
}

void Knob::validateEvent(bool initViewer){
    Node* node = getCallBack()->getNode();
    NodeGui* nodeUI = node->getNodeUi();
    NodeGui* viewer = NodeGui::hasViewerConnected(nodeUI);
    if(viewer){
        //Controler* ctrlPTR = viewer->getControler();
        ctrlPTR->getModel()->clearPlaybackCache();
        ctrlPTR->getModel()->setVideoEngineRequirements(static_cast<OutputNode*>(viewer->getNode()),true);
        ctrlPTR->getModel()->getVideoEngine()->videoEngine(1,initViewer,true,false);
    }
}

//================================================================

//================================================================
IntQSpinBox::IntQSpinBox(Int_Knob *knob,QWidget* parent):FeedBackSpinBox(parent){
    this->knob=knob;
}
void IntQSpinBox::keyPressEvent(QKeyEvent *event){
    if(event->key()==Qt::Key_Return){
        int value = this->value();
        knob->setInteger(value);
        knob->setValues();
        // std::cout << "Missing implementation: keypressevent IntQSpinBox, to validate the integer" << std::endl;
    }
    FeedBackSpinBox::keyPressEvent(event);
}

Knob* Int_Knob::BuildKnob(Knob_Callback *cb, std::string &description, Knob_Mask flags){
    Int_Knob* knob=new Int_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}

Int_Knob::Int_Knob(Knob_Callback *cb,std::string& description, Knob_Mask flags):Knob(cb),integer(0){
    QLabel* desc=new QLabel(description.c_str());
    box=new IntQSpinBox(this,this);
    
    box->setMaximum(INT_MAX);
    box->setMinimum(INT_MIN);
    box->setValue(0);
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
    values.push_back((U64)integer);
}

//================================================================
FileQLineEdit::FileQLineEdit(File_Knob *knob):LineEdit(knob){
    this->knob=knob;
}
void FileQLineEdit::keyPressEvent(QKeyEvent *e){
    if(e->key()==Qt::Key_Return){
        QString str=this->text();
		QStringList strlist(str);
		if(strlist!=*(knob->getStr())){
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

Knob* File_Knob::BuildKnob(Knob_Callback *cb, std::string &description, Knob_Mask flags){
    File_Knob* knob=new File_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
void File_Knob::open_file(){
    str->clear();



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
        _name->setText(strlist.at(0));
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

File_Knob::File_Knob(Knob_Callback *cb, std::string &description, Knob_Mask flags):Knob(cb),str(0)
{
    
    Q_UNUSED(flags);
    QLabel* desc=new QLabel(description.c_str());
    _lastOpened =QString(ROOT);
    _name=new FileQLineEdit(this);
    _name->setPlaceholderText(QString("File path..."));
	
    QPushButton* openFile=new Button(_name);
    QImage img(IMAGES_PATH"open-file.png");
    QPixmap pix=QPixmap::fromImage(img);
    pix.scaled(10,10);
    openFile->setIcon(QIcon(pix));
    QObject::connect(openFile,SIGNAL(clicked()),this,SLOT(open_file()));
    layout->addWidget(desc);
    layout->addWidget(_name);
    layout->addWidget(openFile);
    
    //flags handling: no Knob_Flags makes sense (yet) for the File_Knob. We keep it in parameters in case in the future there're some changes to be made.
    
}
void File_Knob::setValues(){
    values.clear();
    // filenames should not be involved in hash key computation as it defeats all the purpose of the cache
}

Knob* Bool_Knob::BuildKnob(Knob_Callback* cb,std::string& description,Knob_Mask flags){
	Bool_Knob* knob=new Bool_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
	return knob;
    
}
void Bool_Knob::change_checkBox(int checkBoxState){
	if(checkBoxState==0){
		*_boolean=false;
	}else if(checkBoxState==2){
		*_boolean=true;
	}
	this->setValues();
}
void Bool_Knob::setValues(){
    values.clear();
	if(*_boolean){
		values.push_back(1);
	}else{
		values.push_back(0);
	}
}

Bool_Knob::Bool_Knob(Knob_Callback *cb,std::string& description,Knob_Mask flags/* =0 */):Knob(cb) ,_boolean(0){
	Q_UNUSED(flags);
    QLabel* _label = new QLabel(description.c_str(),this);
	checkbox=new QCheckBox(this);
	checkbox->setChecked(false);
	QObject::connect(checkbox,SIGNAL(stateChanged(int)),this,SLOT(change_checkBox(int)));
    layout->addWidget(_label);
	layout->addWidget(checkbox);
    layout->addStretch();
}
//================================================================

void Double_Knob::setValues(){
    values.clear();
    values.push_back(*(reinterpret_cast<U64*>(_value)));
}
Double_Knob::Double_Knob(Knob_Callback * cb,std::string& description,Knob_Mask flags):Knob(cb),_value(0){
    QLabel* desc=new QLabel(description.c_str());
    box=new DoubleQSpinBox(this,this);
    
    box->setMaximum(INT_MAX);
    box->setMinimum(INT_MIN);
    box->setValue(0);
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

Knob* Double_Knob::BuildKnob(Knob_Callback* cb,std::string& description,Knob_Mask flags){
    Double_Knob* knob=new Double_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}

DoubleQSpinBox::DoubleQSpinBox(Double_Knob* knob,QWidget* parent):FeedBackSpinBox(parent,true),knob(knob){
    
}
void DoubleQSpinBox::keyPressEvent(QKeyEvent *event){
    if(event->key()==Qt::Key_Return){
        double value=this->value();
        knob->setDouble(value);
        knob->setValues();
        // std::cout << "Missing implementation: keypressevent IntQSpinBox, to validate the integer" << std::endl;
    }
    FeedBackSpinBox::keyPressEvent(event);
}
/*******/

Knob* Button_Knob::BuildKnob(Knob_Callback* cb,std::string& description,Knob_Mask flags){
    Button_Knob* knob=new Button_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
Button_Knob::Button_Knob(Knob_Callback *cb,std::string& description,Knob_Mask flags):Knob(cb),button(0){
    Q_UNUSED(flags);
    button = new Button(QString(description.c_str()),this);
    layout->addWidget(button);
    layout->addStretch();
}
void Button_Knob::connectButtonToSlot(QObject* object,const char* slot){
    QObject::connect(button, SIGNAL(pressed()), object, slot);
}
/*******/


Knob* OutputFile_Knob::BuildKnob(Knob_Callback* cb,std::string& description,Knob_Mask flags){
    OutputFile_Knob* knob=new OutputFile_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}

OutputFile_Knob::OutputFile_Knob(Knob_Callback *cb,std::string& description,Knob_Mask flags):Knob(cb),str(0){
    Q_UNUSED(flags);
    QLabel* desc=new QLabel(description.c_str());
    _name=new OutputFileQLineEdit(this);
    _name->setPlaceholderText(QString("File path..."));
	
    QPushButton* openFile=new Button(_name);
    QImage img(IMAGES_PATH"open-file.png");
    QPixmap pix=QPixmap::fromImage(img);
    pix.scaled(10,10);
    openFile->setIcon(QIcon(pix));
    QObject::connect(openFile,SIGNAL(clicked()),this,SLOT(open_file()));
    QObject::connect(_name,SIGNAL(textChanged(const QString&)),this,SLOT(setStr(const QString&)));
    layout->addWidget(desc);
    layout->addWidget(_name);
    layout->addWidget(openFile);
}

void OutputFile_Knob::setValues(){
    values.clear();

}

void OutputFile_Knob::open_file(){
    str->clear();
    
    
    
        QString outFile=QFileDialog::getSaveFileName(this,QString("Save File")
                                                          ,QString(ROOT)
                                                          ,"Image Files (*.png *.jpg *.bmp *.exr *.pgm *.ppm *.pbm *.jpeg *.dpx)");
//    QStringList strlist;
//    FrameFileDialog dialog(this,QString("Open File"),_lastOpened,"Image Files (*.png *.jpg *.bmp *.exr *.pgm *.ppm *.pbm *.jpeg *.dpx)");
//    if(dialog.exec()){
//        strlist = dialog.selectedFiles();
//    }
    
    if(!outFile.isEmpty()){
        _name->setText(outFile);
        setStr(outFile);
        setValues();
        std::string className=getCallBack()->getNode()->className();
    }
}

OutputFileQLineEdit::OutputFileQLineEdit(OutputFile_Knob* knob):LineEdit(knob){
    this->knob = knob;
}

void OutputFileQLineEdit::keyPressEvent(QKeyEvent *e){
    if(e->key()==Qt::Key_Return){
        QString str=this->text();
		if(str.toStdString()!=*(knob->getStr())){
			knob->setStr(str);
			knob->setValues();
		}
    }
	QLineEdit::keyPressEvent(e);
}
/*===============================*/

Knob* ComboBox_Knob::BuildKnob(Knob_Callback* cb,std::string& description,Knob_Mask flags){
    ComboBox_Knob* knob=new ComboBox_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;

}
ComboBox_Knob::ComboBox_Knob(Knob_Callback *cb,std::string& description,Knob_Mask flags):Knob(cb),_currentItem(0){
    Q_UNUSED(flags);
    _comboBox = new ComboBox(this);
    _comboBox->addItem("/");
    QLabel* desc = new QLabel(description.c_str());
    QObject::connect(_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setCurrentItem(QString)));
    layout->addWidget(desc);
    layout->addWidget(_comboBox);
    layout->addStretch();
}
void ComboBox_Knob::populate(std::vector<std::string>& entries){
    for (U32 i = 0; i < entries.size(); i++) {
        QString str(entries[i].c_str());
        _comboBox->addItem(str);
    }
}

void ComboBox_Knob::setCurrentItem(QString str){
    std::string stdStr = str.toStdString();
    *_currentItem = stdStr;
    emit entryChanged(stdStr);
}

void ComboBox_Knob::setPointer(std::string* str){
    _currentItem = str;
    setCurrentItem("/"); /*initialises the pointer*/

}

void ComboBox_Knob::setValues(){
    QString out(_currentItem->c_str());
    for (int i =0; i< out.size(); i++) {
        values.push_back(out.at(i).unicode());
    }
}

/*============================*/

Knob* Separator_Knob::BuildKnob(Knob_Callback* cb,std::string& description,Knob_Mask flags){
    Separator_Knob* knob=new Separator_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
Separator_Knob::Separator_Knob(Knob_Callback *cb,std::string& description,Knob_Mask flags):Knob(cb){
    Q_UNUSED(flags);
    QLabel* name = new QLabel(description.c_str(),this);
    layout->addWidget(name);
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(line);
    
}
