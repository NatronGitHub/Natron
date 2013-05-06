#ifndef __POWITER_PW_STRINGS_H__
#define __POWITER_PW_STRINGS_H__

#include "Superviser/powiterFn.h"
#include <string>
#include <QtCore/QString>
#include <QtCore/QByteArray>
/*This header is here to add some convenience functions to work with QString and std::string*/

/*The returned char* must be freeed by the user afterwards
 *If str is empty, it returns NULL*/
static char* QstringCpy(QString str){
	size_t size = str.size();
	if(size==0) return NULL;
	char* dest = (char*) malloc(size+1);
	QByteArray ba = str.toLocal8Bit();
	memcpy(dest, ba.data(), size);
	dest[size]='\0';
	return dest;
}

/*The returned char* must be freeed by the user afterwards*/
static char* stdStrCpy(std::string str){
	size_t size = str.size();
	if(size==0) return NULL;
	char* dest = (char*) malloc(size+1);
	memcpy(dest, str.c_str(), size);
	dest[size]='\0';
	return dest;
}

static int strcmp(const char* str1,QString str2){
	QByteArray ba = str2.toLocal8Bit();
	return strcmp(str1,ba.data());
}
static int strcmp(const char* str1,std::string str2){
	return strcmp(str1,str2.c_str());
}

#endif