/*
	WARNING!!!!!!!
	Consider the case where a Device is created from a temporary stream (stringstream or istream).
	Our constructors require an address or pointer to this stream. As far as the implementation
	of C++ basic_istream, for example, is concerned this originating stream MUST NOT go out of scope.
	You will have to allocate memory for this stream or ensure that is always in scope.
	
	If any questions, ask us.
*/


#ifndef SYSTEMS_H
#define SYSTEMS_H 

#include <pthread.h>
#include <semaphore.h>
#include <climits>

#include <map>
#include <queue>
#include <iomanip>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>
#include <errno.h>
#include <assert.h>
#include <mutex>
#include <condition_variable>

/*
	stream flags in octal - base eight
*/
#define ODD_RDONLY 	00
#define ODD_WRONLY 	01
#define ODD_RDWR 	02

/*
	IOCTL Device Macros
*/
#define ODD_HARDRESET 001
#define ODD_FIONBIO 002
#define ODD_FIONREAD 003
#define	ODD_FIONWRITE 004

using namespace std;

class Device;

//=========================== interrupts ======================


class InterruptSystem {
public:// man sigsetops for details on signal operations.
  //static void handler(int sig);
  // exported pseudo constants
  static sigset_t on;                    
  static sigset_t alrmoff;    
  static sigset_t alloff;     
  InterruptSystem() { 
		//signal( SIGALRM, InterruptSystem::handler );
		sigemptyset( &on ); // none gets blocked.
		sigfillset( &alloff ); // all gets blocked.
		sigdelset( &alloff, SIGINT );
		sigemptyset( &alrmoff );      
		sigaddset( &alrmoff, SIGALRM ); //only SIGALRM gets blocked.
		set( alloff );        // the set() service is defined below.
		struct itimerval time;
		time.it_interval.tv_sec  = 0;
		time.it_interval.tv_usec = 400000;
		time.it_value.tv_sec     = 0;
		time.it_value.tv_usec    = 400000;
		cerr << "\nstarting timer\n";
		setitimer(ITIMER_REAL, &time, NULL);
	}
	sigset_t set(sigset_t mask) {
		sigset_t oldstatus;
		pthread_sigmask( SIG_SETMASK, &mask, &oldstatus );
		return oldstatus;
	} // sets signal/interrupt blockage and returns former status.
	sigset_t block(sigset_t mask) {
		sigset_t oldstatus;
		pthread_sigmask( SIG_BLOCK, &mask, &oldstatus );
		return oldstatus;
	} //like set but leaves blocked those signals already blocked.
};



//============================================================

class Monitor : public mutex {
public:
	sigset_t mask;
	Monitor(sigset_t mask = InterruptSystem::alrmoff);
};


class Sentry {  // To automatically block and restore interrupts.
	const sigset_t old;// Place to store old interrupt status.
	public:
	Sentry( Monitor* m );
	~Sentry();
};


// Translation of my coordination primitives to C++14's.  The only
// thing that C++14 lacks is prioritized waiting, which is very 
// important for scheduling.

// The following version of EXCLUSION works correctly because C++ 
// invokes destructors in the opposite order of the constructors.
#define EXCLUSION Sentry snt_(this); unique_lock<Monitor::mutex> lck(*this);
#define CONDITION condition_variable
#define WAIT      wait(lck)
#define SIGNAL    notify_one()



class Semaphore : public Monitor {
// signal-safe semaphore
// The common C++ version of semaphore is not async-signal safe 
// in the sense defined by POSIX.
private:
  CONDITION available;
  int count;
public:
  Semaphore( int count = 0 );
  void release();
  void acquire();
};


class Inode : Monitor {
public: 
	int linkCount = 0;
	int openCount = 0;
	bool readable = false;
	bool writable = false;
	enum Kind { regular, directory, symlink, pipe, socket, device } kind;

	time_t ctime, mtime, atime;

	Inode();
	~Inode();
	int unlink();
	void cleanup();
	Device* driver;
	string* bytes;
};

//==================================================================


//--------------------------------------------------


#endif
