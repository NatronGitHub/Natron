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

Reader::Reader(Node* node,ViewerGL* ui_context):InputNode(node),current_frame(0),video_sequence(0),
readHandle(0),has_preview(false),ui_context(ui_context){}

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


int Reader::currentFrame(){return current_frame;}

void Reader::incrementCurrentFrameIndex(){current_frame<lastFrame() ? current_frame++ : current_frame;}
void Reader::decrementCurrentFrameIndex(){current_frame>firstFrame() ? current_frame-- : current_frame;}
void Reader::seekFirstFrame(){current_frame=firstFrame();}
void Reader::seekLastFrame(){current_frame=lastFrame();}
void Reader::randomFrame(int f){if(f>=firstFrame() && f<=lastFrame()) current_frame=f;}

Reader::Buffer::DecodedFrameIterator Reader::open(QString filename,DecodeMode mode,bool startNewThread){
    QByteArray ba = filename.toLatin1();
    const char* filename_cstr=ba.constData();
    Reader::Buffer::DecodedFrameIterator found = _buffer.isEnqueued(filename_cstr);
    if(found !=_buffer.end()){
        if(found->second.first && !found->second.first->isFinished()){
            found->second.first->waitForFinished();
        }
        return found;
    }
    QFuture<void> *future = 0;
    Read* _read = 0;
    /*TODO :should do a more appropriate extension mapping in the future*/
    QString extension=filename.right(4);
    if(extension.at(0) == QChar('.') && extension.at(1) == QChar('e') && extension.at(2) == QChar('x') && extension.at(3) == QChar('r') ){
        
        _read=new ReadExr(this,ui_context);
    }else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('j') && extension.at(2) == QChar('p') && extension.at(3) == QChar('g')){
        
        _read=new ReadQt(this,ui_context);
    }else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('p') && extension.at(2) == QChar('n') && extension.at(3) == QChar('g')){
        
        _read=new ReadQt(this,ui_context);
    }
    else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('d') && extension.at(2) == QChar('p') && extension.at(3) == QChar('x') ){
        
        _read=new ReadFFMPEG(this,ui_context);
    }else if(extension.at(0) == QChar('t') && extension.at(1) == QChar('i') && extension.at(2) == QChar('f') && extension.at(3) == QChar('f') ){
    }else{
    }
    if(_read == 0) return _buffer.end();
    bool openStereo = false;
    if(_read->supports_stereo() && mode == STEREO_DECODE) openStereo = true;
    pair<Reader::Buffer::DecodedFrameIterator,bool> ret;
    if(startNewThread){
        future = new QFuture<void>;
        *future = QtConcurrent::run(_read,&Read::open,filename,openStereo);
        ret = _buffer.insert(filename, future, NULL , _read);
    }else{
        _read->open(filename,openStereo);
        ret = _buffer.insert(filename, NULL, _read->getReaderInfo(), _read);
    }
    return ret.first;
}

Reader::Buffer::DecodedFrameIterator Reader::openCachedFrame(FramesIterator frame){
    ReaderInfo *info = new ReaderInfo;
    info->copy(frame->second._frameInfo);
    return _buffer.insert(info->currentFrameName(), 0, info, 0).first;
}


std::vector<Reader::Buffer::DecodedFrameIterator>
Reader::decodeFrames(DecodeMode mode,bool useCurrentThread,int otherThreadCount,bool forward){
    std::vector<Reader::Buffer::DecodedFrameIterator> out;
    if(useCurrentThread){
        Buffer::DecodedFrameIterator ret = open(files[current_frame], mode , false);
        out.push_back(ret);
        int followingFrame = current_frame;
        for(int i =0 ;i < otherThreadCount;i++){
            forward ? followingFrame++ : followingFrame--;
            ret = open(files[followingFrame],mode,true);
            out.push_back(ret);
        }
    }else{
        Buffer::DecodedFrameIterator ret = open(files[current_frame], mode ,true);
        out.push_back(ret);
        int followingFrame = current_frame;
       // while(!_buffer.isFull()){
            for(int i =0 ;i < otherThreadCount - 1;i++){
                forward ? followingFrame++ : followingFrame--;
                if(followingFrame > lastFrame()) followingFrame = firstFrame();
                if(followingFrame < firstFrame()) followingFrame = lastFrame();
                if(_buffer.isFull()) break;
                ret = open(files[followingFrame],mode,true);
                out.push_back(ret);
            }
        //    if(_buffer.isFull()) break;
       //}
        
    }
    return out;
}

void Reader::setFileExtension(){
	if(readHandle){
        ui_context->clearViewer();
        Controler* ctrlPTR= node_gui->getControler();
        VideoEngine* vengine=ctrlPTR->getModel()->getVideoEngine();
        Node* output =dynamic_cast<Node*>(vengine->getOutputNode());
        if(output){
            ctrlPTR->getModel()->getVideoEngine()->clearInfos(output);
        }
    }
    _buffer.clear();
    
    getVideoSequenceFromFilesList();
    
    open(fileNameList.at(0), DEFAULT_DECODE ,false);
    
    if(!makeCurrentDecodedFrame()){
        cout << "Reader failed to open current frame file" << endl;
    }
    QByteArray ba = fileNameList.at(0).toLatin1();
    readHandle->make_preview(ba.constData());
    
}

bool Reader::makeCurrentDecodedFrame(){
    QString currentFile = files[current_frame];
    Reader::Buffer::DecodedFrameIterator frame = _buffer.isEnqueued(currentFile.toStdString());
    if(frame == _buffer.end() || (frame->second.first && !frame->second.first->isFinished())) return false;
    
    readHandle = frame->second.second.second;
    ReaderInfo* readInfo = frame->second.second.first;
    if(!readInfo && frame->second.first){
        readInfo = readHandle->getReaderInfo();
    }
    readInfo->currentFrame(currentFrame());
    readInfo->firstFrame(firstFrame());
    readInfo->lastFrame(lastFrame());
    IntegerBox bbox = readInfo->dataWindow();
    _info->set(bbox.x(), bbox.y(), bbox.right(), bbox.top());
    _info->set_full_size_format(readInfo->displayWindow());
    _info->set_channels(readInfo->channels());
    _info->setYdirection(readInfo->Ydirection());
    _info->rgbMode(readInfo->rgbMode());
    ui_context->getControler()->getModel()->getVideoEngine()->popReaderInfo(this);
    ui_context->getControler()->getModel()->getVideoEngine()->pushReaderInfo(readInfo, this);
    return true;
}


void Reader::engine(int y,int offset,int range,ChannelMask c,Row* out){
	readHandle->engine(y,offset,range,c,out);
	
}

void Reader::createKnobDynamically(){
	readHandle->createKnobDynamically();
	Node::createKnobDynamically();
}


std::pair<Reader::Buffer::DecodedFrameIterator,bool> Reader::Buffer::insert(
                                                                            QString filename,QFuture<void> *future,ReaderInfo* info,Read* readHandle){
    //if buffer is full, we remove previously computed frame
    if(_buffer.size() == _bufferSize){
        DecodedFrameIterator it = _buffer.begin();
        for(;it!=_buffer.end();it++){
            QFuture<void>* future = it->second.first;
            if(!future || (future  && future->isFinished())){
                if(it->second.first)
                    delete it->second.first;//delete future
                if(it->second.second.first)
                    delete it->second.second.first; // delete readerInfo
                if(it->second.second.second)
                    delete it->second.second.second; // delete readHandle
                _buffer.erase(it);
                break;
            }
        }
    }
    std::string _name  = QStringToStdString(filename);
    DecodedFrameDescriptor desc = make_pair(future,make_pair(info,readHandle));
    return _buffer.insert(make_pair(_name,desc));
}
void Reader::Buffer::remove(std::string filename){
    DecodedFrameIterator it = _buffer.find(filename);
    if(it!=_buffer.end()){
        if(it->second.first)
            delete it->second.first;//delete future
        if(it->second.second.first)
            delete it->second.second.first; // delete readerInfo
        if(it->second.second.second)
            delete it->second.second.second; // delete readHandle
        _buffer.erase(it);
    }
}

QFuture<void>* Reader::Buffer::getFuture(std::string filename){
    DecodedFrameIterator it = _buffer.find(filename);
    return it->second.first;
}
bool Reader::Buffer::decodeFinished(std::string filename){
    DecodedFrameIterator it = _buffer.find(filename);
    return it->second.first->isFinished();
}
Reader::Buffer::DecodedFrameIterator Reader::Buffer::isEnqueued(std::string filename){
    return _buffer.find(filename);
}
ReaderInfo* Reader::Buffer::getInfos(std::string filename){
    DecodedFrameIterator it = _buffer.find(filename);
    return it->second.second.first;
}
Read* Reader::Buffer::getReadHandle(std::string filename){
    DecodedFrameIterator it = _buffer.find(filename);
    return it->second.second.second;
}
Reader::Buffer::DecodedFrameDescriptor Reader::Buffer::getDescriptor(std::string filename){
    DecodedFrameIterator it = _buffer.find(filename);
    return it->second;
}
void Reader::Buffer::clear(){
    DecodedFrameReverseIterator it = _buffer.rbegin();
    for(;it!=_buffer.rend();it++){
        delete it->second.first;//delete future
        delete it->second.second.first; // delete readerInfo
        delete it->second.second.second; // delete readHandle
    }
    _buffer.clear();
}


void Reader::getVideoSequenceFromFilesList(){
    files.clear();
	if(fileNameList.size() > 1 ){
		video_sequence=true;
	}else{
        video_sequence=false;
    }
    bool first_time=true;
    QString originalName;
    foreach(QString Qfilename,fileNameList)
    {	if(Qfilename.at(0) == QChar('.')) continue;
        QString const_qfilename=Qfilename;
        if(first_time){
            Qfilename=Qfilename.remove(Qfilename.length()-4,4);
            int j=Qfilename.length()-1;
            QString frameIndex;
            while(j>0 && Qfilename.at(j).isDigit()){
                frameIndex.push_front(Qfilename.at(j));
                j--;
            }
            if(j>0){
				int number=0;
                if(fileNameList.size() > 1){
                    number = frameIndex.toInt();
                }
				files.insert(make_pair(number,const_qfilename));
                originalName=Qfilename.remove(j+1,frameIndex.length());
                
            }else{
                files[0]=const_qfilename;
            }
            first_time=false;
        }else{
            if(Qfilename.contains(originalName) /*&& (extension)*/){
                Qfilename.remove(Qfilename.length()-4,4);
                int j=Qfilename.length()-1;
                QString frameIndex;
                while(j>0 && Qfilename.at(j).isDigit()){
                    frameIndex.push_front(Qfilename.at(j));
                    j--;
                }
                if(j>0){
                    int number = frameIndex.toInt();
                    files[number]=const_qfilename;
                }else{
                    cout << " Read handle : WARNING !! several frames read but no frame count found in their name " << endl;
                }
            }
        }
    }
    std::map<int,QString>::iterator it;
    it=files.begin();
    current_frame=(*it).first;
}
int Reader::firstFrame(){
    int minimum=INT_MAX;
	for(std::map<int,QString>::iterator it=files.begin();it!=files.end();it++){
        int nb=(*it).first;
        if(nb<minimum){
            minimum=nb;
        }
    }
	
	return minimum;
}
int Reader::lastFrame(){
	int maximum=-1;
	for(std::map<int,QString>::iterator it=files.begin();it!=files.end();it++){
        int nb=(*it).first;
        if(nb>maximum){
            maximum=nb;
        }
    }
    return maximum;
}

std::string Reader::getCurrentFrameName(){
    return QStringToStdString(files[current_frame]);
}
std::string Reader::getRandomFrameName(int f){
    return QStringToStdString(files[f]);
}
