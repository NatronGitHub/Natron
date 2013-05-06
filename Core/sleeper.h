#ifndef __SLEEPER_H_INCLUDED__
#define __SLEEPER_H_INCLUDED__
#include <QtCore/QThread>
// utility class to call QThread:sleep since these methods are protected within QThread...
class Sleeper : public QThread
{
public:
	static void sleep(unsigned long secs)
	{
		QThread::sleep(secs);
	}
	static void msleep(unsigned long msecs) 
	{
		QThread::msleep(msecs);
	}
	static void usleep(unsigned long usecs) 
	{
		QThread::usleep(usecs);
	}
};

#endif // __SLEEPER_H_INCLUDED__

