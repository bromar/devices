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

/*
	stream flags in octal - base eight
*/
#define ODD_RDONLY 	00
#define ODD_WRONLY 	01
#define ODD_RDWR 	02

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

class Inode : Monitor {
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
		assert(linkCount > 0);
		--linkCount;
		cleanup();
		return 0;
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

class DeviceDriver: public Monitor {
	public:
		Condition ok2read;
		Condition ok2write;
		Inode *inode;
		bool readable;
		bool writeable;
		int deviceNumber;
		string driverName;

	DeviceDriver(string driverName)
		:Monitor(),ok2read(this),
		ok2write(this),deviceNumber(drivers.size()),
		driverName( driverName )
		{
			drivers.push_back(this);
			//inode->openCount++;
		}

	~DeviceDriver() {
		//inode->openCount--;

		// we must remove device driver from vector
		// this removal run in O(n^2), look into remove_if
		auto i = begin(drivers);
		while (i != end(drivers)) {
		    if ((*i)->deviceNumber == deviceNumber) {
		        i = drivers.erase(i);
		    }
		    else {
		        i++;
		    }
		}
	}
	
	/*virtual int open() {}
	virtual int read() {}
	virtual int write() {}
	virtual int seek() {}
	virtual int rewind() {}
	virtual int ioctl() {}*/
	virtual void open() {}
	virtual void read() {}
	virtual void write() {}
	virtual void seek() {}
	virtual void rewind() {}
	virtual void ioctl() {}
	virtual void online() {}
	virtual void offline() {}
	virtual void fireup() {}
	virtual void suspend() {}

};


class istreamDevice : DeviceDriver {

	public: 
		int inodeCount = 0;
		int openCount = 0;
		
		istream *bytes;

		istreamDevice( istream *io )
			: bytes(io), DeviceDriver("istreamDevice")
		{
			cout << "*Inside istreamDevice()\n";
			readable = false;
			writeable = false;
			cout << "readable: " << readable << endl;
			cout << "writeable: " << writeable << endl;
		}

		~istreamDevice() {
			//
		}

		int open(const char* pathname, int flags) {
			readable = !(flags & 0x01);
			writeable = (flags & 0x01) | (flags & 0x02);
			cout << "readable: " << readable << endl;
			cout << "writeable: " << writeable << endl;
			driverName = pathname;
			cout << "driverName: " << driverName << endl;
			openCount++;
			return deviceNumber; // return fd to device driver
		}

		int close(int fd) {
			//openCount--;
			//~istreamDevice();
			return 0;
		}

		int read(int fd, void* buf, size_t count) {
			return -1;
		}

		int seek(int fd, off_t offset, int whence) {
			return -1;
		}

		int rewind(int pos ) {
			return -1;
		}

		/*
		int ioctl(  ) {

		}
		*/
};

class ostreamDevice : DeviceDriver {

	public: 
		int inodeCount = 0;
		int openCount = 0;
		
		ostream* bytes;

		ostreamDevice( ostream* io )
			: bytes(io), DeviceDriver("ostreamDevice")
		{
			readable = true;
			writeable = true;
		}

		~ostreamDevice() {
		}

		int open(const char* pathname, int flags) {
			return -1;
		}

		int close(int fd) {
			return -1;
		}

		int write(int fd, void* buf, size_t count) {
			return -1;
		}

		/*
		int ioctl(  ) {

		}
		*/
};

int main(int argc, char **argv)
{
	cout << "Hello world!\n\n";

	istreamDevice is1 = istreamDevice(&cin);
	cout << "Read yes --\n";
	int dn1 = is1.open("~/Desktop/tes1.txt",ODD_RDONLY);
	cout << "deviceNumber: " << dn1 << endl << endl;

	istreamDevice is2 = istreamDevice(&cin);
	cout << "Write yes --\n";
	int dn2 = is2.open("~/Desktop/test2.txt",ODD_WRONLY);
	cout << "deviceNumber: " << dn2 << endl << endl;
	
	istreamDevice is3 = istreamDevice(&cin);
	cout << "Read+Write yes --\n";
	int dn3 = is3.open("~/Desktop/test3.txt",ODD_RDWR);
	cout << "deviceNumber: " << dn3 << endl << endl;

	cout << "\nBye cruel world!\n";
}
