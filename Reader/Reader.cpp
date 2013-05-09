#include "Reader/Reader.h"
#include "Reader/readffmpeg.h"
#include "Reader/readExr.h"
#include "Reader/readQt.h"
#include "Core/node.h"
#include "Gui/GLViewer.h"
#include "Superviser/controler.h"
#include "Core/VideoEngine.h"
#include "Core/model.h"
#include "Core/outputnode.h"
/*#ifdef __cplusplus
extern "C" {
#endif
#ifdef _WIN32
	READER_EXPORT Reader* BuildReader(Node *node){return new Reader(node);}
#elif defined(unix) || defined(__unix__) || defined(__unix)
	Reader* BuildReader(Node *node){return new Reader(node);}
#endif
#ifdef __cplusplus
}
#endif*/

using namespace std;

Reader::Reader(Node* node,ViewerGL* ui_context):InputNode(node){
	has_preview=false;
	this->ui_context=ui_context;
    readHandle=NULL;
}

Reader::Reader(Reader& ref):InputNode(ref){}


Reader::~Reader(){
	delete preview;
	delete readHandle;
}
const char* Reader::class_name(){return "Reader";}

const char* Reader::description(){
    return "InputNode";
}
void Reader::initKnobs(Knob_Callback *cb){
	QString desc("File");
	
	Knob::file_Knob(cb,desc,fileNameList);
	
	Node::initKnobs(cb);
}

char* Reader::getCurrentFrameName(){return readHandle->getCurrentFrameName();}
char* Reader::getRandomFrameName(int f){return readHandle->getRandomFrameName(f);}

int Reader::currentFrame(){return readHandle->frame();}

void Reader::incrementCurrentFrameIndex(){readHandle->incrementCurrentFrameIndex();}
void Reader::decrementCurrentFrameIndex(){readHandle->decrementCurrentFrameIndex();}
void Reader::seekFirstFrame(){readHandle->seekFirstFrame();}
void Reader::seekLastFrame(){readHandle->seekLastFrame();}
void Reader::randomFrame(int f){readHandle->randomFrame(f);}

void Reader::setup_for_next_frame(){readHandle->setup_for_next_frame();}

void Reader::setFileExtension(){
	if(readHandle){
        ui_context->clearViewer();
        Controler* ctrlPTR= node_gui->getControler();
        VideoEngine* vengine=ctrlPTR->getModel()->getVideoEngine();
        Node* output =dynamic_cast<Node*>(vengine->getOutputNode());
        if(output){
            ctrlPTR->getModel()->getVideoEngine()->clearInfos(output);
        }
        //vengine->clearCache();
        delete readHandle;
    }
    
	if(!fileNameList.isEmpty()){
		QString extension=fileNameList.at(0).right(4);
		if(extension.at(0) == QChar('.') && extension.at(1) == QChar('e') && extension.at(2) == QChar('x') && extension.at(3) == QChar('r') ){
			//std::cout << "extension: exr" << std::endl;
			filetype=EXR;
			
			readHandle=new ReadExr(fileNameList,this,ui_context);
		}else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('j') && extension.at(2) == QChar('p') && extension.at(3) == QChar('g')){
//			std::cout << "extension: jpg FFmpeg used" << std::endl;
//			filetype=FFMPEG;
//			readHandle=new ReadFFMPEG(fileNameList,this,ui_context);
            //std::cout <<"extension: jpg" << std::endl;
            filetype=JPEG;
            readHandle=new ReadJPEG(fileNameList,this,ui_context);
		}else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('p') && extension.at(2) == QChar('n') && extension.at(3) == QChar('g')){
            
            filetype=PNG;
            readHandle=new ReadJPEG(fileNameList,this,ui_context);
		}
		else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('d') && extension.at(2) == QChar('p') && extension.at(3) == QChar('x') ){
			//filetype=DPX;
            //std::cout << "extension: dpx FFmpeg used" << std::endl;
			filetype=FFMPEG;
			readHandle=new ReadFFMPEG(fileNameList,this,ui_context);
		}else if(extension.at(0) == QChar('t') && extension.at(1) == QChar('i') && extension.at(2) == QChar('f') && extension.at(3) == QChar('f') ){
			filetype=TIFF;
		}else{
			filetype=OTHER;
		}
        
        if(readHandle){
            readHandle->open();
            readHandle->make_preview();
            // setting frame numbers accordingly among classes
            _info->firstFrame(readHandle->firstFrame());
            _info->lastFrame(readHandle->lastFrame());
            ui_context->fileType(filetype);
            
        }
	}
}


void Reader::engine(int y,int offset,int range,ChannelMask c,Row* out){
	readHandle->engine(y,offset,range,c,out);
	
}

void Reader::createKnobDynamically(){
	readHandle->createKnobDynamically();
	Node::createKnobDynamically();
}