#ifndef UTIL_HPP
#define UTIL_HPP
#include <iostream>
#include <fstream>   // ifstream
#include <vector>    //
#include <sstream>   // istringstream
#include <algorithm> //sort , find
#include <typeinfo>
#include <ctime> // time_t
#include <QDebug>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QList>

using namespace std;

QString fread(string path);
void fwrite(QString path, QString data);

// print para 1, 2 y 3 argumentos
template <class T>
void print(T input)
{
    qDebug().nospace() << input;
}

template <class T1, class T2>
void print(T1 input1, T2 input2)
{
    qDebug().nospace() << input1 << " " << input2;
}

template <class T1, class T2, class T3>
void print(T1 input1, T2 input2, T3 input3)
{
    qDebug().nospace() << input1 << " " << input2 << " " << input3;
}
// ----------------------------

#endif //UTIL_HPP