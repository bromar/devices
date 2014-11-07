#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>

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

class istreamDevice : DeviceDriver {

	public: 
		int inodeCount = 0;
		int openCount = 0; 	//access_counter

		istream* bytes;	//data - iostream* gives errors

		//my additions
		mode_t mode;
		size_t offset;

		unsigned int flags;
		
		

		istreamDevice( istream* io )
			: bytes(io), DeviceDriver("istreamDevice")
		{
			readable = true;
			writeable = true;
		}

		~istreamDevice() {
		}

		int open( const char* pathname, int flags) {
			
		}

		int close( int fd) {

		}

		int read( int fd, void* buf, size_t count) {
			istreamDevice* iD = (istreamDevice*)drivers[fd];

			cout <<"drivers vector size " << drivers.size() << endl;
			cout <<"deviceNumber "<< iD->deviceNumber << endl;
			cout <<"driverName "<< iD->driverName << endl;

			int i = 0;

			while((buf = (void*)(bytes->get()))) //while not end of file 
			{
				//bytes->get((char*)buf, count);	
				cout << "reading this into buf " << buf << endl;
				i++;
			}

			return i;		
		}

		int write( int fd, void* buf, size_t count) {

			
		}

		int seek( int fd, off_t offset, int whence) {
			
		}

		int rewind( int pos ) {
			
		}

		/*
		int ioctl(  ) {

		}
		//*/
};

int main() {

	string testString = "";
	cin >> testString;
	cout << "testing this = " << testString << endl;

	char buf[256];
	char hi[6] = "hello";

	istreamDevice i = istreamDevice(&cin);

	while(1){
		cout << "amount read " << i.read(0,buf,256) << endl;		
	}
}


































