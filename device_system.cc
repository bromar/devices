#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>
#include <strstream>
#include <cstring>
#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>
#include <errno.h>

using namespace std;

#define EXCLUSION Sentry exclusion(this); exclusion.touch();

class DeviceDriver;

class Monitor {
  // ...
};

class Condition {
public:
  Condition( Monitor* m ) {}
  // ...
};

//========================== Inode =================================

class Inode: Monitor {
public: 
	int linkCount = 0;
	int openCount = 0;
	bool readable = false;
	bool writable = false;
	enum Kind { regular, directory, symlink, pipe, socket, device } kind;

	time_t ctime, mtime, atime;

	~Inode() {
	}

	int unlink() {
		assert( linkCount > 0);
		--linkCount;
		cleanup();	
	}
	
	void cleanup() {
		if (! openCount && !linkCount ) {
			//throwaway stuff
		}
	}
	DeviceDriver* driver;
	string* bytes;
};

vector<Inode> ilist;
vector<DeviceDriver*> drivers;

//========================== DeviceDriver =================================

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
		: Monitor(),
			ok2read(this), 
			ok2write(this), 
			deviceNumber(drivers.size()), 
			driverName( driverName )
		{
			drivers.push_back(this);
			//++inode->openCount;	//segfault
		}

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

//========================== iostreamDevice =================================

class iostreamDevice : DeviceDriver {

	public: 
		int inodeCount = 0;
		int openCount = 0; 	//access_counter

		iostream* bytes;		//data

		//my additions
		mode_t mode;
		size_t offset = 0;

		unsigned int flags;
		
		iostreamDevice( iostream* io )
			: bytes(io), DeviceDriver("iostreamDevice")
		{
			readable = true;
			writeable = true;
		}

		~iostreamDevice() {
		}

		int open( const char* pathname, int flags) {
			
		}

		int close( int fd) {

		}

		int read( int fd, void* buf, size_t count) {

			iostreamDevice* ioD = (iostreamDevice*)drivers[fd];

			//cout <<"drivers vector size " << drivers.size() << endl;
			//cout <<"deviceNumber "<< ioD->deviceNumber << endl;
			//cout <<"driverName "<< ioD->driverName << endl;

			char* buffer = (char*)buf;

			int i = 0;
			for(i = 0; i < count; i++)
			{
				buffer[i] = bytes->get();
				offset--; 					//increment or decrement as we consume 
			}
			return i;		
		}

		int write( int fd, void const* buf, size_t count) {
			iostreamDevice* ioD = (iostreamDevice*)drivers[fd];

			//cout <<"drivers vector size " << drivers.size() << endl;
			//cout <<"deviceNumber "<< ioD->deviceNumber << endl;
			//cout <<"driverName "<< ioD->driverName << endl;

			char* buffer = (char*)buf;

			int i = 0;
			for(i = 0; i < count; i++)
			{
				bytes->put(buffer[i]);
				offset++;					//increment or decrement as we consume 
			}
			return i;	
			
		}

		int seek( int fd, off_t offset, int whence) {
			iostreamDevice* ioD = (iostreamDevice*)drivers[fd];

			//cout <<"drivers vector size " << drivers.size() << endl;
			//cout <<"deviceNumber "<< ioD->deviceNumber << endl;
			//cout <<"driverName "<< ioD->driverName << endl;

			if(whence == SEEK_SET)
			{
				this->offset = offset;
			}
			else if(whence == SEEK_CUR)
			{
				this->offset += offset;
			}
			else if(whence == SEEK_END)
			{
				//this.offset 
			} 
			bytes->seekg(this->offset);
			return this->offset	;
			
		}

		int rewind( int pos ) {
			
		}

		/*
		int ioctl(  ) {

		}
		//*/
};

int main() {

	char buf[1024];
	strstreambuf sb(buf,1024,buf);
	iostream s(&sb);
	iostreamDevice i = iostreamDevice(&s);

	char* buf2 = "hello there my friend";	

	/*//test read and write
	cout << "amount write: " << i.write(0,buf2,1024) << endl;
	cout << "wrote this: " << buf2 << endl;
	cout << "====================" << endl;

	cout << "amount read: " << i.read(0,buf,1024) << endl;
	cout << "read this: "<< buf << endl;
	cout << "====================" << endl;
	//*/

	//test read and write and seek
	cout << "amount write: " << i.write(0,buf2,1024) << endl;
	cout << "wrote this: " << buf2 << endl;
	cout << "seeking: " << i.seek(0, 4, SEEK_SET) << endl;
	cout << "====================" << endl;

	cout << "amount read: " << i.read(0,buf,1024) << endl;
	cout << "read this: "<< buf << endl;
	cout << "end" << endl;
	//*/


	return 0;
}


































