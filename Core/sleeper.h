//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



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

