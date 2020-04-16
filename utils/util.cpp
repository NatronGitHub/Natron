
//librereias propias
#include "util.hpp"
//------------------------

QString fread(string _path)
{
	QFile file(QString::fromStdString(_path));
	file.open(QIODevice::ReadOnly);

	QTextStream in(&file);
	QString info;
	while (!in.atEnd())
		info += in.readLine() + QString::fromStdString("\n");
	file.close();

	return info.left(info.size() - 1); // borra el ultimo caracter "\n"
}

void fwrite(string _path, string data)
{
	const QString qPath(QString::fromStdString(_path));
	QFile qFile(qPath);
	if (qFile.open(QIODevice::WriteOnly))
	{
		QTextStream out(&qFile);
		out << QString::fromStdString(data);
		qFile.close();
	}
}

QString qs(string str)
{
	return QString::fromStdString(str);
}