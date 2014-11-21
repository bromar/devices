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

class Device;

vector<Inode> ilist;
vector<Device*> drivers;

namespace devices {
	class Device: public Monitor {
		public:
			Condition ok2read;
			Condition ok2write;
			Inode* inode;
			bool readable;
			bool writeable;
			int deviceNumber;
			string driverName;

		Device(string driverName)
			:Monitor(),ok2read(this),
			ok2write(this),deviceNumber(drivers.size()),
			driverName( driverName )
			{}

		~Device() {
			--inode->openCount;
		}

		virtual int open() {}
		virtual int close() {}
		virtual int read() {}
		virtual int write() {}
		virtual int seek() {}
		virtual int rewind() {}
		virtual int ioctl() {}
		virtual void online() {}
		virtual void offline() {}
		virtual void fireup() {}
		virtual void suspend() {}
		virtual void shutdown() {}
		virtual void initialize() {}
		virtual void finalize() {}

	};

	class streamDevice : Device {

		public: 
			int inodeCount;
			int openCount;
		
			iostream* bytes;

			streamDevice( iostream* io )
				: bytes(io), Device("iostreamDevice")
			{}

			~iostreamDevice() {}

			int open( const char* pathname, int flags) {}

			int close() {}

			int read(void* buf, size_t count) {}

			int write(void* buf, size_t count) {}

			int seek(off_t offset, int whence) {}

			int rewind(int pos) {}

		 	void online() {}

		 	void offline() {}

		 	void fireup() {}

			void suspend() {}

			void shutdown() {}

			void initialize() {}

			void finalize() {}

			static void read_occured() {}

			static void write_occured() {}

			int ioctl() {}
	};
	
	class stringstreamDevice : Device {
		public:
			int inodeCount;
			int openCount;
			stringstream* bytes;
		
			stringstreamDevice( stringstream* ss )
			: bytes(ss), Device("stringstreamDevice")
			{}
			~stringstreamDevice() {}
		
			int open(const char*, int) {}
		
			int close() {}
		
			int read(void*, size_t) {}
	
			int write( void*, size_t) {}
	
			int seek(off_t, int) {}
	
			int rewind(int) {}

			void online() {}

		 	void offline() {}

		 	void fireup() {}

			void suspend() {}

			void shutdown() {}

			void initialize() {}

			void finalize() {}

			static void read_occured() {}

			static void write_occured() {}
	
			int ioctl() {}
	};
}


#endif
