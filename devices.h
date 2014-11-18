#ifndef DEVICES_H
#define DEVICES_H 

#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>

#include <sstream>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>
#include <map>
#include <queue>
#include <iomanip>
#include <signal.h>
#include <sys/time.h>
#include <climits>
#include <mutex>
#include <condition_variable>

using namespace std;

class DeviceDriver;

vector<Inode> ilist;
vector<DeviceDriver*> drivers;

namespace devices {
	class DeviceDriver: public Monitor {
		public:
			Condition ok2read;
			Condition ok2write;
			Inode* inode;
			bool readable;
			bool writeable;
			int deviceNumber;
			string driverName;

		DeviceDriver(string driverName)
			:Monitor(),ok2read(this),
			ok2write(this),deviceNumber(drivers.size()),
			driverName( driverName )
			{}

		~DeviceDriver() {
			--inode->openCount;
		}

		virtual int read() {}
		virtual int write() {}
		virtual int seek() {}
		virtual int rewind() {}
		virtual int ioctl() {}
		virtual void online() {}
		virtual void offline() {}
		virtual void fireup() {}
		virtual void suspend() {}

	};

	class iostreamDevice : DeviceDriver {

		public: 
			int inodeCount;
			int openCount;
		
			iostream* bytes;

			iostreamDevice( iostream* io )
				: bytes(io), DeviceDriver("iostreamDevice")
			{}

			~iostreamDevice() {}

			int open( const char* pathname, int flags) {}

			int close( int fd) {}

			int read( int fd, void* buf, size_t count) {}

			int write( int fd, void* buf, size_t count) {}

			int seek( int fd, off_t offset, int whence) {}

			int rewind( int pos ) {}

			/*
			int ioctl(  ) {
	
			}
			//*/
	};
	
	class stringstreamDevice : DeviceDriver {
		public:
			int inodeCount;
			int openCount;
			stringstream* bytes;
		
			stringstreamDevice( stringstream* ss )
			: bytes(ss), Device("stringstreamDevice")
			{}
			~stringstreamDevice() {}
		
			int open(const char*, int) {}
		
			int close(int) {}
		
			int read(int, void*, size_t) {}
	
			int write( int, void*, size_t) {}
	
			int seek(int, off_t, int) {}
	
			int rewind(int) {}
	
			/*
			int ioctl( ) {
			}
			//*/
	};
}


#endif
