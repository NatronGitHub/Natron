#include <cstdlib>
#include <QtGui/qrgb.h>
#include "Reader/Read.h"
#include "Reader/Reader.h"
#include "Gui/GLViewer.h"
#include "Core/lookUpTables.h"
#include "Reader/readerInfo.h"
#include "Superviser/controler.h"
#include "Core/model.h"
#include "Core/VideoEngine.h"
Read::Read(const QStringList& file_list,Reader* op,ViewerGL* ui_context):file_name_list(file_list),is_stereo(false), video_sequence(false),current_frame(0),display_format(),_autoCreateAlpha(false),_premult(false)
{
	_lut=NULL;
    this->ui_context = ui_context;
	this->op=op;
	getVideoSequenceFromFilesList();
    
}
Read::~Read(){ delete _lut; }

int Read::firstFrame(){
    int minimum=INT_MAX;
	for(std::map<int,QString>::iterator it=files.begin();it!=files.end();it++){
        int nb=(*it).first;
        if(nb<minimum){
            minimum=nb;
        }
    }
	
	return minimum;
}
int Read::lastFrame(){
	int maximum=-1;
	for(std::map<int,QString>::iterator it=files.begin();it!=files.end();it++){
        int nb=(*it).first;
        if(nb>maximum){
            maximum=nb;
        }
    }
    return maximum;
}

void Read::setup_for_next_frame(bool makePreview){
    if(ui_context->getCurrentFrameName() == files[current_frame])
        return;
    open();
    if(makePreview)
        make_preview();
}

void Read::getVideoSequenceFromFilesList(){
	
	if(file_name_list.size() > 1 ){
		video_sequence=true;
	}
	
    
	//if(video_sequence){
    bool first_time=true;
    QString originalName;
    foreach(QString Qfilename,file_name_list)
    {	if(Qfilename.at(0) == QChar('.')) continue;
        QString const_qfilename=Qfilename;
        //		bool extension = Qfilename.right(4)==".exr" || Qfilename.right(4)==".dpx" ||  Qfilename.right(4)==".jpg" || Qfilename.right(4)==".bmp";
        if(first_time){
            
			
            //  if(extension){
            Qfilename=Qfilename.remove(Qfilename.length()-4,4);
            //std::cout << "filename of next frame in directory after truncating .exr or .dpx " << Qfilename.toStdString().c_str() << std::endl;
            
            int j=Qfilename.length()-1;
            QString frameIndex;
            while(j>0 && Qfilename.at(j).isDigit()){
                frameIndex.push_front(Qfilename.at(j));
                j--;
            }
            //std::cout << "frame file name contained in the original name,frame number as a string is: " << frameIndex.toStdString().c_str() << std::endl;
            if(j>0){
				int number=0;
                if(file_name_list.size() > 1){
                  number = frameIndex.toInt();
                }
				files.insert(make_pair(number,const_qfilename));
                //std::cout << "to int : " << number << std::endl;
                originalName=Qfilename.remove(j+1,frameIndex.length());
                
            }else{
                files[0]=const_qfilename;
            }
            
            first_time=false;
            //std::cout << "NAME OF SEQUENCE: " << originalName.toStdString().c_str() << std::endl;
            // }
        }else{
            if(Qfilename.contains(originalName) /*&& (extension)*/){
                Qfilename.remove(Qfilename.length()-4,4);
                //std::cout << "filename of next frame in directory after truncating .exr or .dpx " << Qfilename.toStdString().c_str() << std::endl;
                
                int j=Qfilename.length()-1;
                QString frameIndex;
                while(j>0 && Qfilename.at(j).isDigit()){
                    frameIndex.push_front(Qfilename.at(j));
                    j--;
                }
                //std::cout << "frame file name contained in the original name,frame number as a string is: " << frameIndex.toStdString().c_str() << std::endl;
                if(j>0){
					
                    int number = frameIndex.toInt();
                    files[number]=const_qfilename;
                    //std::cout << "to int : " << number << std::endl;
                    
                    
                }else{
                    cout << " Read handle : WARNING !! several frames read but no frame count found in their name " << endl;
                }
                
            }
            
        }
        
    }
//        for(std::map<int,QString>::iterator i=files.begin();i!=files.end();i++){
//            int number=(*i).first;
//            QString name=(*i).second;
//            cout << " Frame number: " << number << " is : " << name.toStdString().c_str() << endl;
//        }
    std::map<int,QString>::iterator it;
    it=files.begin();
    current_frame=(*it).first;
    
    
	
    
    
}

//void Read::from_byte(Channel z, float* to, const uchar* from, const uchar* alpha, int W, int delta ){
//    
//    if((Mask_RGB & z) == z){ // z is  red, green or blue
//        if(alpha!=NULL && premultiplied()){
//            // use lut with alpha
//			if(_lut!=NULL)
//				_lut->from_byte(to, from, alpha, W,delta);
//			else{
//				int index=0;
//				for(int i=0;i< W;i+=delta){
//					to[index++]=Linear::from_byte((float)((int)from[i])/(float)((int)alpha[i])) * (float)((int)alpha[i]);
//				}
//			}
//            
//        }
//        else{
//            // use lut without alpha
//			if(_lut!=NULL)
//				_lut->from_byte(to, from, W,delta);
//			else{
//				int index=0;
//				for(int i=0;i< W;i+=delta){
//					to[index++]=Linear::from_byte((float)((int)from[i]));
//				}
//			}
//        }
//    }else{
//        int index =0;
//        for(int i=0; i < W; i+=delta){
//            to[index++] = from[i] / 255.f;
//        }
//    }
//}

//may enhance by changing Lut::from_byte ,from_float& from_short to take several channels in input at once

void Read::from_byte(float* r,float* g,float* b, const uchar* from, bool hasAlpha, int W, int delta ,float* a,bool qtbuf){
    if(!_lut->linear()){
        _lut->from_byte(r,g,b,from,hasAlpha,premultiplied(),autoAlpha(), W,delta,a,qtbuf);
    }
    else{
        Linear::from_byte(r,g,b,from,hasAlpha,premultiplied(),autoAlpha(),W,delta,a,qtbuf);
    }
	
}

void Read::from_float(float* r,float* g,float* b, const float* fromR,const float* fromG,
                      const float* fromB, int W, int delta ,const float* fromA,float* a){
    if(!_lut->linear()){
        _lut->from_float(r,g,b,fromR,fromG,fromB,premultiplied(),autoAlpha(), W,delta,fromA,a);
    }
    else{
        Linear::from_float(r,g,b,fromR,fromG,fromB,premultiplied(),autoAlpha(),W,delta,fromA,a);
    }
    
    
    
}

//void Read::from_short(Channel z, float* to, const U16* from, const U16* alpha, int W, int bits, int delta ){
//    if(!(Mask_RGB & z)){ // z is  red, green or blue
//        if(alpha!=NULL && premultiplied()){
//            // use lut with alpha
//            _lut->from_short(to, from, alpha, W,delta);
//        }
//        else{
//            // use lut witout alpha
//            _lut->from_short(to, from, W,delta);
//            
//        }
//    }else{
//        int index =0;
//        for(int i=0; i < W; i+=delta){
//            to[index++] = from[i] / 255.f;
//        }
//    }
//}
//void Read::from_float(Channel z, float* to, const float* from, const float* alpha, int W, int delta ){
//    if(!(Mask_RGB & z)){ // z is  red, green or blue
//        if(alpha!=NULL && premultiplied()){
//            // use lut with alpha
//            _lut->from_float(to, from, alpha, W,delta);
//        }
//        else{
//            // use lut witout alpha
//            _lut->from_float(to, from, W,delta);
//            
//        }
//    }else{
//        int index =0;
//        for(int i=0; i < W; i+=delta){
//            to[index++] = from[i] / 255.f;
//        }
//    }
//}

void Read::convertSubSampledBufferToFullSize(float *buf, int samplingFactor,int offset,int range){
    //     if(samplingFactor==0)
    // 		samplingFactor=1;
    // 	int size =(range-offset)/samplingFactor;
    // 	if(size==0) size=1;
    //     float tmp[size];
    //     for(int i =offset;i<(range-offset)/samplingFactor;i++){
    //         tmp[i] = buf[i];
    //     }
    //     int index = offset;
    //     for(int i =offset;i<range-offset;i++){
    //         for(int j = 0; j < samplingFactor-1;j++){
    //             buf[i] = tmp[index];
    //         }
    //         index++;
    //     }
    //
}


void Read::createKnobDynamically(){
    
}

void Read::setReaderInfo(DisplayFormat dispW,
                         IntegerBox dataW,
                         QString currentFrameName,
                         ChannelMask channels,
                         int Ydirection ,
                         bool rgb ,
                         int currentFrame,
                         int firstFrame,
                         int lastFrame){
	_readInfo = new ReaderInfo(dispW,dataW,currentFrameName,channels,Ydirection,rgb,currentFrame,firstFrame,lastFrame);
	ui_context->getControler()->getModel()->getVideoEngine()->pushReaderInfo(_readInfo,op);
}